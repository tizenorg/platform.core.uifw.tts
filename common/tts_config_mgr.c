/*
*  Copyright (c) 2011-2016 Samsung Electronics Co., Ltd All Rights Reserved
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*  http://www.apache.org/licenses/LICENSE-2.0
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
*/

#include <dirent.h>
#include <dlfcn.h>
#include <dlog.h>
#include <Ecore.h>
#include <fcntl.h>
#include <glib.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <vconf.h>

#include "tts_config_mgr.h"
#include "tts_config_parser.h"
#include "tts_defs.h"

typedef struct {
	int	uid;
	tts_config_engine_changed_cb		engine_cb;
	tts_config_voice_changed_cb		voice_cb;
	tts_config_speech_rate_changed_cb	speech_cb;
	tts_config_screen_reader_changed_cb	screen_cb;
	tts_config_pitch_changed_cb		pitch_cb;
	void*	user_data;
} tts_config_client_s;

extern char* tts_tag();

static GSList* g_engine_list = NULL;

static GSList* g_config_client_list = NULL;

static tts_config_s* g_config_info;

static Ecore_Fd_Handler* g_config_fd_handler_noti = NULL;
static int g_config_fd_noti;
static int g_config_wd_noti;

/* For engine directory monitoring */
typedef struct {
	Ecore_Fd_Handler* dir_fd_handler;
	int dir_fd;
	int dir_wd;
} tts_engine_inotify_s;

static GList* g_ino_list = NULL;

int __tts_config_mgr_print_engine_info();
static int __tts_config_mgr_register_engine_config_updated_event(const char* path);
static int __tts_config_mgr_unregister_engine_config_updated_event();

int __tts_config_mgr_check_engine_is_valid(const char* engine_id)
{
	if (NULL == engine_id) {
		SLOG(LOG_ERROR, tts_tag(), "Input parameter is NULL");
		return -1;
	}

	GSList *iter = NULL;
	tts_engine_info_s *engine_info = NULL;

	if (0 >= g_slist_length(g_engine_list)) {
		SLOG(LOG_ERROR, tts_tag(), "There is no engine!!");
		return -1;
	}

	/* Get a first item */
	iter = g_slist_nth(g_engine_list, 0);

	while (NULL != iter) {
		engine_info = iter->data;

		if (NULL == engine_info) {
			SLOG(LOG_ERROR, tts_tag(), "engine info is NULL");
			return -1;
		}

		if (0 == strcmp(engine_id, engine_info->uuid)) {
			SLOG(LOG_DEBUG, tts_tag(), "Default engine is valid : %s", engine_id);
			return 0;
		}

		iter = g_slist_next(iter);
	}

	/* Change default engine */
	iter = g_slist_nth(g_engine_list, 0);
	engine_info = iter->data;

	if (NULL == g_config_info) {
		return TTS_CONFIG_ERROR_OPERATION_FAILED;
	}

	if (NULL != g_config_info->engine_id)	free(g_config_info->engine_id);
	if (NULL != g_config_info->setting)	free(g_config_info->setting);

	g_config_info->engine_id = strdup(engine_info->uuid);
	g_config_info->setting = strdup(engine_info->setting);

	SLOG(LOG_DEBUG, tts_tag(), "Default engine is changed : %s", g_config_info->engine_id);

	/* Change is default voice */
	GSList *iter_voice = NULL;
	tts_config_voice_s* voice = NULL;
	bool is_valid_voice = false;

	/* Get a first item */
	iter_voice = g_slist_nth(engine_info->voices, 0);

	while (NULL != iter_voice) {
		/*Get handle data from list*/
		voice = iter_voice->data;

		if (NULL != voice) {
			if (NULL != voice->language && NULL != g_config_info->language) {
				if (0 == strcmp(voice->language, g_config_info->language)) {
					if (voice->type == g_config_info->type) {
						/* language is valid */
						is_valid_voice = true;

						free(g_config_info->language);
						g_config_info->language = strdup(voice->language);
						g_config_info->type = voice->type;

						SLOG(LOG_DEBUG, tts_tag(), "Default voice is changed : lang(%s) type(%d)", voice->language, voice->type);
						break;
					}
				}
			}
		}

		iter_voice = g_slist_next(iter_voice);
	}

	if (false == is_valid_voice) {
		/* Select first voice as default */
		if (NULL != g_config_info->language) {
			free(g_config_info->language);

			iter_voice = g_slist_nth(engine_info->voices, 0);
			voice = iter_voice->data;

			g_config_info->language = strdup(voice->language);
			g_config_info->type = voice->type;
			SLOG(LOG_DEBUG, tts_tag(), "Default voice is changed : lang(%s) type(%d)", voice->language, voice->type);
		}
	}

	if (0 != tts_parser_set_engine(g_config_info->engine_id, g_config_info->setting, 
		g_config_info->language, g_config_info->type)) {
		SLOG(LOG_ERROR, tts_tag(), " Fail to save config");
		return TTS_CONFIG_ERROR_OPERATION_FAILED;
	}

	return 0;
}

bool __tts_config_mgr_check_lang_is_valid(const char* engine_id, const char* language, int type)
{
	if (NULL == engine_id || NULL == language) {
		SLOG(LOG_ERROR, tts_tag(), "Input parameter is NULL");
		return false;
	}

	GSList *iter = NULL;
	tts_engine_info_s *engine_info = NULL;

	if (0 >= g_slist_length(g_engine_list)) {
		SLOG(LOG_ERROR, tts_tag(), "There is no engine!!");
		return false;
	}

	/* Get a first item */
	iter = g_slist_nth(g_engine_list, 0);

	while (NULL != iter) {
		engine_info = iter->data;

		if (NULL == engine_info) {
			SLOG(LOG_ERROR, tts_tag(), "engine info is NULL");
			return false;
		}

		if (0 != strcmp(engine_id, engine_info->uuid)) {
			iter = g_slist_next(iter);
			continue;
		}

		GSList *iter_voice = NULL;
		tts_config_voice_s* voice = NULL;

		if (g_slist_length(engine_info->voices) <= 0) {
			SLOG(LOG_ERROR, tts_tag(), "There is no voice : %s", engine_info->uuid);
			iter = g_slist_next(iter);
			return false;
		}

		/* Get a first item */
		iter_voice = g_slist_nth(engine_info->voices, 0);

		int i = 1;
		while (NULL != iter_voice) {
			/*Get handle data from list*/
			voice = iter_voice->data;

			if (NULL != voice) {
				if (0 == strcmp(language, voice->language)) {
					if (type == voice->type) {
						return true;
					}
				}
			}

			/*Get next item*/
			iter_voice = g_slist_next(iter_voice);
			i++;
		}

		return false;
	}

	return false;
}

int __tts_config_mgr_select_lang(const char* engine_id, char** language, int* type)
{
	if (NULL == engine_id || NULL == language) {
		SLOG(LOG_ERROR, tts_tag(), "Input parameter is NULL");
		return false;
	}

	GSList *iter = NULL;
	tts_engine_info_s *engine_info = NULL;

	if (0 >= g_slist_length(g_engine_list)) {
		SLOG(LOG_ERROR, tts_tag(), "There is no engine!!");
		return false;
	}

	/* Get a first item */
	iter = g_slist_nth(g_engine_list, 0);

	while (NULL != iter) {
		engine_info = iter->data;

		if (NULL == engine_info) {
			SLOG(LOG_ERROR, tts_tag(), "engine info is NULL");
			return false;
		}

		if (0 != strcmp(engine_id, engine_info->uuid)) {
			iter = g_slist_next(iter);
			continue;
		}

		GSList *iter_voice = NULL;
		tts_config_voice_s* voice = NULL;

		if (g_slist_length(engine_info->voices) <= 0) {
			SLOG(LOG_ERROR, tts_tag(), "There is no voice : %s", engine_info->uuid);
			return -1;
		}

		/* Get a first item */
		iter_voice = g_slist_nth(engine_info->voices, 0);

		while (NULL != iter_voice) {
			voice = iter_voice->data;
			if (NULL != voice) {
				/* Default language */
				if (0 == strcmp(TTS_BASE_LANGUAGE, voice->language)) {
					*language = strdup(voice->language);
					*type = voice->type;

					SECURE_SLOG(LOG_DEBUG, tts_tag(), "Selected language(%s) type(%d)", *language, *type);
					return 0;
				}
			}
			iter_voice = g_slist_next(iter_voice);
		}

		/* Not support base language */
		if (NULL != voice) {
			*language = strdup(voice->language);
			*type = voice->type;

			SECURE_SLOG(LOG_DEBUG, tts_tag(), "Selected language(%s) type(%d)", *language, *type);
			return 0;
		}
		break;
	}

	return -1;
}

Eina_Bool tts_config_mgr_inotify_event_cb(void* data, Ecore_Fd_Handler *fd_handler)
{
	SLOG(LOG_DEBUG, tts_tag(), "===== Config changed callback event");

	int length;
	struct inotify_event event;
	memset(&event, '\0', sizeof(struct inotify_event));

	length = read(g_config_fd_noti, &event, sizeof(struct inotify_event));
	if (0 > length) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Empty Inotify event");
		SLOG(LOG_DEBUG, tts_tag(), "=====");
		SLOG(LOG_DEBUG, tts_tag(), " ");
		return ECORE_CALLBACK_DONE;
	}

	if (IN_CLOSE_WRITE == event.mask) {
		/* check config changed state */
		char* engine = NULL;
		char* setting = NULL;
		char* lang = NULL;
		bool auto_voice = g_config_info->auto_voice;
		int voice_type = -1;
		int speech_rate = -1;
		int pitch = -1;

		GSList *iter = NULL;
		tts_config_client_s* temp_client = NULL;

		if (0 != tts_parser_find_config_changed(&engine, &setting, &auto_voice, &lang, &voice_type, &speech_rate, &pitch))
			return ECORE_CALLBACK_PASS_ON;

		/* engine changed */
		if (NULL != engine || NULL != setting) {
			if (NULL != engine) {
				if (NULL != g_config_info->engine_id)
					free(g_config_info->engine_id);

				g_config_info->engine_id = strdup(engine);
			}
			if (NULL != setting) {
				if (NULL != g_config_info->setting)
					free(g_config_info->setting);

				g_config_info->setting = strdup(setting);
			}

			SECURE_SLOG(LOG_DEBUG, tts_tag(), "Engine change(%s)", g_config_info->engine_id);

			/* Call all callbacks of client*/
			iter = g_slist_nth(g_config_client_list, 0);

			while (NULL != iter) {
				temp_client = iter->data;

				if (NULL != temp_client) {
					if (NULL != temp_client->engine_cb) {
						SECURE_SLOG(LOG_DEBUG, tts_tag(), "Engine changed callback : uid(%d)", temp_client->uid);
						temp_client->engine_cb(g_config_info->engine_id, g_config_info->setting, 
							g_config_info->language, g_config_info->type, 
							g_config_info->auto_voice, temp_client->user_data);
					}
				}

				iter = g_slist_next(iter);
			}
		}

		if (auto_voice != g_config_info->auto_voice) {
			g_config_info->auto_voice = auto_voice;
		}

		if (NULL != lang || -1 != voice_type) {
			char* before_lang = NULL;
			int before_type;

			before_lang = strdup(g_config_info->language);
			before_type = g_config_info->type;

			if (NULL != lang) {
				if (NULL != g_config_info->language)
					free(g_config_info->language);

				g_config_info->language = strdup(lang);
			}
			if (-1 != voice_type) {
				g_config_info->type = voice_type;
			}

			SECURE_SLOG(LOG_DEBUG, tts_tag(), "Voice change(%s, %d)", g_config_info->language, g_config_info->type);

			/* Call all callbacks of client*/
			iter = g_slist_nth(g_config_client_list, 0);

			while (NULL != iter) {
				temp_client = iter->data;

				if (NULL != temp_client) {
					if (NULL != temp_client->voice_cb) {
						SECURE_SLOG(LOG_DEBUG, tts_tag(), "Voice changed callback : uid(%d)", temp_client->uid);
						temp_client->voice_cb(before_lang, before_type, 
							g_config_info->language, g_config_info->type, 
							g_config_info->auto_voice, temp_client->user_data);
					}
				}

				iter = g_slist_next(iter);
			}

			if (NULL != before_lang) {
				free(before_lang);
			}
		}

		if (-1 != speech_rate) {
			g_config_info->speech_rate = speech_rate;

			SECURE_SLOG(LOG_DEBUG, tts_tag(), "Speech rate change(%d)", g_config_info->speech_rate);

			/* Call all callbacks of client*/
			iter = g_slist_nth(g_config_client_list, 0);

			while (NULL != iter) {
				temp_client = iter->data;

				if (NULL != temp_client) {
					if (NULL != temp_client->speech_cb) {
						SECURE_SLOG(LOG_DEBUG, tts_tag(), "Speech rate changed callback : uid(%d)", temp_client->uid);
						temp_client->speech_cb(g_config_info->speech_rate, temp_client->user_data);
					}
				}

				iter = g_slist_next(iter);
			}
		}

		if (-1 != pitch) {
			g_config_info->pitch = pitch;

			SECURE_SLOG(LOG_DEBUG, tts_tag(), "pitch change(%d)", g_config_info->pitch);

			/* Call all callbacks of client*/
			iter = g_slist_nth(g_config_client_list, 0);

			while (NULL != iter) {
				temp_client = iter->data;

				if (NULL != temp_client) {
					if (NULL != temp_client->pitch_cb) {
						SECURE_SLOG(LOG_DEBUG, tts_tag(), "Pitch changed callback : uid(%d)", temp_client->uid);
						temp_client->pitch_cb(g_config_info->pitch, temp_client->user_data);
					}
				}

				iter = g_slist_next(iter);
			}
		}

		if (NULL != engine)	free(engine);
		if (NULL != setting)	free(setting);
		if (NULL != lang)	free(lang);
	} else {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Undefined event");
	}

	SLOG(LOG_DEBUG, tts_tag(), "=====");
	SLOG(LOG_DEBUG, tts_tag(), " ");

	return ECORE_CALLBACK_PASS_ON;
}

int __tts_config_mgr_register_config_event()
{
	/* get file notification handler */
	int fd;
	int wd;

	fd = inotify_init();
	if (fd < 0) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Fail get inotify fd");
		return -1;
	}
	g_config_fd_noti = fd;

	wd = inotify_add_watch(fd, TTS_CONFIG, IN_CLOSE_WRITE);
	g_config_wd_noti = wd;

	g_config_fd_handler_noti = ecore_main_fd_handler_add(fd, ECORE_FD_READ,
		(Ecore_Fd_Cb)tts_config_mgr_inotify_event_cb, NULL, NULL, NULL);
	if (NULL == g_config_fd_handler_noti) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Fail to get handler_noti");
		return -1;
	}

	/* Set non-blocking mode of file */
	int value;
	value = fcntl(fd, F_GETFL, 0);
	value |= O_NONBLOCK;

	if (0 > fcntl(fd, F_SETFL, value)) {
		SLOG(LOG_WARN, tts_tag(), "[WARNING] Fail to set non-block mode");
	}

	return 0;
}

int __tts_config_mgr_unregister_config_event()
{
	/* delete inotify variable */
	ecore_main_fd_handler_del(g_config_fd_handler_noti);
	inotify_rm_watch(g_config_fd_noti, g_config_wd_noti);
	close(g_config_fd_noti);

	return 0;
}

int __tts_config_set_auto_language()
{
	char* value = NULL;
	value = vconf_get_str(TTS_LANGSET_KEY);
	if (NULL == value) {
		SLOG(LOG_ERROR, tts_tag(), "[Config ERROR] Fail to get display language");
		return -1;
	}

	char temp_lang[6] = {'\0', };
	strncpy(temp_lang, value, 5);
	free(value);

	if (true == __tts_config_mgr_check_lang_is_valid(g_config_info->engine_id, temp_lang, g_config_info->type)) {
		/* tts default voice change */
		if (NULL == g_config_info->language) {
			SLOG(LOG_ERROR, tts_tag(), "Current config language is NULL");
			return -1;
		}

		char* before_lang = NULL;
		int before_type;

		if (0 != tts_parser_set_voice(temp_lang, g_config_info->type)) {
			SLOG(LOG_ERROR, tts_tag(), "Fail to save default voice");
			return -1;
		}

		before_lang = strdup(g_config_info->language);
		before_type = g_config_info->type;

		free(g_config_info->language);
		g_config_info->language = strdup(temp_lang);

		SECURE_SLOG(LOG_DEBUG, tts_tag(), "[Config] Default voice : lang(%s) type(%d)", 
			g_config_info->language, g_config_info->type);

		GSList *iter = NULL;
		tts_config_client_s* temp_client = NULL;

		/* Call all callbacks of client*/
		iter = g_slist_nth(g_config_client_list, 0);

		while (NULL != iter) {
			temp_client = iter->data;

			if (NULL != temp_client) {
				if (NULL != temp_client->voice_cb) {
					temp_client->voice_cb(before_lang, before_type, 
						g_config_info->language, g_config_info->type, 
						g_config_info->auto_voice, temp_client->user_data);
				}
			}

			iter = g_slist_next(iter);
		}

		if (NULL != before_lang) {
			free(before_lang);
		}
	} else {
		/* Display language is not valid */
		char* tmp_language = NULL;
		int tmp_type = -1;
		if (0 != __tts_config_mgr_select_lang(g_config_info->engine_id, &tmp_language, &tmp_type)) {
			SLOG(LOG_ERROR, tts_tag(), "[ERROR] Fail to select language");
			return -1;
		}

		if (NULL == tmp_language) {
			SLOG(LOG_ERROR, tts_tag(), "[ERROR] language is NULL");
			return -1;
		}

		if (0 != tts_parser_set_voice(tmp_language, tmp_type)) {
			SLOG(LOG_ERROR, tts_tag(), "[ERROR] Fail to save config");
			return -1;
		}

		SECURE_SLOG(LOG_DEBUG, tts_tag(), "[Config] Default voice : lang(%s) type(%d)", 
			tmp_language, tmp_type);

		GSList *iter = NULL;
		tts_config_client_s* temp_client = NULL;

		/* Call all callbacks of client*/
		iter = g_slist_nth(g_config_client_list, 0);

		while (NULL != iter) {
			temp_client = iter->data;

			if (NULL != temp_client) {
				if (NULL != temp_client->voice_cb) {
					temp_client->voice_cb(g_config_info->language, g_config_info->type, 
						tmp_language, tmp_type, g_config_info->auto_voice, temp_client->user_data);
				}
			}

			iter = g_slist_next(iter);
		}

		if (NULL != g_config_info->language) {
			free(g_config_info->language);
			g_config_info->language = strdup(tmp_language);
		}

		g_config_info->type = tmp_type;

		free(tmp_language);
	}

	return 0;
}

void __tts_config_display_language_changed_cb(keynode_t *key, void *data)
{
	if (true == g_config_info->auto_voice) {
		__tts_config_set_auto_language();
	}

	return;
}

void __tts_config_screen_reader_changed_cb(keynode_t *key, void *data)
{
	int ret;
	int screen_reader;
	ret = vconf_get_bool(TTS_ACCESSIBILITY_KEY, &screen_reader);
	if (0 != ret) {
		SLOG(LOG_ERROR, tts_tag(), "[Config ERROR] Fail to get screen reader");
		return;
	}

	GSList *iter = NULL;
	tts_config_client_s* temp_client = NULL;

	/* Call all callbacks of client*/
	iter = g_slist_nth(g_config_client_list, 0);

	while (NULL != iter) {
		temp_client = iter->data;

		if (NULL != temp_client) {
			if (NULL != temp_client->screen_cb) {
				temp_client->screen_cb((bool)screen_reader);
			}
		}

		iter = g_slist_next(iter);
	}
}

int __tts_config_release_client(int uid)
{
	GSList *iter = NULL;
	tts_config_client_s* temp_client = NULL;

	if (0 < g_slist_length(g_config_client_list)) {
		/* Check uid */
		iter = g_slist_nth(g_config_client_list, 0);

		while (NULL != iter) {
			temp_client = iter->data;

			if (NULL != temp_client) {
				if (uid == temp_client->uid) {
					g_config_client_list = g_slist_remove(g_config_client_list, temp_client);
					free(temp_client);
					temp_client = NULL;
					break;
				}
			}

			iter = g_slist_next(iter);
		}
	}

	SLOG(LOG_DEBUG, tts_tag(), "Client count (%d)", g_slist_length(g_config_client_list));

	return g_slist_length(g_config_client_list);
}

void __tts_config_release_engine()
{
	GSList *iter = NULL;
	tts_engine_info_s *engine_info = NULL;

	if (0 < g_slist_length(g_engine_list)) {

		/* Get a first item */
		iter = g_slist_nth(g_engine_list, 0);

		while (NULL != iter) {
			engine_info = iter->data;

			if (NULL != engine_info) {
				g_engine_list = g_slist_remove(g_engine_list, engine_info);

				tts_parser_free_engine_info(engine_info);
			}

			iter = g_slist_nth(g_engine_list, 0);
		}
	}

	return;
}

int __tts_config_mgr_get_engine_info()
{
	DIR *dp = NULL;
	int ret = -1;
	struct dirent entry;
	struct dirent *dirp = NULL;

	char filepath[512] = {'\0',};
	int filesize;
	tts_engine_info_s* info = NULL;

	__tts_config_release_engine();
	g_engine_list = NULL;
	__tts_config_mgr_unregister_engine_config_updated_event();

	/* Copy default info directory to download directory */
	dp  = opendir(TTS_DEFAULT_ENGINE_INFO);
	if (NULL == dp) {
		SLOG(LOG_DEBUG, tts_tag(), "[CONFIG] No downloadable directory : %s", TTS_DEFAULT_ENGINE_INFO);
	} else {
		do {
			ret = readdir_r(dp, &entry, &dirp);
			if (0 != ret) {
				SLOG(LOG_ERROR, tts_tag(), "[CONFIG] Fail to read directory");
				break;
			}

			if (NULL != dirp) {
				filesize = strlen(TTS_DEFAULT_ENGINE_INFO) + strlen(dirp->d_name) + 2;
				if (filesize >= 512) {
					SECURE_SLOG(LOG_ERROR, tts_tag(), "[CONFIG ERROR] File path is too long : %s", dirp->d_name);
					closedir(dp);
					return -1;
				}

				memset(filepath, '\0', 512);
				snprintf(filepath, 512, "%s/%s", TTS_DEFAULT_ENGINE_INFO, dirp->d_name);

				SECURE_SLOG(LOG_DEBUG, tts_tag(), "[CONFIG] Filepath(%s)", filepath);

				char dest[512] = {'\0',};
				snprintf(dest, 512, "%s/%s", TTS_DOWNLOAD_ENGINE_INFO, dirp->d_name);

				if (0 != access(dest, F_OK)) {
					if (0 != tts_parser_copy_xml(filepath, dest)) {
						SLOG(LOG_ERROR, tts_tag(), "[CONFIG ERROR] Fail to copy engine info");
					}
				}
			}
		} while (NULL != dirp);

		closedir(dp);
	}

	/* Get engine info from default engine directory */
	dp  = opendir(TTS_DOWNLOAD_ENGINE_INFO);
	if (NULL == dp) {
		SLOG(LOG_DEBUG, tts_tag(), "[CONFIG] No downloadable directory : %s", TTS_DEFAULT_ENGINE_INFO);
	} else {
		do {
			ret = readdir_r(dp, &entry, &dirp);
			if (0 != ret) {
				SLOG(LOG_ERROR, tts_tag(), "[CONFIG] Fail to read directory");
				break;
			}

			if (NULL != dirp) {
				filesize = strlen(TTS_DOWNLOAD_ENGINE_INFO) + strlen(dirp->d_name) + 2;
				if (filesize >= 512) {
					SECURE_SLOG(LOG_ERROR, tts_tag(), "[CONFIG ERROR] File path is too long : %s", dirp->d_name);
					closedir(dp);
					return -1;
				}

				memset(filepath, '\0', 512);
				snprintf(filepath, 512, "%s/%s", TTS_DOWNLOAD_ENGINE_INFO, dirp->d_name);

				SECURE_SLOG(LOG_DEBUG, tts_tag(), "[CONFIG] Filepath(%s)", filepath);

				if (0 == tts_parser_get_engine_info(filepath, &info)) {
					g_engine_list = g_slist_append(g_engine_list, info);
					if (0 != __tts_config_mgr_register_engine_config_updated_event(filepath)) {
						SLOG(LOG_ERROR, tts_tag(), "[ERROR] Fail to register engine config updated event");
					}
				}
			}
		} while (NULL != dirp);

		closedir(dp);
	}

	if (0 >= g_slist_length(g_engine_list)) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] No engine");
		return -1;
	}

	return 0;
}

static Eina_Bool __tts_config_mgr_engine_config_inotify_event_callback(void* data, Ecore_Fd_Handler *fd_handler)
{
	SLOG(LOG_DEBUG, tts_tag(), "===== Engine config updated callback event");

	tts_engine_inotify_s *ino = (tts_engine_inotify_s *)data;
	int dir_fd = ino->dir_fd;

	int length;
	struct inotify_event event;
	memset(&event, '\0', sizeof(struct inotify_event));

	length = read(dir_fd, &event, sizeof(struct inotify_event));
	if (0 > length) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Empty Inotify event");
		SLOG(LOG_DEBUG, tts_tag(), "=====");
		SLOG(LOG_DEBUG, tts_tag(), " ");
		return ECORE_CALLBACK_DONE;
	}

	if (IN_CLOSE_WRITE == event.mask) {
		int ret = __tts_config_mgr_get_engine_info();
		if (0 != ret) {
			SLOG(LOG_ERROR, tts_tag(), "[ERROR] Fail to get engine info when config updated");
		}
		__tts_config_mgr_print_engine_info();
		bool support = tts_config_check_default_voice_is_valid(g_config_info->language, g_config_info->type);
		if (false == support) {
			SLOG(LOG_DEBUG, tts_tag(), "[ERROR] Default voice is valid");
			char* temp_lang = NULL;
			int temp_type;
			ret = __tts_config_mgr_select_lang(g_config_info->engine_id, &temp_lang, &temp_type);
			if (0 != ret) {
				SLOG(LOG_ERROR, tts_tag(), "[ERROR] Fail to get voice");
			}

			ret = tts_config_mgr_set_voice(temp_lang, temp_type);
			if (0 != ret) {
				SLOG(LOG_ERROR, tts_tag(), "[ERROR] Fail to set voice");
			} else {
				SLOG(LOG_DEBUG, tts_tag(), "[DEBUG] Saved default voice : lang(%s), type(%d)", g_config_info->language, g_config_info->type);
			}
			if (NULL != temp_lang)	free(temp_lang);
		}

		GSList *iter = NULL;
		tts_config_client_s* temp_client = NULL;
		/* Call all callbacks of client*/
		iter = g_slist_nth(g_config_client_list, 0);

		while (NULL != iter) {
			temp_client = iter->data;

			if (NULL != temp_client) {
				if (NULL != temp_client->engine_cb) {
					SECURE_SLOG(LOG_DEBUG, tts_tag(), "Engine changed callback : uid(%d)", temp_client->uid);
					temp_client->engine_cb(g_config_info->engine_id, g_config_info->setting, 
						g_config_info->language, g_config_info->type, 
						g_config_info->auto_voice, temp_client->user_data);
				}
			}

			iter = g_slist_next(iter);
		}
	} else {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Undefined event");
	}

	SLOG(LOG_DEBUG, tts_tag(), "=====");
	SLOG(LOG_DEBUG, tts_tag(), " ");

	return ECORE_CALLBACK_PASS_ON;
}

static int __tts_config_mgr_register_engine_config_updated_event(const char* path)
{
	/* For engine directory monitoring */
	tts_engine_inotify_s *ino = (tts_engine_inotify_s *)calloc(1, sizeof(tts_engine_inotify_s));

	ino->dir_fd = inotify_init();
	if (ino->dir_fd < 0) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Fail to init inotify");
		return -1;
	}

	 /* FIX_ME *//* It doesn't need check engine directory, because daemon will change engine-process */
	ino->dir_wd = inotify_add_watch(ino->dir_fd, path, IN_CLOSE_WRITE);
	SLOG(LOG_DEBUG, tts_tag(), "Add inotify watch(%s)", path);
	if (ino->dir_wd < 0) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Fail to add watch");
		return -1;
	}

	ino->dir_fd_handler = ecore_main_fd_handler_add(ino->dir_fd, ECORE_FD_READ, (Ecore_Fd_Cb)__tts_config_mgr_engine_config_inotify_event_callback, (void *)ino, NULL, NULL);
	if (NULL == ino->dir_fd_handler) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Fail to add fd handler");
		return -1;
	}

	/* Set non-blocking mode of file */
	int value;
	value = fcntl(ino->dir_fd, F_GETFL, 0);
	value |= O_NONBLOCK;

	if (0 > fcntl(ino->dir_fd, F_SETFL, value)) {
		SLOG(LOG_WARN, tts_tag(), "[WARNING] Fail to set non-block mode");
	}

	g_ino_list = g_list_append(g_ino_list, ino);

	return 0;
}

static int __tts_config_mgr_unregister_engine_config_updated_event()
{
	/* delete all inotify variable */
	if (0 < g_list_length(g_ino_list)) {
		GList *iter = NULL;
		iter = g_list_first(g_ino_list);
		
		while (NULL != iter) {
			tts_engine_inotify_s *tmp = iter->data;
			
			if (NULL != tmp) {
				ecore_main_fd_handler_del(tmp->dir_fd_handler);
				inotify_rm_watch(tmp->dir_fd, tmp->dir_wd);
				close(tmp->dir_fd);

				free(tmp);
			}

			g_ino_list = g_list_remove_link(g_ino_list, iter);

			iter = g_list_first(g_ino_list);
		}
	}

	return 0;
}

int tts_config_mgr_initialize(int uid)
{
	GSList *iter = NULL;
	int* get_uid;
	tts_config_client_s* temp_client = NULL;

	/* Register uid */
	if (0 < g_slist_length(g_config_client_list)) {
		/* Check uid */
		iter = g_slist_nth(g_config_client_list, 0);

		while (NULL != iter) {
			get_uid = iter->data;

			if (uid == *get_uid) {
				SECURE_SLOG(LOG_WARN, tts_tag(), "[CONFIG] uid(%d) has already registered", uid);
				return 0;
			}

			iter = g_slist_next(iter);
		}

		temp_client = (tts_config_client_s*)calloc(1, sizeof(tts_config_client_s));
		if (NULL == temp_client) {
			SLOG(LOG_ERROR, tts_tag(), "[ERROR] Fail to allocate memory");
			return TTS_CONFIG_ERROR_OUT_OF_MEMORY;
		}
		temp_client->uid = uid;

		g_config_client_list = g_slist_append(g_config_client_list, temp_client);

		SECURE_SLOG(LOG_WARN, tts_tag(), "[CONFIG] Add uid(%d) but config has already initialized", uid);
		return 0;
	} else {
		temp_client = (tts_config_client_s*)calloc(1, sizeof(tts_config_client_s));
		if (NULL == temp_client) {
			SLOG(LOG_ERROR, tts_tag(), "[ERROR] Fail to allocate memory");
			return TTS_CONFIG_ERROR_OUT_OF_MEMORY;
		}
		temp_client->uid = uid;

		g_config_client_list = g_slist_append(g_config_client_list, temp_client);
	}

	if (0 != access(TTS_CONFIG_BASE, F_OK)) {
		if (0 != mkdir(TTS_CONFIG_BASE, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)) {
			SLOG(LOG_ERROR, tts_tag(), "[ERROR] Fail to make directory : %s", TTS_CONFIG_BASE);
			return -1;
		} else {
			SLOG(LOG_DEBUG, tts_tag(), "Success to make directory : %s", TTS_CONFIG_BASE);
		}
	}

	if (0 != access(TTS_DOWNLOAD_BASE, F_OK)) {
		if (0 != mkdir(TTS_DOWNLOAD_BASE, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)) {
			SLOG(LOG_ERROR, tts_tag(), "[ERROR] Fail to make directory : %s", TTS_DOWNLOAD_BASE);
			return -1;
		} else {
			SLOG(LOG_DEBUG, tts_tag(), "Success to make directory : %s", TTS_DOWNLOAD_BASE);
		}
	}

	if (0 != access(TTS_DOWNLOAD_ENGINE_INFO, F_OK)) {
		if (0 != mkdir(TTS_DOWNLOAD_ENGINE_INFO, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)) {
			SLOG(LOG_ERROR, tts_tag(), "[ERROR] Fail to make directory : %s", TTS_DOWNLOAD_ENGINE_INFO);
			return -1;
		} else {
			SLOG(LOG_DEBUG, tts_tag(), "Success to make directory : %s", TTS_DOWNLOAD_ENGINE_INFO);
		}
	}

	if (0 != __tts_config_mgr_get_engine_info()) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Fail to get engine info");
		__tts_config_release_client(uid);
		__tts_config_release_engine();
		return TTS_CONFIG_ERROR_ENGINE_NOT_FOUND;
	}

	__tts_config_mgr_print_engine_info();

	if (0 != tts_parser_load_config(&g_config_info)) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Fail to parse configure information");
		__tts_config_release_client(uid);
		__tts_config_release_engine();
		return TTS_CONFIG_ERROR_OPERATION_FAILED;
	}

	/* Check whether engine id is valid */
	if (0 != __tts_config_mgr_check_engine_is_valid(g_config_info->engine_id)) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Fail to get default engine");
		__tts_config_release_client(uid);
		__tts_config_release_engine();
		tts_parser_unload_config(g_config_info);
		return TTS_CONFIG_ERROR_ENGINE_NOT_FOUND;
	}

	if (true == g_config_info->auto_voice) {
		/* Check language with display language */
		__tts_config_set_auto_language();
	} else {
		if (false == __tts_config_mgr_check_lang_is_valid(g_config_info->engine_id, g_config_info->language, g_config_info->type)) {
			/* Default language is not valid */
			char* tmp_language = NULL;
			int tmp_type = -1;
			if (0 != __tts_config_mgr_select_lang(g_config_info->engine_id, &tmp_language, &tmp_type)) {
				SLOG(LOG_ERROR, tts_tag(), "[ERROR] Fail to select language");
				__tts_config_release_client(uid);
				__tts_config_release_engine();
				tts_parser_unload_config(g_config_info);
				return TTS_CONFIG_ERROR_OPERATION_FAILED;
			}

			if (NULL != tmp_language) {
				if (NULL != g_config_info->language) {
					free(g_config_info->language);
					g_config_info->language = strdup(tmp_language);
				}

				g_config_info->type = tmp_type;

				free(tmp_language);

				if (0 != tts_parser_set_voice(g_config_info->language, g_config_info->type)) {
					SLOG(LOG_ERROR, tts_tag(), "[ERROR] Fail to save config");
					__tts_config_release_client(uid);
					__tts_config_release_engine();
					tts_parser_unload_config(g_config_info);
					return TTS_CONFIG_ERROR_OPERATION_FAILED;
				}
			}
		}
	}

	/* print daemon config */
	SLOG(LOG_DEBUG, tts_tag(), "== TTS config ==");
	SECURE_SLOG(LOG_DEBUG, tts_tag(), " engine : %s", g_config_info->engine_id);
	SECURE_SLOG(LOG_DEBUG, tts_tag(), " setting : %s", g_config_info->setting);
	SECURE_SLOG(LOG_DEBUG, tts_tag(), " auto voice : %s", g_config_info->auto_voice ? "on" : "off");
	SECURE_SLOG(LOG_DEBUG, tts_tag(), " language : %s", g_config_info->language);
	SECURE_SLOG(LOG_DEBUG, tts_tag(), " voice type : %d", g_config_info->type);
	SECURE_SLOG(LOG_DEBUG, tts_tag(), " speech rate : %d", g_config_info->speech_rate);
	SECURE_SLOG(LOG_DEBUG, tts_tag(), " pitch : %d", g_config_info->pitch);
	SLOG(LOG_DEBUG, tts_tag(), "=================");

	if (0 != __tts_config_mgr_register_config_event()) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Fail to register config event");
		__tts_config_release_client(uid);
		__tts_config_release_engine();
		tts_parser_unload_config(g_config_info);
		return TTS_CONFIG_ERROR_OPERATION_FAILED;
	}

	/* Register to detect display language change */
	vconf_notify_key_changed(TTS_LANGSET_KEY, __tts_config_display_language_changed_cb, NULL);
	vconf_notify_key_changed(TTS_ACCESSIBILITY_KEY, __tts_config_screen_reader_changed_cb, NULL);

	/* For engine directory monitoring */
	//if (0 != __tts_config_mgr_register_engine_config_updated_event()) {
	//	SLOG(LOG_ERROR, tts_tag(), "[ERROR] Fail to register engine config updated event");
	//	__tts_config_release_client(uid);
	//	__tts_config_release_engine();
	//	tts_parser_unload_config(g_config_info);
	//	__tts_config_mgr_unregister_config_event();
	//	return TTS_CONFIG_ERROR_OPERATION_FAILED;
	//}

	return 0;
}

int tts_config_mgr_finalize(int uid)
{
	if (0 < __tts_config_release_client(uid)) {
		return 0;
	}

	tts_config_mgr_unset_callback(uid);

	__tts_config_release_engine();

	tts_parser_unload_config(g_config_info);

	__tts_config_mgr_unregister_engine_config_updated_event();

	__tts_config_mgr_unregister_config_event();

	vconf_ignore_key_changed(TTS_LANGSET_KEY, __tts_config_display_language_changed_cb);
	vconf_ignore_key_changed(TTS_ACCESSIBILITY_KEY, __tts_config_screen_reader_changed_cb);

	return 0;
}

int tts_config_mgr_set_callback(int uid, 
				tts_config_engine_changed_cb engine_cb, 
				tts_config_voice_changed_cb voice_cb, 
				tts_config_speech_rate_changed_cb speech_cb, 
				tts_config_pitch_changed_cb pitch_cb,
				void* user_data)
{
	GSList *iter = NULL;
	tts_config_client_s* temp_client = NULL;

	/* Call all callbacks of client*/
	iter = g_slist_nth(g_config_client_list, 0);

	while (NULL != iter) {
		temp_client = iter->data;

		if (NULL != temp_client) {
			if (uid == temp_client->uid) {
				temp_client->engine_cb = engine_cb;
				temp_client->voice_cb = voice_cb;
				temp_client->speech_cb = speech_cb;
				temp_client->pitch_cb = pitch_cb;
				temp_client->user_data = user_data;
			}
		}

		iter = g_slist_next(iter);
	}
	return 0;
}

int tts_config_mgr_unset_callback(int uid)
{
	GSList *iter = NULL;
	tts_config_client_s* temp_client = NULL;

	/* Call all callbacks of client*/
	iter = g_slist_nth(g_config_client_list, 0);

	while (NULL != iter) {
		temp_client = iter->data;

		if (NULL != temp_client) {
			if (uid == temp_client->uid) {
				temp_client->engine_cb = NULL;
				temp_client->voice_cb = NULL;
				temp_client->speech_cb = NULL;
				temp_client->pitch_cb = NULL;
				temp_client->user_data = NULL;
			}
		}

		iter = g_slist_next(iter);
	}

	return 0;
}

int tts_config_set_screen_reader_callback(int uid, tts_config_screen_reader_changed_cb callback)
{
	if (NULL == callback) {
		SLOG(LOG_ERROR, tts_tag(), "Input parameter is NULL");
		return TTS_CONFIG_ERROR_INVALID_PARAMETER;
	}

	GSList *iter = NULL;
	tts_config_client_s* temp_client = NULL;

	/* Call all callbacks of client*/
	iter = g_slist_nth(g_config_client_list, 0);

	while (NULL != iter) {
		temp_client = iter->data;

		if (NULL != temp_client) {
			if (uid == temp_client->uid) {
				temp_client->screen_cb = callback;
			}
		}

		iter = g_slist_next(iter);
	}
	return 0;
}

int tts_config_unset_screen_reader_callback(int uid)
{
	GSList *iter = NULL;
	tts_config_client_s* temp_client = NULL;

	/* Call all callbacks of client*/
	iter = g_slist_nth(g_config_client_list, 0);

	while (NULL != iter) {
		temp_client = iter->data;

		if (NULL != temp_client) {
			if (uid == temp_client->uid) {
				temp_client->screen_cb = NULL;
			}
		}

		iter = g_slist_next(iter);
	}
	return 0;
}


int tts_config_mgr_get_engine_list(tts_config_supported_engine_cb callback, void* user_data)
{
	if (0 >= g_slist_length(g_config_client_list)) {
		SLOG(LOG_ERROR, tts_tag(), "Not initialized");
		return TTS_CONFIG_ERROR_INVALID_STATE;
	}

	GSList *iter = NULL;
	tts_engine_info_s *engine_info = NULL;

	if (0 >= g_slist_length(g_engine_list)) {
		SLOG(LOG_ERROR, tts_tag(), "There is no engine!!");
		return TTS_CONFIG_ERROR_ENGINE_NOT_FOUND;
	}

	/* Get a first item */
	iter = g_slist_nth(g_engine_list, 0);

	while (NULL != iter) {
		engine_info = iter->data;

		if (NULL != engine_info) {
			if (false == callback(engine_info->uuid, engine_info->name, engine_info->setting, user_data)) {
				break;
			}
		}

		iter = g_slist_next(iter);
	}

	return 0;
}

int tts_config_mgr_get_engine(char** engine)
{
	if (0 >= g_slist_length(g_config_client_list)) {
		SLOG(LOG_ERROR, tts_tag(), "Not initialized");
		return TTS_CONFIG_ERROR_INVALID_STATE;
	}

	if (NULL == engine) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Input parameter is NULL");
		return TTS_CONFIG_ERROR_INVALID_PARAMETER;
	}

	if (NULL != g_config_info->engine_id) {
		/* Check engine id is valid */
		GSList *iter = NULL;
		tts_engine_info_s *engine_info = NULL;

		if (0 >= g_slist_length(g_engine_list)) {
			SLOG(LOG_ERROR, tts_tag(), "[ERROR] There is no engine!!");
			return TTS_CONFIG_ERROR_ENGINE_NOT_FOUND;
		}

		/* Get a first item */
		iter = g_slist_nth(g_engine_list, 0);

		while (NULL != iter) {
			engine_info = iter->data;

			if (NULL != engine_info) {
				if (0 == strcmp(engine_info->uuid, g_config_info->engine_id)) {
					*engine = strdup(g_config_info->engine_id);
					return 0;
				}
			}
			iter = g_slist_next(iter);
		}

		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Current engine id is not valid");
	} else {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Engine id is NULL");
	}

	return TTS_CONFIG_ERROR_OPERATION_FAILED;
}

int tts_config_mgr_set_engine(const char* engine)
{
	if (0 >= g_slist_length(g_config_client_list)) {
		SLOG(LOG_ERROR, tts_tag(), "Not initialized");
		return TTS_CONFIG_ERROR_INVALID_STATE;
	}

	if (NULL == engine)
		return TTS_CONFIG_ERROR_INVALID_PARAMETER;

	/* Check current engine id with new engine id */
	if (NULL != g_config_info->engine_id) {
		if (0 == strcmp(g_config_info->engine_id, engine))
			return 0;
	}

	if (0 >= g_slist_length(g_engine_list)) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] There is no engine!!");
		return TTS_CONFIG_ERROR_ENGINE_NOT_FOUND;
	}

	SLOG(LOG_DEBUG, tts_tag(), "New engine id : %s", engine);

	GSList *iter = NULL;
	tts_engine_info_s *engine_info = NULL;
	bool is_valid_engine = false;

	/* Get a first item */
	iter = g_slist_nth(g_engine_list, 0);

	while (NULL != iter) {
		engine_info = iter->data;

		if (NULL == engine_info) {
			SLOG(LOG_ERROR, tts_tag(), "[ERROR] Engine info is NULL");
			iter = g_slist_next(iter);
			continue;
		}

		/* Check engine id is valid */
		if (0 != strcmp(engine, engine_info->uuid)) {
			iter = g_slist_next(iter);
			continue;
		}

		if (NULL != g_config_info->engine_id)
			free(g_config_info->engine_id);

		g_config_info->engine_id = strdup(engine);

		if (NULL != g_config_info->setting)
			free(g_config_info->setting);

		if (NULL != engine_info->setting)
			g_config_info->setting = strdup(engine_info->setting);

		/* Engine is valid*/
		GSList *iter_voice = NULL;
		tts_config_voice_s* voice = NULL;
		bool is_valid_voice = false;

		/* Get a first item */
		iter_voice = g_slist_nth(engine_info->voices, 0);

		while (NULL != iter_voice) {
			/*Get handle data from list*/
			voice = iter_voice->data;

			if (NULL != voice) {
				if (NULL == voice->language)
					continue;
				SLOG(LOG_DEBUG, tts_tag(), " lang(%s) type(%d)", voice->language, voice->type);

				if (0 == strcmp(voice->language, g_config_info->language)) {
					if (voice->type == g_config_info->type) {
						/* language is valid */
						is_valid_voice = true;
						g_config_info->type = voice->type;
					}
					break;
				}
			}

			/*Get next item*/
			iter_voice = g_slist_next(iter_voice);
		}

		if (false == is_valid_voice) {
			if (NULL != g_config_info->language) {
				free(g_config_info->language);

				iter_voice = g_slist_nth(engine_info->voices, 0);
				if (NULL != iter_voice) {
					voice = iter_voice->data;
					if (NULL != voice) {
						if (NULL != voice->language)
							g_config_info->language = strdup(voice->language);
						g_config_info->type = voice->type;
					}
				}
			}
		}

		is_valid_engine = true;
		break;
	}

	if (true == is_valid_engine) {
		SLOG(LOG_DEBUG, tts_tag(), "[Config] Engine changed");
		SECURE_SLOG(LOG_DEBUG, tts_tag(), "  Engine : %s", g_config_info->engine_id);
		SECURE_SLOG(LOG_DEBUG, tts_tag(), "  Setting : %s", g_config_info->setting);
		SECURE_SLOG(LOG_DEBUG, tts_tag(), "  Language : %s", g_config_info->language);
		SECURE_SLOG(LOG_DEBUG, tts_tag(), "  Type : %d", g_config_info->type);

		if (0 != tts_parser_set_engine(g_config_info->engine_id, g_config_info->setting,
			g_config_info->language, g_config_info->type)) {
				SLOG(LOG_ERROR, tts_tag(), " Fail to save config");
				return TTS_CONFIG_ERROR_OPERATION_FAILED;
		}
	} else {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Engine id is not valid");
		return TTS_CONFIG_ERROR_OPERATION_FAILED;
	}

	return 0;
}

int tts_config_mgr_get_voice_list(const char* engine_id, tts_config_supported_voice_cb callback, void* user_data)
{
	if (0 >= g_slist_length(g_config_client_list)) {
		SLOG(LOG_ERROR, tts_tag(), "Not initialized");
		return TTS_CONFIG_ERROR_INVALID_STATE;
	}

	if (0 >= g_slist_length(g_engine_list)) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] There is no engine");
		return TTS_CONFIG_ERROR_ENGINE_NOT_FOUND;
	}

	GSList *iter = NULL;
	tts_engine_info_s *engine_info = NULL;

	/* Get a first item */
	iter = g_slist_nth(g_engine_list, 0);

	while (NULL != iter) {
		engine_info = iter->data;

		if (NULL == engine_info) {
			SLOG(LOG_ERROR, tts_tag(), "[ERROR] engine info is NULL");
			return TTS_CONFIG_ERROR_OPERATION_FAILED;
		}

		if (0 != strcmp(engine_id, engine_info->uuid)) {
			iter = g_slist_next(iter);
			continue;
		}

		GSList *iter_voice = NULL;
		tts_config_voice_s* voice = NULL;

		/* Get a first item */
		iter_voice = g_slist_nth(engine_info->voices, 0);

		while (NULL != iter_voice) {
			/*Get handle data from list*/
			voice = iter_voice->data;

			SLOG(LOG_DEBUG, tts_tag(), " lang(%s) type(%d)", voice->language, voice->type);
			if (NULL != voice->language) {
				if (false == callback(engine_info->uuid, voice->language, voice->type, user_data))
					break;
			}

			/*Get next item*/
			iter_voice = g_slist_next(iter_voice);
		}
		break;
	}

	return 0;
}

int tts_config_mgr_get_voice(char** language, int* type)
{
	if (0 >= g_slist_length(g_config_client_list)) {
		SLOG(LOG_ERROR, tts_tag(), "Not initialized");
		return TTS_CONFIG_ERROR_INVALID_PARAMETER;
	}

	if (0 >= g_slist_length(g_engine_list)) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] There is no engine");
		return TTS_CONFIG_ERROR_ENGINE_NOT_FOUND;
	}

	if (NULL == language || NULL == type)
		return TTS_CONFIG_ERROR_INVALID_PARAMETER;

	if (NULL != g_config_info->language) {
		*language = strdup(g_config_info->language);
		*type = g_config_info->type;
	} else {
		SLOG(LOG_ERROR, tts_tag(), "language is NULL");
		return TTS_CONFIG_ERROR_OPERATION_FAILED;
	}

	return 0;
}

int tts_config_mgr_set_voice(const char* language, int type)
{
	if (0 >= g_slist_length(g_config_client_list)) {
		SLOG(LOG_ERROR, tts_tag(), "Not initialized");
		return TTS_CONFIG_ERROR_INVALID_PARAMETER;
	}

	if (NULL == language) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Input parameter is NULL");
		return TTS_CONFIG_ERROR_INVALID_PARAMETER;
	}

	if (0 >= g_slist_length(g_engine_list)) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] There is no engine");
		return TTS_CONFIG_ERROR_ENGINE_NOT_FOUND;
	}

	/* Check language is valid */
	if (NULL != g_config_info->language) {
		if (0 != tts_parser_set_voice(language, type)) {
			SLOG(LOG_ERROR, tts_tag(), "Fail to save default voice");
			return TTS_CONFIG_ERROR_OPERATION_FAILED;
		}
		free(g_config_info->language);
		g_config_info->language = strdup(language);
		g_config_info->type = type;

	} else {
		SLOG(LOG_ERROR, tts_tag(), "language is NULL");
		return TTS_CONFIG_ERROR_OPERATION_FAILED;
	}

	return 0;
}

int tts_config_mgr_get_auto_voice(bool* value)
{
	if (0 >= g_slist_length(g_config_client_list)) {
		SLOG(LOG_ERROR, tts_tag(), "Not initialized");
		return TTS_CONFIG_ERROR_INVALID_PARAMETER;
	}

	if (NULL == value)
		return TTS_CONFIG_ERROR_INVALID_PARAMETER;

	*value = g_config_info->auto_voice;

	return 0;
}

int tts_config_mgr_set_auto_voice(bool value)
{
	if (0 >= g_slist_length(g_config_client_list)) {
		SLOG(LOG_ERROR, tts_tag(), "Not initialized");
		return TTS_CONFIG_ERROR_INVALID_PARAMETER;
	}

	if (g_config_info->auto_voice != value) {
		/* Check language is valid */
		if (0 != tts_parser_set_auto_voice(value)) {
			SLOG(LOG_ERROR, tts_tag(), "Fail to save auto voice option");
			return TTS_CONFIG_ERROR_OPERATION_FAILED;
		}
		g_config_info->auto_voice = value;

		if (true == g_config_info->auto_voice) {
			__tts_config_set_auto_language();
		}
	}

	return 0;
}

int tts_config_mgr_get_speech_rate(int* value)
{
	if (0 >= g_slist_length(g_config_client_list)) {
		SLOG(LOG_ERROR, tts_tag(), "Not initialized");
		return TTS_CONFIG_ERROR_INVALID_PARAMETER;
	}

	if (NULL == value) {
		return TTS_CONFIG_ERROR_INVALID_PARAMETER;
	}

	*value = g_config_info->speech_rate;

	return 0;
}

int tts_config_mgr_set_speech_rate(int value)
{
	if (0 >= g_slist_length(g_config_client_list)) {
		SLOG(LOG_ERROR, tts_tag(), "Not initialized");
		return TTS_CONFIG_ERROR_INVALID_PARAMETER;
	}

	if (TTS_CONFIG_SPEED_MIN <= value && value <= TTS_CONFIG_SPEED_MAX) {
		SLOG(LOG_DEBUG, tts_tag(), "[Config] Set speech rate : %d", value);
		if (0 != tts_parser_set_speech_rate(value)) {
			SLOG(LOG_ERROR, tts_tag(), "Fail to save speech rate");
			return TTS_CONFIG_ERROR_OPERATION_FAILED;
		}

		g_config_info->speech_rate = value;
	} else {
		SLOG(LOG_ERROR, tts_tag(), "[Config ERROR] Speech rate is invalid : %d", value);
	}

	return 0;
}

int tts_config_mgr_get_pitch(int* value)
{
	if (0 >= g_slist_length(g_config_client_list)) {
		SLOG(LOG_ERROR, tts_tag(), "Not initialized");
		return TTS_CONFIG_ERROR_INVALID_PARAMETER;
	}

	if (NULL == value) {
		return TTS_CONFIG_ERROR_INVALID_PARAMETER;
	}

	GSList *iter = NULL;
	tts_engine_info_s *engine_info = NULL;

	if (0 >= g_slist_length(g_engine_list)) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] There is no engine!!");
		return TTS_CONFIG_ERROR_ENGINE_NOT_FOUND;
	}

	/* Get a first item */
	iter = g_slist_nth(g_engine_list, 0);

	while (NULL != iter) {
		engine_info = iter->data;

		if (NULL == engine_info) {
			SLOG(LOG_ERROR, tts_tag(), "engine info is NULL");
			return TTS_CONFIG_ERROR_OPERATION_FAILED;
		}

		if (0 != strcmp(g_config_info->engine_id, engine_info->uuid)) {
			iter = g_slist_next(iter);
			continue;
		}

		if (false == engine_info->pitch_support) {
			return TTS_CONFIG_ERROR_NOT_SUPPORTED_FEATURE;
		}  else {
			break;
		}
	}

	*value = g_config_info->pitch;

	return 0;
}

int tts_config_mgr_set_pitch(int value)
{
	if (0 >= g_slist_length(g_config_client_list)) {
		SLOG(LOG_ERROR, tts_tag(), "Not initialized");
		return -1;
	}

	GSList *iter = NULL;
	tts_engine_info_s *engine_info = NULL;

	if (0 >= g_slist_length(g_engine_list)) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] There is no engine!!");
		return TTS_CONFIG_ERROR_ENGINE_NOT_FOUND;
	}

	/* Get a first item */
	iter = g_slist_nth(g_engine_list, 0);

	while (NULL != iter) {
		engine_info = iter->data;

		if (NULL == engine_info) {
			SLOG(LOG_ERROR, tts_tag(), "engine info is NULL");
			return TTS_CONFIG_ERROR_OPERATION_FAILED;
		}

		if (0 != strcmp(g_config_info->engine_id, engine_info->uuid)) {
			iter = g_slist_next(iter);
			continue;
		}

		if (false == engine_info->pitch_support) {
			return TTS_CONFIG_ERROR_NOT_SUPPORTED_FEATURE;
		} else {
			break;
		}
	}

	if (0 != tts_parser_set_pitch(value)) {
		SLOG(LOG_ERROR, tts_tag(), "Fail to save speech rate");
		return TTS_CONFIG_ERROR_OPERATION_FAILED;
	}

	g_config_info->pitch = value;

	return 0;
}

bool tts_config_check_default_engine_is_valid(const char* engine)
{
	if (0 >= g_slist_length(g_config_client_list)) {
		SLOG(LOG_ERROR, tts_tag(), "Not initialized");
		return -1;
	}

	if (NULL == engine)
		return false;

	if (0 >= g_slist_length(g_engine_list))
		return false;

	GSList *iter = NULL;
	tts_engine_info_s *engine_info = NULL;

	/* Get a first item */
	iter = g_slist_nth(g_engine_list, 0);

	while (NULL != iter) {
		engine_info = iter->data;

		if (NULL != engine_info) {
			if (0 == strcmp(engine, engine_info->uuid)) {
				return true;
			}
		}
		iter = g_slist_next(iter);
	}

	return false;
}

bool tts_config_check_default_voice_is_valid(const char* language, int type)
{
	if (0 >= g_slist_length(g_config_client_list)) {
		SLOG(LOG_ERROR, tts_tag(), "Not initialized");
		return -1;
	}

	if (NULL == language)
		return false;

	if (NULL == g_config_info->engine_id) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Default engine id is NULL");
		return false;
	}

	if (0 >= g_slist_length(g_engine_list))
		return false;

	GSList *iter = NULL;
	tts_engine_info_s *engine_info = NULL;

	/* Get a first item */
	iter = g_slist_nth(g_engine_list, 0);

	while (NULL != iter) {
		engine_info = iter->data;

		if (NULL == engine_info) {
			SLOG(LOG_ERROR, tts_tag(), "[ERROR] Engine info is NULL");
			iter = g_slist_next(iter);
			continue;
		}

		if (0 != strcmp(g_config_info->engine_id, engine_info->uuid)) {
			iter = g_slist_next(iter);
			continue;
		}

		GSList *iter_voice = NULL;
		tts_config_voice_s* voice = NULL;

		/* Get a first item */
		iter_voice = g_slist_nth(engine_info->voices, 0);

		while (NULL != iter_voice) {
			voice = iter_voice->data;

			if (0 == strcmp(language, voice->language) && voice->type == type)
				return true;

			/*Get next item*/
			iter_voice = g_slist_next(iter_voice);
		}

		break;
	}

	return false;
}


int __tts_config_mgr_print_engine_info()
{
	GSList *iter = NULL;
	tts_engine_info_s *engine_info = NULL;

	if (0 >= g_slist_length(g_engine_list)) {
		SLOG(LOG_DEBUG, tts_tag(), "-------------- engine list -----------------");
		SLOG(LOG_DEBUG, tts_tag(), "  No Engine in engine directory");
		SLOG(LOG_DEBUG, tts_tag(), "--------------------------------------------");
		return 0;
	}

	/* Get a first item */
	iter = g_slist_nth(g_engine_list, 0);

	SLOG(LOG_DEBUG, tts_tag(), "--------------- engine list -----------------");

	int i = 1;
	while (NULL != iter) {
		engine_info = iter->data;

		SLOG(LOG_DEBUG, tts_tag(), "[%dth]", i);
		SLOG(LOG_DEBUG, tts_tag(), " name : %s", engine_info->name);
		SLOG(LOG_DEBUG, tts_tag(), " id   : %s", engine_info->uuid);
		SLOG(LOG_DEBUG, tts_tag(), " setting : %s", engine_info->setting);

		SLOG(LOG_DEBUG, tts_tag(), " Voices");
		GSList *iter_voice = NULL;
		tts_config_voice_s* voice = NULL;

		if (g_slist_length(engine_info->voices) > 0) {
			/* Get a first item */
			iter_voice = g_slist_nth(engine_info->voices, 0);

			int j = 1;
			while (NULL != iter_voice) {
				/*Get handle data from list*/
				voice = iter_voice->data;

				SLOG(LOG_DEBUG, tts_tag(), "  [%dth] lang(%s) type(%d)", j, voice->language, voice->type);

				/*Get next item*/
				iter_voice = g_slist_next(iter_voice);
				j++;
			}
		} else {
			SLOG(LOG_ERROR, tts_tag(), "  Voice is NONE");
		}
		iter = g_slist_next(iter);
		i++;
	}
	SLOG(LOG_DEBUG, tts_tag(), "--------------------------------------------");

	return 0;
}
