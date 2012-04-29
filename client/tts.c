/*
*  Copyright (c) 2011 Samsung Electronics Co., Ltd All Rights Reserved 
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

#include "tts_main.h"
#include "tts_client.h"
#include "tts_dbus.h"

#define MAX_TEXT_COUNT 1000
#define CONNECTION_RETRY_COUNT 2

/* Function definition */
int __tts_check_tts_daemon();

int tts_create(tts_h* tts)
{
	int ret = 0; 

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

	/* Send hello */
	if (0 != tts_dbus_request_hello()) {
		__tts_check_tts_daemon();
	}

	if (0 != tts_client_new(tts)) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to create client!!!!!");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_OUT_OF_MEMORY;
	}

	/* do request initialize */
	int i = 1;
	while(1) {
		ret = tts_dbus_request_initialize((*tts)->handle);

		if (TTS_ERROR_ENGINE_NOT_FOUND == ret) {
			tts_client_destroy(*tts);
			SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Engine not found");
			SLOG(LOG_DEBUG, TAG_TTSC, "=====");
			SLOG(LOG_DEBUG, TAG_TTSC, " ");
			return ret;
		} else if (TTS_ERROR_NONE != ret) {
			usleep(1);
			if (i == CONNECTION_RETRY_COUNT) {
			    SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to connection");
			    SLOG(LOG_DEBUG, TAG_TTSC, "=====");
			    SLOG(LOG_DEBUG, TAG_TTSC, " ");
			    return TTS_ERROR_TIMED_OUT;			    
			}    
			i++;
		} else {
			/* success to connect tts-daemon */
			break;
		}
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "[SUCCESS] uid(%d)", (*tts)->handle);

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

	/* Request Finalize */
	int ret = tts_dbus_request_finalize(client->uid);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to request finalize ");
	}    

	/* Free resources */
	tts_client_destroy(tts);

	if (0 == tts_client_get_size()) {
		if (0 != tts_dbus_close_connection()) {
			SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to close connection\n ");
		}
	}

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
		SLOG(LOG_ERROR, TAG_TTSC, "Current state is NOT 'READY'.\n");  
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

	if (TTS_STATE_PLAYING == client->current_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "Current state is 'playing'. This request should be skipped.\n"); 
		return TTS_ERROR_INVALID_STATE;
	} 

	int ret = 0;
	ret = tts_dbus_request_play(client->uid);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] result : %d", ret);
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return ret;
	}

	/* change state */
	client->current_state = TTS_STATE_PLAYING;

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

	SLOG(LOG_DEBUG, TAG_TTSC, "change state to ready\n");
	client->current_state = TTS_STATE_READY;

	int ret = 0;
	ret = tts_dbus_request_stop(client->uid);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] result : %d", ret);
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return ret;
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
		SLOG(LOG_ERROR, TAG_TTSC, "Error : Current state is NOT 'playing'. So this request should be not running.\n");    
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_INVALID_STATE;
	}	
	
	int ret = 0;
	ret = tts_dbus_request_pause(client->uid);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] result : %d", ret);
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return ret;
	}

	client->current_state = TTS_STATE_PAUSED;

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return TTS_ERROR_NONE;
}

int __tts_cb_error(int uid, tts_error_e reason, int utt_id)
{
	tts_client_s* client = tts_client_get_by_uid(uid);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] A handle is not valid");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	/* call callback function */
	if (NULL != client->error_cb) {
		SLOG(LOG_DEBUG, TAG_TTSC, "Call callback function of error");
		tts_client_use_callback(client);
		client->error_cb(client->tts, utt_id, reason, client->error_user_data );
		tts_client_not_use_callback(client);
	} else {
		SLOG(LOG_WARN, TAG_TTSC, "No registered callback function of error \n");
	}
	
	return 0;
}

int __tts_cb_interrupt(int uid, tts_interrupted_code_e code)
{
	tts_client_s* client = tts_client_get_by_uid(uid);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] A handle is not valid");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	/* change state by interrupt code */
	if (TTS_INTERRUPTED_PAUSED == code) {
		SLOG(LOG_DEBUG, TAG_TTSC, "change state to ready");
		client->current_state = TTS_STATE_PAUSED;
	} else if (TTS_INTERRUPTED_STOPPED == code) {
		SLOG(LOG_DEBUG, TAG_TTSC, "change state to ready");
		client->current_state = TTS_STATE_READY;
	} else {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Interrupt code is not available");
		return -1;
	}

	/* call callback function */
	if (NULL != client->interrupted_cb) {
		SLOG(LOG_DEBUG, TAG_TTSC, "Call callback function of stopped \n");
		tts_client_use_callback(client);
		client->interrupted_cb(client->tts, code, client->interrupted_user_data);
		tts_client_not_use_callback(client);
	} else {
		SLOG(LOG_WARN, TAG_TTSC, "No registered callback function of stopped \n");
	}

	return 0;
}

int __tts_cb_utt_started(int uid, int utt_id)
{
	tts_client_s* client = tts_client_get_by_uid(uid);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] A handle is not valid");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "utterance started : uttid(%d) \n", utt_id);

	/* call callback function */
	if (NULL != client->utt_started_cb) {
		SLOG(LOG_DEBUG, TAG_TTSC, "Call callback function of utterance started \n");
		tts_client_use_callback(client);
		client->utt_started_cb(client->tts, utt_id, client->utt_started_user_data);
		tts_client_not_use_callback(client);
	} else {
		SLOG(LOG_WARN, TAG_TTSC, "No registered callback function of utterance started \n");
	}

	return 0;
}

int __tts_cb_utt_completed(int uid, int utt_id)
{
	tts_client_s* client = tts_client_get_by_uid(uid);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] A handle is not valid");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "utterance completed : uttid(%d) \n", utt_id);

	/* call callback function */
	if (NULL != client->utt_completeted_cb) {
		SLOG(LOG_DEBUG, TAG_TTSC, "Call callback function of utterance completed \n");
		tts_client_use_callback(client);
		client->utt_completeted_cb(client->tts, utt_id, client->utt_completed_user_data);
		tts_client_not_use_callback(client);
	} else {
		SLOG(LOG_WARN, TAG_TTSC, "No registered callback function of utterance completed \n");
	}

	return 0;
}

int tts_set_interrupted_cb(tts_h tts, tts_interrupted_cb callback, void* user_data)
{
	if (NULL == tts || NULL == callback) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Set interrupted cb : Input parameter is null");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	tts_client_s* client = tts_client_get(tts);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Set interrupted cb : A handle is not valid");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	if (TTS_STATE_READY != client->current_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Set interrupted cb : Current state is not 'ready'."); 
		return TTS_ERROR_INVALID_STATE;
	}

	client->interrupted_cb = callback;
	client->interrupted_user_data = user_data;

	SLOG(LOG_DEBUG, TAG_TTSC, "[SUCCESS] Set interrupted cb");

	return 0;
}

int tts_unset_interrupted_cb(tts_h tts)
{
	if (NULL == tts) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Unset interrupted cb : Input parameter is null");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	tts_client_s* client = tts_client_get(tts);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Unset interrupted cb : A handle is not valid");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	if (TTS_STATE_READY != client->current_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Unset interrupted cb : Current state is not 'ready'."); 
		return TTS_ERROR_INVALID_STATE;
	}

	client->interrupted_cb = NULL;
	client->interrupted_user_data = NULL;

	SLOG(LOG_DEBUG, TAG_TTSC, "[SUCCESS] Unset interrupted cb");

	return 0;
}

int tts_set_utterance_started_cb(tts_h tts, tts_utterance_started_cb callback, void* user_data)
{
	if (NULL == tts || NULL == callback) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Input parameter is null");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	tts_client_s* client = tts_client_get(tts);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] A handle is not valid");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	if (TTS_STATE_READY != client->current_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Set utt started cb : Current state is not 'ready'."); 
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

	if (TTS_STATE_READY != client->current_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Unset utt started cb : Current state is not 'ready'."); 
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

	if (TTS_STATE_READY != client->current_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Set utt completed cb : Current state is not 'ready'."); 
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

	if (TTS_STATE_READY != client->current_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Unset utt completed cb : Current state is not 'ready'."); 
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

	if (TTS_STATE_READY != client->current_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Set error cb : Current state is not 'ready'."); 
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
	
	if (TTS_STATE_READY != client->current_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Unset error cb : Current state is not 'ready'."); 
		return TTS_ERROR_INVALID_STATE;
	}

	client->error_cb = NULL;
	client->error_user_data = NULL;

	SLOG(LOG_DEBUG, TAG_TTSC, "[SUCCESS] Unset error cb");

	return 0;
}

static bool _tts_is_alive()
{
	FILE *fp = NULL;
	char buff[256];
	char cmd[256];

	memset(buff, '\0', sizeof(char) * 256);
	memset(cmd, '\0', sizeof(char) * 256);

	if ((fp = popen("ps -eo \"cmd\"", "r")) == NULL) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] popen error \n");
		return FALSE;
	}

	while(fgets(buff, 255, fp)) {
		sscanf(buff, "%s", cmd);

		if (0 == strncmp(cmd, "[tts-daemon]", strlen("[tts-daemon]")) ||
			0 == strncmp(cmd, "tts-daemon", strlen("tts-daemon")) ||
			0 == strncmp(cmd, "/usr/bin/tts-daemon", strlen("/usr/bin/tts-daemon"))) {
			SLOG(LOG_DEBUG, TAG_TTSC, "tts-daemon is ALIVE !! \n");
			fclose(fp);
			return TRUE;
		}
	}

	fclose(fp);

	SLOG(LOG_DEBUG, TAG_TTSC, "THERE IS NO tts-daemon !! \n");

	return FALSE;
}


static void __my_sig_child(int signo, siginfo_t *info, void *data)
{
	int status;
	pid_t child_pid, child_pgid;

	child_pgid = getpgid(info->si_pid);
	SLOG(LOG_DEBUG, TAG_TTSC, "Signal handler: dead pid = %d, pgid = %d\n", info->si_pid, child_pgid);

	while (0 < (child_pid = waitpid(-1, &status, WNOHANG))) {
		if(child_pid == child_pgid)
			killpg(child_pgid, SIGKILL);
	}

	return;
}

int __tts_check_tts_daemon()
{
	if (TRUE == _tts_is_alive())
		return 0;
	
	/* fork-exec tts-daemom */
	int pid, i;
	struct sigaction act, dummy;

	act.sa_handler = NULL;
	act.sa_sigaction = __my_sig_child;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_NOCLDSTOP | SA_SIGINFO;
	
	if (sigaction(SIGCHLD, &act, &dummy) < 0) {
		SLOG(LOG_ERROR, TAG_TTSC, "Cannot make a signal handler");
		return -1;
	}

	pid = fork();

	switch(pid) {
	case -1:
		SLOG(LOG_ERROR, TAG_TTSC, "fail to create TTS-DAEMON \n");
		break;

	case 0:
		setsid();
		for (i = 0;i < _NSIG;i++)
			signal(i, SIG_DFL);

		execl("/usr/bin/tts-daemon", "/usr/bin/tts-daemon", NULL);
		break;

	default:
		sleep(1);
		break;
	}

	return 0;
}
