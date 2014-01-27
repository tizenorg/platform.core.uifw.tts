/*
*  Copyright (c) 2012, 2013 Samsung Electronics Co., Ltd All Rights Reserved 
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

#include <Ecore.h>
#include <Ecore_File.h>
#include <runtime_info.h>
#include <sys/inotify.h>
#include <vconf.h>

/* For multi-user support */
#include <tzplatform_config.h>

#include "ttsd_main.h"
#include "ttsd_config.h"
#include "ttsd_engine_agent.h"
#include "ttsd_data.h"

#define NOTI_ERROR_FILE_NAME		tzplatform_mkpath(TZ_USER_HOME, ".voice/ttsd_noti.err")

#define ENGINE_ID	"ENGINE_ID"
#define VOICE		"VOICE"
#define SPEED		"SPEED"


static char*	g_engine_id;
static char*	g_language;
static int	g_vc_type;
static int	g_speed;

static Ecore_Fd_Handler* g_fd_handler_noti = NULL;
static int g_fd_noti;
static int g_wd_noti;

static ttsd_config_changed_cb g_callback;


int __ttsd_config_compare()
{
	FILE* config_fp;
	char buf_id[256] = {0};
	char buf_param[256] = {0};
	int int_param = 0;

	config_fp = fopen(DEFAULT_CONFIG_FILE_NAME, "r");

	if (NULL == config_fp) {
		SLOG(LOG_ERROR, get_tag(), "[Config ERROR] Not open file(%s)", DEFAULT_CONFIG_FILE_NAME);

		return -1;
	}

	/* Read engine id */
	if (EOF == fscanf(config_fp, "%s %s", buf_id, buf_param)) {
		fclose(config_fp);
		SLOG(LOG_ERROR, get_tag(), "[Config WARNING] Fail to read config (engine id)");
		return -1;
	} else {
		if (0 == strncmp(ENGINE_ID, buf_id, strlen(ENGINE_ID))) {
			if (NULL != g_engine_id) {
				if (0 != strcmp(g_engine_id, buf_param)) {
					free(g_engine_id);
					g_engine_id = strdup(buf_param);

					SLOG(LOG_DEBUG, get_tag(), "[Config] engine id is changed : %s", g_engine_id);

					if (NULL != g_callback)
						g_callback(TTS_CONFIG_TYPE_ENGINE, g_engine_id, 0);
				}
			} else {
				SLOG(LOG_ERROR, get_tag(), "[Config ERROR] Current engine id is not available");
				g_engine_id = strdup(buf_param);
				if (NULL != g_callback)
					g_callback(TTS_CONFIG_TYPE_ENGINE, g_engine_id, 0);
			}
		} else {
			fclose(config_fp);
			SLOG(LOG_ERROR, get_tag(), "[Config] Fail to load config (engine id)");
			return -1;
		}
	}

	/* Read voice */
	if (EOF == fscanf(config_fp, "%s %s %d", buf_id, buf_param, &int_param)) {
		fclose(config_fp);
		SLOG(LOG_WARN, get_tag(), "[Config WARNING] Fail to read config (voice)");
		return -1;
	} else {
		if (0 == strncmp(VOICE, buf_id, strlen(VOICE))) {
			if (NULL != g_language) {
				if ((0 != strcmp(g_language, buf_param)) || (g_vc_type != int_param)) {
					free(g_language);
					g_language = strdup(buf_param);
					g_vc_type = int_param;

					SLOG(LOG_DEBUG, get_tag(), "[Config] voice is changed : lang(%s) type(%d)", g_language, g_vc_type);

					if (NULL != g_callback)
						g_callback(TTS_CONFIG_TYPE_VOICE, g_language, g_vc_type);
				}
			} else {
				SLOG(LOG_ERROR, get_tag(), "[Config ERROR] Current voice is not available");
				g_language = strdup(buf_param);
				g_vc_type = int_param;

				if (NULL != g_callback)
					g_callback(TTS_CONFIG_TYPE_VOICE, g_language, g_vc_type);
			}
		} else {
			fclose(config_fp);
			SLOG(LOG_WARN, get_tag(), "[Config WARNING] Fail to load config (voice)");
			return -1;
		}
	}

	/* Read speed */
	if (EOF == fscanf(config_fp, "%s %d", buf_id, &int_param)) {
		fclose(config_fp);
		SLOG(LOG_WARN, get_tag(), "[Config WARNING] Fail to read config (speed)");
		return -1;
	} else {
		if (0 == strncmp(SPEED, buf_id, strlen(SPEED))) {
			if (g_speed != int_param) {
				g_speed = int_param;

				SLOG(LOG_DEBUG, get_tag(), "[Config] speech rate is changed : %d", g_speed);

				if (NULL != g_callback)
					g_callback(TTS_CONFIG_TYPE_SPEED, NULL, g_speed);
			}
		} else {
			fclose(config_fp);
			SLOG(LOG_WARN, get_tag(), "[Config WARNING] Fail to load config (speed)");
			return -1;
		}
	}

	fclose(config_fp);

	return 0;
}

int __ttsd_config_save()
{
	if (0 != access(DEFAULT_CONFIG_FILE_NAME, R_OK|W_OK)) {
		if (0 == ecore_file_mkpath(CONFIG_DIRECTORY)) {
			SLOG(LOG_WARN, get_tag(), "[Config WARNING] Fail to create directory (%s)", CONFIG_DIRECTORY);
		} else {
			SLOG(LOG_DEBUG, get_tag(), "[Config] Create directory (%s)", CONFIG_DIRECTORY);
		}
	}

	FILE* config_fp;
	config_fp = fopen(DEFAULT_CONFIG_FILE_NAME, "w+");

	if (NULL == config_fp) {
		/* make file and file default */
		SLOG(LOG_WARN, get_tag(), "[Config WARNING] Fail to open config (%s)", DEFAULT_CONFIG_FILE_NAME);
		return -1;
	}

	SLOG(LOG_DEBUG, get_tag(), "[Config] Rewrite config file");

	/* Write engine id */
	fprintf(config_fp, "%s %s\n", ENGINE_ID, g_engine_id);

	/* Write voice */
	fprintf(config_fp, "%s %s %d\n", VOICE, g_language, g_vc_type);

	/* Read speed */
	fprintf(config_fp, "%s %d\n", SPEED, g_speed);

	fclose(config_fp);

	return 0;
}

int __ttsd_config_load()
{
	FILE* config_fp;
	char buf_id[256] = {0};
	char buf_param[256] = {0};
	int int_param = 0;
	bool is_default_open = false;

	config_fp = fopen(DEFAULT_CONFIG_FILE_NAME, "r");

	if (NULL == config_fp) {
		SLOG(LOG_WARN, get_tag(), "[Config WARNING] Not open file(%s)", DEFAULT_CONFIG_FILE_NAME);

		config_fp = fopen(CONFIG_DEFAULT, "r");
		if (NULL == config_fp) {
			SLOG(LOG_ERROR, get_tag(), "[Config WARNING] Not open original config file(%s)", CONFIG_DEFAULT);
			return -1;
		}

		is_default_open = true;
	}

	/* Read engine id */
	if (EOF == fscanf(config_fp, "%s %s", buf_id, buf_param)) {
		fclose(config_fp);
		SLOG(LOG_WARN, get_tag(), "[Config WARNING] Fail to read config (engine id)");
		return -1;
	} else {
		if (0 == strncmp(ENGINE_ID, buf_id, strlen(ENGINE_ID))) {
			g_engine_id = strdup(buf_param);
		} else {
			fclose(config_fp);
			SLOG(LOG_WARN, get_tag(), "[Config WARNING] Fail to load config (engine id)");
			return -1;
		}
	}

	/* Read voice */
	if (EOF == fscanf(config_fp, "%s %s %d", buf_id, buf_param, &int_param)) {
		fclose(config_fp);
		SLOG(LOG_WARN, get_tag(), "[Config WARNING] Fail to read config (voice)");
		__ttsd_config_save();
		return -1;
	} else {
		if (0 == strncmp(VOICE, buf_id, strlen(VOICE))) {
			g_language = strdup(buf_param);
			g_vc_type = int_param;
		} else {
			fclose(config_fp);
			SLOG(LOG_WARN, get_tag(), "[Config WARNING] Fail to load config (voice)");
			return -1;
		}
	}

	if (true == is_default_open) {
		/* Change default language to display language */
		char* value;
		value = vconf_get_str(VCONFKEY_LANGSET);

		if (NULL != value) {
			SLOG(LOG_DEBUG, get_tag(), "[Config] System language : %s", value);
			strncpy(g_language, value, strlen(g_language));
			SLOG(LOG_DEBUG, get_tag(), "[Config] Default language : %s", g_language);

			free(value);
		} else {
			SLOG(LOG_ERROR, get_tag(), "[Config ERROR] Fail to get system language");
		}
	}

	/* Read speed */
	if (EOF == fscanf(config_fp, "%s %d", buf_id, &int_param)) {
		fclose(config_fp);
		SLOG(LOG_WARN, get_tag(), "[Config WARNING] Fail to read config (speed)");
		return -1;
	} else {
		if (0 == strncmp(SPEED, buf_id, strlen(SPEED))) {
			g_speed = int_param;
		} else {
			fclose(config_fp);
			SLOG(LOG_WARN, get_tag(), "[Config WARNING] Fail to load config (speed)");
			return -1;
		}
	}

	fclose(config_fp);

	SLOG(LOG_DEBUG, get_tag(), "[Config] Load config : engine(%s), voice(%s,%d), speed(%d)",
		g_engine_id, g_language, g_vc_type, g_speed);

	if (true == is_default_open) {
		if(0 == __ttsd_config_save()) {
			SLOG(LOG_DEBUG, get_tag(), "[Config] Create config(%s)", DEFAULT_CONFIG_FILE_NAME);
		}
	}

	return 0;
}

static Eina_Bool inotify_event_callback(void* data, Ecore_Fd_Handler *fd_handler)
{
	SLOG(LOG_DEBUG, get_tag(), "===== [Config] Inotify event call");

	int length;
	char buffer[sizeof(struct inotify_event)];
	memset(buffer, 0, (sizeof(struct inotify_event)));

	length = read(g_fd_noti, buffer, (sizeof(struct inotify_event)));
	if (0 > length) {
		SLOG(LOG_ERROR, get_tag(), "[Config] Empty Inotify event");
		SLOG(LOG_DEBUG, get_tag(), "=====");
		SLOG(LOG_DEBUG, get_tag(), " ");
		return ECORE_CALLBACK_RENEW; 
	}

	struct inotify_event *event;
	event = (struct inotify_event *)&buffer;

	if (IN_CLOSE_WRITE == event->mask) {
		__ttsd_config_compare();
	} else {
		SLOG(LOG_ERROR, get_tag(), "[Config] Undefined event");
	}

	SLOG(LOG_DEBUG, get_tag(), "=====");
	SLOG(LOG_DEBUG, get_tag(), " ");

	return ECORE_CALLBACK_PASS_ON;
}

int __config_file_open_connection()
{
	/* get file notification handler */
	int fd;
	int wd;

	fd = inotify_init();
	if (fd < 0) {
		SLOG(LOG_ERROR, get_tag(), "[Config ERROR] Fail get inotify_fd");
		return -1;
	}
	g_fd_noti = fd;

	wd = inotify_add_watch(fd, DEFAULT_CONFIG_FILE_NAME, IN_CLOSE_WRITE);
	g_wd_noti = wd;
	g_fd_handler_noti = ecore_main_fd_handler_add(fd, ECORE_FD_READ, (Ecore_Fd_Cb)inotify_event_callback, NULL, NULL, NULL);		
	if (NULL == g_fd_handler_noti) {
		SLOG(LOG_ERROR, get_tag(), "[Config ERROR] Fail to get handler_noti");
		return -1;
	}

	return 0;
}

int ttsd_config_initialize(ttsd_config_changed_cb callback)
{
	g_engine_id = NULL;
	g_language = NULL;
	g_vc_type = 1;
	g_speed = 3;

	g_callback = callback;

	ecore_file_mkpath(CONFIG_DIRECTORY);

	__ttsd_config_load();

	if (0 != __config_file_open_connection()) {
		return -1;
	}

	return 0;
}

int ttsd_config_finalize()
{
	if (NULL != g_language) {
		free(g_language);
	}

	if (NULL != g_engine_id) {
		free(g_engine_id);
	}

	/* del inotify variable */
	ecore_main_fd_handler_del(g_fd_handler_noti);
	inotify_rm_watch(g_fd_noti, g_wd_noti);
	close(g_fd_noti);

	return 0;
}

int ttsd_config_update_language()
{
	/* no work in notification mode */
	return 0;
}

int ttsd_config_get_default_engine(char** engine_id)
{
	if (NULL == engine_id)
		return -1;

	if (NULL != g_engine_id) {
		*engine_id = strdup(g_engine_id);
	} else {
		SLOG(LOG_ERROR, get_tag(), "[Config ERROR] Current engine id is NULL");
		return -1;
	}

	return 0;
}

int ttsd_config_set_default_engine(const char* engine_id)
{
	/* Not available in notification mode */
	return 0;
}

int ttsd_config_get_default_voice(char** language, int* type)
{
	if (NULL == language || NULL == type)
		return -1;

	if (NULL != g_language) {
		*language = strdup(g_language);
		*type = g_vc_type;
	} else {
		SLOG(LOG_ERROR, get_tag(), "[Config ERROR] Current language is NULL");
		return -1;
	}
	
	return 0;
}

int ttsd_config_set_default_voice(const char* language, int type)
{
	/* Not available in notification mode */
	return 0;
}

int ttsd_config_get_default_speed(int* speed)
{
	if (NULL == speed)
		return -1;

	*speed = g_speed;

	return 0;
}

int ttsd_config_set_default_speed(int speed)
{
	/* Not available in notification mode */
	return 0;
}

int ttsd_config_save_error(int uid, int uttid, const char* lang, int vctype, const char* text, 
			   const char* func, int line, const char* message)
{
	SLOG(LOG_ERROR, get_tag(), "=============== TTS ERROR LOG ====================");

	SLOG(LOG_ERROR, get_tag(), "uid(%d) uttid(%d)", uid, uttid);

	if (NULL != func) 	SLOG(LOG_ERROR, get_tag(), "Function(%s) Line(%d)", func, line);
	if (NULL != message) 	SLOG(LOG_ERROR, get_tag(), "Message(%s)", message);
	if (NULL != lang) 	SLOG(LOG_ERROR, get_tag(), "Lang(%s), type(%d)", lang, vctype);
	if (NULL != text) 	SLOG(LOG_ERROR, get_tag(), "Text(%s)", text);

	SLOG(LOG_ERROR, get_tag(), "==================================================");

	return 0;
}
