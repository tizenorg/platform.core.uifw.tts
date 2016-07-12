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
typedef struct _ttsengine_info {
	/* base info */
	char*	engine_uuid;
	char*	engine_path;
	char*	engine_name;
	char*	engine_setting_path;

	/* info for using engine load */
	bool	is_loaded;
	bool	use_network;

	tts_engine_callback_s* callbacks;

	/* engine base setting */
	char*	default_lang;
	int	default_vctype;
	int	default_speed;
	int	default_pitch;
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

/** Current engine information */
static ttsengine_info_s* g_engine_info = NULL;

/** Current voice information */
static GSList* g_cur_voices = NULL;

/** Callback function for voice list */
static bool __supported_voice_cb(const char* language, int type, void* user_data);

/** Free voice list */
static void __free_voice_list(GList* voice_list);

static int ttsd_print_voicelist();

/** Get engine info */
int __internal_get_engine_info(ttse_request_callback_s* callback);

static const char* __ttsd_get_engine_error_code(ttse_error_e err)
{
	switch (err) {
	case TTSE_ERROR_NONE:			return "TTSE_ERROR_NONE";
	case TTSE_ERROR_OUT_OF_MEMORY:		return "TTSE_ERROR_OUT_OF_MEMORY";
	case TTSE_ERROR_IO_ERROR:		return "TTSE_ERROR_IO_ERROR";
	case TTSE_ERROR_INVALID_PARAMETER:	return "TTSE_ERROR_INVALID_PARAMETER";
	case TTSE_ERROR_NETWORK_DOWN:		return "TTSE_ERROR_NETWORK_DOWN";
	case TTSE_ERROR_INVALID_STATE:		return "TTSE_ERROR_INVALID_STATE";
	case TTSE_ERROR_INVALID_VOICE:		return "TTSE_ERROR_INVALID_VOICE";
	case TTSE_ERROR_OPERATION_FAILED:	return "TTSE_ERROR_OPERATION_FAILED";
	default:
		return "Invalid error code";
	}
}

int ttsd_engine_agent_init()
{
	if (true == g_agent_init) {
		SLOG(LOG_WARN, tts_tag(), "[Engine Agent] Already initialized");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	ttsengine_info_s* temp;
	temp = (ttsengine_info_s*)calloc(1, sizeof(ttsengine_info_s));
	if (NULL == temp) {
		SLOG(LOG_WARN, tts_tag(), "[Engine Agent] Fail to alloc memory");
		return TTSD_ERROR_OUT_OF_MEMORY;
	}

	if (0 != ttsd_config_get_default_voice(&(temp->default_lang), &(temp->default_vctype))) {
		SLOG(LOG_WARN, tts_tag(), "[Server WARNING] There is No default voice in config");
		/* Set default voice */
		temp->default_lang = strdup(TTS_BASE_LANGUAGE);
		temp->default_vctype = TTSE_VOICE_TYPE_FEMALE;
	}

	SLOG(LOG_DEBUG, tts_tag(), "[Server DEBUG] language(%s), type(%d)", temp->default_lang, temp->default_vctype);

	if (0 != ttsd_config_get_default_speed(&(temp->default_speed))) {
		SLOG(LOG_WARN, tts_tag(), "[Server WARNING] There is No default speed in config");
		temp->default_speed = TTS_SPEED_NORMAL;
	}

	if (0 != ttsd_config_get_default_pitch(&(temp->default_pitch))) {
		SLOG(LOG_WARN, tts_tag(), "[Server WARNING] There is No default pitch in config");
		temp->default_pitch = TTS_PITCH_NORMAL;
	}

	temp->is_loaded = false;
	g_engine_info = temp;

	g_agent_init = true;

	SLOG(LOG_DEBUG, tts_tag(), "[Engine Agent SUCCESS] Initialize Engine Agent");
	return 0;
}

int ttsd_engine_agent_release()
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Not Initialized");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	/* unload current engine */
	if (g_engine_info->is_loaded) {
	ttsd_engine_agent_unload_current_engine();
                        }

	if (NULL != g_engine_info->default_lang) {
		free(g_engine_info->default_lang);
		g_engine_info->default_lang = NULL;
		}

	if (NULL != g_engine_info->engine_uuid) {
		free(g_engine_info->engine_uuid);
		g_engine_info->engine_uuid = NULL;
				}

	if (NULL != g_engine_info->engine_name) {
		free(g_engine_info->engine_name);
		g_engine_info->engine_name = NULL;
				}

	if (NULL != g_engine_info->engine_setting_path) {
		free(g_engine_info->engine_setting_path);
		g_engine_info->engine_setting_path = NULL;
			}

	if (NULL != g_engine_info->engine_path) {
		free(g_engine_info->engine_path);
		g_engine_info->engine_path = NULL;
	}

	free(g_engine_info);
	g_engine_info = NULL;

	g_agent_init = false;

	SLOG(LOG_DEBUG, tts_tag(), "[Engine Agent SUCCESS] Release Engine Agent");

	return 0;
}

static bool __set_voice_info_cb(const char* language, int type, void* user_data)
{
	if (NULL == language) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Input parameter is NULL in voice list callback!!!!");
		return false;
	}

	ttsvoice_s* voice = calloc(1, sizeof(ttsvoice_s));
	if (NULL == voice) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Fail to allocate memory");
		return false;
	}
	voice->lang = strdup(language);
	voice->type = type;

	voice->client_ref_count = 0;
	voice->is_loaded = false;

	if (0 == strcmp(g_engine_info->default_lang, language) && g_engine_info->default_vctype == type) {
		voice->is_default = true;
		voice->is_loaded = true;
	} else {
		voice->is_default = false;
	}

	g_cur_voices = g_slist_append(g_cur_voices, voice);

	return true;
}

static int __update_voice_list()
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Not Initialized");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	/* Get voice list */
	if (NULL != g_cur_voices) {
		GSList *iter = NULL;
		ttsvoice_s* data = NULL;

		iter = g_slist_nth(g_cur_voices, 0);
		while (NULL != iter) {
			data = iter->data;

			if (NULL != data) {
				if (NULL != data->lang) {
					free(data->lang);
					data->lang = NULL;
				}

				g_cur_voices = g_slist_remove(g_cur_voices, data);
				free(data);
				data = NULL;
			}

			iter = g_slist_nth(g_cur_voices, 0);
		}
	}

	g_cur_voices = NULL;

	int ret = 0;
	ret = g_engine_info->callbacks->foreach_voices(__set_voice_info_cb, NULL);

	if (0 != ret || 0 >= g_slist_length(g_cur_voices)) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] fail to get voice list : result(%d)", ret);
		return -1;
	}

#ifdef ENGINE_AGENT_DEBUG
	ttsd_print_voicelist();
#endif
	return 0;
}

int ttsd_engine_agent_load_current_engine(ttse_request_callback_s* callback)
{
	SLOG(LOG_DEBUG, tts_tag(), "[Engine Agent DEBUG] === ttsd_engine_agent_load_current_engine START" );


	if (false == g_agent_init) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Not Initialized" );
		return TTSD_ERROR_OPERATION_FAILED;
	}

	/* check whether current engine is loaded or not */
	if (true == g_engine_info->is_loaded) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent] Engine has already been loaded ");
		return 0;
	}

	if (NULL == callback) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Invalid engine");
		return TTSD_ERROR_ENGINE_NOT_FOUND;
	}

	if (NULL == callback->get_info
		|| NULL == callback->initialize || NULL == callback->deinitialize
		|| NULL == callback->foreach_voices || NULL == callback->is_valid_voice
		|| NULL == callback->set_pitch
		|| NULL == callback->load_voice || NULL == callback->unload_voice
		|| NULL == callback->start_synth || NULL == callback->cancel_synth
		|| NULL == callback->check_app_agreed|| NULL == callback->need_app_credential) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Invalid engine");
		return TTSD_ERROR_ENGINE_NOT_FOUND;
	}

	/* Get current engine info */
	SLOG(LOG_DEBUG, tts_tag(), "[Engine Agent DEBUG] === before __internal_get_engine_info" );

	int ret = __internal_get_engine_info(callback);
	if (0 != ret) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Fail to get engine info");
		return ret;
	}

	/* Initialize engine */
	ret = g_engine_info->callbacks->initialize();
	if (0 != ret) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Fail to initialize current engine : %s", __ttsd_get_engine_error_code(ret));
		return TTSD_ERROR_OPERATION_FAILED;
	}

	/* Get voice info of current engine */
	ret = __update_voice_list();
	if (0 != ret) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Fail to set voice info : result(%d)", ret);
		return TTSD_ERROR_OPERATION_FAILED;
	}

	/* Select default voice */
	if (NULL != g_engine_info->default_lang) {
		bool is_valid = false;
		ret = g_engine_info->callbacks->is_valid_voice(g_engine_info->default_lang, g_engine_info->default_vctype, &is_valid);
		if (0 == ret) {
			if (true == is_valid) {
				SLOG(LOG_DEBUG, tts_tag(), "[Engine Agent SUCCESS] Set origin default voice to current engine : lang(%s), type(%d)", 
					g_engine_info->default_lang, g_engine_info->default_vctype);
		} else {
				SLOG(LOG_WARN, tts_tag(), "[Engine Agent WARNING] Fail to set origin default voice : lang(%s), type(%d)",
					g_engine_info->default_lang, g_engine_info->default_vctype);

				/* TODO - Error Tolerance when Default voice is not valid */
				return TTSD_ERROR_OPERATION_FAILED;
			}
		} else {
			SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Fail(Engine ERROR) : %s", __ttsd_get_engine_error_code(ret));
			return TTSD_ERROR_OPERATION_FAILED;
		}
	}

	/* load default voice */
	ret = g_engine_info->callbacks->load_voice(g_engine_info->default_lang, g_engine_info->default_vctype);
		if (0 == ret) {
		SLOG(LOG_DEBUG, tts_tag(), "[Engine Agent SUCCESS] Load default voice : lang(%s), type(%d)",
			g_engine_info->default_lang, g_engine_info->default_vctype);
		} else {
		SLOG(LOG_WARN, tts_tag(), "[Engine Agent ERROR] Fail to load default voice : lang(%s), type(%d) result(%s)",
			g_engine_info->default_lang, g_engine_info->default_vctype, __ttsd_get_engine_error_code(ret));
	
			return TTSD_ERROR_OPERATION_FAILED;
		}


	/* set default pitch */
	ret = g_engine_info->callbacks->set_pitch(g_engine_info->default_pitch);
		if (0 != ret) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Fail to set pitch : pitch(%d), result(%s)", 
			g_engine_info->default_pitch, __ttsd_get_engine_error_code(ret));
			return TTSD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, tts_tag(), "[Engine Agent SUCCESS] Set default pitch : pitch(%d)", g_engine_info->default_pitch);
	}

	g_engine_info->is_loaded = true;

	return 0;
}

int ttsd_engine_agent_unload_current_engine()
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Not Initialized");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (false == g_engine_info->is_loaded) {
		SLOG(LOG_DEBUG, tts_tag(), "[Engine Agent] Engine has already been unloaded ");
		return 0;
	}

	/* shutdown engine */
	int ret = 0;
	ret = g_engine_info->callbacks->deinitialize();
	if (0 != ret) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent] Fail deinitialize() : %s", __ttsd_get_engine_error_code(ret));
	}

	/* reset current engine data */
	g_engine_info->is_loaded = false;

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

	SLOG(LOG_DEBUG, tts_tag(), "[Engine Agent Success] Unload current engine");

	return 0;
}

bool ttsd_engine_agent_need_network()
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Not Initialized");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	return g_engine_info->use_network;
}

bool ttsd_engine_agent_is_same_engine(const char* engine_id)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Not Initialized");
		return false;
	}

	if (NULL == engine_id) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] engine_id is NULL");
		return false;
	}

	if (NULL == g_engine_info) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] g_engine_info is NULL");
		return false;
	}

	if (false == g_engine_info->is_loaded) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Not loaded engine");
		return false;
	}

	/* compare current engine and engine id.*/
	if (0 == strncmp(g_engine_info->engine_uuid, engine_id, strlen(engine_id))) {
		return true;
	}

	return false;
}

bool ttsd_engine_select_valid_voice(const char* lang, int type, char** out_lang, int* out_type)
{
	if (NULL == lang || NULL == out_lang || NULL == out_type) {
		return false;
	}

	if (NULL == g_engine_info) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] g_engine_info is NULL");
		return false;
	}

	if (false == g_engine_info->is_loaded) {
		SLOG(LOG_WARN, tts_tag(), "[Engine Agent WARNING] Not loaded engine");
		return false;
	}

	SECURE_SLOG(LOG_DEBUG, tts_tag(), "[Engine Agent] Select voice : input lang(%s), input type(%d), default lang(%s), default type(%d)", 
		(NULL == lang) ? "NULL" : lang, type, (NULL == g_engine_info->default_lang) ? "NULL" : g_engine_info->default_lang, g_engine_info->default_vctype);

	/* case 1 : Both are default */
	if (0 == strncmp(lang, "default", strlen("default")) && 0 == type) {
		*out_lang = strdup(g_engine_info->default_lang);
		*out_type = g_engine_info->default_vctype;
		return true;
	}

	/* Get voice list */
	GList* voice_list = NULL;
	int ret = 0;
	ret = g_engine_info->callbacks->foreach_voices(__supported_voice_cb, &voice_list);
	if (0 != ret || 0 >= g_list_length(voice_list)) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] fail to get voice list : result(%d)", ret);
		return false;
	}

	bool result;
	result = false;

	GList *iter = NULL;
	voice_s* voice;

	/* case 2 : lang and type are not default type */
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
		if (0 == strncmp(lang, g_engine_info->default_lang, strlen(g_engine_info->default_lang))) {
			*out_lang = strdup(g_engine_info->default_lang);
			*out_type = g_engine_info->default_vctype;
			result = true;
		} else {
			voice_s* voice_selected = NULL;
			iter = g_list_first(voice_list);
			while (NULL != iter) {
				/* Get handle data from list */
				voice = iter->data;

				if (0 == strncmp(voice->language, lang, strlen(lang))) {
					voice_selected = voice;
					if (voice->type == g_engine_info->default_vctype) {
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
		if (type == g_engine_info->default_vctype) {
			*out_lang = strdup(g_engine_info->default_lang);
			*out_type = g_engine_info->default_vctype;
			result = true;
		} else {
			voice_s* voice_selected = NULL;
			iter = g_list_first(voice_list);
			while (NULL != iter) {
				/* Get handle data from list */
				voice = iter->data;

				if (0 == strncmp(voice->language, g_engine_info->default_lang, strlen(g_engine_info->default_lang))) {
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
		SECURE_SLOG(LOG_DEBUG, tts_tag(), "[Engine Agent] Selected voice : lang(%s), type(%d)", *out_lang, *out_type);
	}

	__free_voice_list(voice_list);

	return result;
}



int ttsd_engine_agent_set_default_voice(const char* language, int vctype)
{
	if (NULL == language) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] language is NULL");
		return false;
	}

	if (false == g_agent_init) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Not Initialized");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_engine_info) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] g_engine_info is NULL");
		return false;
	}

	if (false == g_engine_info->is_loaded) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] No loaded engine");
		return TTSD_ERROR_ENGINE_NOT_FOUND;
	}

	int ret = -1;
	bool is_valid = false;
	ret = g_engine_info->callbacks->is_valid_voice(language, vctype, &is_valid);
	if (0 == ret) {
		if (true == is_valid) {
			SLOG(LOG_DEBUG, tts_tag(), "[Engine Agent SUCCESS] Current voice is valid : lang(%s), type(%d)", 
				(NULL == g_engine_info->default_lang) ? "NULL" : g_engine_info->default_lang, g_engine_info->default_vctype);
		} else {
			SLOG(LOG_WARN, tts_tag(), "[Engine Agent WARNING] Current voice is invalid : lang(%s), type(%d)",
				(NULL == g_engine_info->default_lang) ? "NULL" : g_engine_info->default_lang, g_engine_info->default_vctype);
		
		return TTSD_ERROR_OPERATION_FAILED;
	}
		} else {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Fail(Engine ERROR) : %s", __ttsd_get_engine_error_code(ret));
		return TTSD_ERROR_OPERATION_FAILED;
	}


	GSList *iter = NULL;
	ttsvoice_s* data = NULL;

	/* Update new default voice info */
	iter = g_slist_nth(g_cur_voices, 0);
	while (NULL != iter) {
		/* Get handle data from list */
		data = iter->data;

		if (NULL == data) {
			SECURE_SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Voice data is NULL");
			return TTSD_ERROR_OPERATION_FAILED;
		}

		if (NULL != data->lang) {
		if (0 == strcmp(data->lang, language) && data->type == vctype) {
			data->is_default = true;
			if (0 == data->client_ref_count) {
				/* load voice */
					ret = g_engine_info->callbacks->load_voice(data->lang, data->type);
					if (0 == ret) {
						SECURE_SLOG(LOG_DEBUG, tts_tag(), "[Engine Agent SUCCESS] load voice : lang(%s), type(%d)", 
							data->lang, data->type);
						data->is_loaded = true;
					} else {
						SECURE_SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Fail to load voice : lang(%s), type(%d), result(%s)",
							data->lang, data->type, __ttsd_get_engine_error_code(ret));
					}
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
			SECURE_SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Voice data is NULL");
			return TTSD_ERROR_OPERATION_FAILED;
		}

		if (0 == strcmp(data->lang, g_engine_info->default_lang) && data->type == g_engine_info->default_vctype) {
			data->is_default = false;
			if (0 == data->client_ref_count) {
				/* Unload voice */
				ret = g_engine_info->callbacks->unload_voice(data->lang, data->type);
					if (0 == ret) {
					SECURE_SLOG(LOG_DEBUG, tts_tag(), "[Engine Agent SUCCESS] Unload voice : lang(%s), type(%d)", 
							data->lang, data->type);
						data->is_loaded = false;
					} else {
					SECURE_SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Fail to unload voice : lang(%s), type(%d), result(%s)",
							data->lang, data->type, __ttsd_get_engine_error_code(ret));
					}
			}
			break;
		}

		/*Get next item*/
		iter = g_slist_next(iter);
	}

	if (NULL != g_engine_info->default_lang)	free(g_engine_info->default_lang);

	g_engine_info->default_lang = strdup(language);
	g_engine_info->default_vctype = vctype;

#ifdef ENGINE_AGENT_DEBUG
	ttsd_print_voicelist();
#endif

	SECURE_SLOG(LOG_DEBUG, tts_tag(), "[Engine Agent SUCCESS] Set default voice : lang(%s), type(%d)",
		g_engine_info->default_lang, g_engine_info->default_vctype);

	return 0;
}

int ttsd_engine_agent_set_default_speed(int speed)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Not Initialized");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	g_engine_info->default_speed = speed;

	return 0;
}

int ttsd_engine_agent_set_default_pitch(int pitch)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Not Initialized");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (false == g_engine_info->is_loaded) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] No loaded engine");
		return TTSD_ERROR_ENGINE_NOT_FOUND;
	}

	int ret = g_engine_info->callbacks->set_pitch(pitch);
	if (0 != ret) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Fail to set pitch : pitch(%d), result(%s)",
			 pitch, __ttsd_get_engine_error_code(ret));
		return TTSD_ERROR_OPERATION_FAILED;
	}

	g_engine_info->default_pitch = pitch;

	return 0;
}

int ttsd_engine_agent_is_credential_needed(int uid, bool* credential_needed)
{
	if (NULL == credential_needed) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Invalid Parameter");
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	if (false == g_agent_init) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Not Initialized");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (false == g_engine_info->is_loaded) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Not loaded engine");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (NULL == g_engine_info->callbacks->need_app_credential) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Not support to check app credential");
		return TTSD_ERROR_NOT_SUPPORTED_FEATURE;
	}

	bool result = false;
	result = g_engine_info->callbacks->need_app_credential();
	*credential_needed = result;

	return TTSD_ERROR_NONE;
}

/******************************************************************************************
* TTS Engine Interfaces for client
*******************************************************************************************/

int ttsd_engine_load_voice(const char* lang, const int vctype)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Not Initialized");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (false == g_engine_info->is_loaded) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] No loaded engine");
		return TTSD_ERROR_ENGINE_NOT_FOUND;
	}

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
				SLOG(LOG_DEBUG, tts_tag(), "[Engine Agent] Find voice : default(%d) loaded(%d) ref(%d) lang(%s) type(%d)",
					 data->is_default, data->is_loaded,  data->client_ref_count, data->lang, data->type);
				break;
			}
		}

		/*Get next item*/
		iter = g_slist_next(iter);
		data = NULL;
	}

	if (NULL == data) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] This voice is not supported voice : lang(%s) type(%d)", lang, vctype);
		return TTSD_ERROR_OPERATION_FAILED;
	}

	/* 2. increse ref count */
	data->client_ref_count++;

	/* 3. if ref count change 0 to 1 and not default, load voice */
	if (1 == data->client_ref_count && false == data->is_default) {
		/* load voice */
		ret = g_engine_info->callbacks->load_voice(data->lang, data->type);
			if (0 == ret) {
			SECURE_SLOG(LOG_DEBUG, tts_tag(), "[Engine Agent SUCCESS] Load voice : lang(%s), type(%d)", 
					data->lang, data->type);
				data->is_loaded = true;
			} else {
			SECURE_SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Fail to load voice : lang(%s), type(%d), result(%s)",
					data->lang, data->type, __ttsd_get_engine_error_code(ret));

				return TTSD_ERROR_OPERATION_FAILED;
			}
		} else {
		SECURE_SLOG(LOG_DEBUG, tts_tag(), "[Engine Agent] Not load voice : default voice(%d) or ref count(%d)",
			data->is_default, data->client_ref_count);
	}

#ifdef ENGINE_AGENT_DEBUG
	ttsd_print_voicelist();
#endif

	return 0;
}

int ttsd_engine_unload_voice(const char* lang, const int vctype)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Not Initialized");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (false == g_engine_info->is_loaded) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] No loaded engine");
		return TTSD_ERROR_ENGINE_NOT_FOUND;
	}

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
				SLOG(LOG_DEBUG, tts_tag(), "[Engine Agent] Find voice : default(%d) loaded(%d) ref(%d) lang(%s) type(%d)",
					 data->is_default, data->is_loaded,  data->client_ref_count, data->lang, data->type);
				break;
			}
		}

		/*Get next item*/
		iter = g_slist_next(iter);
		data = NULL;
	}

	if (NULL == data) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] This voice is not supported voice : lang(%s) type(%d)", lang, vctype);
		return TTSD_ERROR_OPERATION_FAILED;
	}

	/* 2. Decrese ref count */
	data->client_ref_count--;

	/* 3. if ref count change 0 and not default, load voice */
	if (0 == data->client_ref_count && false == data->is_default) {
		/* load voice */
		ret = g_engine_info->callbacks->unload_voice(data->lang, data->type);
			if (0 == ret) {
			SECURE_SLOG(LOG_DEBUG, tts_tag(), "[Engine Agent SUCCESS] Unload voice : lang(%s), type(%d)", 
					data->lang, data->type);
				data->is_loaded = false;
			} else {
			SECURE_SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Fail to unload voice : lang(%s), type(%d), result(%s)",
					data->lang, data->type, __ttsd_get_engine_error_code(ret));

				return TTSD_ERROR_OPERATION_FAILED;
			}
		} else {
		SECURE_SLOG(LOG_DEBUG, tts_tag(), "[Engine Agent] Not unload voice : default voice(%d) or ref count(%d)",
			data->is_default, data->client_ref_count);
	}

#ifdef ENGINE_AGENT_DEBUG
	ttsd_print_voicelist();
#endif
	return 0;
}

int ttsd_engine_start_synthesis(const char* lang, int vctype, const char* text, int speed, const char* appid, const char* credential, void* user_param)
{
	if (NULL == lang || NULL == text || NULL == appid || NULL == credential) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Invalid parameter");
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	if (false == g_agent_init) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Not Initialized");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (false == g_engine_info->is_loaded) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] No loaded engine");
		return TTSD_ERROR_ENGINE_NOT_FOUND;
	}

	/* select voice for default */
	char* temp_lang = NULL;
	int temp_type;
	if (true != ttsd_engine_select_valid_voice(lang, vctype, &temp_lang, &temp_type)) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Fail to select default voice");
		if (NULL != temp_lang)	free(temp_lang);
		return TTSD_ERROR_INVALID_VOICE;
	} else {
		SECURE_SLOG(LOG_DEBUG, tts_tag(), "[Engine Agent] Start synthesis : language(%s), type(%d), speed(%d), text(%s), credential(%s)", 
			temp_lang, temp_type, speed, text, credential);
	}

	int temp_speed;

	if (0 == speed) {
		temp_speed = g_engine_info->default_speed;
	} else {
		temp_speed = speed;
	}

	/* synthesize text */
	int ret = 0;
	ret = g_engine_info->callbacks->start_synth(temp_lang, temp_type, text, temp_speed, appid, credential, user_param);
	if (0 != ret) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] ***************************************");
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] * synthesize error : %s *", __ttsd_get_engine_error_code(ret));
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] ***************************************");
		if (NULL != temp_lang)	free(temp_lang);
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (NULL != temp_lang)	free(temp_lang);
	return 0;
}

int ttsd_engine_cancel_synthesis()
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Not Initialized");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (false == g_engine_info->is_loaded) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] No loaded engine");
		return TTSD_ERROR_ENGINE_NOT_FOUND;
	}

	/* stop synthesis */
	int ret = 0;
	ret = g_engine_info->callbacks->cancel_synth();
	if (0 != ret) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] fail cancel synthesis : %s", __ttsd_get_engine_error_code(ret));
		return TTSD_ERROR_OPERATION_FAILED;
	}

	return 0;
}

bool __supported_voice_cb(const char* language, int type, void* user_data)
{
	GList** voice_list = (GList**)user_data;

	if (NULL == language || NULL == voice_list) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Input parameter is NULL in voice list callback!!!!");
		return false;
	}

	voice_s* voice = calloc(1, sizeof(voice_s));
	if (NULL == voice) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Fail to allocate memory");
		return false;
	}
	voice->language = strdup(language);
	voice->type = type;

	*voice_list = g_list_append(*voice_list, voice);

	return true;
}

int ttsd_engine_get_voice_list(GList** voice_list)
{
	if (NULL == voice_list) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Invalid parameter");
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	if (false == g_agent_init) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Not Initialized");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (false == g_engine_info->is_loaded) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] No loaded engine");
		return TTSD_ERROR_ENGINE_NOT_FOUND;
	}

	int ret = 0;
	ret = g_engine_info->callbacks->foreach_voices(__supported_voice_cb, (void*)voice_list);
	if (0 != ret) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Fail to get voice list : %s", __ttsd_get_engine_error_code(ret));
		return TTSD_ERROR_OPERATION_FAILED;
	}

	return 0;
}

int ttsd_engine_get_default_voice(char** lang, int* vctype)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Not Initialized");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (false == g_engine_info->is_loaded) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] No loaded engine");
		return TTSD_ERROR_ENGINE_NOT_FOUND;
	}

	if (NULL == lang || NULL == vctype) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] BAD Parameter");
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	if (NULL != g_engine_info->default_lang) {
		*lang = strdup(g_engine_info->default_lang);
		*vctype = g_engine_info->default_vctype;

		SECURE_SLOG(LOG_DEBUG, tts_tag(), "[Engine] Get default voice : language(%s), type(%d)", *lang, *vctype);
	} else {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Default voice is NULL");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	return 0;
}

int ttsd_engine_set_private_data(const char* key, const char* data)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Not Initialized");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (false == g_engine_info->is_loaded) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] No loaded engine");
		return TTSD_ERROR_ENGINE_NOT_FOUND;
	}

	if (NULL == key) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Invalid parameter");
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	if (NULL == g_engine_info->callbacks->private_data_set) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Not supported feature");
		return TTSD_ERROR_NOT_SUPPORTED_FEATURE;
	}

	int ret = g_engine_info->callbacks->private_data_set(key, data);

	if (0 != ret) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Fail to set private data(%d)", ret);
	}

	return ret;
}

int ttsd_engine_get_private_data(const char* key, char** data)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Not Initialized");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (false == g_engine_info->is_loaded) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] No loaded engine");
		return TTSD_ERROR_ENGINE_NOT_FOUND;
	}

	if (NULL == key || NULL == data) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Invalid parameter");
		return TTSD_ERROR_INVALID_PARAMETER;
	}


	if (NULL == g_engine_info->callbacks->private_data_requested) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Not supported feature");
		return TTSD_ERROR_NOT_SUPPORTED_FEATURE;
	}

	char* temp = NULL;
	int ret = 0;
	ret = g_engine_info->callbacks->private_data_requested(key, &temp);
	if (0 != ret) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Fail to get private data(%d)", ret);
	}

	*data = strdup(temp);

	return ret;
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
			g_list_free(iter);
			iter = g_list_first(voice_list);
		}
	}
}

/*
* TTS Engine Callback Functions											`				  *
*/

/* function for debugging */
int ttsd_print_voicelist()
{
	/* Test log */
	GSList *iter = NULL;
	ttsvoice_s* data = NULL;

	SLOG(LOG_DEBUG, tts_tag(), "=== Voice list ===");

	if (g_slist_length(g_cur_voices) > 0) {
		/* Get a first item */
		iter = g_slist_nth(g_cur_voices, 0);

		int i = 1;
		while (NULL != iter) {
			/*Get handle data from list*/
			data = iter->data;

			if (NULL == data || NULL == data->lang) {
				SLOG(LOG_ERROR, tts_tag(), "[ERROR] Data is invalid");
				return 0;
			}

			SLOG(LOG_DEBUG, tts_tag(), "[%dth] default(%d) loaded(%d) ref(%d) lang(%s) type(%d)",
				 i, data->is_default, data->is_loaded,  data->client_ref_count, data->lang, data->type);

			/*Get next item*/
			iter = g_slist_next(iter);
			i++;
		}
	}

	SLOG(LOG_DEBUG, tts_tag(), "==================");

	return 0;
}

int __internal_get_engine_info(ttse_request_callback_s* callback)
{
	SLOG(LOG_DEBUG, tts_tag(), "[Engine Agent DEBUG] === inside __internal_get_engine_info" );

	if (NULL == callback) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Invalid engine");
		return TTSD_ERROR_ENGINE_NOT_FOUND;
	}

	if (NULL == callback->get_info) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Invalid engine");
		return TTSD_ERROR_ENGINE_NOT_FOUND;
	}

	if (0 != callback->get_info(&(g_engine_info->engine_uuid), &(g_engine_info->engine_name), &(g_engine_info->engine_setting_path), &(g_engine_info->use_network))) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Fail to get engine info");
		return TTSD_ERROR_ENGINE_NOT_FOUND;
	}

	if (NULL != g_engine_info->engine_path) {
		free(g_engine_info->engine_path);
		g_engine_info->engine_path = NULL;
	}
	g_engine_info->engine_path = strdup("empty");
	g_engine_info->is_loaded = false;

	if (NULL != g_engine_info->callbacks) {
		free(g_engine_info->callbacks);
		g_engine_info->callbacks = NULL;
	}
	g_engine_info->callbacks = (tts_engine_callback_s*)calloc(1, sizeof(tts_engine_callback_s));
	if (NULL == g_engine_info->callbacks) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Fail to allocate memory");
		return TTSD_ERROR_OUT_OF_MEMORY;
	}

	g_engine_info->callbacks->get_info = callback->get_info;
	g_engine_info->callbacks->initialize = callback->initialize;
	g_engine_info->callbacks->deinitialize = callback->deinitialize;
	g_engine_info->callbacks->foreach_voices = callback->foreach_voices;
	g_engine_info->callbacks->is_valid_voice = callback->is_valid_voice;
	g_engine_info->callbacks->set_pitch = callback->set_pitch;
	g_engine_info->callbacks->load_voice = callback->load_voice;
	g_engine_info->callbacks->unload_voice = callback->unload_voice;
	g_engine_info->callbacks->start_synth = callback->start_synth;
	g_engine_info->callbacks->cancel_synth = callback->cancel_synth;
	g_engine_info->callbacks->check_app_agreed = callback->check_app_agreed;
	g_engine_info->callbacks->need_app_credential = callback->need_app_credential;

	g_engine_info->callbacks->private_data_set = NULL;
	g_engine_info->callbacks->private_data_requested = NULL;
	
	SLOG(LOG_DEBUG, tts_tag(), "--- Valid Engine ---");
	SLOG(LOG_DEBUG, tts_tag(), "Engine uuid : %s", g_engine_info->engine_uuid);
	SLOG(LOG_DEBUG, tts_tag(), "Engine name : %s", g_engine_info->engine_name);
	SLOG(LOG_DEBUG, tts_tag(), "Engine path : %s", g_engine_info->engine_path);
	SLOG(LOG_DEBUG, tts_tag(), "Engine setting path : %s", g_engine_info->engine_setting_path);
	SLOG(LOG_DEBUG, tts_tag(), "Use network : %s", g_engine_info->use_network ? "true" : "false");
	SLOG(LOG_DEBUG, tts_tag(), "--------------------");
	SLOG(LOG_DEBUG, tts_tag(), "  ");

	return TTSD_ERROR_NONE;
	
}


/** Set callbacks of the current engine */
int ttsd_engine_agent_set_private_data_set_cb(ttse_private_data_set_cb callback)
{
	if(false == g_agent_init) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Not Initialized");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if(NULL == g_engine_info) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] The engine is not valid");
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	if(false == g_engine_info->is_loaded) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Not loaded engine");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	g_engine_info->callbacks->private_data_set = callback;
	
	return TTSD_ERROR_NONE;
}

int ttsd_engine_agent_set_private_data_requested_cb(ttse_private_data_requested_cb callback)
{
	if(false == g_agent_init) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Not Initialized");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if(NULL == g_engine_info) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] The engine is not valid");
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	if(false == g_engine_info->is_loaded) {
		SLOG(LOG_ERROR, tts_tag(), "[Engine Agent ERROR] Not loaded engine");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	g_engine_info->callbacks->private_data_requested = callback;
	
	return TTSD_ERROR_NONE;
}


