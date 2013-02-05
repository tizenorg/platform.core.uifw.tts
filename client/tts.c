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


#include <sys/wait.h>
#include <Ecore.h>
#include <sys/stat.h>
#include <sys/types.h> 
#include <dirent.h>

#include "tts_main.h"
#include "tts_client.h"
#include "tts_dbus.h"

#define MAX_TEXT_COUNT 1000

static bool g_is_daemon_started = false;

static Ecore_Timer* g_connect_timer = NULL;

/* Function definition */
static int __tts_check_tts_daemon();
static Eina_Bool __tts_notify_state_changed(void *data);
static Eina_Bool __tts_notify_error(void *data);

int tts_create(tts_h* tts)
{
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
			SLOG(LOG_DEBUG, TAG_TTSC, "=====");
			SLOG(LOG_DEBUG, TAG_TTSC, " ");
			return TTS_ERROR_OPERATION_FAILED;
		}
	}

	if (0 != tts_client_new(tts)) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to create client!!!!!");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_OUT_OF_MEMORY;
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return TTS_ERROR_NONE;
}

int tts_destroy(tts_h tts)
{
	SLOG(LOG_DEBUG, TAG_TTSC, "===== Destroy TTS");

	if (NULL == tts) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Input handle is null");
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

	/* check used callback */
	if (0 != tts_client_get_use_callback(client)) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Cannot destroy in Callback function");
		return TTS_ERROR_OPERATION_FAILED;
	}

	int ret = -1;

	/* check state */
	switch (client->current_state) {
	case TTS_STATE_PAUSED:
	case TTS_STATE_PLAYING:
	case TTS_STATE_READY:
		/* Request Finalize */
		ret = tts_dbus_request_finalize(client->uid);
		if (0 != ret) {
			SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to request finalize");
		}
		g_is_daemon_started = false;
	case TTS_STATE_CREATED:
		if (NULL != g_connect_timer) {
			SLOG(LOG_DEBUG, TAG_TTSC, "Connect Timer is deleted");
			ecore_timer_del(g_connect_timer);
		}
		/* Free resources */
		tts_client_destroy(tts);
		break;
	}
 
	if (0 == tts_client_get_size()) {
		if (0 != tts_dbus_close_connection()) {
			SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to close connection");
		}
	} 

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return TTS_ERROR_NONE;
}

int tts_set_mode(tts_h tts, tts_mode_e mode)
{
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

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return TTS_ERROR_NONE;
}

int tts_get_mode(tts_h tts, tts_mode_e* mode)
{
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
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_INVALID_PARAMETER;
	} 

	*mode = client->mode;
	
	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return TTS_ERROR_NONE;
}

static Eina_Bool __tts_connect_daemon(void *data)
{
	SLOG(LOG_DEBUG, TAG_TTSC, "===== Connect daemon");

	tts_h tts = (tts_h)data;

	tts_client_s* client = tts_client_get(tts);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] A handle is not valid");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return EINA_FALSE;
	}

	/* Send hello */
	if (0 != tts_dbus_request_hello()) {
		if (false == g_is_daemon_started) {
			g_is_daemon_started = true;
			__tts_check_tts_daemon();
		}
		return EINA_TRUE;
	}

	/* do request initialize */
	int ret = -1;

	ret = tts_dbus_request_initialize(client->uid);

	if (TTS_ERROR_ENGINE_NOT_FOUND == ret) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Engine not found");
		
		client->reason = TTS_ERROR_ENGINE_NOT_FOUND;
		client->utt_id = -1;

		ecore_timer_add(0, __tts_notify_error, (void*)client->tts);
		return EINA_FALSE;

	} else if (TTS_ERROR_NONE != ret) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to connection");

		client->reason = TTS_ERROR_TIMED_OUT;
		client->utt_id = -1;

		ecore_timer_add(0, __tts_notify_error, (void*)client->tts);
		return EINA_FALSE;
	} else {
		/* success to connect tts-daemon */
	}

	client->before_state = client->current_state;
	client->current_state = TTS_STATE_READY;

	ecore_timer_add(0, __tts_notify_state_changed, (void*)client->tts);

	g_connect_timer = NULL;

	SLOG(LOG_DEBUG, TAG_TTSC, "[SUCCESS] uid(%d)", client->uid);

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return EINA_FALSE;
}

int tts_prepare(tts_h tts)
{
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
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_INVALID_STATE;
	}

	int ret = tts_dbus_request_finalize(client->uid);
	if (0 != ret) {
		SLOG(LOG_WARN, TAG_TTSC, "[ERROR] Fail to request finalize");
	}
	g_is_daemon_started = false;

	client->before_state = client->current_state;
	client->current_state = TTS_STATE_CREATED;

	ecore_timer_add(0, __tts_notify_state_changed, (void*)tts);

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return TTS_ERROR_NONE;
}

int tts_foreach_supported_voices(tts_h tts, tts_supported_voice_cb callback, void* user_data)
{
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

	if (TTS_STATE_READY != client->current_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "Current state is NOT 'READY'.");  
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_INVALID_STATE;
	}

	int ret = 0;
	ret = tts_dbus_request_get_support_voice(client->uid, client->tts, callback, user_data);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] result : %d", ret);
	}    

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return ret;
}

int tts_get_default_voice(tts_h tts, char** lang, tts_voice_type_e* vctype)
{
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

	if (TTS_STATE_READY != client->current_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Current state is NOT 'READY'. ");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_INVALID_STATE;
	}

	/* Request call remote method */
	int ret = 0;
	ret = tts_dbus_request_get_default_voice(client->uid, lang, vctype );
    
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] result : %d", ret);
	}
	
	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return ret;
}

int tts_get_max_text_count(tts_h tts, int* count)
{
	if (NULL == tts || NULL == count) {
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

	*count = MAX_TEXT_COUNT;

	SLOG(LOG_DEBUG, TAG_TTSC, "[Suceess] Get max text count");
	return TTS_ERROR_NONE;
}

int tts_get_state(tts_h tts, tts_state_e* state)
{
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

	switch(*state) {
		case TTS_STATE_CREATED:	SLOG(LOG_DEBUG, TAG_TTSC, "Current state is 'Created'");	break;
		case TTS_STATE_READY:	SLOG(LOG_DEBUG, TAG_TTSC, "Current state is 'Ready'");		break;
		case TTS_STATE_PLAYING:	SLOG(LOG_DEBUG, TAG_TTSC, "Current state is 'Playing'");	break;
		case TTS_STATE_PAUSED:	SLOG(LOG_DEBUG, TAG_TTSC, "Current state is 'Paused'");		break;
	}

	return TTS_ERROR_NONE;
}

int tts_add_text(tts_h tts, const char* text, const char* language, tts_voice_type_e voice_type, tts_speed_e speed, int* utt_id)
{
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

	/* change default language value */
	char* temp = NULL;

	if (NULL == language)
		temp = strdup("default");
	else 
		temp = strdup(language);

	client->current_utt_id ++;
	if (client->current_utt_id == 10000) {
		client->current_utt_id = 1;
	}

	/* do request */
	int ret = 0;
	ret = tts_dbus_request_add_text(client->uid, text, temp, voice_type, speed, client->current_utt_id);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] result : %d", ret);
	}

	*utt_id = client->current_utt_id;

	if (NULL != temp)
		free(temp);

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return ret;
}

int tts_play(tts_h tts)
{
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

	int ret = 0;
	ret = tts_dbus_request_play(client->uid);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Request play : result(%d)", ret);
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return ret;
	}

	client->before_state = client->current_state;
	client->current_state = TTS_STATE_PLAYING;

	if (NULL != client->state_changed_cb) {
		ecore_timer_add(0, __tts_notify_state_changed, (void*)tts);
	} else {
		SLOG(LOG_WARN, TAG_TTSC, "[WARNING] State changed callback is null");
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return TTS_ERROR_NONE;
}


int tts_stop(tts_h tts)
{
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

	int ret = 0;
	ret = tts_dbus_request_stop(client->uid);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] result : %d", ret);
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return ret;
	}	

	client->before_state = client->current_state;
	client->current_state = TTS_STATE_READY;

	if (NULL != client->state_changed_cb) {
		ecore_timer_add(0, __tts_notify_state_changed, (void*)tts);
	} else {
		SLOG(LOG_WARN, TAG_TTSC, "[WARNING] State changed callback is null");
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return TTS_ERROR_NONE;
}


int tts_pause(tts_h tts)
{
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
	
	int ret = 0;
	ret = tts_dbus_request_pause(client->uid);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Request pause : result(%d)", ret);
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return ret;
	}

	client->before_state = client->current_state;
	client->current_state = TTS_STATE_PAUSED;

	if (NULL != client->state_changed_cb) {
		ecore_timer_add(0, __tts_notify_state_changed, (void*)tts);
	} else {
		SLOG(LOG_WARN, TAG_TTSC, "[WARNING] State changed callback is null");
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

	if (NULL != client->error_cb) {
		SLOG(LOG_DEBUG, TAG_TTSC, "Call callback function of error");
		tts_client_use_callback(client);
		client->error_cb(client->tts, client->utt_id, client->reason, client->error_user_data );
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
		SLOG(LOG_DEBUG, TAG_TTSC, "State changed callback is called");
	} else {
		SLOG(LOG_WARN, TAG_TTSC, "[WARNING] State changed callback is null");
	}

	return EINA_FALSE;
}

int __tts_cb_set_state(int uid, int state)
{
	tts_client_s* client = tts_client_get_by_uid(uid);
	if( NULL == client ) {
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

int __get_cmd_line(char *file, char *buf) 
{
	FILE *fp = NULL;

	fp = fopen(file, "r");
	if (fp == NULL) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Get command line");
		return -1;
	}

	memset(buf, 0, 256);
	if (NULL == fgets(buf, 256, fp)) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to fget command line");
		fclose(fp);
		return -1;
	}
	fclose(fp);
	return 0;
}

static bool _tts_is_alive()
{
	DIR *dir;
	struct dirent *entry;
	struct stat filestat;
	
	int pid;
	char cmdLine[256];
	char tempPath[256];

	dir  = opendir("/proc");
	if (NULL == dir) {
		SLOG(LOG_ERROR, TAG_TTSC, "process checking is FAILED");
		return FALSE;
	}

	while ((entry = readdir(dir)) != NULL) {
		if (0 != lstat(entry->d_name, &filestat))
			continue;

		if (!S_ISDIR(filestat.st_mode))
			continue;

		pid = atoi(entry->d_name);
		if (pid <= 0) continue;

		sprintf(tempPath, "/proc/%d/cmdline", pid);
		if (0 != __get_cmd_line(tempPath, cmdLine)) {
			continue;
		}

		if ( 0 == strncmp(cmdLine, "[tts-daemon]", strlen("[tts-daemon]")) ||
			0 == strncmp(cmdLine, "tts-daemon", strlen("tts-daemon")) ||
			0 == strncmp(cmdLine, "/usr/bin/tts-daemon", strlen("/usr/bin/tts-daemon"))) {
				SLOG(LOG_DEBUG, TAG_TTSC, "tts-daemon is ALIVE !!");
				closedir(dir);
				return TRUE;
		}
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "THERE IS NO tts-daemon !!");

	closedir(dir);
	return FALSE;
}

static int __tts_check_tts_daemon()
{
	if (TRUE == _tts_is_alive()) {
		return 0;
	}
	
	/* fork-exec tts-daemom */
	int pid, i;

	pid = fork();

	switch(pid) {
	case -1:
		SLOG(LOG_ERROR, TAG_TTSC, "Fail to create tts-daemon");
		break;

	case 0:
		setsid();
		for (i = 0;i < _NSIG;i++)
			signal(i, SIG_DFL);

		execl("/usr/bin/tts-daemon", "/usr/bin/tts-daemon", NULL);
		break;

	default:
		break;
	}

	return 0;
}
