/*
*  Copyright (c) 2011-2014 Samsung Electronics Co., Ltd All Rights Reserved
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


#include <dlfcn.h>
#include <dirent.h>

#include "tts_defs.h"
#include "ttsd_main.h"
#include "ttsd_engine_agent.h"
#include "ttsd_config.h"

#define	ENGINE_PATH_SIZE	256

/*
* Internal data structure
*/
typedef struct {
	/* base info */
	char*	engine_uuid;
	char*	engine_name;
	char*	engine_path;

	/* info for using engine load */
	bool	is_set;
	bool	is_loaded;
	bool	need_network;
	void	*handle;

	/* engine base setting */
	char*	default_lang;
	int	default_vctype;
	int	default_speed;
	int	default_pitch;

	ttspe_funcs_s*	pefuncs;
	ttspd_funcs_s*	pdfuncs;

	int (*ttsp_load_engine)(const ttspd_funcs_s* pdfuncs, ttspe_funcs_s* pefuncs);
	int (*ttsp_unload_engine)();
} ttsengine_s;

typedef struct {
	char*	engine_uuid;
	char*	engine_path;
	char*	engine_name;
	char*	setting_ug_path;
	bool	use_network;
} ttsengine_info_s;

typedef struct {
	bool	is_default;
	bool	is_loaded;
	int	client_ref_count;

	char*	lang;
	int	type;
} ttsvoice_s;


/** Init flag */
static bool g_agent_init;

/** TTS engine list */
static GList *g_engine_list;

/** Current engine information */
static ttsengine_s g_cur_engine;

/** Current voice information */
static GSList* g_cur_voices;

/** Result callback function */
static synth_result_callback g_result_cb;


/** Set current engine */
int __internal_set_current_engine(const char* engine_uuid);

/** Check engine id */
int __internal_check_engine_id(const char* engine_uuid);

/** Update engine list */
int __internal_update_engine_list();

/** Get engine info */
int __internal_get_engine_info(const char* filepath, ttsengine_info_s** info);

/** Callback function for result */
bool __result_cb(ttsp_result_event_e event, const void* data, unsigned int data_size, 
		 ttsp_audio_type_e audio_type, int rate, void *user_data);

/** Callback function for voice list */
bool __supported_voice_cb(const char* language, int type, void* user_data);

/** Free voice list */
void __free_voice_list(GList* voice_list);

/** Callback function for engine info */
void __engine_info_cb(const char* engine_uuid, const char* engine_name, const char* setting_ug_name, 
		      bool use_network, void* user_data);

/** Callback fucntion for engine setting */
bool __engine_setting_cb(const char* key, const char* value, void* user_data);


/** Print list */
int ttsd_print_enginelist();

int ttsd_print_voicelist();

static const char* __ttsd_get_engine_error_code(ttsp_error_e err)
{
	switch (err) {
	case TTSP_ERROR_NONE:			return "TTSP_ERROR_NONE";
	case TTSP_ERROR_OUT_OF_MEMORY:		return "TTSP_ERROR_OUT_OF_MEMORY";
	case TTSP_ERROR_IO_ERROR:		return "TTSP_ERROR_IO_ERROR";
	case TTSP_ERROR_INVALID_PARAMETER:	return "TTSP_ERROR_INVALID_PARAMETER";
	case TTSP_ERROR_OUT_OF_NETWORK:		return "TTSP_ERROR_OUT_OF_NETWORK";
	case TTSP_ERROR_INVALID_STATE:		return "TTSP_ERROR_INVALID_STATE";
	case TTSP_ERROR_INVALID_VOICE:		return "TTSP_ERROR_INVALID_VOICE";
	case TTSP_ERROR_OPERATION_FAILED:	return "TTSP_ERROR_OPERATION_FAILED";
	default:
		return "Invalid error code";
	}
}

int ttsd_engine_agent_init(synth_result_callback result_cb)
{
	/* initialize static data */
	if (result_cb == NULL) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] invalid parameter");
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	g_result_cb = result_cb;

	g_cur_engine.engine_uuid = NULL;
	g_cur_engine.engine_name = NULL;
	g_cur_engine.engine_path = NULL;

	g_cur_engine.is_set = false;
	g_cur_engine.handle = NULL;
	g_cur_engine.pefuncs = (ttspe_funcs_s*)calloc(1, sizeof(ttspe_funcs_s));
	g_cur_engine.pdfuncs = (ttspd_funcs_s*)calloc(1, sizeof(ttspd_funcs_s));

	g_agent_init = true;

	if (0 != ttsd_config_get_default_voice(&(g_cur_engine.default_lang), &(g_cur_engine.default_vctype))) {
		SLOG(LOG_WARN, get_tag(), "[Server WARNING] There is No default voice in config");
		/* Set default voice */
		g_cur_engine.default_lang = strdup(TTS_BASE_LANGUAGE);
		g_cur_engine.default_vctype = TTSP_VOICE_TYPE_FEMALE;
	}

	if (0 != ttsd_config_get_default_speed(&(g_cur_engine.default_speed))) {
		SLOG(LOG_WARN, get_tag(), "[Server WARNING] There is No default speed in config");
		g_cur_engine.default_speed = TTS_SPEED_NORMAL;
	}

	if (0 != ttsd_config_get_default_pitch(&(g_cur_engine.default_pitch))) {
		SLOG(LOG_WARN, get_tag(), "[Server WARNING] There is No default pitch in config");
		g_cur_engine.default_pitch = TTS_PITCH_NORMAL;
	}

	SLOG(LOG_DEBUG, get_tag(), "[Engine Agent SUCCESS] Initialize Engine Agent");

	return 0;
}

int ttsd_engine_agent_release()
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not Initialized");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	/* unload current engine */
	ttsd_engine_agent_unload_current_engine();

	/* release engine list */
	GList *iter = NULL;
	ttsengine_info_s *data = NULL;

	if (g_list_length(g_engine_list) > 0) {
		/* Get a first item */
		iter = g_list_first(g_engine_list);
		while (NULL != iter) {
			/* Get data from item */
			data = iter->data;
			iter = g_list_remove(iter, data);

			if (NULL != data) {
				if (NULL != data->engine_uuid)		free(data->engine_uuid);
				if (NULL != data->engine_name)		free(data->engine_name);
				if (NULL != data->setting_ug_path)	free(data->setting_ug_path);
				if (NULL != data->engine_path)		free(data->engine_path);
				free(data);
			}
		}
	}
	g_list_free(iter);

	/* release current engine data */
	if (g_cur_engine.engine_uuid != NULL)	free(g_cur_engine.engine_uuid);
	if (g_cur_engine.engine_name != NULL)	free(g_cur_engine.engine_name);
	if (g_cur_engine.engine_path != NULL)	free(g_cur_engine.engine_path);

	if (g_cur_engine.pefuncs != NULL)	free(g_cur_engine.pefuncs);
	if (g_cur_engine.pdfuncs != NULL)	free(g_cur_engine.pdfuncs);
	if (g_cur_engine.default_lang != NULL)	free(g_cur_engine.default_lang);
	g_result_cb = NULL;
	g_agent_init = false;

	SLOG(LOG_DEBUG, get_tag(), "[Engine Agent SUCCESS] Release Engine Agent");

	return 0;
}

int ttsd_engine_agent_initialize_current_engine()
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not Initialized");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	/* update engine list */
	if (0 != __internal_update_engine_list()) {
		SLOG(LOG_WARN, get_tag(), "[Engine Agent WARNING] No engine error");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	/* 2. get current engine from config */
	char* cur_engine_uuid = NULL;
	bool is_get_engineid_from_config = false;

	if (0 != ttsd_config_get_default_engine(&cur_engine_uuid)) {
		/*not set current engine */
		/*set system default engine*/
		GList *iter = NULL;
		ttsengine_info_s *data = NULL;

		if (g_list_length(g_engine_list) > 0) {
			iter = g_list_first(g_engine_list);
			data = iter->data;

			if (NULL != data) {
				if (NULL != data->engine_uuid) {
					cur_engine_uuid = strdup(data->engine_uuid);
					ttsd_config_set_default_engine(cur_engine_uuid);
				} else {
					SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Data of current engine is corrupt");
					return TTSD_ERROR_OPERATION_FAILED;
				}
			}
		} else {
			SLOG(LOG_WARN, get_tag(), "[Engine Agent WARNING] Fail to set a engine of engine list");
			return TTSD_ERROR_OPERATION_FAILED;
		}

		is_get_engineid_from_config = false;
	} else {
		is_get_engineid_from_config = true;
	}

	if (NULL == cur_engine_uuid) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Current engine id is NULL");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	/* check whether cur engine uuid is valid or not. */
	if (0 != __internal_check_engine_id(cur_engine_uuid)) {
		SLOG(LOG_WARN, get_tag(), "[Engine Agent WARNING] It is not valid engine id from config");

		GList *iter = NULL;

		if (g_list_length(g_engine_list) > 0)
			iter = g_list_first(g_engine_list);
		else {
			SLOG(LOG_WARN, get_tag(), "[Engine Agent ERROR] NO TTS Engine !!");
			if (NULL != cur_engine_uuid)	free(cur_engine_uuid);
			return TTSD_ERROR_OPERATION_FAILED;
		}

		if (cur_engine_uuid != NULL)	free(cur_engine_uuid);
		ttsengine_info_s *data = NULL;
		data = iter->data;

		cur_engine_uuid = strdup(data->engine_uuid);

		is_get_engineid_from_config = false;
	}

	if (NULL != cur_engine_uuid)
		SECURE_SLOG(LOG_DEBUG, get_tag(), "[Engine Agent] Current Engine Id : %s", cur_engine_uuid);
	else
		return TTSD_ERROR_OPERATION_FAILED;

	/* set current engine */
	if (0 != __internal_set_current_engine(cur_engine_uuid)) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] fail to set current engine ");
		if (NULL != cur_engine_uuid)	free(cur_engine_uuid);
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (false == is_get_engineid_from_config) {
		if (0 != ttsd_config_set_default_engine(cur_engine_uuid)) {
			SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] fail to set id to config");
		}
	}

	if (NULL != cur_engine_uuid)	free(cur_engine_uuid);

	SLOG(LOG_DEBUG, get_tag(), "[Engine Agent SUCCESS] Set current engine");

	return 0;
}

int __internal_check_engine_id(const char* engine_uuid)
{
	GList *iter = NULL;
	ttsengine_s *data = NULL;

	if (g_list_length(g_engine_list) > 0) {
		iter = g_list_first(g_engine_list);

		while (NULL != iter) {
			data = iter->data;

			if (0 == strncmp(engine_uuid, data->engine_uuid, strlen(data->engine_uuid)))
				return 0;

			iter = g_list_next(iter);
		}
	}

	return -1;
}

void __engine_info_cb(const char* engine_uuid, const char* engine_name, const char* setting_ug_name, 
			     bool use_network, void* user_data)
{
	ttsengine_info_s* temp = (ttsengine_info_s*)user_data;

	if (NULL != engine_uuid)
		temp->engine_uuid = strdup(engine_uuid);

	if (NULL != engine_name)
		temp->engine_name = strdup(engine_name);

	if (NULL != setting_ug_name)
		temp->setting_ug_path = strdup(setting_ug_name);

	temp->use_network = use_network;
	return;
}

int __internal_get_engine_info(const char* filepath, ttsengine_info_s** info)
{
	char *error;
	void* handle;

	handle = dlopen(filepath, RTLD_LAZY);

	if (!handle) {
		SECURE_SLOG(LOG_WARN, get_tag(), "[Engine Agent] Invalid engine : %s", filepath);
		return TTSD_ERROR_OPERATION_FAILED;
	}

	/* link engine to daemon */
	dlsym(handle, "ttsp_load_engine");
	if ((error = dlerror()) != NULL) {
		SECURE_SLOG(LOG_WARN, get_tag(), "[Engine Agent] Fail to open ttsp_load_engine : path(%s) message(%s)", filepath, error);
		dlclose(handle);
		return TTSD_ERROR_OPERATION_FAILED;
	}

	dlsym(handle, "ttsp_unload_engine");
	if ((error = dlerror()) != NULL) {
		SECURE_SLOG(LOG_WARN, get_tag(), "[Engine Agent] Fail to open ttsp_unload_engine : path(%s) message(%s)", filepath, error);
		dlclose(handle);
		return TTSD_ERROR_OPERATION_FAILED;
	}

	int (*get_engine_info)(ttsp_engine_info_cb callback, void* user_data);

	get_engine_info = (int (*)(ttsp_engine_info_cb, void*))dlsym(handle, "ttsp_get_engine_info");
	if (NULL != (error = dlerror()) || NULL == get_engine_info) {
		SLOG(LOG_WARN, get_tag(), "[Engine Agent] Fail to open ttsp_get_engine_info() :path(%s) message(%s)", filepath, error);
		dlclose(handle);
		return TTSD_ERROR_OPERATION_FAILED;
	}

	ttsengine_info_s* temp;
	temp = (ttsengine_info_s*)calloc(1, sizeof(ttsengine_info_s));
	if (NULL == temp) {
		SLOG(LOG_WARN, get_tag(), "[Engine Agent] Fail to alloc memory");
		dlclose(handle);
		return TTSD_ERROR_OUT_OF_MEMORY;
	}

	/* get engine info */
	if (0 != get_engine_info(&__engine_info_cb, (void*)temp)) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] fail to get engine info");
		dlclose(handle);
		free(temp);
		return TTSD_ERROR_OPERATION_FAILED;
	}

	/* close engine */
	dlclose(handle);

	if (TTSD_MODE_SCREEN_READER == ttsd_get_mode() && true == temp->use_network) {
		SECURE_SLOG(LOG_ERROR, get_tag(), "[Engine Agent WARNING] %s is invalid because of network based", temp->engine_name);
		if (NULL != temp->engine_uuid)		free(temp->engine_uuid);
		if (NULL != temp->engine_name)		free(temp->engine_name);
		if (NULL != temp->setting_ug_path)	free(temp->setting_ug_path);
		free(temp);
		return TTSD_ERROR_OPERATION_FAILED;
	}

	temp->engine_path = strdup(filepath);

	SLOG(LOG_DEBUG, get_tag(), "----- Valid engine");
	SECURE_SLOG(LOG_DEBUG, get_tag(), "Engine uuid : %s", temp->engine_uuid);
	SECURE_SLOG(LOG_DEBUG, get_tag(), "Engine name : %s", temp->engine_name);
	SECURE_SLOG(LOG_DEBUG, get_tag(), "Setting path : %s", temp->setting_ug_path);
	SECURE_SLOG(LOG_DEBUG, get_tag(), "Engine path : %s", temp->engine_path);
	SECURE_SLOG(LOG_DEBUG, get_tag(), "Use network : %s", temp->use_network ? "true" : "false");
	SLOG(LOG_DEBUG, get_tag(), "-----");
	SLOG(LOG_DEBUG, get_tag(), "  ");

	*info = temp;

	return 0;
}

int __internal_update_engine_list()
{
	/* relsease engine list */
	GList *iter = NULL;
	ttsengine_info_s *data = NULL;

	if (g_list_length(g_engine_list) > 0) {
		iter = g_list_first(g_engine_list);

		while (NULL != iter) {
			data = iter->data;

			if (data != NULL)	free(data);
			g_engine_list = g_list_remove_link(g_engine_list, iter);
			iter = g_list_first(g_engine_list);
		}
	}

	/* get file name from engine directory and get engine information from each filename */
	DIR *dp = NULL;
	int ret = -1;
	struct dirent entry;
	struct dirent *dirp = NULL;
	dp = opendir(TTS_DEFAULT_ENGINE);

	if (dp != NULL) {
		do {
			ret = readdir_r(dp, &entry, &dirp);
			if (0 != ret) {
				SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Fail to read directory");
				break;
			}

			if (NULL != dirp) {
				ttsengine_info_s* info;
				char* filepath = NULL;
				int file_size;

				file_size = strlen(TTS_DEFAULT_ENGINE) + strlen(dirp->d_name) + 5;
				filepath = (char*)calloc(file_size, sizeof(char));

				if (NULL != filepath) {
					snprintf(filepath, file_size, "%s/%s", TTS_DEFAULT_ENGINE, dirp->d_name);
				} else {
					SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not enough memory!!" );
					continue;
				}

				/* get its info and update engine list */
				if (0 == __internal_get_engine_info(filepath, &info)) {
					/* add engine info to g_engine_list */
					g_engine_list = g_list_append(g_engine_list, info);
				}

				if (NULL != filepath)	free(filepath);
			}
		} while (NULL != dirp);

		closedir(dp);
	}

	dp = opendir(TTS_DOWNLOAD_ENGINE);

	if (dp != NULL) {
		do {
			ret = readdir_r(dp, &entry, &dirp);
			if (0 != ret) {
				SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Fail to read directory");
				break;
			}

			if (NULL != dirp) {
				ttsengine_info_s* info;
				char* filepath = NULL;
				int file_size;

				file_size = strlen(TTS_DOWNLOAD_ENGINE) + strlen(dirp->d_name) + 5;
				filepath = (char*)calloc(file_size, sizeof(char));

				if (NULL != filepath) {
					snprintf(filepath, file_size, "%s/%s", TTS_DOWNLOAD_ENGINE, dirp->d_name);
				} else {
					SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not enough memory!!" );
					continue;
				}

				/* get its info and update engine list */
				if (0 == __internal_get_engine_info(filepath, &info)) {
					/* add engine info to g_engine_list */
					g_engine_list = g_list_append(g_engine_list, info);
				}

				if (NULL != filepath)	free(filepath);
			}
		} while (NULL != dirp);

		closedir(dp);
	}

	if (g_list_length(g_engine_list) <= 0) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] No Engine");
		return TTSD_ERROR_OPERATION_FAILED;
	}

#ifdef ENGINE_AGENT_DEBUG
	ttsd_print_enginelist();
#endif
	return 0;
}

int __internal_set_current_engine(const char* engine_uuid)
{
	/* check whether engine id is valid or not. */
	GList *iter = NULL;
	ttsengine_info_s *data = NULL;

	bool flag = false;
	if (g_list_length(g_engine_list) > 0) {
		iter = g_list_first(g_engine_list);

		while (NULL != iter) {
			data = iter->data;

			if (0 == strncmp(data->engine_uuid, engine_uuid, strlen(engine_uuid))) {
				flag = true;
				break;
			}

			/*Get next item*/
			iter = g_list_next(iter);
		}
	}

	/* If current engine does not exist, return error */
	if (false == flag) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Cannot find engine id");
		return TTSD_ERROR_OPERATION_FAILED;
	} else {
		if (g_cur_engine.engine_uuid != NULL) {
			/*compare current engine uuid */
			if (0 == strncmp(g_cur_engine.engine_uuid, data->engine_uuid, strlen(engine_uuid))) {
				SLOG(LOG_DEBUG, get_tag(), "[Engine Agent] tts engine has already been set");
				return 0;
			}
		}
	}

	/* set data from g_engine_list */
	if (g_cur_engine.engine_uuid != NULL)	free(g_cur_engine.engine_uuid);
	if (g_cur_engine.engine_name != NULL)	free(g_cur_engine.engine_name);
	if (g_cur_engine.engine_path != NULL)	free(g_cur_engine.engine_path);

	if (NULL == data->engine_uuid || NULL == data->engine_name || NULL == data->engine_path) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Engine data is NULL");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	g_cur_engine.engine_uuid = strdup(data->engine_uuid);
	g_cur_engine.engine_name = strdup(data->engine_name);
	g_cur_engine.engine_path = strdup(data->engine_path);

	g_cur_engine.handle = NULL;
	g_cur_engine.is_loaded = false;
	g_cur_engine.is_set = true;
	g_cur_engine.need_network = data->use_network;

	SLOG(LOG_DEBUG, get_tag(), "-----");
	SECURE_SLOG(LOG_DEBUG, get_tag(), "Current engine uuid : %s", g_cur_engine.engine_uuid);
	SECURE_SLOG(LOG_DEBUG, get_tag(), "Current engine name : %s", g_cur_engine.engine_name);
	SECURE_SLOG(LOG_DEBUG, get_tag(), "Current engine path : %s", g_cur_engine.engine_path);
	SLOG(LOG_DEBUG, get_tag(), "-----");

	return 0;
}

bool __set_voice_info_cb(const char* language, int type, void* user_data)
{
	if (NULL == language) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Input parameter is NULL in voice list callback!!!!");
		return false;
	}

	ttsvoice_s* voice = calloc(1, sizeof(ttsvoice_s));
	voice->lang = strdup(language);
	voice->type = type;

	voice->client_ref_count = 0;
	voice->is_loaded = false;

	if (0 == strcmp(g_cur_engine.default_lang, language) && g_cur_engine.default_vctype == type) {
		voice->is_default = true;
		voice->is_loaded = true;
	} else {
		voice->is_default = false;
	}

	g_cur_voices = g_slist_append(g_cur_voices, voice);

	return true;
}

int __update_voice_list()
{
	/* Get voice list */
	g_cur_voices = NULL;
	int ret = 0;

	ret = g_cur_engine.pefuncs->foreach_voices(__set_voice_info_cb, NULL);
	if (0 != ret || 0 >= g_slist_length(g_cur_voices)) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] fail to get voice list : result(%d)", ret);
		return -1;
	}

#ifdef ENGINE_AGENT_DEBUG
	ttsd_print_voicelist();
#endif
	return 0;
}

int ttsd_engine_agent_load_current_engine()
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not Initialized");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (false == g_cur_engine.is_set) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] No Current Engine ");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	/* check whether current engine is loaded or not */
	if (true == g_cur_engine.is_loaded) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent] Engine has already been loaded ");
		return 0;
	}

	/* open engine */
	char *error = NULL;
	g_cur_engine.handle = dlopen(g_cur_engine.engine_path, RTLD_LAZY); /* RTLD_LAZY RTLD_NOW*/

	if (NULL != (error = dlerror()) || NULL == g_cur_engine.handle) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Fail to get current engine handle : %s", error);
		return -2;
	}

	g_cur_engine.ttsp_unload_engine = (int (*)())dlsym(g_cur_engine.handle, "ttsp_unload_engine");
	if (NULL != (error = dlerror()) || NULL == g_cur_engine.ttsp_unload_engine) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Fail to link daemon to ttsp_unload_engine() : %s", error);
		return -3;
	}

	g_cur_engine.ttsp_load_engine = (int (*)(const ttspd_funcs_s* , ttspe_funcs_s*))dlsym(g_cur_engine.handle, "ttsp_load_engine");
	if (NULL != (error = dlerror()) || NULL == g_cur_engine.ttsp_load_engine) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Fail to link daemon to ttsp_load_engine() : %s", error);
		return -3;
	}

	/* load engine */
	g_cur_engine.pdfuncs->version = 1;
	g_cur_engine.pdfuncs->size = sizeof(ttspd_funcs_s);

	int ret = 0;
	ret = g_cur_engine.ttsp_load_engine(g_cur_engine.pdfuncs, g_cur_engine.pefuncs);
	if (0 != ret) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] fail to load engine - %s : result(%s)",
			 g_cur_engine.engine_path, __ttsd_get_engine_error_code(ret));
		return TTSD_ERROR_OPERATION_FAILED;
	}

	/* engine error check */
	if (g_cur_engine.pefuncs->size != sizeof(ttspe_funcs_s)) {
		SLOG(LOG_WARN, get_tag(), "[Engine Agent WARNING] The size of engine function is not valid");
	}

	if (NULL == g_cur_engine.pefuncs->initialize ||
		NULL == g_cur_engine.pefuncs->deinitialize ||
		NULL == g_cur_engine.pefuncs->foreach_voices ||
		NULL == g_cur_engine.pefuncs->is_valid_voice ||
		NULL == g_cur_engine.pefuncs->start_synth ||
		NULL == g_cur_engine.pefuncs->cancel_synth) {
		SLOG(LOG_ERROR, get_tag(), "[Engine ERROR] The engine functions are NOT valid");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	ret = g_cur_engine.pefuncs->initialize(__result_cb);
	if (0 != ret) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Fail to initialize current engine : %s", __ttsd_get_engine_error_code(ret));
		return TTSD_ERROR_OPERATION_FAILED;
	}

	/* Get voice info of current engine */
	ret = __update_voice_list();
	if (0 != ret) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Fail to set voice info : result(%d)", ret);
		return TTSD_ERROR_OPERATION_FAILED;
	}

	/* Select default voice */
	if (NULL != g_cur_engine.default_lang) {
		if (true == g_cur_engine.pefuncs->is_valid_voice(g_cur_engine.default_lang, g_cur_engine.default_vctype)) {
			SLOG(LOG_DEBUG, get_tag(), "[Engine Agent SUCCESS] Set origin default voice to current engine : lang(%s), type(%d)",
				 g_cur_engine.default_lang,  g_cur_engine.default_vctype);
		} else {
			SLOG(LOG_WARN, get_tag(), "[Engine Agent WARNING] Fail set origin default voice : lang(%s), type(%d)",
				 g_cur_engine.default_lang, g_cur_engine.default_vctype);

			return TTSD_ERROR_OPERATION_FAILED;
		}
	}

	/* load default voice */
	if (NULL != g_cur_engine.pefuncs->load_voice) {
		ret = g_cur_engine.pefuncs->load_voice(g_cur_engine.default_lang, g_cur_engine.default_vctype);
		if (0 == ret) {
			SLOG(LOG_DEBUG, get_tag(), "[Engine Agent SUCCESS] Load default voice : lang(%s), type(%d)",
				 g_cur_engine.default_lang,  g_cur_engine.default_vctype);
		} else {
			SLOG(LOG_WARN, get_tag(), "[Engine Agent ERROR] Fail to load default voice : lang(%s), type(%d) result(%s)",
				 g_cur_engine.default_lang, g_cur_engine.default_vctype, __ttsd_get_engine_error_code(ret));

			return TTSD_ERROR_OPERATION_FAILED;
		}
	}

#if 0
	if (false == set_voice) {
		/* get language list */
		int ret;
		GList* voice_list = NULL;

		ret = g_cur_engine.pefuncs->foreach_voices(__supported_voice_cb, &voice_list);

		if (0 == ret && 0 < g_list_length(voice_list)) {
			GList *iter = NULL;
			voice_s* voice = NULL;

			iter = g_list_first(voice_list);

			/* check english */
			while (NULL != iter) {
				voice = iter->data;

				if (NULL != voice) {
					if (0 == strcmp("en_US", voice->language))
						break;
				}

				iter = g_list_next(iter);
			}
			if (NULL == voice) {
				SLOG(LOG_ERROR, get_tag(), "[Engine ERROR] Fail to find voice in list");
				return TTSD_ERROR_OPERATION_FAILED;
			}

			/* Set selected language and type */
			if (true != g_cur_engine.pefuncs->is_valid_voice(voice->language, voice->type)) {
				SLOG(LOG_ERROR, get_tag(), "[Engine ERROR] Fail voice is NOT valid");
				return TTSD_ERROR_OPERATION_FAILED;
			}

			ttsd_config_set_default_voice(voice->language, (int)voice->type);

			g_cur_engine.default_lang = strdup(voice->language);
			g_cur_engine.default_vctype = voice->type;

			SLOG(LOG_DEBUG, get_tag(), "[Engine Agent SUCCESS] Select default voice : lang(%s), type(%d)",
				 voice->language,  voice->type);

			__free_voice_list(voice_list);
		} else {
			SLOG(LOG_ERROR, get_tag(), "[Engine ERROR] Fail to get language list : result(%d)", ret);
			return TTSD_ERROR_OPERATION_FAILED;
		}
	}
#endif
	g_cur_engine.is_loaded = true;

	return 0;
}

int ttsd_engine_agent_unload_current_engine()
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not Initialized");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (false == g_cur_engine.is_set) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] No Current Engine ");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (false == g_cur_engine.is_loaded) {
		SLOG(LOG_DEBUG, get_tag(), "[Engine Agent] Engine has already been unloaded ");
		return 0;
	}

	/* shutdown engine */
	int ret = 0;
	ret = g_cur_engine.pefuncs->deinitialize();
	if (0 != ret) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent] Fail deinitialize() : %s", __ttsd_get_engine_error_code(ret));
	}

	/* unload engine */
	g_cur_engine.ttsp_unload_engine();

	dlclose(g_cur_engine.handle);

	/* reset current engine data */
	g_cur_engine.handle = NULL;
	g_cur_engine.is_loaded = false;

	GSList *iter = NULL;
	ttsvoice_s* data = NULL;

	iter = g_slist_nth(g_cur_voices, 0);
	while (NULL != iter) {
		data = iter->data;

		if (NULL != data) {
			if (NULL != data->lang)		free(data->lang);
			g_cur_voices = g_slist_remove(g_cur_voices, data);
			free(data);
			data = NULL;
		}

		iter = g_slist_nth(g_cur_voices, 0);
	}

	return 0;
}

bool ttsd_engine_agent_need_network()
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not Initialized");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (false == g_cur_engine.is_loaded) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not loaded engine");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	return g_cur_engine.need_network;
}

bool ttsd_engine_select_valid_voice(const char* lang, int type, char** out_lang, int* out_type)
{
	if (NULL == lang || NULL == out_lang || NULL == out_type) {
		return false;
	}

	if (false == g_cur_engine.is_loaded) {
		SLOG(LOG_WARN, get_tag(), "[Engine Agent WARNING] Not loaded engine");
		return false;
	}

	SECURE_SLOG(LOG_DEBUG, get_tag(), "[Engine Agent] Select voice : input lang(%s), input type(%d), default lang(%s), default type(%d)", 
		lang, type, g_cur_engine.default_lang, g_cur_engine.default_vctype);

	/* case 1 : Both are default */
	if (0 == strncmp(lang, "default", strlen("default")) && 0 == type) {
		*out_lang = strdup(g_cur_engine.default_lang);
		*out_type = g_cur_engine.default_vctype;
		return true;
	}

	/* Get voice list */
	GList* voice_list = NULL;
	int ret = 0;

	ret = g_cur_engine.pefuncs->foreach_voices(__supported_voice_cb, &voice_list);
	if (0 != ret || 0 >= g_list_length(voice_list)) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] fail to get voice list : result(%d)", ret);
		return false;
	}

	bool result;
	result = false;

	GList *iter = NULL;
	voice_s* voice;

	/* lang and type are not default type */
	if (0 != strncmp(lang, "default", strlen("default")) && 0 != type) {
		iter = g_list_first(voice_list);

		while (NULL != iter) {
			/* Get handle data from list */
			voice = iter->data;

			if (0 == strncmp(voice->language, lang, strlen(lang)) &&  voice->type == type) {
				*out_lang = strdup(voice->language);
				*out_type = voice->type;
				result = true;
				break;
			}

			iter = g_list_next(iter);
		}

	} else if (0 != strncmp(lang, "default", strlen("default")) && 0 == type) {
		/* Only type is default */
		if (0 == strncmp(lang, g_cur_engine.default_lang, strlen(g_cur_engine.default_lang))) {
			*out_lang = strdup(g_cur_engine.default_lang);
			*out_type = g_cur_engine.default_vctype;
			result = true;
		} else {
			voice_s* voice_selected = NULL;
			iter = g_list_first(voice_list);
			while (NULL != iter) {
				/* Get handle data from list */
				voice = iter->data;

				if (0 == strncmp(voice->language, lang, strlen(lang))) {
					voice_selected = voice;
					if (voice->type == g_cur_engine.default_vctype) {
						voice_selected = voice;
						break;
					}
				}
				iter = g_list_next(iter);
			}

			if (NULL != voice_selected) {
				*out_lang = strdup(voice_selected->language);
				*out_type = voice_selected->type;
				result = true;
			}
		}
	} else if (0 == strncmp(lang, "default", strlen("default")) && 0 != type) {
		/* Only lang is default */
		if (type == g_cur_engine.default_vctype) {
			*out_lang = strdup(g_cur_engine.default_lang);
			*out_type = g_cur_engine.default_vctype;
			result = true;
		} else {
			voice_s* voice_selected = NULL;
			iter = g_list_first(voice_list);
			while (NULL != iter) {
				/* Get handle data from list */
				voice = iter->data;

				if (0 == strncmp(voice->language, g_cur_engine.default_lang, strlen(g_cur_engine.default_lang))) {
					voice_selected = voice;
					if (voice->type == type) {
						voice_selected = voice;
						break;
					}
				}
				iter = g_list_next(iter);
			}

			if (NULL != voice_selected) {
				*out_lang = strdup(voice->language);
				*out_type = voice_selected->type;
				result = true;
			}
		}
	}

	if (true == result) {
		SECURE_SLOG(LOG_DEBUG, get_tag(), "[Engine Agent] Selected voice : lang(%s), type(%d)", *out_lang, *out_type);
	}

	__free_voice_list(voice_list);

	return result;
}

bool ttsd_engine_agent_is_same_engine(const char* engine_id)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not Initialized");
		return false;
	}

	if (false == g_cur_engine.is_loaded) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not loaded engine");
		return false;
	}

	/* compare current engine and engine id.*/
	if (0 == strncmp(g_cur_engine.engine_uuid, engine_id, strlen(engine_id))) {
		return true;
	}

	return false;
}

int ttsd_engine_agent_set_default_engine(const char* engine_id)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not Initialized");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	/* compare current engine and new engine.*/
	if (0 == strncmp(g_cur_engine.engine_uuid, engine_id, strlen(engine_id))) {
		SECURE_SLOG(LOG_DEBUG, get_tag(), "[Engine Agent] new engine(%s) is the same as current engine", engine_id);
		return 0;
	}

	bool is_engine_loaded = false;
	char* tmp_uuid = NULL;
	tmp_uuid = strdup(g_cur_engine.engine_uuid);
	if (NULL == tmp_uuid) {
			SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Current engine id is NULL");
			return TTSD_ERROR_OPERATION_FAILED;
	}

	is_engine_loaded = g_cur_engine.is_loaded;

	if (true == is_engine_loaded) {
		/* unload engine */
		if (0 != ttsd_engine_agent_unload_current_engine()) {
			SLOG(LOG_ERROR, get_tag(), "[Engine Agent Error] fail to unload current engine");
		}
	}

	/* change current engine */
	if (0 != __internal_set_current_engine(engine_id)) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Fail to set current engine. Recovery origin engine");

		/* roll back to old current engine. */
		__internal_set_current_engine(tmp_uuid);
		if (true == is_engine_loaded) {
			ttsd_engine_agent_load_current_engine();
		}

		if (tmp_uuid != NULL)	free(tmp_uuid);
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (true == is_engine_loaded) {
		/* load engine */
		if (0 != ttsd_engine_agent_load_current_engine()) {
			SLOG(LOG_ERROR, get_tag(), "[Engine Agent Error] Fail to load new engine. Recovery origin engine");

			/* roll back to old current engine. */
			__internal_set_current_engine(tmp_uuid);
			if (true == is_engine_loaded) {
				ttsd_engine_agent_load_current_engine();
			}

			if (tmp_uuid != NULL)	free(tmp_uuid);
			return TTSD_ERROR_OPERATION_FAILED;
		} else {
			SECURE_SLOG(LOG_DEBUG, get_tag(), "[Engine Agent SUCCESS] load new engine : %s", engine_id);
		}
	}

	if (tmp_uuid != NULL)	free(tmp_uuid);
	return 0;
}

int ttsd_engine_agent_set_default_voice(const char* language, int vctype)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not Initialized");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (false == g_cur_engine.is_loaded) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not loaded engine");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (false == g_cur_engine.pefuncs->is_valid_voice(language, vctype)) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Voice is NOT valid !!");
		return TTSD_ERROR_INVALID_VOICE;
	}

	int ret = -1;
	GSList *iter = NULL;
	ttsvoice_s* data = NULL;

	/* Update new default voice info */
	iter = g_slist_nth(g_cur_voices, 0);
	while (NULL != iter) {
		/* Get handle data from list */
		data = iter->data;

		if (NULL == data) {
			SECURE_SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Voice data is NULL");
			return TTSD_ERROR_OPERATION_FAILED;
		}

		if (0 == strcmp(data->lang, language) && data->type == vctype) {
			data->is_default = true;
			if (0 == data->client_ref_count) {
				/* load voice */
				if (NULL != g_cur_engine.pefuncs->load_voice) {
					ret = g_cur_engine.pefuncs->load_voice(data->lang, data->type);
					if (0 == ret) {
						SECURE_SLOG(LOG_DEBUG, get_tag(), "[Engine Agent SUCCESS] load voice : lang(%s), type(%d)", 
							data->lang, data->type);
						data->is_loaded = true;
					} else {
						SECURE_SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Fail to load voice : lang(%s), type(%d), result(%s)",
							data->lang, data->type, __ttsd_get_engine_error_code(ret));
					}
				} else {
					SECURE_SLOG(LOG_DEBUG, get_tag(), "[Engine Agent ERROR] Load voice of engine function is NULL");
				}
			}
			break;
		}

		/*Get next item*/
		iter = g_slist_next(iter);
	}

#ifdef ENGINE_AGENT_DEBUG
	ttsd_print_voicelist();
#endif

	/* Update old default voice info */
	iter = g_slist_nth(g_cur_voices, 0);
	while (NULL != iter) {
		/*Get handle data from list*/
		data = iter->data;

		if (NULL == data) {
			SECURE_SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Voice data is NULL");
			return TTSD_ERROR_OPERATION_FAILED;
		}

		if (0 == strcmp(data->lang, g_cur_engine.default_lang) && data->type == g_cur_engine.default_vctype) {
			data->is_default = false;
			if (0 == data->client_ref_count) {
				/* Unload voice */
				if (NULL != g_cur_engine.pefuncs->unload_voice) {
					ret = g_cur_engine.pefuncs->unload_voice(data->lang, data->type);
					if (0 == ret) {
						SECURE_SLOG(LOG_DEBUG, get_tag(), "[Engine Agent SUCCESS] Unload voice : lang(%s), type(%d)", 
							data->lang, data->type);
						data->is_loaded = false;
					} else {
						SECURE_SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Fail to unload voice : lang(%s), type(%d), result(%s)",
							data->lang, data->type, __ttsd_get_engine_error_code(ret));
					}
				} else {
					SECURE_SLOG(LOG_DEBUG, get_tag(), "[Engine Agent ERROR] Unload voice of engine function is NULL");
				}
			}
			break;
		}

		/*Get next item*/
		iter = g_slist_next(iter);
	}

	if (NULL != g_cur_engine.default_lang)	free(g_cur_engine.default_lang);

	g_cur_engine.default_lang = strdup(language);
	g_cur_engine.default_vctype = vctype;

#ifdef ENGINE_AGENT_DEBUG
	ttsd_print_voicelist();
#endif

	SECURE_SLOG(LOG_DEBUG, get_tag(), "[Engine Agent SUCCESS] Set default voice : lang(%s), type(%d)",
		g_cur_engine.default_lang, g_cur_engine.default_vctype);

	return 0;
}

int ttsd_engine_agent_set_default_speed(int speed)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not Initialized");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	g_cur_engine.default_speed = speed;

	return 0;
}

int ttsd_engine_agent_set_default_pitch(int pitch)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not Initialized");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (false == g_cur_engine.is_loaded) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not loaded engine");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_cur_engine.pefuncs->set_pitch) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not support pitch");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	int ret = g_cur_engine.pefuncs->set_pitch(pitch);
	if (0 != ret) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Fail to set pitch : pitch(%d), result(%s)",
			 pitch, __ttsd_get_engine_error_code(ret));
		return TTSD_ERROR_OPERATION_FAILED;
	}

	g_cur_engine.default_pitch = pitch;

	return 0;
}

/******************************************************************************************
* TTS Engine Interfaces for client
*******************************************************************************************/

int ttsd_engine_load_voice(const char* lang, const int vctype)
{
	/* 1. Find voice info */
	int ret = -1;
	GSList *iter = NULL;
	ttsvoice_s* data = NULL;

	iter = g_slist_nth(g_cur_voices, 0);
	while (NULL != iter) {
		/*Get handle data from list*/
		data = iter->data;

		if (NULL != data) {
			if (0 == strcmp(data->lang, lang) && data->type == vctype) {
				SLOG(LOG_DEBUG, get_tag(), "[Engine Agent] Find voice : default(%d) loaded(%d) ref(%d) lang(%s) type(%d)",
					 data->is_default, data->is_loaded,  data->client_ref_count, data->lang, data->type);
				break;
			}
		}

		/*Get next item*/
		iter = g_slist_next(iter);
		data = NULL;
	}

	if (NULL == data) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] This voice is not supported voice : lang(%s) type(%d)", lang, vctype);
		return TTSD_ERROR_OPERATION_FAILED;
	}

	/* 2. increse ref count */
	data->client_ref_count++;

	/* 3. if ref count change 0 to 1 and not default, load voice */
	if (1 == data->client_ref_count && false == data->is_default) {
		/* load voice */
		if (NULL != g_cur_engine.pefuncs->load_voice) {
			ret = g_cur_engine.pefuncs->load_voice(data->lang, data->type);
			if (0 == ret) {
				SECURE_SLOG(LOG_DEBUG, get_tag(), "[Engine Agent SUCCESS] Load voice : lang(%s), type(%d)", 
					data->lang, data->type);
				data->is_loaded = true;
			} else {
				SECURE_SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Fail to load voice : lang(%s), type(%d), result(%s)",
					data->lang, data->type, __ttsd_get_engine_error_code(ret));

				return TTSD_ERROR_OPERATION_FAILED;
			}
		} else {
			SECURE_SLOG(LOG_DEBUG, get_tag(), "[Engine Agent ERROR] Load voice of engine function is NULL");
		}
	} else {
		SECURE_SLOG(LOG_DEBUG, get_tag(), "[Engine Agent] Not load voice : default voice(%d) or ref count(%d)",
			data->is_default, data->client_ref_count);
	}

#ifdef ENGINE_AGENT_DEBUG
	ttsd_print_voicelist();
#endif

	return 0;
}

int ttsd_engine_unload_voice(const char* lang, const int vctype)
{
	/* 1. Find voice info */
	int ret = -1;
	GSList *iter = NULL;
	ttsvoice_s* data = NULL;

	iter = g_slist_nth(g_cur_voices, 0);
	while (NULL != iter) {
		/*Get handle data from list*/
		data = iter->data;

		if (NULL != data) {
			if (0 == strcmp(data->lang, lang) && data->type == vctype) {
				SLOG(LOG_DEBUG, get_tag(), "[Engine Agent] Find voice : default(%d) loaded(%d) ref(%d) lang(%s) type(%d)",
					 data->is_default, data->is_loaded,  data->client_ref_count, data->lang, data->type);
				break;
			}
		}

		/*Get next item*/
		iter = g_slist_next(iter);
		data = NULL;
	}

	if (NULL == data) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] This voice is not supported voice : lang(%s) type(%d)", lang, vctype);
		return TTSD_ERROR_OPERATION_FAILED;
	}

	/* 2. Decrese ref count */
	data->client_ref_count--;

	/* 3. if ref count change 0 and not default, load voice */
	if (0 == data->client_ref_count && false == data->is_default) {
		/* load voice */
		if (NULL != g_cur_engine.pefuncs->unload_voice) {
			ret = g_cur_engine.pefuncs->unload_voice(data->lang, data->type);
			if (0 == ret) {
				SECURE_SLOG(LOG_DEBUG, get_tag(), "[Engine Agent SUCCESS] Unload voice : lang(%s), type(%d)", 
					data->lang, data->type);
				data->is_loaded = false;
			} else {
				SECURE_SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Fail to unload voice : lang(%s), type(%d), result(%s)",
					data->lang, data->type, __ttsd_get_engine_error_code(ret));

				return TTSD_ERROR_OPERATION_FAILED;
			}
		} else {
			SECURE_SLOG(LOG_DEBUG, get_tag(), "[Engine Agent ERROR] Unload voice of engine function is NULL");
		}
	} else {
		SECURE_SLOG(LOG_DEBUG, get_tag(), "[Engine Agent] Not unload voice : default voice(%d) or ref count(%d)",
			data->is_default, data->client_ref_count);
	}

#ifdef ENGINE_AGENT_DEBUG
	ttsd_print_voicelist();
#endif
	return 0;
}

int ttsd_engine_start_synthesis(const char* lang, int vctype, const char* text, int speed, void* user_param)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not Initialized");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (false == g_cur_engine.is_loaded) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not loaded engine");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	/* select voice for default */
	char* temp_lang = NULL;
	int temp_type;
	if (true != ttsd_engine_select_valid_voice(lang, vctype, &temp_lang, &temp_type)) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Fail to select default voice");
		if (NULL != temp_lang)	free(temp_lang);
		return TTSD_ERROR_INVALID_VOICE;
	} else {
		SECURE_SLOG(LOG_DEBUG, get_tag(), "[Engine Agent] Start synthesis : language(%s), type(%d), speed(%d), text(%s)", 
			temp_lang, temp_type, speed, text);
	}

	if (NULL == g_cur_engine.pefuncs->start_synth) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] start_synth() of engine is NULL!!");
		if (NULL != temp_lang)	free(temp_lang);
		return TTSD_ERROR_OPERATION_FAILED;
	}

	int temp_speed;

	if (0 == speed) {
		temp_speed = g_cur_engine.default_speed;
	} else {
		temp_speed = speed;
	}

	/* synthesize text */
	int ret = 0;
	ret = g_cur_engine.pefuncs->start_synth(temp_lang, temp_type, text, temp_speed, user_param);
	if (0 != ret) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] ***************************************");
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] * synthesize error : %s *", __ttsd_get_engine_error_code(ret));
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] ***************************************");
		if (NULL != temp_lang)	free(temp_lang);
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (NULL != temp_lang)	free(temp_lang);
	return 0;
}

int ttsd_engine_cancel_synthesis()
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not Initialized");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (false == g_cur_engine.is_loaded) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not loaded engine");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_cur_engine.pefuncs->cancel_synth) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] cancel_synth() of engine is NULL!!");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	/* stop synthesis */
	int ret = 0;
	ret = g_cur_engine.pefuncs->cancel_synth();
	if (0 != ret) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] fail cancel synthesis : %s", __ttsd_get_engine_error_code(ret));
		return TTSD_ERROR_OPERATION_FAILED;
	}

	return 0;
}

bool __supported_voice_cb(const char* language, int type, void* user_data)
{
	GList** voice_list = (GList**)user_data;

	if (NULL == language || NULL == voice_list) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Input parameter is NULL in voice list callback!!!!");
		return false;
	}

	voice_s* voice = calloc(1, sizeof(voice_s));
	voice->language = strdup(language);
	voice->type = type;

	*voice_list = g_list_append(*voice_list, voice);

	return true;
}

int ttsd_engine_get_voice_list(GList** voice_list)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not Initialized");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (false == g_cur_engine.is_loaded) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not loaded engine");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	int ret = 0;
	ret = g_cur_engine.pefuncs->foreach_voices(__supported_voice_cb, (void*)voice_list);
	if (0 != ret) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Fail to get voice list : %s", __ttsd_get_engine_error_code(ret));
		return TTSD_ERROR_OPERATION_FAILED;
	}

	return 0;
}

int ttsd_engine_get_default_voice(char** lang, int* vctype)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not Initialized");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (false == g_cur_engine.is_loaded) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not loaded engine");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (NULL == lang || NULL == vctype) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] BAD Parameter");
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	if (NULL != g_cur_engine.default_lang) {
		*lang = strdup(g_cur_engine.default_lang);
		*vctype = g_cur_engine.default_vctype;

		SECURE_SLOG(LOG_DEBUG, get_tag(), "[Engine] Get default voice : language(%s), type(%d)", *lang, *vctype);
	} else {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Default voice is NULL");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	return 0;
}

void __free_voice_list(GList* voice_list)
{
	GList *iter = NULL;
	voice_s* data = NULL;

	/* if list have item */
	if (g_list_length(voice_list) > 0) {
		/* Get a first item */
		iter = g_list_first(voice_list);

		while (NULL != iter) {
			data = iter->data;

			if (NULL != data) {
				if (NULL != data->language)	free(data->language);
				free(data);
			}

			voice_list = g_list_remove_link(voice_list, iter);

			iter = g_list_first(voice_list);
		}
	}
}

/*
* TTS Engine Callback Functions											`				  *
*/
bool __result_cb(ttsp_result_event_e event, const void* data, unsigned int data_size, ttsp_audio_type_e audio_type, int rate, void *user_data)
{
	g_result_cb(event, data, data_size, audio_type, rate, user_data);
	return true;
}

/* function for debugging */
int ttsd_print_enginelist()
{
	GList *iter = NULL;
	ttsengine_info_s *data = NULL;

	if (g_list_length(g_engine_list) > 0) {
		iter = g_list_first(g_engine_list);

		SLOG(LOG_DEBUG, get_tag(), "----- engine list -----");

		int i = 1;
		while (NULL != iter) {
			data = iter->data;

			SECURE_SLOG(LOG_DEBUG, get_tag(), "[%dth]", i);
			SECURE_SLOG(LOG_DEBUG, get_tag(), "engine uuid : %s", data->engine_uuid);
			SECURE_SLOG(LOG_DEBUG, get_tag(), "engine name : %s", data->engine_name);
			SECURE_SLOG(LOG_DEBUG, get_tag(), "engine path : %s", data->engine_path);
			SECURE_SLOG(LOG_DEBUG, get_tag(), "setting ug path : %s", data->setting_ug_path);

			iter = g_list_next(iter);
			i++;
		}
		SLOG(LOG_DEBUG, get_tag(), "-----------------------");
		SLOG(LOG_DEBUG, get_tag(), "  ");
	} else {
		SLOG(LOG_DEBUG, get_tag(), "----- engine list -----");
		SLOG(LOG_DEBUG, get_tag(), "No Engine in directory");
		SLOG(LOG_DEBUG, get_tag(), "-----------------------");
	}

	return 0;
}

int ttsd_print_voicelist()
{
	/* Test log */
	GSList *iter = NULL;
	ttsvoice_s* data = NULL;

	SLOG(LOG_DEBUG, get_tag(), "=== Voice list ===");

	if (g_slist_length(g_cur_voices) > 0) {
		/* Get a first item */
		iter = g_slist_nth(g_cur_voices, 0);

		int i = 1;
		while (NULL != iter) {
			/*Get handle data from list*/
			data = iter->data;

			if (NULL == data || NULL == data->lang) {
				SLOG(LOG_ERROR, get_tag(), "[ERROR] Data is invalid");
				return 0;
			}

			SLOG(LOG_DEBUG, get_tag(), "[%dth] default(%d) loaded(%d) ref(%d) lang(%s) type(%d)",
				 i, data->is_default, data->is_loaded,  data->client_ref_count, data->lang, data->type);

			/*Get next item*/
			iter = g_slist_next(iter);
			i++;
		}
	}

	SLOG(LOG_DEBUG, get_tag(), "==================");

	return 0;
}