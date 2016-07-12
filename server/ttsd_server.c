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

#include <aul.h>
#include <Ecore.h>

#include "ttsd_config.h"
#include "ttsd_data.h"
#include "ttsd_dbus.h"
#include "ttsd_dbus_server.h"
#include "ttsd_engine_agent.h"
#include "ttsd_main.h"
#include "ttsd_network.h"
#include "ttsd_player.h"
#include "ttsd_server.h"


typedef enum {
	TTSD_SYNTHESIS_CONTROL_DOING	= 0,
	TTSD_SYNTHESIS_CONTROL_DONE	= 1,
	TTSD_SYNTHESIS_CONTROL_EXPIRED	= 2
} ttsd_synthesis_control_e;

typedef struct {
	int uid;
	int uttid;
} utterance_t;

/* If current engine exist */
//static bool	g_is_engine;

/* If engine is running */
static ttsd_synthesis_control_e	g_synth_control;

static Ecore_Timer* g_wait_timer = NULL;

static utterance_t g_utt;

static GList *g_proc_list = NULL;

/* Function definitions */
static int __synthesis(int uid, const char* credential);

static int __server_set_synth_control(ttsd_synthesis_control_e control)
{
	g_synth_control = control;
	return 0;
}

static ttsd_synthesis_control_e __server_get_synth_control()
{
	return g_synth_control;
}

static Eina_Bool __wait_synthesis(void *data)
{
	/* get current play */
	char* credential = (char*)data;
	int uid = ttsd_data_get_current_playing();

	if (uid > 0) {
		if (TTSD_SYNTHESIS_CONTROL_DOING == __server_get_synth_control()) {
			usleep(100000);
			return EINA_TRUE;
		} else {
			g_wait_timer = NULL;
			if (TTSD_SYNTHESIS_CONTROL_DONE == __server_get_synth_control()) {
				/* Start next synthesis */
				__synthesis(uid, credential);
			}
		}
	} else {
		g_wait_timer = NULL;
	}

	return EINA_FALSE;
}

static int __synthesis(int uid, const char* credential)
{
	SLOG(LOG_DEBUG, tts_tag(), "===== SYNTHESIS  START");

	speak_data_s* speak_data = NULL;
	if (0 == ttsd_data_get_speak_data(uid, &speak_data)) {
		if (NULL == speak_data) {
			return 0;
		}

		int pid = ttsd_data_get_pid(uid);
		char appid[128] = {0, };
		if (0 != aul_app_get_appid_bypid(pid, appid, sizeof(appid))) {
			SLOG(LOG_ERROR, tts_tag(), "[Server ERROR] Fail to get app id");
		}

		if (NULL == speak_data->lang || NULL == speak_data->text) {
			SLOG(LOG_ERROR, tts_tag(), "[Server ERROR] Current data is NOT valid");
			ttsd_server_stop(uid);

			ttsdc_send_set_state_message(pid, uid, APP_STATE_READY);

			if (NULL != speak_data) {
				if (NULL != speak_data->lang)	free(speak_data->lang);
				if (NULL != speak_data->text)	free(speak_data->text);

				speak_data->lang = NULL;
				speak_data->text = NULL;

				free(speak_data);
				speak_data = NULL;
			}

			return 0;
		}

		g_utt.uid = uid;
		g_utt.uttid = speak_data->utt_id;

		SLOG(LOG_DEBUG, tts_tag(), "-----------------------------------------------------------");
		SECURE_SLOG(LOG_DEBUG, tts_tag(), "ID : uid (%d), uttid(%d) ", g_utt.uid, g_utt.uttid);
		SECURE_SLOG(LOG_DEBUG, tts_tag(), "Voice : langauge(%s), type(%d), speed(%d)", speak_data->lang, speak_data->vctype, speak_data->speed);
		SECURE_SLOG(LOG_DEBUG, tts_tag(), "Text : %s", speak_data->text);
		SECURE_SLOG(LOG_DEBUG, tts_tag(), "Credential : %s", credential);
		SLOG(LOG_DEBUG, tts_tag(), "-----------------------------------------------------------");

		int ret = 0;
		__server_set_synth_control(TTSD_SYNTHESIS_CONTROL_DOING);
		ret = ttsd_engine_start_synthesis(speak_data->lang, speak_data->vctype, speak_data->text, speak_data->speed, appid, credential, NULL);
		if (0 != ret) {
			SLOG(LOG_ERROR, tts_tag(), "[Server ERROR] * FAIL to start SYNTHESIS !!!! * ");

			__server_set_synth_control(TTSD_SYNTHESIS_CONTROL_DONE);

			ttsd_server_stop(uid);

			int pid = ttsd_data_get_pid(uid);
			ttsdc_send_set_state_message(pid, uid, APP_STATE_READY);
		} else {
			g_wait_timer = ecore_timer_add(0, __wait_synthesis, (void*)credential);
		}

		if (NULL != speak_data) {
			if (NULL != speak_data->lang)	free(speak_data->lang);
			if (NULL != speak_data->text)	free(speak_data->text);

			speak_data->lang = NULL;
			speak_data->text = NULL;

			free(speak_data);
			speak_data = NULL;
		}
	}

	SLOG(LOG_DEBUG, tts_tag(), "===== SYNTHESIS  END");
	SLOG(LOG_DEBUG, tts_tag(), "  ");

	return 0;
}

/*
* TTS Server Callback Functions
*/
int ttsd_send_error(ttse_error_e error, const char* msg)
{
	return 0;
}

int ttsd_send_result(ttse_result_event_e event, const void* data, unsigned int data_size, ttse_audio_type_e audio_type, int rate, void* user_data)

{
	SLOG(LOG_DEBUG, tts_tag(), "===== SEND SYNTHESIS RESULT START");

	int uid = g_utt.uid;
	int uttid = g_utt.uttid;

	/* Synthesis is success */
	if (TTSE_RESULT_EVENT_START == event || TTSE_RESULT_EVENT_CONTINUE == event || TTSE_RESULT_EVENT_FINISH == event) {
		if (TTSE_RESULT_EVENT_START == event) {
			SLOG(LOG_DEBUG, tts_tag(), "[SERVER] Event : TTSE_RESULT_EVENT_START");
			SECURE_SLOG(LOG_DEBUG, tts_tag(), "[SERVER] Result Info : uid(%d), utt(%d), data(%p), data size(%d) audiotype(%d) rate(%d)", 
				uid, uttid, data, data_size, audio_type, rate);
		} else if (TTSE_RESULT_EVENT_FINISH == event) {
			SLOG(LOG_DEBUG, tts_tag(), "[SERVER] Event : TTSE_RESULT_EVENT_FINISH");
			SECURE_SLOG(LOG_DEBUG, tts_tag(), "[SERVER] Result Info : uid(%d), utt(%d), data(%p), data size(%d) audiotype(%d) rate(%d)", 
				uid, uttid, data, data_size, audio_type, rate);
		} else {
			/*if (TTSE_RESULT_EVENT_CONTINUE == event)  SLOG(LOG_DEBUG, tts_tag(), "[SERVER] Event : TTSE_RESULT_EVENT_CONTINUE");*/
		}


		if (false == ttsd_data_is_uttid_valid(uid, uttid)) {
			__server_set_synth_control(TTSD_SYNTHESIS_CONTROL_DONE);
			SLOG(LOG_ERROR, tts_tag(), "[SERVER ERROR] uttid is NOT valid !!!! - uid(%d), uttid(%d)", uid, uttid);
			SLOG(LOG_DEBUG, tts_tag(), "=====");
			SLOG(LOG_DEBUG, tts_tag(), "  ");
			return 0;
		}

		if (rate <= 0 || audio_type < 0 || audio_type > TTSE_AUDIO_TYPE_MAX) {
			__server_set_synth_control(TTSD_SYNTHESIS_CONTROL_DONE);
			SLOG(LOG_ERROR, tts_tag(), "[SERVER ERROR] audio data is invalid");
			SLOG(LOG_DEBUG, tts_tag(), "=====");
			SLOG(LOG_DEBUG, tts_tag(), "  ");
			return 0;
		}

		/* add wav data */
		sound_data_s* temp_sound_data = NULL;
		temp_sound_data = (sound_data_s*)calloc(1, sizeof(sound_data_s));
		if (NULL == temp_sound_data) {
			SLOG(LOG_ERROR, tts_tag(), "[SERVER ERROR] Out of memory");
			return 0;
		}
		
		temp_sound_data->data = NULL;
		temp_sound_data->rate = 0;
		temp_sound_data->data_size = 0;

		if (0 < data_size) {
			temp_sound_data->data = (char*)calloc(data_size + 5, sizeof(char));
			if (NULL != temp_sound_data->data) {
				memcpy(temp_sound_data->data, data, data_size);
				temp_sound_data->data_size = data_size;
				SLOG(LOG_ERROR, tts_tag(), "[DEBUG][free] uid(%d), event(%d) sound_data(%p) data(%p) size(%d)", 
					uid, event, temp_sound_data, temp_sound_data->data, temp_sound_data->data_size);
			} else {
				SLOG(LOG_ERROR, tts_tag(), "Fail to allocate memory");
			}
		} else {
			SLOG(LOG_ERROR, tts_tag(), "Sound data is NULL");
		}

		temp_sound_data->utt_id = uttid;
		temp_sound_data->event = event;
		temp_sound_data->audio_type = audio_type;
		temp_sound_data->rate = rate;

		if (0 != ttsd_data_add_sound_data(uid, temp_sound_data)) {
			SECURE_SLOG(LOG_ERROR, tts_tag(), "[SERVER ERROR] Fail to add sound data : uid(%d)", uid);
		}

		if (event == TTSE_RESULT_EVENT_FINISH) {
			__server_set_synth_control(TTSD_SYNTHESIS_CONTROL_DONE);
		}

		if (0 != ttsd_player_play(uid)) {
			SLOG(LOG_ERROR, tts_tag(), "[Server ERROR] Fail to play sound : uid(%d)", uid);

			/* Change ready state */
			ttsd_server_stop(uid);

			int tmp_pid;
			tmp_pid = ttsd_data_get_pid(uid);
			ttsdc_send_set_state_message(tmp_pid, uid, APP_STATE_READY);
		}
	} else {
		SLOG(LOG_DEBUG, tts_tag(), "[SERVER] Event : TTSE_RESULT_EVENT_ERROR");
		__server_set_synth_control(TTSD_SYNTHESIS_CONTROL_EXPIRED);
	}


	/*SLOG(LOG_DEBUG, tts_tag(), "===== SYNTHESIS RESULT CALLBACK END");
	SLOG(LOG_DEBUG, tts_tag(), "  ");*/

	return 0;
}

bool __get_client_cb(int pid, int uid, app_state_e state, void* user_data)
{
	/* clear client data */
	ttsd_data_clear_data(uid);
	ttsd_data_set_client_state(uid, APP_STATE_READY);

	/* send message */
	if (0 != ttsdc_send_set_state_message(pid, uid, APP_STATE_READY)) {
		/* remove client */
		ttsd_data_delete_client(uid);
	}

	return true;
}

void __config_changed_cb(tts_config_type_e type, const char* str_param, int int_param)
{
	switch (type) {
	case TTS_CONFIG_TYPE_ENGINE:
	{
		/* TODO - Determine the policy when engine process get engine changed cb */
		if (NULL == str_param) {
			SLOG(LOG_ERROR, tts_tag(), "[Server] engine id from config is NULL");
			return;
		}

		int ret = 0;
		if (true == ttsd_engine_agent_is_same_engine(str_param)) {
			SLOG(LOG_DEBUG, tts_tag(), "[Server Setting] new engine is the same as current engine");
			ret = ttsd_engine_agent_unload_current_engine();
			if (0 != ret) {
				SLOG(LOG_ERROR, tts_tag(), "[Server ERROR] Fail to unload current engine : result(%d)", ret);
			}

			ret = ttsd_engine_agent_load_current_engine();
			if (0 != ret) {
				SLOG(LOG_ERROR, tts_tag(), "[Server ERROR] Fail to load current engine : result (%d)", ret);
			}
			return;
		}

		/* stop all player */ 
		ttsd_player_all_stop();

		/* send interrupt message to  all clients */
		ttsd_data_foreach_clients(__get_client_cb, NULL);

		ttsd_engine_cancel_synthesis();

		break;
	}

	case TTS_CONFIG_TYPE_VOICE:
	{
		if (NULL == str_param) {
			SLOG(LOG_ERROR, tts_tag(), "[Server] language from config is NULL");
			return;
		}

		char* out_lang = NULL;
		int out_type;
		int ret = -1;

		if (true == ttsd_engine_select_valid_voice(str_param, int_param, &out_lang, &out_type)) {
			SLOG(LOG_ERROR, tts_tag(), "[Server] valid language : lang(%s), type(%d)", out_lang, out_type);
			ret = ttsd_engine_agent_set_default_voice(out_lang, out_type);
			if (0 != ret)
				SLOG(LOG_ERROR, tts_tag(), "[Server ERROR] Fail to set valid language : lang(%s), type(%d)", out_lang, out_type);
		} else {
			/* Current language is not available */
			SLOG(LOG_WARN, tts_tag(), "[Server WARNING] Fail to set voice : lang(%s), type(%d)", str_param, int_param);
		}
		if (NULL != out_lang)	free(out_lang);
		break;
	}

	case TTS_CONFIG_TYPE_SPEED:
	{
		if (TTS_SPEED_MIN <= int_param && int_param <= TTS_SPEED_MAX) {
			/* set default speed */
			int ret = 0;
			ret = ttsd_engine_agent_set_default_speed(int_param);
			if (0 != ret) {
				SLOG(LOG_ERROR, tts_tag(), "[Server ERROR] Fail to set default speed : result(%d)", ret);
			}	
		}
		break;
	}

	case TTS_CONFIG_TYPE_PITCH:
	{
		if (TTS_PITCH_MIN <= int_param && int_param <= TTS_PITCH_MAX) {
			/* set default speed */
			int ret = 0;
			ret = ttsd_engine_agent_set_default_pitch(int_param);
			if (0 != ret) {
				SLOG(LOG_ERROR, tts_tag(), "[Server ERROR] Fail to set default pitch : result(%d)", ret);
			}	
		}
		break;
	}

	default:
		break;
	}
	
	return;
}

bool __terminate_client(int pid, int uid, app_state_e state, void* user_data)
{
	SLOG(LOG_DEBUG, tts_tag(), "=== Start to terminate client [%d] ===", uid);
	ttsd_server_finalize(uid);
	return true;
}

Eina_Bool ttsd_terminate_daemon(void *data) 
{
	ttsd_data_foreach_clients(__terminate_client, NULL);
	return EINA_FALSE;
}

void __screen_reader_changed_cb(bool value)
{
	if (TTSD_MODE_SCREEN_READER == ttsd_get_mode() && false == value) {
		SLOG(LOG_DEBUG, tts_tag(), "[Server] Screen reader is OFF. Start to terminate tts daemon");
		ecore_timer_add(1, ttsd_terminate_daemon, NULL);
	} else {
		SLOG(LOG_DEBUG, tts_tag(), "[Server] Screen reader is %s", value ? "ON" : "OFF");
	}
	return;
}

/*
* Server APIs
*/
static bool __send_reset_signal(int pid, int uid, app_state_e state, void* user_data)
{
	ttsdc_send_error_message(pid, uid, -1, TTSD_ERROR_SERVICE_RESET, "TTS service reset");
	return true;
}

static void __sig_handler(int signo)
{
	/* restore signal handler */
	signal(signo, SIG_DFL);

	/* Send error signal via foreach clients */
	ttsd_data_foreach_clients(__send_reset_signal, NULL);

	/* invoke signal again */
	raise(signo);
}

static void __register_sig_handler()
{
	signal(SIGSEGV, __sig_handler);
	signal(SIGABRT, __sig_handler);
	signal(SIGTERM, __sig_handler);
	signal(SIGINT, __sig_handler);
	signal(SIGQUIT, __sig_handler);
}

int ttsd_initialize(ttse_request_callback_s *callback)
{
	/* Register signal handler */
	__register_sig_handler();
	
	if (ttsd_config_initialize(__config_changed_cb)) {
		SLOG(LOG_ERROR, tts_tag(), "[Server WARNING] Fail to initialize config.");
	}

	/* player init */
	if (ttsd_player_init()) {
		SLOG(LOG_ERROR, tts_tag(), "[Server ERROR] Fail to initialize player init.");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	/* Engine Agent initialize */
	if (0 != ttsd_engine_agent_init()) {
		SLOG(LOG_ERROR, tts_tag(), "[Server ERROR] Fail to engine agent initialize.");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	/* set current engine */
	//if (0 != ttsd_engine_agent_initialize_current_engine(callback)) {
	//	SLOG(LOG_WARN, tts_tag(), "[Server WARNING] No Engine !!!" );
	//	g_is_engine = false;
	//} else
	//	g_is_engine = true;

	if (0 != ttsd_engine_agent_load_current_engine(callback)) {
		SLOG(LOG_ERROR, tts_tag(), "[Server ERROR] Fail to load current engine");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	__server_set_synth_control(TTSD_SYNTHESIS_CONTROL_EXPIRED);

	if (TTSD_MODE_SCREEN_READER == ttsd_get_mode()) {
		ttsd_config_set_screen_reader_callback(__screen_reader_changed_cb);
	}

	return TTSD_ERROR_NONE;
}

int ttsd_finalize()
{
	GList *iter = NULL;
	if (0 < g_list_length(g_proc_list)) {
		iter = g_list_first(g_proc_list);
		while (NULL != iter) {
			g_proc_list = g_list_remove_link(g_proc_list, iter);
			g_list_free(iter);
			iter = g_list_first(g_proc_list);
		}
	}
	
	ttsd_config_finalize();

	ttsd_player_release();

	ttsd_engine_agent_release();

	return TTSD_ERROR_NONE;
}

/*
* TTS Server Functions for Client
*/

int ttsd_server_initialize(int pid, int uid, bool* credential_needed)
{
	if (-1 != ttsd_data_is_client(uid)) {
		SLOG(LOG_WARN, tts_tag(), "[Server WARNING] Uid has already been registered");
		return TTSD_ERROR_NONE;
	}

	if (0 != ttsd_engine_agent_is_credential_needed(uid, credential_needed)) {
		SLOG(LOG_ERROR, tts_tag(), "Server ERROR] Fail to get credential necessity");
		return TTSD_ERROR_OPERATION_FAILED;
	}
	if (0 != ttsd_data_new_client(pid, uid)) {
		SLOG(LOG_ERROR, tts_tag(), "[Server ERROR] Fail to add client info");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (0 != ttsd_player_create_instance(uid)) {
		SLOG(LOG_ERROR, tts_tag(), "[Server ERROR] Fail to create player");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	return TTSD_ERROR_NONE;
}

static Eina_Bool __quit_ecore_loop(void *data)
{
	ttsd_dbus_close_connection();

	ttsd_network_finalize();

	ttsd_finalize();

	ecore_main_loop_quit();
	return EINA_FALSE;
}


static void __read_proc()
{
	DIR *dp = NULL;
	struct dirent entry;
	struct dirent *dirp = NULL;
	int ret = -1;
	int tmp;

	GList *iter = NULL;
	if (0 < g_list_length(g_proc_list)) {
		iter = g_list_first(g_proc_list);
		while (NULL != iter) {
			g_proc_list = g_list_remove_link(g_proc_list, iter);
			g_list_free(iter);
			iter = g_list_first(g_proc_list);
		}
	}

	dp = opendir("/proc");
	if (NULL == dp) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Fail to open proc");
	} else {
		do {
			ret = readdir_r(dp, &entry, &dirp);
			if (0 != ret) {
				SLOG(LOG_ERROR, tts_tag(), "[ERROR] Fail to readdir");
				break;
			}

			if (NULL != dirp) {
				tmp = atoi(dirp->d_name);
				if (0 >= tmp)	continue;
				g_proc_list = g_list_append(g_proc_list, GINT_TO_POINTER(tmp));
			}
		} while (NULL != dirp);
		closedir(dp);
	}
	return;
}

bool __get_client_for_clean_up(int pid, int uid, app_state_e state, void* user_data)
{
	bool exist = false;
	int i = 0;
	
	GList *iter = NULL;
	for (i = 0; i < g_list_length(g_proc_list); i++) {
		iter = g_list_nth(g_proc_list, i);
		if (NULL != iter) {
			if (pid == GPOINTER_TO_INT(iter->data)) {
				SLOG(LOG_DEBUG, tts_tag(), "uid (%d) is running", uid);
				exist = true;
				break;
			}
		}
	}
	
	if (false == exist) {
		SLOG(LOG_ERROR, tts_tag(), "uid (%d) should be removed", uid);
		ttsd_server_finalize(uid);
	}

	return true;
#if 0
	char appid[128] = {0, };
	if (0 != aul_app_get_appid_bypid(pid, appid, sizeof(appid))) {
		SLOG(LOG_ERROR, tts_tag(), "[Server ERROR] Fail to get app id");
	}

	if (0 < strlen(appid)) {
		SLOG(LOG_DEBUG, tts_tag(), "[%d] is running app - %s", pid, appid);
	} else {
		SLOG(LOG_DEBUG, tts_tag(), "[%d] is daemon or no_running app", pid);

		int result = 1;
		result = ttsdc_send_hello(pid, uid);

		if (0 == result) {
			SLOG(LOG_DEBUG, tts_tag(), "[Server] uid(%d) should be removed.", uid);
			ttsd_server_finalize(uid);
		} else if (-1 == result) {
			SLOG(LOG_ERROR, tts_tag(), "[Server ERROR] Hello result has error");
		}
	}
	return true;
#endif
}


Eina_Bool ttsd_cleanup_client(void *data)
{
	SLOG(LOG_DEBUG, tts_tag(), "===== CLEAN UP CLIENT START");
	__read_proc();

	if (0 < ttsd_data_get_client_count()) {
	ttsd_data_foreach_clients(__get_client_for_clean_up, NULL);
		} else {
		ecore_timer_add(0, __quit_ecore_loop, NULL);
	}

	SLOG(LOG_DEBUG, tts_tag(), "=====");
	SLOG(LOG_DEBUG, tts_tag(), "  ");

	return EINA_TRUE;
}

void __used_voice_cb(const char* lang, int type)
{
	SLOG(LOG_DEBUG, tts_tag(), "[Server] Request to unload voice (%s,%d)", lang, type);
	if (0 != ttsd_engine_unload_voice(lang, type)) {
		SLOG(LOG_ERROR, tts_tag(), "[Server ERROR] Fail to unload voice");
	}
}

int ttsd_server_finalize(int uid)
{
	app_state_e state;
	if (0 > ttsd_data_get_client_state(uid, &state)) {
		SLOG(LOG_ERROR, tts_tag(), "[Server ERROR] ttsd_server_finalize : uid is not valid");
	}

	ttsd_server_stop(uid);
	ttsd_player_stop(uid);
	
	ttsd_player_destroy_instance(uid);

	/* Need to unload voice when used voice is unregistered */
	if (0 != ttsd_data_reset_used_voice(uid, __used_voice_cb)) {
		SLOG(LOG_ERROR, tts_tag(), "[Server ERROR] Fail to set used voice");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	ttsd_data_delete_client(uid);

	/* unload engine, if ref count of client is 0 */
	if (0 == ttsd_data_get_client_count()) {
		SLOG(LOG_DEBUG, tts_tag(), "[Server] Quit main loop");
		ecore_timer_add(0, __quit_ecore_loop, NULL);
	}

	return TTSD_ERROR_NONE;
}

int ttsd_server_add_queue(int uid, const char* text, const char* lang, int voice_type, int speed, int utt_id, const char* credential)
{
	app_state_e state;
	if (0 > ttsd_data_get_client_state(uid, &state)) {
		SLOG(LOG_ERROR, tts_tag(), "[Server ERROR] ttsd_server_add_queue : uid is not valid");
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	/* check valid voice */
	char* temp_lang = NULL;
	int temp_type;
	if (true != ttsd_engine_select_valid_voice((const char*)lang, voice_type, &temp_lang, &temp_type)) {
		SLOG(LOG_ERROR, tts_tag(), "[Server ERROR] Fail to select valid voice");
		if (NULL != temp_lang)	free(temp_lang);
		return TTSD_ERROR_INVALID_VOICE;
	}

	if (NULL == temp_lang) {
		SLOG(LOG_ERROR, tts_tag(), "[Server ERROR] Fail to select valid voice : result lang is NULL");
		return TTSD_ERROR_INVALID_VOICE;
	}
	
	speak_data_s* speak_data = NULL;
	speak_data = (speak_data_s*)calloc(1, sizeof(speak_data_s));
	if (NULL == speak_data) {
		SLOG(LOG_ERROR, tts_tag(), "[Server ERROR] Fail to allocate memory");
		if (NULL != temp_lang)	free(temp_lang);
		return TTSD_ERROR_OPERATION_FAILED;
	}

	speak_data->lang = strdup(lang);
	speak_data->vctype = voice_type;

	speak_data->speed = speed;
	speak_data->utt_id = utt_id;

	speak_data->text = strdup(text);

	/* if state is APP_STATE_READY , APP_STATE_PAUSED , only need to add speak data to queue*/
	if (0 != ttsd_data_add_speak_data(uid, speak_data)) {
		SLOG(LOG_ERROR, tts_tag(), "[Server ERROR] Fail to add speak data");
		if (NULL != temp_lang)	free(temp_lang);
		if (NULL != speak_data) {
			if (NULL != speak_data->lang)	free(speak_data->lang);
			if (NULL != speak_data->text)	free(speak_data->text);

			speak_data->lang = NULL;
			speak_data->text = NULL;

			free(speak_data);
			speak_data = NULL;
		}

		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (0 != ttsd_data_set_used_voice(uid, temp_lang, temp_type)) {
		/* Request load voice */
		SLOG(LOG_DEBUG, tts_tag(), "[Server] Request to load voice");
		if (0 != ttsd_engine_load_voice(temp_lang, temp_type)) {
			SLOG(LOG_ERROR, tts_tag(), "[Server ERROR] Fail to load voice");
		}
	}

	if (NULL != temp_lang)	free(temp_lang);

	if (APP_STATE_PLAYING == state) {
		/* check if engine use network */
		if (ttsd_engine_agent_need_network()) {
			if (false == ttsd_network_is_connected()) {
				SLOG(LOG_ERROR, tts_tag(), "[Server ERROR] Disconnect network. Current engine needs network.");
				return TTSD_ERROR_OPERATION_FAILED;
			}
		}

		/* Check whether tts-engine is running or not */
		if (TTSD_SYNTHESIS_CONTROL_DOING == __server_get_synth_control()) {
			SLOG(LOG_WARN, tts_tag(), "[Server WARNING] Engine has already been running.");
		} else {
			__synthesis(uid, credential);
		}
	}

	return TTSD_ERROR_NONE;
}

Eina_Bool __send_interrupt_client(void *data)
{
	intptr_t puid = (intptr_t)data;
	int uid = (int)puid;

	int pid = ttsd_data_get_pid(uid);

	if (TTSD_MODE_DEFAULT != ttsd_get_mode()) {
		/* send message to client about changing state */
		ttsdc_send_set_state_message(pid, uid, APP_STATE_READY);
	} else {
		ttsdc_send_set_state_message(pid, uid, APP_STATE_PAUSED);
	}

	return EINA_FALSE;
}

int ttsd_server_play(int uid, const char* credential)
{
	app_state_e state;
	if (0 > ttsd_data_get_client_state(uid, &state)) {
		SLOG(LOG_ERROR, tts_tag(), "[Server ERROR] uid(%d) is NOT valid  ", uid);
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	if (APP_STATE_PLAYING == state) {
		SLOG(LOG_WARN, tts_tag(), "[Server WARNING] Current state(%d) is 'play' ", uid);
		return TTSD_ERROR_NONE;
	}

	/* check if engine use network */
	if (ttsd_engine_agent_need_network()) {
		if (false == ttsd_network_is_connected()) {
			SLOG(LOG_ERROR, tts_tag(), "[Server ERROR] Disconnect network. Current engine needs network service!!!.");
			return TTSD_ERROR_OUT_OF_NETWORK;
		}
	}

	int current_uid = ttsd_data_get_current_playing();
	SLOG(LOG_ERROR, tts_tag(), "[Server] playing uid (%d)", current_uid);

	if (uid != current_uid && -1 != current_uid) {
		if (TTSD_MODE_DEFAULT != ttsd_get_mode()) {
			/* Send interrupt message */
			SLOG(LOG_DEBUG, tts_tag(), "[Server] Old uid(%d) will be interrupted into 'Stop' state ", current_uid);

			/* pause player */
			if (0 != ttsd_server_stop(current_uid)) {
				SLOG(LOG_WARN, tts_tag(), "[Server ERROR] Fail to stop : uid (%d)", current_uid);
			}
			if (0 != ttsd_player_stop(current_uid)) {
				SLOG(LOG_WARN, tts_tag(), "[Server ERROR] Fail to player stop : uid (%d)", current_uid);
			}

			intptr_t pcurrent_uid = (intptr_t)current_uid;
			ecore_timer_add(0, __send_interrupt_client, (void*)pcurrent_uid);
		} else {
			/* Default mode policy of interrupt is "Pause" */

			/* Send interrupt message */
			SLOG(LOG_DEBUG, tts_tag(), "[Server] Old uid(%d) will be interrupted into 'Pause' state ", current_uid);

			/* pause player */
			if (0 != ttsd_player_pause(current_uid)) {
				SLOG(LOG_WARN, tts_tag(), "[Server ERROR] Fail to ttsd_player_pause() : uid (%d)", current_uid);
			}

			/* change state */
			ttsd_data_set_client_state(current_uid, APP_STATE_PAUSED);

			intptr_t pcurrent_uid = (intptr_t)current_uid;
			ecore_timer_add(0, __send_interrupt_client, (void*)pcurrent_uid);
		}
	}

	/* Change current play */
	if (0 != ttsd_data_set_client_state(uid, APP_STATE_PLAYING)) {
		SLOG(LOG_ERROR, tts_tag(), "[Server ERROR] Fail to set state : uid(%d)", uid);
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (APP_STATE_PAUSED == state) {
		SLOG(LOG_DEBUG, tts_tag(), "[Server] uid(%d) is 'Pause' state : resume player", uid);

		/* Resume player */
		if (0 != ttsd_player_resume(uid)) {
			SLOG(LOG_WARN, tts_tag(), "[Server WARNING] Fail to ttsd_player_resume()");
		}
	}

	/* Check whether tts-engine is running or not */
	if (TTSD_SYNTHESIS_CONTROL_DOING == __server_get_synth_control()) {
		SLOG(LOG_WARN, tts_tag(), "[Server WARNING] Engine has already been running.");
	} else {
		__synthesis(uid, credential);
	}

	return TTSD_ERROR_NONE;
}


int ttsd_server_stop(int uid)
{
	app_state_e state;
	if (0 > ttsd_data_get_client_state(uid, &state)) {
		SLOG(LOG_ERROR, tts_tag(), "[Server ERROR] uid is not valid");
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	if (APP_STATE_PLAYING == state || APP_STATE_PAUSED == state) {
		if (TTSD_SYNTHESIS_CONTROL_DOING == __server_get_synth_control()) {
			SLOG(LOG_DEBUG, tts_tag(), "[Server] TTS-engine is running");

			int ret = 0;
			ret = ttsd_engine_cancel_synthesis();
			if (0 != ret)
				SLOG(LOG_ERROR, tts_tag(), "[Server ERROR] Fail to cancel synthesis : ret(%d)", ret);
		}

		__server_set_synth_control(TTSD_SYNTHESIS_CONTROL_EXPIRED);

		if (0 != ttsd_player_clear(uid))
			SLOG(LOG_WARN, tts_tag(), "[Server] Fail to ttsd_player_stop()");

		ttsd_data_set_client_state(uid, APP_STATE_READY);
	}

	/* Reset all data */
	ttsd_data_clear_data(uid);

	return TTSD_ERROR_NONE;
}

int ttsd_server_pause(int uid, int* utt_id)
{
	app_state_e state;
	if (0 > ttsd_data_get_client_state(uid, &state)) {
		SLOG(LOG_ERROR, tts_tag(), "[Server ERROR] ttsd_server_pause : uid is not valid");
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	if (APP_STATE_PLAYING != state)	{
		SLOG(LOG_WARN, tts_tag(), "[Server WARNING] Current state is not 'play'");
		return TTSD_ERROR_INVALID_STATE;
	}

	int ret = 0;
	ret = ttsd_player_pause(uid);
	if (0 != ret) {
		SLOG(LOG_ERROR, tts_tag(), "[Server ERROR] Fail player_pause() : ret(%d)", ret);
		return TTSD_ERROR_OPERATION_FAILED;
	}

	ttsd_data_set_client_state(uid, APP_STATE_PAUSED);

	return TTSD_ERROR_NONE;
}

int ttsd_server_get_support_voices(int uid, GList** voice_list)
{
	app_state_e state;
	if (0 > ttsd_data_get_client_state(uid, &state)) {
		SLOG(LOG_ERROR, tts_tag(), "[Server ERROR] uid is not valid");
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	/* get voice list*/
	if (0 != ttsd_engine_get_voice_list(voice_list)) {
		SLOG(LOG_ERROR, tts_tag(), "[Server ERROR] Fail ttsd_server_get_support_voices()");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	SLOG(LOG_DEBUG, tts_tag(), "[Server SUCCESS] Get supported voices");

	return TTSD_ERROR_NONE;
}

int ttsd_server_get_current_voice(int uid, char** language, int* voice_type)
{
	app_state_e state;
	if (0 > ttsd_data_get_client_state(uid, &state)) {
		SLOG(LOG_ERROR, tts_tag(), "[Server ERROR] ttsd_server_get_current_voice : uid is not valid");
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	/* get current voice */
	int ret = ttsd_engine_get_default_voice(language, voice_type);
	if (0 != ret) {
		SLOG(LOG_ERROR, tts_tag(), "[Server ERROR] Fail ttsd_server_get_support_voices()");
		return ret;
	}

	SLOG(LOG_DEBUG, tts_tag(), "[Server] Get default language (%s), voice type(%d) ", *language, *voice_type);

	return TTSD_ERROR_NONE;
}

int ttsd_server_set_private_data(int uid, const char* key, const char* data)
{
	app_state_e state;
	if (0 > ttsd_data_get_client_state(uid, &state)) {
		SLOG(LOG_ERROR, tts_tag(), "[Server ERROR] uid(%d) is NOT valid", uid);
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	if (APP_STATE_READY != state) {
		SLOG(LOG_ERROR, tts_tag(), "[Server ERROR] Current state(%d) is NOT 'READY'", uid);
		return TTSD_ERROR_INVALID_STATE;
	}

	int ret = ttsd_engine_set_private_data(key, data);
	if (0 != ret) {
		SLOG(LOG_ERROR, tts_tag(), "[Server ERROR] Fail to set private data");
	} else {
		SLOG(LOG_DEBUG, tts_tag(), "[Server] Set private data");
	}

	return ret;
}

int ttsd_server_get_private_data(int uid, const char* key, char** data)
{
	app_state_e state;
	if (0 > ttsd_data_get_client_state(uid, &state)) {
		SLOG(LOG_ERROR, tts_tag(), "[Server ERROR] uid(%d) is NOT valid", uid);
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	if (APP_STATE_READY != state) {
		SLOG(LOG_WARN, tts_tag(), "[Server ERROR] Current state(%d) is NOT 'READY'", uid);
		return TTSD_ERROR_INVALID_STATE;
	}

	int ret = ttsd_engine_get_private_data(key, data);
	if (0 != ret) {
		SLOG(LOG_ERROR, tts_tag(), "[Server ERROR] Fail to get private data");
	} else {
		SLOG(LOG_DEBUG, tts_tag(), "[Server] Get private data");
	}

	return ret;
}

int ttsd_set_private_data_set_cb(ttse_private_data_set_cb callback)
{
	SLOG(LOG_DEBUG, tts_tag(), "[Server] Set private data set cb");

	int ret = 0;
	ret = ttsd_engine_agent_set_private_data_set_cb(callback);
	if(0 != ret) {
		SLOG(LOG_ERROR, tts_tag(), "[Server ERROR] Fail to set private data set cb");
		return ret;
	}

	return ret;
}

int ttsd_set_private_data_requested_cb(ttse_private_data_requested_cb callback)
{
	SLOG(LOG_DEBUG, tts_tag(), "[Server] Set private data requested cb");

	int ret = 0;
	ret = ttsd_engine_agent_set_private_data_requested_cb(callback);
	if(0 != ret) {
		SLOG(LOG_ERROR, tts_tag(), "[Server ERROR] Fail to set private data requested cb");
		return ret;
	}

	return ret;
}

/*
int ttsd_send_result(ttse_result_event_e event, const void* data, unsigned int data_size, ttse_audio_type_e audio_type, int rate, void* user_data)
{}

int ttsd_send_error(ttse_error_e error, const char* msg)
{}
*/

