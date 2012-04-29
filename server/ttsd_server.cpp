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


#include "ttsd_main.h"
#include "ttsd_player.h"
#include "ttsd_data.h"
#include "ttsd_engine_agent.h"
#include "ttsd_server.h"
#include "ttsd_dbus_server.h"
#include "ttsp.h"
#include "ttsd_dbus.h"
#include "ttsd_config.h"
#include "ttsd_network.h"


typedef struct {
	int uid;
	int uttid;
} utterance_t;

/* If current engine exist */
static bool g_is_engine;

/* If engine is running */
static bool g_is_synthesizing;

int __server_set_is_synthesizing(bool flag)
{
	g_is_synthesizing = flag;
	return 0;
}

bool __server_get_current_synthesis()
{
	return g_is_synthesizing;
}

int __server_send_error(int uid, int utt_id, int error_code)
{
	int pid = ttsd_data_get_pid(uid);

	/* send error */
	if ( 0 != ttsdc_send_error_message(pid, uid, utt_id, error_code)) {
		ttsd_data_delete_client(uid);			
	} 
	
	return 0;
}

int __server_interrupt_client(int org_uid)
{
	int pid = ttsd_data_get_pid(org_uid);

	/* pause player */
	if (0 != ttsd_player_pause(org_uid)) {
		SLOG(LOG_WARN, TAG_TTSD, "[Server ERROR] fail to ttsd_player_pause() : uid (%d)\n", org_uid);
	} 

	/* send message to client about changing state */
	ttsdc_send_interrupt_message (pid, org_uid, TTSD_INTERRUPTED_PAUSED);

	/* change state */
	ttsd_data_set_client_state(org_uid, APP_STATE_PAUSED);

	return 0;
}

int __server_start_synthesis(int uid)
{
	int result = 0;

	/* check if tts-engine is running */
	if (true == __server_get_current_synthesis()) {
		SLOG(LOG_DEBUG, TAG_TTSD, "[Server] TTS-engine is running \n");
	} else {
		speak_data_s sdata;
		if (0 == ttsd_data_get_speak_data(uid, &sdata)) {
			utterance_t* utt = (utterance_t*)g_malloc0(sizeof(utterance_t));

			if (NULL == utt) {
				SLOG(LOG_ERROR, TAG_TTSD, "[Server ERROR] Out of memory : utterance \n");
				return TTSD_ERROR_OUT_OF_MEMORY;
			}

			utt->uid = uid;
			utt->uttid = sdata.utt_id;
			
			SLOG(LOG_DEBUG, TAG_TTSD, "-----------------------------------------------------------");
			SLOG(LOG_DEBUG, TAG_TTSD, "ID : uid (%d), uttid(%d) ", utt->uid, utt->uttid );
			SLOG(LOG_DEBUG, TAG_TTSD, "Voice : langauge(%s), type(%d), speed(%d)", sdata.lang, sdata.vctype, sdata.speed);
			SLOG(LOG_DEBUG, TAG_TTSD, "Text : %s", sdata.text);
			SLOG(LOG_DEBUG, TAG_TTSD, "-----------------------------------------------------------");

			__server_set_is_synthesizing(true);
			int ret = 0;
			ret = ttsd_engine_start_synthesis( sdata.lang, sdata.vctype, sdata.text, sdata.speed, (void*)utt);
			if (0 != ret) {
				SLOG(LOG_ERROR, TAG_TTSD, "[Server ERROR] * FAIL to start SYNTHESIS !!!! * ");

				__server_set_is_synthesizing(false);

				result = TTSD_ERROR_OPERATION_FAILED;

				g_free(utt);
			} else {
				SLOG(LOG_DEBUG, TAG_TTSD, "[Server] SUCCESS to start synthesis");
			}

			if(sdata.text != NULL)	
				g_free(sdata.text);

		} else {
			SLOG(LOG_DEBUG, TAG_TTSD, "[Server] Text List is EMPTY!! ");
		}
	}

	return result;
}

int __server_play_internal(int uid, app_state_e state)
{
	/* precondition			*/
	/* - uid is valid		*/
	/* - input uid is current play	*/

	int ret = 0;

	if (APP_STATE_PAUSED == state) {

		SLOG(LOG_DEBUG, TAG_TTSD, "[Server] uid(%d) is 'Pause' state : Next step is resume player and start synthesis ", uid);

		/* resume player and start speech synthesis */
		if (0 != ttsd_player_resume(uid)) {
			SLOG(LOG_WARN, TAG_TTSD, "[Server WARNING] fail to ttsd_player_resume() \n");
		} 
		
		ret = __server_start_synthesis(uid);

	} else if(APP_STATE_READY == state) {

		SLOG(LOG_DEBUG, TAG_TTSD, "[Server] uid(%d) is 'Ready' state : Next step is start synthesis ", uid);

		ret = __server_start_synthesis(uid);
	} else {
		/* NO this case */
	}

	return ret;
}

int __server_next_synthesis(int uid)
{
	SLOG(LOG_DEBUG, TAG_TTSD, "===== START NEXT SYNTHESIS & PLAY");

	/* get current playing client */
	int current_uid = ttsd_data_get_current_playing();

	if (0 > current_uid) {
		SLOG(LOG_WARN, TAG_TTSD, "[Server WARNING] Current uid is not valid");
		SLOG(LOG_DEBUG, TAG_TTSD, "=====");
		SLOG(LOG_DEBUG, TAG_TTSD, "  ");
		return 0;
	}

	if (true == __server_get_current_synthesis()) {
		SLOG(LOG_WARN, TAG_TTSD, "[Server WARNING] Engine has already been running. \n");
		SLOG(LOG_DEBUG, TAG_TTSD, "=====");
		SLOG(LOG_DEBUG, TAG_TTSD, "  ");
		return 0;
	}

	/* synthesize next text */
	speak_data_s sdata;
	if (0 == ttsd_data_get_speak_data(current_uid, &sdata)) {

		utterance_t* utt = (utterance_t*)g_malloc0(sizeof(utterance_t));

		if (NULL == utt) {
			SLOG(LOG_ERROR, TAG_TTSD, "[Server ERROR] fail to allocate memory : utterance \n");

			__server_send_error(current_uid, sdata.utt_id, TTSD_ERROR_OUT_OF_MEMORY);
			return TTSD_ERROR_OUT_OF_MEMORY;
		}

		utt->uid = current_uid;
		utt->uttid = sdata.utt_id;

		SLOG(LOG_DEBUG, TAG_TTSD, "-----------------------------------------------------------");
		SLOG(LOG_DEBUG, TAG_TTSD, "ID : uid (%d), uttid(%d) ", utt->uid, utt->uttid );
		SLOG(LOG_DEBUG, TAG_TTSD, "Voice : langauge(%s), type(%d), speed(%d)", sdata.lang, sdata.vctype, sdata.speed);
		SLOG(LOG_DEBUG, TAG_TTSD, "Text : %s", sdata.text);
		SLOG(LOG_DEBUG, TAG_TTSD, "-----------------------------------------------------------");

		__server_set_is_synthesizing(true);

		int ret = 0;
		ret = ttsd_engine_start_synthesis(sdata.lang, sdata.vctype, sdata.text, sdata.speed, (void*)utt);
		if (0 != ret) {
			SLOG(LOG_ERROR, TAG_TTSD, "[Server ERROR] * FAIL to start SYNTHESIS !!!! * ");

			__server_set_is_synthesizing(false);

			__server_send_error(current_uid, sdata.utt_id, TTSD_ERROR_OPERATION_FAILED);

			g_free(utt);
		}

		if(sdata.text != NULL)	
			g_free(sdata.text);
	}

	if (0 != ttsd_player_play(current_uid)) {
		SLOG(LOG_WARN, TAG_TTSD, "[Server WARNING] __synthesis_result_callback : fail ttsd_player_play() \n");
	} else {
		/* success playing */
		SLOG(LOG_DEBUG, TAG_TTSD, "[Server] Success to start player");
	}

	SLOG(LOG_DEBUG, TAG_TTSD, "=====");
	SLOG(LOG_DEBUG, TAG_TTSD, "  ");

	return 0;
}

/*
* TTS Server Callback Functions	
*/

int __player_result_callback(player_event_e event, int uid, int utt_id)
{
	switch(event) {
	case PLAYER_EMPTY_SOUND_QUEUE:
		/* check whether synthesis is running */
		if (false == __server_get_current_synthesis()) {
			/* check text queue is empty */
			if (0 == ttsd_data_get_speak_data_size(uid) && 0 == ttsd_data_get_sound_data_size(uid)) {
				SLOG(LOG_DEBUG, TAG_TTSD, "[SERVER Callback] all play completed \n");
			}
		} 
		break;

	case PLAYER_ERROR:
		SLOG(LOG_ERROR, TAG_TTSD, "[SERVER Callback ERROR] callback : player error \n");

		__server_send_error(uid, utt_id, TTSD_ERROR_OPERATION_FAILED);
		break;
	
	case PLAYER_END_OF_PLAYING:
		break;
	}

	return 0;
}

int __synthesis_result_callback(ttsp_result_event_e event, const void* data, unsigned int data_size, void *user_data)
{
	SLOG(LOG_DEBUG, TAG_TTSD, "===== SYNTHESIS RESULT CALLBACK");

	utterance_t* utt_get_param;
	utt_get_param = (utterance_t*)user_data;

	int uid = utt_get_param->uid;
	int uttid = utt_get_param->uttid;

	if (NULL == utt_get_param) {
		SLOG(LOG_ERROR, TAG_TTSD, "[SERVER ERROR] User data is NULL \n" );
		SLOG(LOG_DEBUG, TAG_TTSD, "=====");
		SLOG(LOG_DEBUG, TAG_TTSD, "  ");
		return -1;
	}

	/* Synthesis is success */
	if (TTSP_RESULT_EVENT_START == event || TTSP_RESULT_EVENT_CONTINUE == event || TTSP_RESULT_EVENT_FINISH == event) {
		
		if (TTSP_RESULT_EVENT_START == event)		SLOG(LOG_DEBUG, TAG_TTSD, "[SERVER] Event : TTSP_RESULT_EVENT_START");
		if (TTSP_RESULT_EVENT_CONTINUE == event)	SLOG(LOG_DEBUG, TAG_TTSD, "[SERVER] Event : TTSP_RESULT_EVENT_CONTINUE");
		if (TTSP_RESULT_EVENT_FINISH == event)		SLOG(LOG_DEBUG, TAG_TTSD, "[SERVER] Event : TTSP_RESULT_EVENT_FINISH");

		if (false == ttsd_data_is_uttid_valid(uid, uttid)) {
			SLOG(LOG_ERROR, TAG_TTSD, "[SERVER ERROR] uttid is NOT valid !!!! \n" );
			SLOG(LOG_DEBUG, TAG_TTSD, "=====");
			SLOG(LOG_DEBUG, TAG_TTSD, "  ");

			return 0;
		}


		SLOG(LOG_DEBUG, TAG_TTSD, "[SERVER] Result Info : uid(%d), utt(%d), data(%p), data size(%d) \n", 
			uid, uttid, data, data_size);

		/* add wav data */
		sound_data_s temp_data;
		temp_data.data = (char*)g_malloc0( sizeof(char) * data_size );
		memcpy(temp_data.data, data, data_size);

		temp_data.data_size = data_size;
		temp_data.utt_id = utt_get_param->uttid;
		temp_data.event = event;

		ttsp_audio_type_e audio_type;
		int rate;
		int channels;

		if (ttsd_engine_get_audio_format(&audio_type, &rate, &channels)) {
			SLOG(LOG_ERROR, TAG_TTSD, "[Server ERROR] Fail to get audio format ");
			SLOG(LOG_DEBUG, TAG_TTSD, "=====");
			SLOG(LOG_DEBUG, TAG_TTSD, "  ");
			return -1;
		}
		
		temp_data.audio_type = audio_type;
		temp_data.rate = rate;
		temp_data.channels = channels;
		
		if (0 != ttsd_data_add_sound_data(uid, temp_data)) {
			SLOG(LOG_ERROR, TAG_TTSD, "[SERVER ERROR] Fail to add sound data : uid(%d)\n", utt_get_param->uid);
		}

		if (event == TTSP_RESULT_EVENT_FINISH) {
			__server_set_is_synthesizing(false);
			
			if (0 != ttsd_send_start_next_synthesis_message(uid)) {
				/* critical error */
				SLOG(LOG_ERROR, TAG_TTSD, "[SERVER ERROR] IPC ERROR FOR NEXT SYNTHESIS \n");
			}
		}
	} 
	
	else if (event == TTSP_RESULT_EVENT_CANCEL) {
		SLOG(LOG_DEBUG, TAG_TTSD, "[SERVER] Event : TTSP_RESULT_EVENT_CANCEL");
		__server_set_is_synthesizing(false);

		if (0 != ttsd_send_start_next_synthesis_message(uid)) {
			/* critical error */
			SLOG(LOG_ERROR, TAG_TTSD, "[SERVER ERROR] IPC ERROR FOR NEXT SYNTHESIS \n");
		}
	} 
	
	else {
		SLOG(LOG_DEBUG, TAG_TTSD, "[SERVER] Event : TTSP_RESULT_EVENT_CANCEL");
		
		__server_set_is_synthesizing(false);
		if (0 != ttsd_send_start_next_synthesis_message(uid)) {
			/* critical error */
			SLOG(LOG_ERROR, TAG_TTSD, "[SERVER ERROR] IPC ERROR FOR NEXT SYNTHESIS \n");
		}
	} 

	if (TTSP_RESULT_EVENT_FINISH == event || TTSP_RESULT_EVENT_CANCEL == event || TTSP_RESULT_EVENT_FAIL == event) {
		if (NULL != utt_get_param)		
			free(utt_get_param);
	}

	SLOG(LOG_DEBUG, TAG_TTSD, "=====");
	SLOG(LOG_DEBUG, TAG_TTSD, "  ");

	return 0;
}

/*
* Daemon init
*/

int ttsd_initialize()
{
	/* player init */
	if (ttsd_player_init(__player_result_callback)) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Server ERROR] Fail to initialize player init \n");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	/* Engine Agent initialize */
	if (0 != ttsd_engine_agent_init(__synthesis_result_callback)) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Server ERROR] Fail to engine agent initialize \n");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	/* set current engine */
	if (0 != ttsd_engine_agent_initialize_current_engine()) {
		SLOG(LOG_WARN, TAG_TTSD, "[Server WARNING] No Engine !!! \n" );
		g_is_engine = false;
	} else 
		g_is_engine = true;

	return TTSD_ERROR_NONE;
}


/*
* TTS Server Functions for Client
*/

int ttsd_server_initialize(int pid, int uid)
{
	if (false == g_is_engine) {
		if (0 != ttsd_engine_agent_initialize_current_engine()) {
			SLOG(LOG_WARN, TAG_TTSD, "[Server WARNING] No Engine !!! \n" );
			g_is_engine = false;

			return TTSD_ERROR_ENGINE_NOT_FOUND;
		} else {
			g_is_engine = true;
		}
	}

	if (-1 != ttsd_data_is_client(uid)) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Server ERROR] Uid has already been registered \n");
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	if (0 == ttsd_data_get_client_count()) {
		if (0 != ttsd_engine_agent_load_current_engine()) {
			SLOG(LOG_ERROR, TAG_TTSD, "[Server ERROR] Fail to load current engine \n");
			return TTSD_ERROR_OPERATION_FAILED;
		}
	}

	if (0 != ttsd_data_new_client(pid, uid)) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Server ERROR] Fail to add client info \n");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (0 != ttsd_player_create_instance(uid)) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Server ERROR] Fail to create player \n");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	return TTSD_ERROR_NONE;
}


int ttsd_server_finalize(int uid)
{
	app_state_e state;
	if (0 > ttsd_data_get_client_state(uid, &state)) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Server ERROR] ttsd_server_finalize : uid is not valid  \n");
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	ttsd_server_stop(uid);
	
	ttsd_player_destroy_instance(uid);

	ttsd_data_delete_client(uid);

	/* unload engine, if ref count of client is 0 */
	if (0 == ttsd_data_get_client_count()) {
		if (0 != ttsd_engine_agent_unload_current_engine()) {
			SLOG(LOG_ERROR, TAG_TTSD, "[Server ERROR] fail to unload current engine \n");
		} else {
			SLOG(LOG_DEBUG, TAG_TTSD, "[Server SUCCESS] unload current engine \n");
		}
	}

	return TTSD_ERROR_NONE;
}

int ttsd_server_add_queue(int uid, const char* text, const char* lang, int voice_type, int speed, int utt_id)
{
	app_state_e state;
	if (0 > ttsd_data_get_client_state(uid, &state)) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Server ERROR] ttsd_server_add_queue : uid is not valid  \n");
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	/* check valid voice */
	char* temp_lang = NULL;
	ttsp_voice_type_e temp_type;
	if (true != ttsd_engine_select_valid_voice((const char*)lang, (const ttsp_voice_type_e)voice_type, &temp_lang, &temp_type)) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Server ERROR] Fail to select valid voice \n");
		return TTSD_ERROR_INVALID_VOICE;
	} else {		
		if (NULL == temp_lang)
			free(temp_lang);
	}
	
	speak_data_s data;

	data.lang = strdup(lang);
	data.vctype = (ttsp_voice_type_e)voice_type;

	data.speed = (ttsp_speed_e)speed;
	data.utt_id = utt_id;
		
	data.text = strdup(text);

	/* if state is APP_STATE_READY , APP_STATE_PAUSED , only need to add speak data to queue*/
	if (0 != ttsd_data_add_speak_data(uid, data)) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Server ERROR] ttsd_server_add_queue : Current state of uid is not 'ready' \n");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (APP_STATE_PLAYING == state) {
		/* check if engine use network */
		if (ttsd_engine_agent_need_network()) {
			if (false == ttsd_network_is_connected()) {
				SLOG(LOG_ERROR, TAG_TTSD, "[Server ERROR] Disconnect network. Current engine needs network.\n");
				return TTSD_ERROR_OPERATION_FAILED;
			}
		}

		if (0 != __server_start_synthesis(uid)) {
			SLOG(LOG_ERROR, TAG_TTSD, "[Server ERROR] fail to schedule synthesis : uid(%d)\n", uid);
			return TTSD_ERROR_OPERATION_FAILED;
		}
	}

	return TTSD_ERROR_NONE;
}

int ttsd_server_play(int uid)
{
	app_state_e state;
	if (0 > ttsd_data_get_client_state(uid, &state)) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Server ERROR] uid(%d) is NOT valid  \n", uid);
		return TTSD_ERROR_INVALID_PARAMETER;
	}
	
	if (APP_STATE_PLAYING == state) {
		SLOG(LOG_WARN, TAG_TTSD, "[Server WARNING] Current state(%d) is 'play' \n", uid);
		return TTSD_ERROR_NONE;
	}

	/* check if engine use network */
	if (ttsd_engine_agent_need_network()) {
		if (false == ttsd_network_is_connected()) {
			SLOG(LOG_ERROR, TAG_TTSD, "[Server ERROR] Disconnect network. Current engine needs network service!!!.\n");
			return TTSD_ERROR_OUT_OF_NETWORK;
		}
	}

	int current_uid = ttsd_data_get_current_playing();

	if (uid != current_uid && -1 != current_uid) {
		/* Send interrupt message */
		SLOG(LOG_DEBUG, TAG_TTSD, "[Server] Old uid(%d) will be interrupted into 'Pause' state \n", current_uid);
		__server_interrupt_client(current_uid);
	}
	
	/* Change current play */
	if (0 != ttsd_data_set_client_state(uid, APP_STATE_PLAYING)) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Server ERROR] Fail to set state : uid(%d)\n", uid);
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (0 != __server_play_internal(uid, state)) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Server ERROR] Fail to start synthesis : uid(%d)\n", uid);
		return TTSD_ERROR_OPERATION_FAILED;
	}

	return TTSD_ERROR_NONE;
}


int ttsd_server_stop(int uid)
{
	app_state_e state;
	if (0 > ttsd_data_get_client_state(uid, &state)) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Server ERROR] uid is not valid  \n");
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	/* Reset all data */
	ttsd_data_clear_data(uid);

	if (APP_STATE_PLAYING == state || APP_STATE_PAUSED == state) {
		ttsd_data_set_client_state(uid, APP_STATE_READY);

		if (0 != ttsd_player_stop(uid)) 
			SLOG(LOG_WARN, TAG_TTSD, "[Server] Fail to ttsd_player_stop()\n");

		if (true == __server_get_current_synthesis()) {
			SLOG(LOG_DEBUG, TAG_TTSD, "[Server] TTS-engine is running \n");

			int ret = 0;
			ret = ttsd_engine_cancel_synthesis();
			if (0 != ret)
				SLOG(LOG_ERROR, TAG_TTSD, "[Server ERROR] Fail to cancel synthesis : ret(%d)", ret);

			__server_set_is_synthesizing(false);
		} 
	} else {
		SLOG(LOG_WARN, TAG_TTSD, "[Server WARNING] Current state is 'ready' \n");
	}

	return TTSD_ERROR_NONE;
}

int ttsd_server_pause(int uid, int* utt_id)
{
	app_state_e state;
	if (0 > ttsd_data_get_client_state(uid, &state)) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Server ERROR] ttsd_server_pause : uid is not valid  \n");
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	if (APP_STATE_PLAYING != state)	{
		SLOG(LOG_WARN, TAG_TTSD, "[Server WARNING] Current state is not 'play' \n");
		return TTSD_ERROR_INVALID_STATE;
	}

	int ret = 0;
	ret = ttsd_player_pause(uid);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Server ERROR] fail player_pause() : ret(%d)\n", ret);
		return TTSD_ERROR_OPERATION_FAILED;
	}

	ttsd_data_set_client_state(uid, APP_STATE_PAUSED);

	return TTSD_ERROR_NONE;
}

int ttsd_server_get_support_voices(int uid, GList** voice_list)
{
	app_state_e state;
	if (0 > ttsd_data_get_client_state(uid, &state)) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Server ERROR] uid is not valid  \n");
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	/* get voice list*/
	if (0 != ttsd_engine_get_voice_list(voice_list)) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Server ERROR] Fail ttsd_server_get_support_voices() \n");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	SLOG(LOG_DEBUG, TAG_TTSD, "[Server SUCCESS] Get supported voices \n");

	return TTSD_ERROR_NONE;
}

int ttsd_server_get_current_voice(int uid, char** language, int* voice_type)
{
	app_state_e state;
	if (0 > ttsd_data_get_client_state(uid, &state)) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Server ERROR] ttsd_server_get_current_voice : uid is not valid  \n");
		return TTSD_ERROR_INVALID_PARAMETER;
	}		

	/* get current voice */
	int ret = ttsd_engine_get_default_voice(language, (ttsp_voice_type_e*)voice_type);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Server ERROR] fail ttsd_server_get_support_voices() \n");
		return ret;
	}

	SLOG(LOG_DEBUG, TAG_TTSD, "[Server] Get default language (%s), voice type(%d) \n", *language, *voice_type); 

	return TTSD_ERROR_NONE;
}


/*
* TTS Server Functions for Setting														  *
*/

int ttsd_server_setting_initialize(int uid)
{
	if (false == g_is_engine) {
		if (0 != ttsd_engine_agent_initialize_current_engine()) {
			SLOG(LOG_WARN, TAG_TTSD, "[Server Setting WARNING] No Engine !!! \n" );
			g_is_engine = false;
			return TTSD_ERROR_ENGINE_NOT_FOUND;
		} else {
			g_is_engine = true;
		}
	}

	if (-1 != ttsd_data_is_client(uid)) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Server Setting ERROR] pid has already been registered \n");
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	if (0 == ttsd_data_get_client_count()) {
		if( 0 != ttsd_engine_agent_load_current_engine() ) {
			SLOG(LOG_ERROR, TAG_TTSD, "[Server Setting ERROR] Fail to load current engine \n");
			return TTSD_ERROR_OPERATION_FAILED;
		}
	}

	/* register pid */
	if (0 != ttsd_data_new_client(uid, uid)) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Server Setting ERROR] Fail to add client info \n");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	return TTSD_ERROR_NONE;
}

int ttsd_server_setting_finalize(int uid)
{
	app_state_e state;
	if (0 > ttsd_data_get_client_state(uid, &state)) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Server Setting ERROR] uid is not valid  (%s)\n", uid);
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	ttsd_data_delete_client(uid);

	/* unload engine, if ref count of client is 0 */
	if (0 == ttsd_data_get_client_count())
	{
		if (0 != ttsd_engine_agent_unload_current_engine()) 
			SLOG(LOG_ERROR, TAG_TTSD, "[Server Setting ERROR] fail to unload current engine \n");
		else
			SLOG(LOG_DEBUG, TAG_TTSD, "[Server Setting SUCCESS] unload current engine \n");
	}

	return TTSD_ERROR_NONE;
}

int ttsd_server_setting_get_engine_list(int uid, GList** engine_list)
{
	app_state_e state;
	if (0 > ttsd_data_get_client_state(uid, &state)) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Server Setting ERROR] uid is not valid  \n");
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	int ret = 0;
	ret = ttsd_engine_setting_get_engine_list(engine_list);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Server Setting ERROR] fail to get engine list : result(%d)\n", ret);
		return ret;
	}

	return TTSD_ERROR_NONE;
}

int ttsd_server_setting_get_current_engine(int uid, char** engine_id)
{
	app_state_e state;
	if (0 > ttsd_data_get_client_state(uid, &state)) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Server Setting ERROR] uid is not valid  \n");
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	int ret = 0;
	ret = ttsd_engine_setting_get_engine(engine_id);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Server Setting ERROR] Fail to get current engine : result(%d) \n", ret);
		return ret;
	}


	return TTSD_ERROR_NONE;
}

bool __get_client_cb(int pid, int uid, app_state_e state, void* user_data)
{
	/* clear client data */
	ttsd_data_clear_data(uid);			
	ttsd_data_set_client_state(uid, APP_STATE_READY);

	/* send message */
	if ( 0 != ttsdc_send_interrupt_message(pid, uid, TTSD_INTERRUPTED_STOPPED)) {
		/* remove client */
		ttsd_data_delete_client(uid);
	} 

	return true;
}

int ttsd_server_setting_set_current_engine(int uid, const char* engine_id)
{
	/* check if pid is valid */
	app_state_e state;
	if (0 > ttsd_data_get_client_state(uid, &state)) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Server Setting ERROR] uid is not valid  \n");
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	if (true == ttsd_engine_agent_is_same_engine(engine_id)) {
		SLOG(LOG_DEBUG, TAG_TTSD, "[Server Setting] new engine is the same as current engine \n");
		return TTSD_ERROR_NONE;
	}

	/* stop all player */ 
	ttsd_player_all_stop();

	/* send interrupt message to  all clients */
	ttsd_data_foreach_clients(__get_client_cb, NULL);

	/* set engine */
	int ret = 0;
	ret = ttsd_engine_setting_set_engine(engine_id);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Server Setting ERROR] fail to set current engine : result(%d) \n", ret);
		return ret;
	}

	return TTSD_ERROR_NONE;
}

int ttsd_server_setting_get_voice_list(int uid, char** engine_id, GList** voice_list)
{
	app_state_e state;
	if (0 > ttsd_data_get_client_state(uid, &state)) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Server Setting ERROR] uid is not valid  \n");
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	/* get language list from engine */
	int ret = 0;
	ret = ttsd_engine_setting_get_voice_list(engine_id, voice_list);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Server Setting ERROR] Fail to get voice list : result(%d)\n", ret);
		return ret;
	}

	return TTSD_ERROR_NONE;
}

int ttsd_server_setting_get_default_voice(int uid, char** language, ttsp_voice_type_e* vctype)
{
	app_state_e state;
	if (0 > ttsd_data_get_client_state(uid, &state)) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Server Setting ERROR] uid is not valid  \n");
		return TTSD_ERROR_INVALID_PARAMETER;
	}
	
	int ret = 0;
	ret = ttsd_engine_setting_get_default_voice(language, vctype);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Server Setting ERROR] Fail to get default voice : result(%d) \n", ret);
		return ret;
	}

	return TTSD_ERROR_NONE;
}

int ttsd_server_setting_set_default_voice(int uid, const char* language, int vctype)
{
	app_state_e state;
	if (0 > ttsd_data_get_client_state(uid, &state)) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Server Setting ERROR] uid is not valid  \n");
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	/* set current language */
	int ret = 0;
	ret = ttsd_engine_setting_set_default_voice((const char*)language, (const ttsp_voice_type_e)vctype);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Server Setting ERROR] fail to set default voice : result(%d) \n", ret);
		return ret;
	}	

	return TTSD_ERROR_NONE;
}

int ttsd_server_setting_get_engine_setting(int uid, char** engine_id, GList** engine_setting_list)
{
	app_state_e state;
	if (0 > ttsd_data_get_client_state(uid, &state)) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Server Setting ERROR] uid is not valid  \n");
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	int ret = 0;
	ret = ttsd_engine_setting_get_engine_setting_info(engine_id, engine_setting_list);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Server Setting ERROR] fail to get engine setting info : result(%d)\n", ret);
		return ret;
	}

	return TTSD_ERROR_NONE;
}

int ttsd_server_setting_set_engine_setting(int uid, const char* key, const char* value)
{
	app_state_e state;
	if (0 > ttsd_data_get_client_state(uid, &state)) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Server Setting ERROR] uid is not valid  \n");
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	int ret = 0;
	ret = ttsd_engine_setting_set_engine_setting(key, value);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Server Setting ERROR] fail to set engine setting info : result(%d)\n", ret);
		return ret;
	}

	return TTSD_ERROR_NONE;
}

int ttsd_server_setting_get_default_speed(int uid, int* default_speed)
{
	app_state_e state;
	if (0 > ttsd_data_get_client_state(uid, &state)) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Server Setting ERROR] uid is not valid  \n");
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	/* get current speed */
	int ret = 0;
	ret = ttsd_engine_setting_get_default_speed((ttsp_speed_e*)default_speed);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Server Setting ERROR] fail to get default speed : result(%d)\n", ret);
		return ret;
	}	

	return TTSD_ERROR_NONE;
}

int ttsd_server_setting_set_default_speed(int uid, int default_speed)
{
	app_state_e state;
	if (0 > ttsd_data_get_client_state(uid, &state)) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Server Setting ERROR] uid is not valid  \n");
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	/* set default speed */
	int ret = 0;
	ret = ttsd_engine_setting_set_default_speed((ttsp_speed_e)default_speed);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Server Setting ERROR] fail to set default speed : result(%d)\n", ret);
		return ret;
	}	

	return TTSD_ERROR_NONE;
}

/*
* Server API for Internal event
*/

int ttsd_server_start_next_play(int uid)
{
	SLOG(LOG_DEBUG, TAG_TTSD, "===== NEXT PLAY START");
	
	int ret = ttsd_player_next_play(uid);

	SLOG(LOG_DEBUG, TAG_TTSD, "===== ");
	SLOG(LOG_DEBUG, TAG_TTSD, " ");

	return ret ;
}

int ttsd_server_start_next_synthesis(int uid)
{
	return __server_next_synthesis(uid);
}


