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

#include <dirent.h>
#include <Ecore.h>
#include <iconv.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <system_info.h>
#include <vconf.h>

#include "tts.h"
#include "tts_client.h"
#include "tts_config_mgr.h"
#include "tts_dbus.h"
#include "tts_main.h"


static Ecore_Timer* g_connect_timer = NULL;

static bool g_screen_reader;

/* Function definition */
static Eina_Bool __tts_notify_state_changed(void *data);
static Eina_Bool __tts_notify_error(void *data);

const char* tts_tag()
{
	return "ttsc";
}

static const char* __tts_get_error_code(tts_error_e err)
{
	switch (err) {
	case TTS_ERROR_NONE:			return "TTS_ERROR_NONE";
	case TTS_ERROR_OUT_OF_MEMORY:		return "TTS_ERROR_OUT_OF_MEMORY";
	case TTS_ERROR_IO_ERROR:		return "TTS_ERROR_IO_ERROR";
	case TTS_ERROR_INVALID_PARAMETER:	return "TTS_ERROR_INVALID_PARAMETER";
	case TTS_ERROR_OUT_OF_NETWORK:		return "TTS_ERROR_OUT_OF_NETWORK";
	case TTS_ERROR_TIMED_OUT:		return "TTS_ERROR_TIMED_OUT";
	case TTS_ERROR_PERMISSION_DENIED:	return "TTS_ERROR_PERMISSION_DENIED";
	case TTS_ERROR_NOT_SUPPORTED:		return "TTS_ERROR_NOT_SUPPORTED";
	case TTS_ERROR_INVALID_STATE:		return "TTS_ERROR_INVALID_STATE";
	case TTS_ERROR_INVALID_VOICE:		return "TTS_ERROR_INVALID_VOICE";
	case TTS_ERROR_ENGINE_NOT_FOUND:	return "TTS_ERROR_ENGINE_NOT_FOUND";
	case TTS_ERROR_OPERATION_FAILED:	return "TTS_ERROR_OPERATION_FAILED";
	case TTS_ERROR_AUDIO_POLICY_BLOCKED:	return "TTS_ERROR_AUDIO_POLICY_BLOCKED";
	default:
		return "Invalid error code";
	}
	return NULL;
}

static int __tts_convert_config_error_code(tts_config_error_e code)
{
	if (code == TTS_CONFIG_ERROR_NONE)			return TTS_ERROR_NONE;
	if (code == TTS_CONFIG_ERROR_OUT_OF_MEMORY)		return TTS_ERROR_OUT_OF_MEMORY;
	if (code == TTS_CONFIG_ERROR_IO_ERROR)			return TTS_ERROR_IO_ERROR;
	if (code == TTS_CONFIG_ERROR_INVALID_PARAMETER)		return TTS_ERROR_INVALID_PARAMETER;
	if (code == TTS_CONFIG_ERROR_INVALID_STATE)		return TTS_ERROR_INVALID_STATE;
	if (code == TTS_CONFIG_ERROR_INVALID_VOICE)		return TTS_ERROR_INVALID_VOICE;
	if (code == TTS_CONFIG_ERROR_ENGINE_NOT_FOUND)		return TTS_ERROR_ENGINE_NOT_FOUND;
	if (code == TTS_CONFIG_ERROR_OPERATION_FAILED)		return TTS_ERROR_OPERATION_FAILED;
	if (code == TTS_CONFIG_ERROR_NOT_SUPPORTED_FEATURE)	return TTS_ERROR_OPERATION_FAILED;

	return code;
}

void __tts_config_voice_changed_cb(const char* before_lang, int before_voice_type, const char* language, int voice_type, bool auto_voice, void* user_data)
{
	SLOG(LOG_DEBUG, TAG_TTSC, "Voice changed : Before lang(%s) type(%d) , Current lang(%s), type(%d)",
		 before_lang, before_voice_type, language, voice_type);

	GList* client_list = NULL;
	client_list = tts_client_get_client_list();

	GList *iter = NULL;
	tts_client_s *data = NULL;

	if (g_list_length(client_list) > 0) {
		/* Get a first item */
		iter = g_list_first(client_list);

		while (NULL != iter) {
			data = iter->data;
			if (NULL != data->default_voice_changed_cb) {
				SLOG(LOG_DEBUG, TAG_TTSC, "Call default voice changed callback : uid(%d)", data->uid);
				data->default_voice_changed_cb(data->tts, before_lang, before_voice_type, 
					language, voice_type, data->default_voice_changed_user_data);
			}

			/* Next item */
			iter = g_list_next(iter);
		}
	}

	return;
}

int tts_create(tts_h* tts)
{
	bool tts_supported = false;
	if (0 == system_info_get_platform_bool(TTS_FEATURE_PATH, &tts_supported)) {
		if (false == tts_supported) {
			SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] TTS NOT supported");
			return TTS_ERROR_NOT_SUPPORTED;
		}
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "===== Create TTS");

	/* check param */
	if (NULL == tts) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Input handle is null");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	if (0 == tts_client_get_size()) {
		if (0 != tts_dbus_open_connection()) {
			SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to open dbus connection");
			return TTS_ERROR_OPERATION_FAILED;
		}
	}

	if (0 != tts_client_new(tts)) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to create client!!!!!");
		return TTS_ERROR_OUT_OF_MEMORY;
	}

	tts_client_s* client = tts_client_get(*tts);
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to get client");
		return TTS_ERROR_OPERATION_FAILED;
	}

	int ret = tts_config_mgr_initialize(client->uid);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to init config manager : %d", ret);
		tts_client_destroy(*tts);
		return __tts_convert_config_error_code(ret);
	}

	ret = tts_config_mgr_set_callback(client->uid, NULL, __tts_config_voice_changed_cb, NULL, NULL, NULL);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to set config changed : %d", ret);
		tts_client_destroy(*tts);
		return __tts_convert_config_error_code(ret);
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return TTS_ERROR_NONE;
}

int tts_destroy(tts_h tts)
{
	bool tts_supported = false;
	if (0 == system_info_get_platform_bool(TTS_FEATURE_PATH, &tts_supported)) {
		if (false == tts_supported) {
			SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] TTS NOT supported");
			return TTS_ERROR_NOT_SUPPORTED;
		}
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "===== Destroy TTS");

	if (NULL == tts) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Input handle is null");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	tts_client_s* client = tts_client_get(tts);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] A handle is not valid");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	/* check used callback */
	if (0 != tts_client_get_use_callback(client)) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Cannot destroy in Callback function");
		return TTS_ERROR_OPERATION_FAILED;
	}

	tts_config_mgr_finalize(client->uid);

	int ret = -1;
	int count = 0;

	/* check state */
	switch (client->current_state) {
	case TTS_STATE_PAUSED:
	case TTS_STATE_PLAYING:
	case TTS_STATE_READY:
		if (!(false == g_screen_reader && TTS_MODE_SCREEN_READER == client->mode)) {
			while (0 != ret) {
				ret = tts_dbus_request_finalize(client->uid);
				if (0 != ret) {
					if (TTS_ERROR_TIMED_OUT != ret) {
						SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] result : %s", __tts_get_error_code(ret));
						break;
					} else {
						SLOG(LOG_WARN, TAG_TTSC, "[WARNING] retry finalize");
						usleep(10000);
						count++;
						if (TTS_RETRY_COUNT == count) {
							SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to request");
							break;
						}
					}
				}
			}
		} else {
			SLOG(LOG_WARN, TAG_TTSC, "[WARNING] Do not request finalize : g_sr(%d) mode(%d)", g_screen_reader, client->mode);
		}

		client->before_state = client->current_state;
		client->current_state = TTS_STATE_CREATED;

	case TTS_STATE_CREATED:
		if (NULL != g_connect_timer) {
			SLOG(LOG_DEBUG, TAG_TTSC, "Connect Timer is deleted");
			ecore_timer_del(g_connect_timer);
		}
		/* Free resources */
		tts_client_destroy(tts);
		break;

	default:
		break;
	}

	if (0 == tts_client_get_size()) {
		if (0 != tts_dbus_close_connection()) {
			SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to close connection");
		}
	}

	tts = NULL;

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return TTS_ERROR_NONE;
}

void __tts_screen_reader_changed_cb(bool value)
{
	g_screen_reader = value;
}

int tts_set_mode(tts_h tts, tts_mode_e mode)
{
	bool tts_supported = false;
	if (0 == system_info_get_platform_bool(TTS_FEATURE_PATH, &tts_supported)) {
		if (false == tts_supported) {
			SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] TTS NOT supported");
			return TTS_ERROR_NOT_SUPPORTED;
		}
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "===== Set TTS mode");

	tts_client_s* client = tts_client_get(tts);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	/* check state */
	if (client->current_state != TTS_STATE_CREATED) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Invalid State: Current state is not 'CREATED'");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_INVALID_STATE;
	}

	if (TTS_MODE_DEFAULT <= mode && mode <= TTS_MODE_SCREEN_READER) {
		client->mode = mode;
	} else {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] mode is not valid : %d", mode);
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	if (TTS_MODE_SCREEN_READER == mode) {
		int ret;
		int screen_reader;
		ret = vconf_get_bool(TTS_ACCESSIBILITY_KEY, &screen_reader);
		if (0 != ret) {
			SLOG(LOG_ERROR, tts_tag(), "[Config ERROR] Fail to get screen reader");
			return TTS_ERROR_OPERATION_FAILED;
		}
		g_screen_reader = (bool)screen_reader;
		tts_config_set_screen_reader_callback(client->uid, __tts_screen_reader_changed_cb);
	} else {
		tts_config_unset_screen_reader_callback(client->uid);
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return TTS_ERROR_NONE;
}

int tts_get_mode(tts_h tts, tts_mode_e* mode)
{
	bool tts_supported = false;
	if (0 == system_info_get_platform_bool(TTS_FEATURE_PATH, &tts_supported)) {
		if (false == tts_supported) {
			SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] TTS NOT supported");
			return TTS_ERROR_NOT_SUPPORTED;
		}
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "===== Get TTS mode");

	tts_client_s* client = tts_client_get(tts);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	/* check state */
	if (client->current_state != TTS_STATE_CREATED) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Invalid State: Current state is not 'CREATED'");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_INVALID_STATE;
	}

	if (NULL == mode) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Input parameter(mode) is NULL");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	*mode = client->mode;

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return TTS_ERROR_NONE;
}

static Eina_Bool __tts_connect_daemon(void *data)
{
	tts_h tts = (tts_h)data;
	tts_client_s* client = tts_client_get(tts);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] A handle is not valid");
		g_connect_timer = NULL;
		return EINA_FALSE;
	}

	/* Send hello */
	if (0 != tts_dbus_request_hello(client->uid)) {
		return EINA_TRUE;
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "===== Connect daemon");

	/* do request initialize */
	int ret = -1;
	ret = tts_dbus_request_initialize(client->uid);

	if (TTS_ERROR_ENGINE_NOT_FOUND == ret) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to initialize : %s", __tts_get_error_code(ret));

		client->reason = TTS_ERROR_ENGINE_NOT_FOUND;
		client->utt_id = -1;

		ecore_timer_add(0, __tts_notify_error, (void*)client->tts);
		g_connect_timer = NULL;
		return EINA_FALSE;

	} else if (TTS_ERROR_NONE != ret) {
		SLOG(LOG_WARN, TAG_TTSC, "[WARNING] Fail to connection. Retry to connect : %s", __tts_get_error_code(ret));
		return EINA_TRUE;

	} else {
		/* success to connect tts-daemon */
	}

	g_connect_timer = NULL;

	client = tts_client_get(tts);
	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] A handle is not valid");
		return EINA_FALSE;
	}

	client->before_state = client->current_state;
	client->current_state = TTS_STATE_READY;

	if (NULL != client->state_changed_cb) {
		tts_client_use_callback(client);
		client->state_changed_cb(client->tts, client->before_state, client->current_state, client->state_changed_user_data);
		tts_client_not_use_callback(client);
	} else {
		SLOG(LOG_WARN, TAG_TTSC, "State changed callback is NULL");
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return EINA_FALSE;
}

int tts_prepare(tts_h tts)
{
	bool tts_supported = false;
	if (0 == system_info_get_platform_bool(TTS_FEATURE_PATH, &tts_supported)) {
		if (false == tts_supported) {
			SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] TTS NOT supported");
			return TTS_ERROR_NOT_SUPPORTED;
		}
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "===== Prepare TTS");

	tts_client_s* client = tts_client_get(tts);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	/* check state */
	if (client->current_state != TTS_STATE_CREATED) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Invalid State: Current state is not 'CREATED'");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_INVALID_STATE;
	}

	g_connect_timer = ecore_timer_add(0, __tts_connect_daemon, (void*)tts);

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return TTS_ERROR_NONE;
}

int tts_unprepare(tts_h tts)
{
	bool tts_supported = false;
	if (0 == system_info_get_platform_bool(TTS_FEATURE_PATH, &tts_supported)) {
		if (false == tts_supported) {
			SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] TTS NOT supported");
			return TTS_ERROR_NOT_SUPPORTED;
		}
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "===== Unprepare TTS");

	tts_client_s* client = tts_client_get(tts);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] A handle is not available");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	/* check state */
	if (client->current_state != TTS_STATE_READY) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Invalid State: Current state is not 'READY'");
		return TTS_ERROR_INVALID_STATE;
	}

	int ret = -1;
	int count = 0;
	if (!(false == g_screen_reader && TTS_MODE_SCREEN_READER == client->mode)) {
		while (0 != ret) {
			ret = tts_dbus_request_finalize(client->uid);
			if (0 != ret) {
				if (TTS_ERROR_TIMED_OUT != ret) {
					SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] result : %s", __tts_get_error_code(ret));
					break;
				} else {
					SLOG(LOG_WARN, TAG_TTSC, "[WARNING] retry finalize : %s", __tts_get_error_code(ret));
					usleep(10000);
					count++;
					if (TTS_RETRY_COUNT == count) {
						SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to request");
						break;
					}
				}
			}
		}
	} else {
		SLOG(LOG_WARN, TAG_TTSC, "[WARNING] Do not request finalize : g_sr(%d) mode(%d)", g_screen_reader, client->mode);
	}

	client->before_state = client->current_state;
	client->current_state = TTS_STATE_CREATED;

	if (NULL != client->state_changed_cb) {
		tts_client_use_callback(client);
		client->state_changed_cb(client->tts, client->before_state, client->current_state, client->state_changed_user_data);
		tts_client_not_use_callback(client);
		SLOG(LOG_DEBUG, TAG_TTSC, "State changed callback is called");
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return TTS_ERROR_NONE;
}

bool __tts_supported_voice_cb(const char* engine_id, const char* language, int type, void* user_data)
{
	tts_h tts = (tts_h)user_data;

	tts_client_s* client = tts_client_get(tts);
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[WARNING] A handle is not valid");
		return false;
	}

	/* call callback function */
	if (NULL != client->supported_voice_cb) {
		return client->supported_voice_cb(tts, language, type, client->supported_voice_user_data);
	} else {
		SLOG(LOG_WARN, TAG_TTSC, "No registered callback function of supported voice");
	}

	return false;
}

int tts_foreach_supported_voices(tts_h tts, tts_supported_voice_cb callback, void* user_data)
{
	bool tts_supported = false;
	if (0 == system_info_get_platform_bool(TTS_FEATURE_PATH, &tts_supported)) {
		if (false == tts_supported) {
			SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] TTS NOT supported");
			return TTS_ERROR_NOT_SUPPORTED;
		}
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "===== Foreach supported voices");

	if (NULL == tts || NULL == callback) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Input parameter is null");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	tts_client_s* client = tts_client_get(tts);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] A handle is not valid");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	int ret = 0;
	char* current_engine = NULL;
	ret = tts_config_mgr_get_engine(&current_engine);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to get current engine : %d", ret);
		return __tts_convert_config_error_code(ret);
	}

	client->supported_voice_cb = callback;
	client->supported_voice_user_data = user_data;

	ret = tts_config_mgr_get_voice_list(current_engine, __tts_supported_voice_cb, client->tts);

	if (NULL != current_engine)
		free(current_engine);

	client->supported_voice_cb = NULL;
	client->supported_voice_user_data = NULL;

	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Result : %d", ret);
		ret = TTS_ERROR_OPERATION_FAILED;
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return ret;
}

int tts_get_default_voice(tts_h tts, char** lang, int* vctype)
{
	bool tts_supported = false;
	if (0 == system_info_get_platform_bool(TTS_FEATURE_PATH, &tts_supported)) {
		if (false == tts_supported) {
			SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] TTS NOT supported");
			return TTS_ERROR_NOT_SUPPORTED;
		}
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "===== Get default voice");

	if (NULL == tts) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Input handle is null");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_INVALID_PARAMETER;

	}
	tts_client_s* client = tts_client_get(tts);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] A handle is not valid");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	/* Request call remote method */
	int ret = 0;
	ret = tts_config_mgr_get_voice(lang, vctype);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] result : %d", ret);
		return __tts_convert_config_error_code(ret);
	} else {
		SLOG(LOG_DEBUG, TAG_TTSC, "[DEBUG] Default language(%s), type(%d)", *lang, *vctype);
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return ret;
}

int tts_get_max_text_size(tts_h tts, unsigned int* size)
{
	bool tts_supported = false;
	if (0 == system_info_get_platform_bool(TTS_FEATURE_PATH, &tts_supported)) {
		if (false == tts_supported) {
			SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] TTS NOT supported");
			return TTS_ERROR_NOT_SUPPORTED;
		}
	}

	if (NULL == tts || NULL == size) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Get max text count : Input parameter is null");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	tts_client_s* client = tts_client_get(tts);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Get max text count : A handle is not valid");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	if (TTS_STATE_READY != client->current_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Get max text count : Current state is NOT 'READY'.");
		return TTS_ERROR_INVALID_STATE;
	}

	*size = TTS_MAX_TEXT_SIZE;

	SLOG(LOG_DEBUG, TAG_TTSC, "Get max text count : %d", *size);
	return TTS_ERROR_NONE;
}

int tts_get_state(tts_h tts, tts_state_e* state)
{
	bool tts_supported = false;
	if (0 == system_info_get_platform_bool(TTS_FEATURE_PATH, &tts_supported)) {
		if (false == tts_supported) {
			SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] TTS NOT supported");
			return TTS_ERROR_NOT_SUPPORTED;
		}
	}

	if (NULL == tts || NULL == state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Get state : Input parameter is null");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	tts_client_s* client = tts_client_get(tts);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Get state : A handle is not valid");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	*state = client->current_state;

	switch (*state) {
		case TTS_STATE_CREATED:	SLOG(LOG_DEBUG, TAG_TTSC, "Current state is 'Created'");	break;
		case TTS_STATE_READY:	SLOG(LOG_DEBUG, TAG_TTSC, "Current state is 'Ready'");		break;
		case TTS_STATE_PLAYING:	SLOG(LOG_DEBUG, TAG_TTSC, "Current state is 'Playing'");	break;
		case TTS_STATE_PAUSED:	SLOG(LOG_DEBUG, TAG_TTSC, "Current state is 'Paused'");		break;
		default:		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Invalid value");		break;
	}

	return TTS_ERROR_NONE;
}

int tts_get_speed_range(tts_h tts, int* min, int* normal, int* max)
{
	bool tts_supported = false;
	if (0 == system_info_get_platform_bool(TTS_FEATURE_PATH, &tts_supported)) {
		if (false == tts_supported) {
			SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] TTS NOT supported");
			return TTS_ERROR_NOT_SUPPORTED;
		}
	}

	if (NULL == tts || NULL == min || NULL == normal || NULL == max) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Input parameter is null");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	tts_client_s* client = tts_client_get(tts);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Get state : A handle is not valid");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	*min = TTS_SPEED_MIN;
	*normal = TTS_SPEED_NORMAL;
	*max = TTS_SPEED_MAX;

	return TTS_ERROR_NONE;
}

int tts_add_text(tts_h tts, const char* text, const char* language, int voice_type, int speed, int* utt_id)
{
	bool tts_supported = false;
	if (0 == system_info_get_platform_bool(TTS_FEATURE_PATH, &tts_supported)) {
		if (false == tts_supported) {
			SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] TTS NOT supported");
			return TTS_ERROR_NOT_SUPPORTED;
		}
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "===== Add text");

	if (NULL == tts || NULL == utt_id) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Input parameter is null");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	tts_client_s* client = tts_client_get(tts);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] A handle is not valid");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	if (TTS_STATE_CREATED == client->current_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Current state is 'CREATED'.");
		return TTS_ERROR_INVALID_STATE;
	}

	if (TTS_MAX_TEXT_SIZE < strlen(text) || strlen(text) <= 0) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Input text size is invalid.");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	if (TTS_SPEED_AUTO > speed || TTS_SPEED_MAX < speed) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] speed value(%d) is invalid.", speed);
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	if (false == g_screen_reader && TTS_MODE_SCREEN_READER == client->mode) {
		SLOG(LOG_WARN, TAG_TTSC, "[WARNING] Screen reader option is NOT available. Ignore this request");
		return TTS_ERROR_INVALID_STATE;
	}

	/* check valid utf8 */
	iconv_t *ict;
	ict = iconv_open("utf-8", "");
	if ((iconv_t)-1 == ict) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to init for text check");
		return TTS_ERROR_OPERATION_FAILED;
	}

	size_t len = strlen(text);
	char *in_tmp = NULL;
	char in_buf[TTS_MAX_TEXT_SIZE];
	char *out_tmp = NULL;
	char out_buf[TTS_MAX_TEXT_SIZE];
	size_t len_tmp = sizeof(out_buf);

	memset(in_buf, 0, TTS_MAX_TEXT_SIZE);
	snprintf(in_buf, TTS_MAX_TEXT_SIZE, "%s", text);
	in_tmp = in_buf;

	memset(out_buf, 0, TTS_MAX_TEXT_SIZE);
	out_tmp = out_buf;

	size_t st;
	st = iconv(ict, &in_tmp, &len, &out_tmp, &len_tmp);
	if ((size_t)-1 == st) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Text is invalid - '%s'", in_buf);
		iconv_close(ict);
		return TTS_ERROR_INVALID_PARAMETER;
	}
	iconv_close(ict);
	SLOG(LOG_DEBUG, TAG_TTSC, "Text is valid - Converted text is '%s'", out_buf);

	/* change default language value */
	char* temp = NULL;

	if (NULL == language)
		temp = strdup("default");
	else
		temp = strdup(language);

	client->current_utt_id++;
	if (client->current_utt_id == 10000) {
		client->current_utt_id = 1;
	}

	/* do request */
	int ret = -1;
	int count = 0;
	while (0 != ret) {
		ret = tts_dbus_request_add_text(client->uid, out_buf, temp, voice_type, speed, client->current_utt_id);
		if (0 != ret) {
			if (TTS_ERROR_TIMED_OUT != ret) {
				SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] result : %s", __tts_get_error_code(ret));
				break;
			} else {
				SLOG(LOG_WARN, TAG_TTSC, "[WARNING] retry add text : %s", __tts_get_error_code(ret));
				usleep(10000);
				count++;
				if (TTS_RETRY_COUNT == count) {
					SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to request");
					break;
				}
			}
		} else {
			*utt_id = client->current_utt_id;
		}
	}

	if (NULL != temp)	free(temp);

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return ret;
}

int tts_play(tts_h tts)
{
	bool tts_supported = false;
	if (0 == system_info_get_platform_bool(TTS_FEATURE_PATH, &tts_supported)) {
		if (false == tts_supported) {
			SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] TTS NOT supported");
			return TTS_ERROR_NOT_SUPPORTED;
		}
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "===== Play tts");

	if (NULL == tts) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Input handle is null.");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	tts_client_s* client = tts_client_get(tts);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] A handle is not valid.");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	if (TTS_STATE_PLAYING == client->current_state || TTS_STATE_CREATED == client->current_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] The current state is invalid.");
		return TTS_ERROR_INVALID_STATE;
	}

	if (false == g_screen_reader && TTS_MODE_SCREEN_READER == client->mode) {
		SLOG(LOG_WARN, TAG_TTSC, "[WARNING] Screen reader option is NOT available. Ignore this request");
		return TTS_ERROR_INVALID_STATE;
	}

	int ret = -1;
	int count = 0;
	while (0 != ret) {
		ret = tts_dbus_request_play(client->uid);
		if (0 != ret) {
			if (TTS_ERROR_TIMED_OUT != ret) {
				SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] result : %s", __tts_get_error_code(ret));
				return ret;
			} else {
				SLOG(LOG_WARN, TAG_TTSC, "[WARNING] retry play : %s", __tts_get_error_code(ret));
				usleep(10000);
				count++;
				if (TTS_RETRY_COUNT == count) {
					SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to request");
					return ret;
				}
			}
		}
	}

	client->before_state = client->current_state;
	client->current_state = TTS_STATE_PLAYING;

	if (NULL != client->state_changed_cb) {
		tts_client_use_callback(client);
		client->state_changed_cb(client->tts, client->before_state, client->current_state, client->state_changed_user_data);
		tts_client_not_use_callback(client);
		SLOG(LOG_DEBUG, TAG_TTSC, "State changed callback is called");
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return TTS_ERROR_NONE;
}


int tts_stop(tts_h tts)
{
	bool tts_supported = false;
	if (0 == system_info_get_platform_bool(TTS_FEATURE_PATH, &tts_supported)) {
		if (false == tts_supported) {
			SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] TTS NOT supported");
			return TTS_ERROR_NOT_SUPPORTED;
		}
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "===== Stop tts");

	if (NULL == tts) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Input handle is null");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	tts_client_s* client = tts_client_get(tts);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] A handle is not valid");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	if (TTS_STATE_CREATED == client->current_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Current state is 'CREATED'.");
		return TTS_ERROR_INVALID_STATE;
	}

	if (false == g_screen_reader && TTS_MODE_SCREEN_READER == client->mode) {
		SLOG(LOG_WARN, TAG_TTSC, "[WARNING] Screen reader option is NOT available. Ignore this request");
		return TTS_ERROR_INVALID_STATE;
	}

	int ret = -1;
	int count = 0;
	while (0 != ret) {
		ret = tts_dbus_request_stop(client->uid);
		if (0 != ret) {
			if (TTS_ERROR_TIMED_OUT != ret) {
				SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] result : %s", __tts_get_error_code(ret));
				return ret;
			} else {
				SLOG(LOG_WARN, TAG_TTSC, "[WARNING] retry stop : %s", __tts_get_error_code(ret));
				usleep(10000);
				count++;
				if (TTS_RETRY_COUNT == count) {
					SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to request");
					return ret;
				}
			}
		}
	}

	client->before_state = client->current_state;
	client->current_state = TTS_STATE_READY;

	if (NULL != client->state_changed_cb) {
		tts_client_use_callback(client);
		client->state_changed_cb(client->tts, client->before_state, client->current_state, client->state_changed_user_data);
		tts_client_not_use_callback(client);
		SLOG(LOG_DEBUG, TAG_TTSC, "State changed callback is called");
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return TTS_ERROR_NONE;
}


int tts_pause(tts_h tts)
{
	bool tts_supported = false;
	if (0 == system_info_get_platform_bool(TTS_FEATURE_PATH, &tts_supported)) {
		if (false == tts_supported) {
			SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] TTS NOT supported");
			return TTS_ERROR_NOT_SUPPORTED;
		}
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "===== Pause tts");

	if (NULL == tts) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Input handle is null");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	tts_client_s* client = tts_client_get(tts);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] A handle is not valid");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	if (TTS_STATE_PLAYING != client->current_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[Error] The Current state is NOT 'playing'. So this request should be not running.");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_INVALID_STATE;
	}

	if (false == g_screen_reader && TTS_MODE_SCREEN_READER == client->mode) {
		SLOG(LOG_WARN, TAG_TTSC, "[WARNING] Screen reader option is NOT available. Ignore this request");
		return TTS_ERROR_INVALID_STATE;
	}

	int ret = -1;
	int count = 0;
	while (0 != ret) {
		ret = tts_dbus_request_pause(client->uid);
		if (0 != ret) {
			if (TTS_ERROR_TIMED_OUT != ret) {
				SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] result : %s", __tts_get_error_code(ret));
				return ret;
			} else {
				SLOG(LOG_WARN, TAG_TTSC, "[WARNING] retry pause : %s", __tts_get_error_code(ret));
				usleep(10000);
				count++;
				if (TTS_RETRY_COUNT == count) {
					SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to request");
					return ret;
				}
			}
		}
	}

	client->before_state = client->current_state;
	client->current_state = TTS_STATE_PAUSED;

	if (NULL != client->state_changed_cb) {
		tts_client_use_callback(client);
		client->state_changed_cb(client->tts, client->before_state, client->current_state, client->state_changed_user_data);
		tts_client_not_use_callback(client);
		SLOG(LOG_DEBUG, TAG_TTSC, "State changed callback is called");
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return TTS_ERROR_NONE;
}

static Eina_Bool __tts_notify_error(void *data)
{
	tts_h tts = (tts_h)data;

	tts_client_s* client = tts_client_get(tts);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_WARN, TAG_TTSC, "Fail to notify error msg : A handle is not valid");
		return EINA_FALSE;
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "Error data : uttid(%d) reason(%s)", client->utt_id, __tts_get_error_code(client->reason));

	if (NULL != client->error_cb) {
		SLOG(LOG_DEBUG, TAG_TTSC, "Call callback function of error");
		tts_client_use_callback(client);
		client->error_cb(client->tts, client->utt_id, client->reason, client->error_user_data);
		tts_client_not_use_callback(client);
	} else {
		SLOG(LOG_WARN, TAG_TTSC, "No registered callback function of error ");
	}

	return EINA_FALSE;
}

int __tts_cb_error(int uid, tts_error_e reason, int utt_id)
{
	tts_client_s* client = tts_client_get_by_uid(uid);

	if (NULL == client) {
		SLOG(LOG_WARN, TAG_TTSC, "[WARNING] A handle is not valid");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	client->utt_id = utt_id;
	client->reason = reason;

	/* call callback function */
	if (NULL != client->error_cb) {
		ecore_timer_add(0, __tts_notify_error, client->tts);
	} else {
		SLOG(LOG_WARN, TAG_TTSC, "No registered callback function of error ");
	}

	return 0;
}

static Eina_Bool __tts_notify_state_changed(void *data)
{
	tts_h tts = (tts_h)data;

	tts_client_s* client = tts_client_get(tts);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_WARN, TAG_TTSC, "Fail to notify state changed : A handle is not valid");
		return EINA_FALSE;
	}

	if (NULL != client->state_changed_cb) {
		tts_client_use_callback(client);
		client->state_changed_cb(client->tts, client->before_state, client->current_state, client->state_changed_user_data);
		tts_client_not_use_callback(client);
		SLOG(LOG_DEBUG, TAG_TTSC, "State changed callback is called : pre(%d) cur(%d)", client->before_state, client->current_state);
	} else {
		SLOG(LOG_WARN, TAG_TTSC, "[WARNING] State changed callback is null");
	}

	return EINA_FALSE;
}

int __tts_cb_set_state(int uid, int state)
{
	tts_client_s* client = tts_client_get_by_uid(uid);
	if (NULL == client) {
		SLOG(LOG_WARN, TAG_TTSC, "[WARNING] The handle is not valid");
		return -1;
	}

	tts_state_e state_from_daemon = (tts_state_e)state;

	if (client->current_state == state_from_daemon) {
		SLOG(LOG_DEBUG, TAG_TTSC, "Current state has already been %d", client->current_state);
		return 0;
	}

	if (NULL != client->state_changed_cb) {
		ecore_timer_add(0, __tts_notify_state_changed, client->tts);
	} else {
		SLOG(LOG_WARN, TAG_TTSC, "[WARNING] State changed callback is null");
	}

	client->before_state = client->current_state;
	client->current_state = state_from_daemon;

	return 0;
}

static Eina_Bool __tts_notify_utt_started(void *data)
{
	tts_h tts = (tts_h)data;

	tts_client_s* client = tts_client_get(tts);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_WARN, TAG_TTSC, "[WARNING] Fail to notify utt started : A handle is not valid");
		return EINA_FALSE;
	}

	if (NULL != client->utt_started_cb) {
		SLOG(LOG_DEBUG, TAG_TTSC, "Call callback function of utterance started ");
		tts_client_use_callback(client);
		client->utt_started_cb(client->tts, client->utt_id, client->utt_started_user_data);
		tts_client_not_use_callback(client);
	} else {
		SLOG(LOG_WARN, TAG_TTSC, "No registered callback function of utterance started ");
	}

	return EINA_FALSE;
}

int __tts_cb_utt_started(int uid, int utt_id)
{
	tts_client_s* client = tts_client_get_by_uid(uid);

	if (NULL == client) {
		SLOG(LOG_WARN, TAG_TTSC, "[WARNING] A handle is not valid");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "utterance started : utt id(%d) ", utt_id);

	client->utt_id = utt_id;

	/* call callback function */
	if (NULL != client->utt_started_cb) {
		ecore_timer_add(0, __tts_notify_utt_started, client->tts);
	} else {
		SLOG(LOG_WARN, TAG_TTSC, "No registered callback function of utterance started ");
	}

	return 0;
}

static Eina_Bool __tts_notify_utt_completed(void *data)
{
	tts_h tts = (tts_h)data;

	tts_client_s* client = tts_client_get(tts);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_WARN, TAG_TTSC, "[WARNING] Fail to notify utt completed : A handle is not valid");
		return EINA_FALSE;
	}

	if (NULL != client->utt_completeted_cb) {
		SLOG(LOG_DEBUG, TAG_TTSC, "Call callback function of utterance completed ");
		tts_client_use_callback(client);
		client->utt_completeted_cb(client->tts, client->utt_id, client->utt_completed_user_data);
		tts_client_not_use_callback(client);
	} else {
		SLOG(LOG_WARN, TAG_TTSC, "No registered callback function of utterance completed ");
	}

	return EINA_FALSE;
}

int __tts_cb_utt_completed(int uid, int utt_id)
{
	tts_client_s* client = tts_client_get_by_uid(uid);

	if (NULL == client) {
		SLOG(LOG_WARN, TAG_TTSC, "[WARNING] A handle is not valid");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "utterance completed : uttid(%d) ", utt_id);

	client->utt_id = utt_id;

	/* call callback function */
	if (NULL != client->utt_completeted_cb) {
		ecore_timer_add(0, __tts_notify_utt_completed, client->tts);
	} else {
		SLOG(LOG_WARN, TAG_TTSC, "No registered callback function of utterance completed ");
	}

	return 0;
}

int tts_set_state_changed_cb(tts_h tts, tts_state_changed_cb callback, void* user_data)
{
	bool tts_supported = false;
	if (0 == system_info_get_platform_bool(TTS_FEATURE_PATH, &tts_supported)) {
		if (false == tts_supported) {
			SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] TTS NOT supported");
			return TTS_ERROR_NOT_SUPPORTED;
		}
	}

	if (NULL == tts || NULL == callback) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Set state changed cb : Input parameter is null");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	tts_client_s* client = tts_client_get(tts);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Set state changed cb : A handle is not valid");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	if (TTS_STATE_CREATED != client->current_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Set state changed cb : Current state is not 'Created'.");
		return TTS_ERROR_INVALID_STATE;
	}

	client->state_changed_cb = callback;
	client->state_changed_user_data = user_data;

	SLOG(LOG_DEBUG, TAG_TTSC, "[SUCCESS] Set state changed cb");

	return 0;
}

int tts_unset_state_changed_cb(tts_h tts)
{
	bool tts_supported = false;
	if (0 == system_info_get_platform_bool(TTS_FEATURE_PATH, &tts_supported)) {
		if (false == tts_supported) {
			SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] TTS NOT supported");
			return TTS_ERROR_NOT_SUPPORTED;
		}
	}

	if (NULL == tts) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Unset state changed cb : Input parameter is null");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	tts_client_s* client = tts_client_get(tts);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Unset state changed cb : A handle is not valid");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	if (TTS_STATE_CREATED != client->current_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Unset state changed cb : Current state is not 'Created'.");
		return TTS_ERROR_INVALID_STATE;
	}

	client->state_changed_cb = NULL;
	client->state_changed_user_data = NULL;

	SLOG(LOG_DEBUG, TAG_TTSC, "[SUCCESS] Unset state changed cb");

	return 0;
}

int tts_set_utterance_started_cb(tts_h tts, tts_utterance_started_cb callback, void* user_data)
{
	bool tts_supported = false;
	if (0 == system_info_get_platform_bool(TTS_FEATURE_PATH, &tts_supported)) {
		if (false == tts_supported) {
			SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] TTS NOT supported");
			return TTS_ERROR_NOT_SUPPORTED;
		}
	}

	if (NULL == tts || NULL == callback) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Set utt started cb : Input parameter is null");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	tts_client_s* client = tts_client_get(tts);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Set utt started cb : A handle is not valid");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	if (TTS_STATE_CREATED != client->current_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Set utt started cb : Current state is not 'Created'.");
		return TTS_ERROR_INVALID_STATE;
	}

	client->utt_started_cb = callback;
	client->utt_started_user_data = user_data;

	SLOG(LOG_DEBUG, TAG_TTSC, "[SUCCESS] Set utt started cb");

	return 0;
}

int tts_unset_utterance_started_cb(tts_h tts)
{
	bool tts_supported = false;
	if (0 == system_info_get_platform_bool(TTS_FEATURE_PATH, &tts_supported)) {
		if (false == tts_supported) {
			SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] TTS NOT supported");
			return TTS_ERROR_NOT_SUPPORTED;
		}
	}

	if (NULL == tts) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Unset utt started cb : Input parameter is null");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	tts_client_s* client = tts_client_get(tts);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Unset utt started cb : A handle is not valid");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	if (TTS_STATE_CREATED != client->current_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Unset utt started cb : Current state is not 'Created'.");
		return TTS_ERROR_INVALID_STATE;
	}

	client->utt_started_cb = NULL;
	client->utt_started_user_data = NULL;

	SLOG(LOG_DEBUG, TAG_TTSC, "[SUCCESS] Unset utt started cb");

	return 0;
}

int tts_set_utterance_completed_cb(tts_h tts, tts_utterance_completed_cb callback, void* user_data)
{
	bool tts_supported = false;
	if (0 == system_info_get_platform_bool(TTS_FEATURE_PATH, &tts_supported)) {
		if (false == tts_supported) {
			SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] TTS NOT supported");
			return TTS_ERROR_NOT_SUPPORTED;
		}
	}

	if (NULL == tts || NULL == callback) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Set utt completed cb : Input parameter is null");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	tts_client_s* client = tts_client_get(tts);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Set utt completed cb : A handle is not valid");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	if (TTS_STATE_CREATED != client->current_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Set utt completed cb : Current state is not 'Created'.");
		return TTS_ERROR_INVALID_STATE;
	}

	client->utt_completeted_cb = callback;
	client->utt_completed_user_data = user_data;

	SLOG(LOG_DEBUG, TAG_TTSC, "[SUCCESS] Set utt completed cb");

	return 0;
}

int tts_unset_utterance_completed_cb(tts_h tts)
{
	bool tts_supported = false;
	if (0 == system_info_get_platform_bool(TTS_FEATURE_PATH, &tts_supported)) {
		if (false == tts_supported) {
			SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] TTS NOT supported");
			return TTS_ERROR_NOT_SUPPORTED;
		}
	}

	if (NULL == tts) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Unset utt completed cb : Input parameter is null");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	tts_client_s* client = tts_client_get(tts);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Unset utt completed cb : A handle is not valid");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	if (TTS_STATE_CREATED != client->current_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Unset utt completed cb : Current state is not 'Created'.");
		return TTS_ERROR_INVALID_STATE;
	}

	client->utt_completeted_cb = NULL;
	client->utt_completed_user_data = NULL;

	SLOG(LOG_DEBUG, TAG_TTSC, "[SUCCESS] Unset utt completed cb");
	return 0;
}

int tts_set_error_cb(tts_h tts, tts_error_cb callback, void* user_data)
{
	bool tts_supported = false;
	if (0 == system_info_get_platform_bool(TTS_FEATURE_PATH, &tts_supported)) {
		if (false == tts_supported) {
			SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] TTS NOT supported");
			return TTS_ERROR_NOT_SUPPORTED;
		}
	}

	if (NULL == tts || NULL == callback) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Set error cb : Input parameter is null");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	tts_client_s* client = tts_client_get(tts);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Set error cb : A handle is not valid");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	if (TTS_STATE_CREATED != client->current_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Set error cb : Current state is not 'Created'.");
		return TTS_ERROR_INVALID_STATE;
	}

	client->error_cb = callback;
	client->error_user_data = user_data;

	SLOG(LOG_DEBUG, TAG_TTSC, "[SUCCESS] Set error cb");

	return 0;
}

int tts_unset_error_cb(tts_h tts)
{
	bool tts_supported = false;
	if (0 == system_info_get_platform_bool(TTS_FEATURE_PATH, &tts_supported)) {
		if (false == tts_supported) {
			SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] TTS NOT supported");
			return TTS_ERROR_NOT_SUPPORTED;
		}
	}

	if (NULL == tts) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Unset error cb : Input parameter is null");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	tts_client_s* client = tts_client_get(tts);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Unset error cb : A handle is not valid");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	if (TTS_STATE_CREATED != client->current_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Unset error cb : Current state is not 'Created'.");
		return TTS_ERROR_INVALID_STATE;
	}

	client->error_cb = NULL;
	client->error_user_data = NULL;

	SLOG(LOG_DEBUG, TAG_TTSC, "[SUCCESS] Unset error cb");

	return 0;
}

int tts_set_default_voice_changed_cb(tts_h tts, tts_default_voice_changed_cb callback, void* user_data)
{
	bool tts_supported = false;
	if (0 == system_info_get_platform_bool(TTS_FEATURE_PATH, &tts_supported)) {
		if (false == tts_supported) {
			SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] TTS NOT supported");
			return TTS_ERROR_NOT_SUPPORTED;
		}
	}

	if (NULL == tts || NULL == callback) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Set default voice changed cb : Input parameter is null");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	tts_client_s* client = tts_client_get(tts);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Set default voice changed cb : A handle is not valid");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	if (TTS_STATE_CREATED != client->current_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Set default voice changed cb : Current state is not 'Created'.");
		return TTS_ERROR_INVALID_STATE;
	}

	client->default_voice_changed_cb = callback;
	client->default_voice_changed_user_data = user_data;

	SLOG(LOG_DEBUG, TAG_TTSC, "[SUCCESS] Set default voice changed cb");

	return 0;
}

int tts_unset_default_voice_changed_cb(tts_h tts)
{
	bool tts_supported = false;
	if (0 == system_info_get_platform_bool(TTS_FEATURE_PATH, &tts_supported)) {
		if (false == tts_supported) {
			SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] TTS NOT supported");
			return TTS_ERROR_NOT_SUPPORTED;
		}
	}

	if (NULL == tts) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Unset default voice changed cb : Input parameter is null");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	tts_client_s* client = tts_client_get(tts);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Unset default voice changed cb : A handle is not valid");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	if (TTS_STATE_CREATED != client->current_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Unset default voice changed cb : Current state is not 'Created'.");
		return TTS_ERROR_INVALID_STATE;
	}

	client->default_voice_changed_cb = NULL;
	client->default_voice_changed_user_data = NULL;

	SLOG(LOG_DEBUG, TAG_TTSC, "[SUCCESS] Unset default voice changed cb");

	return 0;
}
