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


#include <dlfcn.h>
#include <dirent.h>

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

	/* info for using engine load*/
	bool	is_set;
	bool	is_loaded;		
	bool	need_network;
	void	*handle;

	/* engine base setting */
	char*	default_lang;
	int	default_vctype;
	int	default_speed;

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


/** Init flag */
static bool g_agent_init;

/** TTS engine list */
static GList *g_engine_list;		

/** Current engine information */
static ttsengine_s g_cur_engine;

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
bool __result_cb(ttsp_result_event_e event, const void* data, unsigned int data_size, void *user_data);

/** Callback function for voice list */
bool __supported_voice_cb(const char* language, ttsp_voice_type_e type, void* user_data);

/** Free voice list */
void __free_voice_list(GList* voice_list);

/** Callback function for engine info */
void __engine_info_cb(const char* engine_uuid, const char* engine_name, const char* setting_ug_name, 
		      bool use_network, void* user_data);

/** Callback fucntion for engine setting */
bool __engine_setting_cb(const char* key, const char* value, void* user_data);


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
	g_cur_engine.pefuncs = (ttspe_funcs_s*)g_malloc0( sizeof(ttspe_funcs_s) );
	g_cur_engine.pdfuncs = (ttspd_funcs_s*)g_malloc0( sizeof(ttspd_funcs_s) );

	g_agent_init = true;

	if (0 != ttsd_config_get_default_voice(&(g_cur_engine.default_lang), &(g_cur_engine.default_vctype))) {
		SLOG(LOG_WARN, get_tag(), "[Server WARNING] There is No default voice in config"); 
		/* Set default voice */
		g_cur_engine.default_lang = strdup("en_US");
		g_cur_engine.default_vctype = TTSP_VOICE_TYPE_FEMALE;
	}

	if (0 != ttsd_config_get_default_speed(&(g_cur_engine.default_speed))) {
		SLOG(LOG_WARN, get_tag(), "[Server WARNING] There is No default speed in config"); 
		ttsd_config_set_default_speed((int)TTSP_SPEED_NORMAL);
		g_cur_engine.default_speed = TTSP_SPEED_NORMAL;
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
	ttsengine_s *data = NULL;

	if (g_list_length(g_engine_list) > 0) {
		/* Get a first item */
		iter = g_list_first(g_engine_list);
		while (NULL != iter) {
			/* Get data from item */
			data = iter->data;
			iter = g_list_remove(iter, data);
		}
	}
	g_list_free(iter);
	/* release current engine data */
	if (g_cur_engine.pefuncs != NULL)
		g_free(g_cur_engine.pefuncs);
	
	if (g_cur_engine.pdfuncs != NULL)
		g_free(g_cur_engine.pdfuncs);

	g_result_cb = NULL;
	g_agent_init = false;

	if (g_cur_engine.default_lang != NULL) 
		g_free(g_cur_engine.default_lang);


	SLOG(LOG_DEBUG, get_tag(), "[Engine Agent SUCCESS] Release Engine Agent");

	return 0;
}

int ttsd_engine_agent_initialize_current_engine()
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not Initialized" );
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
			char* default_engine = "27F277E9-BBC4-4dca-B553-D9884A3CDAA0";

			while (NULL != iter) {
				data = iter->data;

				if (0 == strncmp(data->engine_uuid, default_engine, strlen(default_engine))) {
					/* current data is default engine */
					break;
				}

				iter = g_list_next(iter);
			}
		} else {
			SLOG(LOG_WARN, get_tag(), "[Engine Agent WARNING] Fail to set a engine of engine list\n");
			return TTSD_ERROR_OPERATION_FAILED;	
		}

		if (NULL != data->engine_uuid) {
			cur_engine_uuid = strdup(data->engine_uuid); 
		} else {
			SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Data of current engine is corrupt\n");
			return TTSD_ERROR_OPERATION_FAILED;
		}
		
		is_get_engineid_from_config = false;
	} else {
		is_get_engineid_from_config = true;
	}

	/* check whether cur engine uuid is valid or not. */
	if (0 != __internal_check_engine_id(cur_engine_uuid)) {
		SLOG(LOG_WARN, get_tag(), "[Engine Agent WARNING] It is not valid engine id from config");

		GList *iter = NULL;
		
		if (g_list_length(g_engine_list) > 0) 
			iter = g_list_first(g_engine_list);
		else {
			SLOG(LOG_WARN, get_tag(), "[Engine Agent ERROR] NO TTS Engine !!");
			return TTSD_ERROR_OPERATION_FAILED;	
		}

		if (cur_engine_uuid != NULL)	
			g_free(cur_engine_uuid);

		ttsengine_info_s *data = NULL;
		data = iter->data;

		cur_engine_uuid = g_strdup(data->engine_uuid);

		is_get_engineid_from_config = false;
	}

	if (NULL != cur_engine_uuid) 
		SLOG(LOG_DEBUG, get_tag(), "[Engine Agent] Current Engine Id : %s", cur_engine_uuid);
	else 
		return TTSD_ERROR_OPERATION_FAILED;

	/* set current engine */
	if (0 != __internal_set_current_engine(cur_engine_uuid)) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] fail to set current engine ");
		return TTSD_ERROR_OPERATION_FAILED;
	} 

	if (false == is_get_engineid_from_config) {
		if (0 != ttsd_config_set_default_engine(cur_engine_uuid))
			SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] fail to set id to config"); 
	}

	if (NULL != cur_engine_uuid)	
		g_free(cur_engine_uuid);

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

	temp->engine_uuid = g_strdup(engine_uuid);
	temp->engine_name = g_strdup(engine_name);
	temp->setting_ug_path = g_strdup(setting_ug_name);
	temp->use_network = use_network;
}

int __internal_get_engine_info(const char* filepath, ttsengine_info_s** info)
{
	char *error;
	void* handle;

	handle = dlopen (filepath, RTLD_LAZY );

	if (!handle) {
		SLOG(LOG_WARN, get_tag(), "[Engine Agent] Invalid engine : %s", filepath); 
		return TTSD_ERROR_OPERATION_FAILED;
	}

	/* link engine to daemon */
	dlsym(handle, "ttsp_load_engine");
	if ((error = dlerror()) != NULL) {
		SLOG(LOG_WARN, get_tag(), "[Engine Agent] Invalid engine. Fail to open ttsp_load_engine : %s", filepath);
		dlclose(handle);
		return TTSD_ERROR_OPERATION_FAILED;
	}

	dlsym(handle, "ttsp_unload_engine");
	if ((error = dlerror()) != NULL) {
		SLOG(LOG_WARN, get_tag(), "[Engine Agent] Invalid engine. Fail to open ttsp_unload_engine : %s", filepath);
		dlclose(handle);
		return TTSD_ERROR_OPERATION_FAILED;
	}

	int (*get_engine_info)(ttsp_engine_info_cb callback, void* user_data);

	get_engine_info = (int (*)(ttsp_engine_info_cb, void*))dlsym(handle, "ttsp_get_engine_info");
	if (NULL != (error = dlerror()) || NULL == get_engine_info) {
		SLOG(LOG_WARN, get_tag(), "[Engine Agent] ttsp_get_engine_info() link error");
		dlclose(handle);
		return TTSD_ERROR_OPERATION_FAILED;
	}

	ttsengine_info_s* temp;
	temp = (ttsengine_info_s*)g_malloc0(sizeof(ttsengine_info_s));

	/* get engine info */
	if (0 != get_engine_info(&__engine_info_cb, (void*)temp)) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] fail to get engine info");
		dlclose(handle);
		g_free(temp);
		return TTSD_ERROR_OPERATION_FAILED;
	}

	/* close engine */
	dlclose(handle);

	if (TTSD_MODE_NOTIFICATION == ttsd_get_mode() || TTSD_MODE_SCREEN_READER == ttsd_get_mode()) {
		if (true == temp->use_network) {
			free(temp);
			SLOG(LOG_ERROR, get_tag(), "[Engine Agent WARNING] %s is invalid because of network based engine.", temp->engine_name);
			return TTSD_ERROR_OPERATION_FAILED;
		}
	}

	temp->engine_path = g_strdup(filepath);
	
	SLOG(LOG_DEBUG, get_tag(), "----- Valid engine");
	SLOG(LOG_DEBUG, get_tag(), "Engine uuid : %s", temp->engine_uuid);
	SLOG(LOG_DEBUG, get_tag(), "Engine name : %s", temp->engine_name);
	SLOG(LOG_DEBUG, get_tag(), "Setting ug path : %s", temp->setting_ug_path);
	SLOG(LOG_DEBUG, get_tag(), "Engine path : %s", temp->engine_path);
	SLOG(LOG_DEBUG, get_tag(), "Use network : %s", temp->use_network ? "true":"false");
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

			if (data != NULL)
				g_free(data);

			g_engine_list = g_list_remove_link(g_engine_list, iter);
			iter = g_list_first(g_engine_list);
		}
	}

	/* get file name from engine directory and get engine information from each filename */
	DIR *dp;
	struct dirent *dirp;
	dp = opendir(ENGINE_DIRECTORY_DEFAULT);

	if (dp != NULL) {
		while ((dirp = readdir(dp)) != NULL) {
			ttsengine_info_s* info;
			char* filepath = NULL;
			int file_size;

			file_size = strlen(ENGINE_DIRECTORY_DEFAULT) + strlen(dirp->d_name) + 5;
			filepath = (char*)g_malloc0( sizeof(char) * file_size);

			if (NULL != filepath) { 
				strncpy(filepath, ENGINE_DIRECTORY_DEFAULT, strlen(ENGINE_DIRECTORY_DEFAULT) );
				strncat(filepath, "/", strlen("/") );
				strncat(filepath, dirp->d_name, strlen(dirp->d_name) );
			} else {
				SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not enough memory!!" );
				continue;	
			}

			/* get its info and update engine list */
			if (0 == __internal_get_engine_info(filepath, &info)) {
				/* add engine info to g_engine_list */
				g_engine_list = g_list_append(g_engine_list, info);
			}

			if (NULL != filepath)
				g_free(filepath);
		}

		closedir(dp);
	}

	dp = opendir(ENGINE_DIRECTORY_DOWNLOAD);

	if (dp != NULL) {
		while ((dirp = readdir(dp)) != NULL) {
			ttsengine_info_s* info;
			char* filepath = NULL;
			int file_size;

			file_size = strlen(ENGINE_DIRECTORY_DOWNLOAD) + strlen(dirp->d_name) + 5;
			filepath = (char*)g_malloc0( sizeof(char) * file_size);

			if (NULL != filepath) { 
				strcpy(filepath, ENGINE_DIRECTORY_DOWNLOAD);
				strcat(filepath, "/");
				strcat(filepath, dirp->d_name);
			} else {
				SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not enough memory!!" );
				continue;	
			}

			/* get its info and update engine list */
			if (0 == __internal_get_engine_info(filepath, &info)) {
				/* add engine info to g_engine_list */
				g_engine_list = g_list_append(g_engine_list, info);
			}

			if (NULL != filepath)
				g_free(filepath);
		}

		closedir(dp);
	}

	if (g_list_length(g_engine_list) <= 0) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] No Engine\n");
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
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] __internal_set_current_engine : Cannot find engine id");
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
	if (g_cur_engine.engine_uuid != NULL)	g_free(g_cur_engine.engine_uuid);
	if (g_cur_engine.engine_name != NULL)	g_free(g_cur_engine.engine_name);
	if (g_cur_engine.engine_path != NULL)	g_free(g_cur_engine.engine_path);

	if (NULL == data->engine_uuid || NULL == data->engine_name || NULL == data->engine_path) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] __internal_set_current_engine : Engine data is NULL");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	g_cur_engine.engine_uuid = g_strdup(data->engine_uuid);
	g_cur_engine.engine_name = g_strdup(data->engine_name);
	g_cur_engine.engine_path = g_strdup(data->engine_path);

	g_cur_engine.handle = NULL;
	g_cur_engine.is_loaded = false;
	g_cur_engine.is_set = true;
	g_cur_engine.need_network = data->use_network;

	SLOG(LOG_DEBUG, get_tag(), "-----");
	SLOG(LOG_DEBUG, get_tag(), "Current engine uuid : %s", g_cur_engine.engine_uuid);
	SLOG(LOG_DEBUG, get_tag(), "Current engine name : %s", g_cur_engine.engine_name);
	SLOG(LOG_DEBUG, get_tag(), "Current engine path : %s", g_cur_engine.engine_path);
	SLOG(LOG_DEBUG, get_tag(), "-----");

	return 0;
}

int ttsd_engine_agent_load_current_engine()
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not Initialized" );
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (false == g_cur_engine.is_set) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] ttsd_engine_agent_load_current_engine : No Current Engine ");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	/* check whether current engine is loaded or not */
	if (true == g_cur_engine.is_loaded ) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent] ttsd_engine_agent_load_current_engine : Engine has already been loaded " );
		return 0;
	}

	SLOG(LOG_DEBUG, get_tag(), "[Engine Agent] current engine path : %s\n", g_cur_engine.engine_path);

	/* open engine */
	char *error = NULL;
	g_cur_engine.handle = dlopen(g_cur_engine.engine_path, RTLD_LAZY); /* RTLD_LAZY RTLD_NOW*/

	if (NULL != (error = dlerror()) || NULL == g_cur_engine.handle) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] fail to get current engine handle : dlopen error ($s)", error);
		return -2;
	}

	g_cur_engine.ttsp_unload_engine = (int (*)())dlsym(g_cur_engine.handle, "ttsp_unload_engine");
	if (NULL != (error = dlerror()) || NULL == g_cur_engine.ttsp_unload_engine) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] fail to link daemon to ttsp_unload_engine() of current engine : (%s)", error);
		return -3;
	}

	g_cur_engine.ttsp_load_engine = (int (*)(const ttspd_funcs_s* , ttspe_funcs_s*) )dlsym(g_cur_engine.handle, "ttsp_load_engine");
	if (NULL != (error = dlerror()) || NULL == g_cur_engine.ttsp_load_engine) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] fail to link daemon to ttsp_load_engine() of current engine : %s", error);
		return -3;
	}

	/* load engine */
	g_cur_engine.pdfuncs->version = 1;
	g_cur_engine.pdfuncs->size = sizeof(ttspd_funcs_s);

	int ret = 0;
	ret = g_cur_engine.ttsp_load_engine(g_cur_engine.pdfuncs, g_cur_engine.pefuncs); 
	if (0 != ret) {
		printf("Fail load '%s' engine\n", g_cur_engine.engine_path);
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] fail to load engine : result(%d)");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	SLOG(LOG_DEBUG, get_tag(), "[Engine Agent] engine info : version(%d), size(%d)\n",g_cur_engine.pefuncs->version, g_cur_engine.pefuncs->size );

	/* engine error check */
	if (g_cur_engine.pefuncs->size != sizeof(ttspe_funcs_s)) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] ttsd_engine_agent_load_current_engine : current engine is not valid");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	/* initalize engine */
	if (NULL == g_cur_engine.pefuncs->initialize) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] init function of engine is NULL!!");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	ret = g_cur_engine.pefuncs->initialize(__result_cb);
	if (0 != ret) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] fail to initialize current engine : result(%d)\n", ret);
		return TTSD_ERROR_OPERATION_FAILED;
	}

	/* select default voice */
	bool set_voice = false;
	if (NULL != g_cur_engine.default_lang) {
		if (NULL == g_cur_engine.pefuncs->is_valid_voice) {
			SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] is_valid_voice() of engine is NULL!!");
			return TTSD_ERROR_OPERATION_FAILED;
		}

		if (true == g_cur_engine.pefuncs->is_valid_voice(g_cur_engine.default_lang, g_cur_engine.default_vctype)) {
			set_voice = true;
			SLOG(LOG_DEBUG, get_tag(), "[Engine Agent SUCCESS] Set origin default voice to current engine : lang(%s), type(%d)", 
				g_cur_engine.default_lang,  g_cur_engine.default_vctype);
		} else {
			SLOG(LOG_WARN, get_tag(), "[Engine Agent WARNING] Fail set origin default voice : lang(%s), type(%d)\n",
				g_cur_engine.default_lang, g_cur_engine.default_vctype);
		}
	}

	if (false == set_voice) {
		if (NULL == g_cur_engine.pefuncs->foreach_voices) {
			SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] get_voice_list of engine is NULL!!");
			return TTSD_ERROR_OPERATION_FAILED;
		}

		/* get language list */
		int ret;
		GList* voice_list = NULL;

		ret = g_cur_engine.pefuncs->foreach_voices(__supported_voice_cb, &voice_list);

		if (0 == ret && 0 < g_list_length(voice_list)) {
			GList *iter = NULL;
			voice_s* voice;
			
			iter = g_list_first(voice_list);

			if (NULL != iter) {
				voice = iter->data;

				if (true != g_cur_engine.pefuncs->is_valid_voice(voice->language, voice->type)) {
					SLOG(LOG_ERROR, get_tag(), "[Engine ERROR] Fail voice is NOT valid");
					return TTSD_ERROR_OPERATION_FAILED;
				}

				ttsd_config_set_default_voice(voice->language, (int)voice->type);
				
				g_cur_engine.default_lang = g_strdup(voice->language);
				g_cur_engine.default_vctype = voice->type;

				SLOG(LOG_DEBUG, get_tag(), "[Engine Agent SUCCESS] Select default voice : lang(%s), type(%d)", 
					voice->language,  voice->type);

				
			} else {
				SLOG(LOG_ERROR, get_tag(), "[Engine ERROR] Fail to get language list : result(%d)\n", ret);
				return TTSD_ERROR_OPERATION_FAILED;
			}

			__free_voice_list(voice_list);
		} else {
			SLOG(LOG_ERROR, get_tag(), "[Engine ERROR] Fail to get language list : result(%d)\n", ret);
			return TTSD_ERROR_OPERATION_FAILED;
		}
	} 
 
	g_cur_engine.is_loaded = true;

	return 0;
}

int ttsd_engine_agent_unload_current_engine()
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not Initialized" );
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (false == g_cur_engine.is_set) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] No Current Engine ");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (false == g_cur_engine.is_loaded) {
		SLOG(LOG_DEBUG, get_tag(), "[Engine Agent] Engine has already been unloaded " );
		return 0;
	}

	/* shutdown engine */
	if (NULL == g_cur_engine.pefuncs->deinitialize) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] The deinitialize() of engine is NULL!!");
	} else {
		int ret = 0;
		ret = g_cur_engine.pefuncs->deinitialize();
		if (0 != ret) {
			SLOG(LOG_ERROR, get_tag(), "[Engine Agent] Fail deinitialize() : result(%d)\n", ret);
		}
	}

	/* unload engine */
	g_cur_engine.ttsp_unload_engine();
	
	dlclose(g_cur_engine.handle);

	/* reset current engine data */
	g_cur_engine.handle = NULL;
	g_cur_engine.is_loaded = false;

	return 0;
}

bool ttsd_engine_agent_need_network()
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not Initialized" );
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (false == g_cur_engine.is_loaded) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not loaded engine");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	return g_cur_engine.need_network;
}

bool ttsd_engine_select_valid_voice(const char* lang, int type, char** out_lang, ttsp_voice_type_e* out_type)
{
	if (NULL == lang || NULL == out_lang || NULL == out_type) {
		return false;
	}

	SLOG(LOG_DEBUG, get_tag(), "[Engine Agent] Select voice : input lang(%s) , input type(%d), default lang(%s), default type(%d)", 
		lang, type, g_cur_engine.default_lang, g_cur_engine.default_vctype);

	
	/* case 1 : Both are default */
	if (0 == strncmp(lang, "default", strlen("default")) && 0 == type) {
		*out_lang = strdup(g_cur_engine.default_lang);
		*out_type = g_cur_engine.default_vctype;
		return true;
	} 
	
	/* Get voice list */
	if (NULL == g_cur_engine.pefuncs->foreach_voices) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] foreach_voices of engine is NULL!!");
		return TTSD_ERROR_OPERATION_FAILED;
	}

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

				if (0 == strncmp(voice->language, g_cur_engine.default_lang, strlen(g_cur_engine.default_lang)) ) {
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
		SLOG(LOG_DEBUG, get_tag(), "[Engine Agent] Selected voice : lang(%s), type(%d)", *out_lang, *out_type);
	}

	__free_voice_list(voice_list);

	return result;
}

bool ttsd_engine_agent_is_same_engine(const char* engine_id)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not Initialized" );
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

int ttsd_engine_agent_set_default_voice(const char* language, ttsp_voice_type_e vctype)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not Initialized" );
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (false == g_cur_engine.is_loaded) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not loaded engine");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_cur_engine.pefuncs->is_valid_voice) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] The function of engine is NULL!!");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	int ret = -1;
	if(false == g_cur_engine.pefuncs->is_valid_voice(language, vctype)) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Voice is NOT valid !!");
		return TTSD_ERROR_INVALID_VOICE;
	}

	if (NULL != g_cur_engine.default_lang)
		free(g_cur_engine.default_lang);

	g_cur_engine.default_lang = strdup(language);
	g_cur_engine.default_vctype = vctype;

	ret = ttsd_config_set_default_voice(language, (int)vctype);
	if (0 == ret) {
		SLOG(LOG_DEBUG, get_tag(), "[Engine Agent SUCCESS] Set default voice : lang(%s), type(%d)",
			g_cur_engine.default_lang, g_cur_engine.default_vctype); 
	} else {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] fail to write default voice to config (%d)", ret); 
		return TTSD_ERROR_OPERATION_FAILED;
	}

	return 0;
}

/******************************************************************************************
* TTS Engine Interfaces for client
*******************************************************************************************/

int ttsd_engine_start_synthesis(const char* lang, const ttsp_voice_type_e vctype, const char* text, const int speed, void* user_param)
{
	SLOG(LOG_DEBUG, get_tag(), "[Engine Agent] start ttsd_engine_start_synthesis()");

	if (false == g_agent_init) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not Initialized" );
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (false == g_cur_engine.is_loaded) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not loaded engine");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	/* select voice for default */
	char* temp_lang = NULL;
	ttsp_voice_type_e temp_type;
	if (true != ttsd_engine_select_valid_voice(lang, vctype, &temp_lang, &temp_type)) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Fail to select default voice");
		return TTSD_ERROR_INVALID_VOICE;
	} else {
		SLOG(LOG_DEBUG, get_tag(), "[Engine Agent] Start synthesis : language(%s), type(%d), speed(%d), text(%s)", 
			temp_lang, temp_type, speed, text);
	}

	if (NULL == g_cur_engine.pefuncs->start_synth) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] start_synth() of engine is NULL!!");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	ttsp_speed_e temp_speed;

	if (0 == speed) {
		temp_speed = g_cur_engine.default_speed;
	} else {
		temp_speed = speed;
	}

	/* synthesize text */
	int ret = 0;
	ret = g_cur_engine.pefuncs->start_synth(temp_lang, temp_type, text, temp_speed, user_param);
	if (0 != ret) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] ***************************************", ret);
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] * synthesize error : result(%6d) *", ret);
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] ***************************************", ret);
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (NULL == temp_lang)
		free(temp_lang);

	return 0;
}


int ttsd_engine_cancel_synthesis()
{
	SLOG(LOG_DEBUG, get_tag(), "[Engine Agent] start ttsd_engine_cancel_synthesis()");
	
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not Initialized" );
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
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] fail cancel synthesis : result(%d)", ret);
		return TTSD_ERROR_OPERATION_FAILED;
	}

	return 0;
}

int ttsd_engine_get_audio_format( ttsp_audio_type_e* type, int* rate, int* channels)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not Initialized" );
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (false == g_cur_engine.is_loaded) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not loaded engine");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_cur_engine.pefuncs->get_audio_format) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] get_audio_format() of engine is NULL!!");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	int ret = 0;
	ret = g_cur_engine.pefuncs->get_audio_format(type, rate, channels);
	if (0 != ret) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] fail to get audio format : result(%d)", ret);
		return TTSD_ERROR_OPERATION_FAILED;
	} else
		SLOG(LOG_DEBUG, get_tag(), "[Engine Agent] get audio format : type(%d), rate(%d), channel(%d)", *type, *rate, *channels);
	
	return 0;
}

bool __supported_voice_cb(const char* language, ttsp_voice_type_e type, void* user_data)
{
	GList** voice_list = (GList**)user_data;

	if (NULL == language || NULL == voice_list) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Input parameter is NULL in voice list callback!!!!");
		return false;
	}

	SLOG(LOG_DEBUG, get_tag(), "-- Language(%s), Type(%d)", language, type);

	voice_s* voice = g_malloc0(sizeof(voice_s));
	voice->language = strdup(language);
	voice->type = type;

	*voice_list = g_list_append(*voice_list, voice);

	return true;
}

int ttsd_engine_get_voice_list(GList** voice_list)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not Initialized" );
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (false == g_cur_engine.is_loaded) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not loaded engine");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_cur_engine.pefuncs->foreach_voices) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] The function of engine is NULL!!");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	int ret = 0;
	ret = g_cur_engine.pefuncs->foreach_voices(__supported_voice_cb, (void*)voice_list);
	if (0 != ret) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Fail to get voice list : result(%d)", ret);
		return TTSD_ERROR_OPERATION_FAILED;
	}

	return 0;
}

int ttsd_engine_get_default_voice( char** lang, ttsp_voice_type_e* vctype )
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not Initialized" );
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
		*lang = g_strdup(g_cur_engine.default_lang);
		*vctype = g_cur_engine.default_vctype;

		SLOG(LOG_DEBUG, get_tag(), "[Engine] Get default voice : language(%s), type(%d)\n", *lang, *vctype);
	} else {
		if (NULL == g_cur_engine.pefuncs->foreach_voices) {
			SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] get_voice_list of engine is NULL!!");
			return TTSD_ERROR_OPERATION_FAILED;
		}

		/* get language list */
		int ret;
		GList* voice_list = NULL;

		ret = g_cur_engine.pefuncs->foreach_voices(__supported_voice_cb, &voice_list);

		if (0 == ret && 0 < g_list_length(voice_list)) {
			GList *iter = NULL;
			voice_s* voice;

			iter = g_list_first(voice_list);

			if (NULL != iter) {
				voice = iter->data;
				
				if (true != g_cur_engine.pefuncs->is_valid_voice(voice->language, voice->type)) {
					SLOG(LOG_ERROR, get_tag(), "[Engine ERROR] Fail voice is NOT valid ");
					return TTSD_ERROR_OPERATION_FAILED;
				}
				
				ttsd_config_set_default_voice(voice->language, (int)voice->type);
				
				if (NULL != g_cur_engine.default_lang)
					g_free(g_cur_engine.default_lang);

				g_cur_engine.default_lang = g_strdup(voice->language);
				g_cur_engine.default_vctype = voice->type;

				*lang = g_strdup(g_cur_engine.default_lang);
				*vctype = g_cur_engine.default_vctype = voice->type;

				SLOG(LOG_DEBUG, get_tag(), "[Engine] Get default voice (New selected) : language(%s), type(%d)\n", *lang, *vctype);
			} else {
				SLOG(LOG_ERROR, get_tag(), "[Engine ERROR] Fail to get language list : result(%d)\n", ret);
				return TTSD_ERROR_OPERATION_FAILED;
			}

			__free_voice_list(voice_list);
		} else {
			SLOG(LOG_ERROR, get_tag(), "[Engine ERROR] Fail to get language list : result(%d)\n", ret);
			return TTSD_ERROR_OPERATION_FAILED;
		}
	}
	
	return 0;
}

/*
* TTS Engine Interfaces for setting
*/
int ttsd_engine_setting_get_engine_list(GList** engine_list)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not Initialized" );
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (NULL == engine_list) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Input parameter is NULL" );
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	/* update engine list */
	if (0 != __internal_update_engine_list()) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] fail __internal_update_engine_list()");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	GList *iter = NULL;
	ttsengine_info_s *data = NULL;

	iter = g_list_first(g_engine_list);

	SLOG(LOG_DEBUG, get_tag(), "----- [Engine Agent] engine list -----");
	
	while (NULL != iter) {
		engine_s* temp_engine;
	
		temp_engine = (engine_s*)g_malloc0(sizeof(engine_s));

		data = iter->data;

		temp_engine->engine_id = strdup(data->engine_uuid);
		temp_engine->engine_name = strdup(data->engine_name);
		temp_engine->ug_name = strdup(data->setting_ug_path);

		*engine_list = g_list_append(*engine_list, temp_engine);

		iter = g_list_next(iter);

		SLOG(LOG_DEBUG, get_tag(), " -- engine id(%s) engine name(%s) ug name(%s)", 
			temp_engine->engine_id, temp_engine->engine_name, temp_engine->ug_name);
		
	}

	SLOG(LOG_DEBUG, get_tag(), "--------------------------------------");
	
	return 0;
}

int ttsd_engine_setting_get_engine(char** engine_id)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not Initialized" );
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (false == g_cur_engine.is_loaded) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not loaded engine");
		return TTSD_ERROR_ENGINE_NOT_FOUND;
	}

	*engine_id = strdup(g_cur_engine.engine_uuid);

	return 0;
}

int ttsd_engine_setting_set_engine(const char* engine_id)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not Initialized" );
		return TTSD_ERROR_OPERATION_FAILED;
	}

	/* compare current engine and new engine.*/
	if (0 == strncmp(g_cur_engine.engine_uuid, engine_id, strlen(engine_id))) {
		SLOG(LOG_DEBUG, get_tag(), "[Engine Agent] new engine(%s) is the same as current engine(%s)", engine_id, g_cur_engine.engine_uuid);
		return 0;
	}

	char* tmp_uuid = NULL;
	tmp_uuid = strdup(g_cur_engine.engine_uuid);

	/* unload engine */
	if (0 != ttsd_engine_agent_unload_current_engine()) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent Error] fail to unload current engine");
	} else {
		SLOG(LOG_DEBUG, get_tag(), "[Engine Agent SUCCESS] unload current engine");
	}

	/* change current engine */
	if (0 != __internal_set_current_engine(engine_id)) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] fail __internal_set_current_engine()");

		/* roll back to old current engine. */
		__internal_set_current_engine(tmp_uuid);
		ttsd_engine_agent_load_current_engine();
		
		if (tmp_uuid != NULL)	
			free(tmp_uuid);

		return TTSD_ERROR_OPERATION_FAILED;
	}

	/* load engine */
	if (0 != ttsd_engine_agent_load_current_engine()) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent Error] fail to load new engine");
		
		if( tmp_uuid != NULL )	
			free(tmp_uuid);

		return TTSD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, get_tag(), "[Engine Agent SUCCESS] load new engine");
	}

	/* save engine id to config */
	if (0 != ttsd_config_set_default_engine(engine_id)) {
		SLOG(LOG_WARN, get_tag(), "[Engine Agent WARNING] Fail to save engine id to config"); 
	}

	if (tmp_uuid != NULL)	
		free(tmp_uuid);

	return 0;
}

int ttsd_engine_setting_get_voice_list(char** engine_id, GList** voice_list)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not Initialized" );
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (false == g_cur_engine.is_loaded) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not loaded engine");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	int ret = 0;

	/* get language list from engine*/
	ret = ttsd_engine_get_voice_list(voice_list);
	if (0 != ret) {
		SLOG(LOG_ERROR, get_tag(), "[Server Error] fail ttsd_engine_get_voice_list() ");
		return ret;
	}

	*engine_id = strdup(g_cur_engine.engine_uuid);
	
	return 0;
}

int ttsd_engine_setting_get_default_voice(char** language, ttsp_voice_type_e* vctype)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not Initialized" );
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (false == g_cur_engine.is_loaded) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not loaded engine");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	/* get current language from engine*/
	int ret = 0;
	char* temp_lang;
	ttsp_voice_type_e temp_int;

	ret = ttsd_engine_get_default_voice(&temp_lang, &temp_int);
	if (0 != ret) {
		SLOG(LOG_ERROR, get_tag(), "[Server Error] fail ttsd_engine_get_default_voice()");
		return ret;
	} 

	if (NULL != temp_lang) {
		*language = strdup(temp_lang);
		*vctype = temp_int;
		free(temp_lang);
	} else {
		SLOG(LOG_ERROR, get_tag(), "[Server Error] fail to get default language");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	return 0;
}

int ttsd_engine_setting_set_default_voice(const char* language, ttsp_voice_type_e vctype)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not Initialized" );
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (false == g_cur_engine.is_loaded) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not loaded engine");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_cur_engine.pefuncs->is_valid_voice) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] The function of engine is NULL!!");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	int ret = -1;
	if(false == g_cur_engine.pefuncs->is_valid_voice(language, vctype)) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Voice is NOT valid !!");
		return TTSD_ERROR_INVALID_VOICE;
	}

	if (NULL != g_cur_engine.default_lang)
		free(g_cur_engine.default_lang);

	g_cur_engine.default_lang = strdup(language);
	g_cur_engine.default_vctype = vctype;

	ret = ttsd_config_set_default_voice(language, (int)vctype);
	if (0 == ret) {
		SLOG(LOG_DEBUG, get_tag(), "[Engine Agent SUCCESS] Set default voice : lang(%s), type(%d)",
				g_cur_engine.default_lang, g_cur_engine.default_vctype); 
	} else {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] fail to write default voice to config (%d)", ret); 
	}

	return 0;
}

int ttsd_engine_setting_get_default_speed(ttsp_speed_e* speed)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not Initialized" );
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (false == g_cur_engine.is_loaded) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not loaded engine");
		return TTSD_ERROR_OPERATION_FAILED;
	}
	
	/* get current language */
	*speed = g_cur_engine.default_speed;

	return 0;
}

int ttsd_engine_setting_set_default_speed(const ttsp_speed_e speed)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not Initialized" );
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (false == g_cur_engine.is_loaded) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not loaded engine");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	g_cur_engine.default_speed = speed;

	if (0 != ttsd_config_set_default_speed(speed)) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] fail to set default speed to config");
	}

	return 0;
}

bool __engine_setting_cb(const char* key, const char* value, void* user_data)
{
	GList** engine_setting_list = (GList**)user_data;

	if (NULL == engine_setting_list || NULL == key || NULL == value) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Input parameter is NULL in engine setting callback!!!!");
		return false;
	}

	engine_setting_s* temp = g_malloc0(sizeof(engine_setting_s));
	temp->key = g_strdup(key);
	temp->value = g_strdup(value);

	*engine_setting_list = g_list_append(*engine_setting_list, temp);

	return true;
}


int ttsd_engine_setting_get_engine_setting_info(char** engine_id, GList** setting_list)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not Initialized" );
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (false == g_cur_engine.is_loaded) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not loaded engine");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (NULL == setting_list) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Input parameter is NULL");
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	if (NULL == g_cur_engine.pefuncs->foreach_engine_setting) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] foreach_engine_setting() of engine is NULL!!");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	/* get setting info and move setting info to input parameter */
	int result = 0;

	result = g_cur_engine.pefuncs->foreach_engine_setting(__engine_setting_cb, setting_list);

	if (0 == result && 0 < g_list_length(*setting_list)) {
		*engine_id = strdup(g_cur_engine.engine_uuid);
	} else {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] fail to get setting info : result(%d)\n", result);
		result = TTSD_ERROR_OPERATION_FAILED;
	}

	return result;
}

int ttsd_engine_setting_set_engine_setting(const char* key, const char* value)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not Initialized" );
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (false == g_cur_engine.is_loaded) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] Not loaded engine");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_cur_engine.pefuncs->set_engine_setting) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] set_setting_info of engine is NULL!!");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	/* get setting info and move setting info to input parameter */
	int ret = 0;
	ret = g_cur_engine.pefuncs->set_engine_setting(key, value);

	if (0 != ret) {
		SLOG(LOG_ERROR, get_tag(), "[Engine Agent ERROR] fail to set engine setting : result(%d)\n", ret); 
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
				if (NULL != data->language)
					g_free(data->language);

				g_free(data);
			}
			
			voice_list = g_list_remove_link(voice_list, iter);
			
			iter = g_list_first(voice_list);
		}
	}
}

/*
* TTS Engine Callback Functions											`				  *
*/
bool __result_cb(ttsp_result_event_e event, const void* data, unsigned int data_size, void *user_data)
{
	g_result_cb(event, data, data_size, user_data);

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

			SLOG(LOG_DEBUG, get_tag(), "[%dth]\n", i);
			SLOG(LOG_DEBUG, get_tag(), "engine uuid : %s\n", data->engine_uuid);
			SLOG(LOG_DEBUG, get_tag(), "engine name : %s\n", data->engine_name);
			SLOG(LOG_DEBUG, get_tag(), "engine path : %s\n", data->engine_path);
			SLOG(LOG_DEBUG, get_tag(), "setting ug path : %s\n", data->setting_ug_path);

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







