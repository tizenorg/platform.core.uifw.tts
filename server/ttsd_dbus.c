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


#include <dbus/dbus.h>
#include <Ecore.h>

#include "ttsd_main.h"
#include "ttsd_dbus_server.h"
#include "ttsd_dbus.h"
#include "ttsd_server.h"


static DBusConnection* g_conn;

int ttsdc_send_message(int pid, int uid, int uttid, char *method)
{   
	char service_name[64];
	memset(service_name, 0, 64);
	snprintf(service_name, 64, "%s%d", TTS_CLIENT_SERVICE_NAME, pid);

	char target_if_name[64];
	snprintf(target_if_name, sizeof(target_if_name), "%s%d", TTS_CLIENT_SERVICE_INTERFACE, pid);

	DBusMessage* msg;

	/* create a message & check for errors */
	msg = dbus_message_new_method_call(
		service_name, 
		TTS_CLIENT_SERVICE_OBJECT_PATH, 
		target_if_name, 
		method);

	if (NULL == msg) { 
		SLOG(LOG_ERROR, TAG_TTSD, "[Dbus ERROR] Fail to create message : type(%s), uid(%d)\n", method, uid); 
		return -1;
	}

	dbus_message_append_args(msg, DBUS_TYPE_INT32, &uid, DBUS_TYPE_INT32, &uttid, DBUS_TYPE_INVALID);

	/* send the message and flush the connection */
	if (!dbus_connection_send(g_conn, msg, NULL)) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Dbus ERROR] <<<< send message : Out Of Memory, type(%s), ifname(%s), uid(%d), uttid(%d)", method, target_if_name, uid, uttid); 
	} else {
		SLOG(LOG_DEBUG, TAG_TTSD, "<<<< send message : type(%s), uid(%d), uttid(%d)", method, uid, uttid);

		dbus_connection_flush(g_conn);
	}

	dbus_message_unref(msg);

	return 0;
}

int ttsdc_send_utt_start_message(int pid, int uid, int uttid)
{
	return ttsdc_send_message(pid, uid, uttid, TTS_METHOD_UTTERANCE_STARTED);
}

int ttsdc_send_utt_finish_message(int pid, int uid, int uttid) 
{
	return ttsdc_send_message(pid, uid, uttid, TTS_METHOD_UTTERANCE_COMPLETED);
}

int ttsdc_send_interrupt_message(int pid, int uid, ttsd_interrupted_code_e code)
{
	return ttsdc_send_message(pid, uid, (int)code, TTS_METHOD_INTERRUPT);
}

int ttsdc_send_error_message(int pid, int uid, int uttid, int reason)
{
	char service_name[64];
	memset(service_name, 0, 64);
	snprintf(service_name, 64, "%s%d", TTS_CLIENT_SERVICE_NAME, pid);

	char target_if_name[128];
	snprintf(target_if_name, sizeof(target_if_name), "%s%d", TTS_CLIENT_SERVICE_INTERFACE, pid);

	DBusMessage* msg;

	msg = dbus_message_new_method_call(
		service_name, 
		TTS_CLIENT_SERVICE_OBJECT_PATH, 
		target_if_name, 
		TTS_METHOD_ERROR);

	if (NULL == msg) { 
		SLOG(LOG_ERROR, TAG_TTSD, "[Dbus ERROR] Fail to create error message : uid(%d)\n", uid); 
		return -1;
	}

	dbus_message_append_args( msg, 
		DBUS_TYPE_INT32, &uid, 
		DBUS_TYPE_INT32, &uttid, 
		DBUS_TYPE_INT32, &reason, 
		DBUS_TYPE_INVALID);
	
	if (!dbus_connection_send(g_conn, msg, NULL)) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Dbus ERROR] <<<< error message : Out Of Memory !\n"); 
	} else {
		SLOG(LOG_DEBUG, TAG_TTSD, "<<<< Send error signal : uid(%d), reason(%d), uttid(%d)", uid, reason, uttid);
		dbus_connection_flush(g_conn);
	}

	dbus_message_unref(msg);

	return 0;
}


static Eina_Bool listener_event_callback(void* data, Ecore_Fd_Handler *fd_handler)
{
	DBusConnection* conn = (DBusConnection*)data;

	if (NULL == conn)	return ECORE_CALLBACK_RENEW;

	dbus_connection_read_write_dispatch(conn, 50);

	DBusMessage* msg = NULL;
	msg = dbus_connection_pop_message(conn);

	/* loop again if we haven't read a message */
	if (NULL == msg) { 
		return ECORE_CALLBACK_RENEW;
	}
	
	/* client event */
	if (dbus_message_is_method_call(msg, TTS_SERVER_SERVICE_INTERFACE, TTS_METHOD_HELLO))
		ttsd_dbus_server_hello(conn, msg);

	else if( dbus_message_is_method_call(msg, TTS_SERVER_SERVICE_INTERFACE, TTS_METHOD_INITIALIZE) )
		ttsd_dbus_server_initialize(conn, msg);
	
	else if( dbus_message_is_method_call(msg, TTS_SERVER_SERVICE_INTERFACE, TTS_METHOD_FINALIZE) )
		ttsd_dbus_server_finalize(conn, msg);
	
	else if( dbus_message_is_method_call(msg, TTS_SERVER_SERVICE_INTERFACE, TTS_METHOD_GET_SUPPORT_VOICES) )
		ttsd_dbus_server_get_support_voices(conn, msg);

	else if( dbus_message_is_method_call(msg, TTS_SERVER_SERVICE_INTERFACE, TTS_METHOD_GET_CURRENT_VOICE) )
		ttsd_dbus_server_get_current_voice(conn, msg);

	else if( dbus_message_is_method_call(msg, TTS_SERVER_SERVICE_INTERFACE, TTS_METHOD_ADD_QUEUE) )
		ttsd_dbus_server_add_text(conn, msg);

	else if (dbus_message_is_method_call(msg, TTS_SERVER_SERVICE_INTERFACE, TTS_METHOD_PLAY)) 
		ttsd_dbus_server_play(conn, msg);
	
	else if (dbus_message_is_method_call(msg, TTS_SERVER_SERVICE_INTERFACE, TTS_METHOD_STOP)) 
		ttsd_dbus_server_stop(conn, msg);

	else if (dbus_message_is_method_call(msg, TTS_SERVER_SERVICE_INTERFACE, TTS_METHOD_PAUSE)) 
		ttsd_dbus_server_pause(conn, msg);

	/* setting event */
	else if (dbus_message_is_method_call(msg, TTS_SERVER_SERVICE_INTERFACE, TTS_SETTING_METHOD_HELLO))
		ttsd_dbus_server_hello(conn, msg);

	else if (dbus_message_is_method_call(msg, TTS_SERVER_SERVICE_INTERFACE, TTS_SETTING_METHOD_INITIALIZE) )
		ttsd_dbus_server_setting_initialize(conn, msg);

	else if (dbus_message_is_method_call(msg, TTS_SERVER_SERVICE_INTERFACE, TTS_SETTING_METHOD_FINALIZE) )
		ttsd_dbus_server_setting_finalize(conn, msg);

	else if (dbus_message_is_method_call(msg, TTS_SERVER_SERVICE_INTERFACE, TTS_SETTING_METHOD_GET_ENGINE_LIST) )
		ttsd_dbus_server_setting_get_engine_list(conn, msg);

	else if (dbus_message_is_method_call(msg, TTS_SERVER_SERVICE_INTERFACE, TTS_SETTING_METHOD_GET_ENGINE) )
		ttsd_dbus_server_setting_get_engine(conn, msg);

	else if (dbus_message_is_method_call(msg, TTS_SERVER_SERVICE_INTERFACE, TTS_SETTING_METHOD_SET_ENGINE) )
		ttsd_dbus_server_setting_set_engine(conn, msg);

	else if (dbus_message_is_method_call(msg, TTS_SERVER_SERVICE_INTERFACE, TTS_SETTING_METHOD_GET_VOICE_LIST) )
		ttsd_dbus_server_setting_get_voice_list(conn, msg);

	else if (dbus_message_is_method_call(msg, TTS_SERVER_SERVICE_INTERFACE, TTS_SETTING_METHOD_GET_DEFAULT_VOICE) )
		ttsd_dbus_server_setting_get_default_voice(conn, msg);

	else if (dbus_message_is_method_call(msg, TTS_SERVER_SERVICE_INTERFACE, TTS_SETTING_METHOD_SET_DEFAULT_VOICE) )
		ttsd_dbus_server_setting_set_default_voice(conn, msg);

	else if (dbus_message_is_method_call(msg, TTS_SERVER_SERVICE_INTERFACE, TTS_SETTING_METHOD_GET_DEFAULT_SPEED) )
		ttsd_dbus_server_setting_get_speed(conn, msg);

	else if (dbus_message_is_method_call(msg, TTS_SERVER_SERVICE_INTERFACE, TTS_SETTING_METHOD_SET_DEFAULT_SPEED) )
		ttsd_dbus_server_setting_set_speed(conn, msg);

	else if (dbus_message_is_method_call(msg, TTS_SERVER_SERVICE_INTERFACE, TTS_SETTING_METHOD_GET_ENGINE_SETTING) )
		ttsd_dbus_server_setting_get_engine_setting(conn, msg);

	else if (dbus_message_is_method_call(msg, TTS_SERVER_SERVICE_INTERFACE, TTS_SETTING_METHOD_SET_ENGINE_SETTING) )
		ttsd_dbus_server_setting_set_engine_setting(conn, msg);


	else if (dbus_message_is_method_call(msg, TTS_SERVER_SERVICE_INTERFACE, TTS_METHOD_NEXT_PLAY)) 
		ttsd_dbus_server_start_next_play(msg);

	else if (dbus_message_is_method_call(msg, TTS_SERVER_SERVICE_INTERFACE, TTS_METHOD_NEXT_SYNTHESIS)) 
		ttsd_dbus_server_start_next_synthesis(msg);
	

	/* free the message */
	dbus_message_unref(msg);

	return ECORE_CALLBACK_RENEW;
}

int ttsd_dbus_open_connection()
{
	DBusError err;
	dbus_error_init(&err);

	int ret;

	/* connect to the bus and check for errors */
	g_conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_TTSD, "[Dbus ERROR] fail dbus_bus_get : %s\n", err.message);
		dbus_error_free(&err); 
	}

	if (NULL == g_conn) { 
		SLOG(LOG_ERROR, TAG_TTSD, "[Dbus ERROR] fail to get dbus connection \n" );
		return -1;
	}

	/* request our name on the bus and check for errors */
	ret = dbus_bus_request_name(g_conn, TTS_SERVER_SERVICE_NAME, DBUS_NAME_FLAG_REPLACE_EXISTING , &err);

	if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) {
		printf("Fail to be primary owner in dbus request. \n");
		SLOG(LOG_ERROR, TAG_TTSD, "[Dbus ERROR] fail to be primary owner \n");
		return -1;
	}

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_TTSD, "[Dbus ERROR] dbus_bus_request_name() : %s \n", err.message);
		dbus_error_free(&err); 

		return -1;
	}

	/* add a rule for getting signal */
	char rule[128];
	snprintf(rule, 128, "type='signal',interface='%s'", TTS_SERVER_SERVICE_INTERFACE);

	/* add a rule for which messages we want to see */
	dbus_bus_add_match(g_conn, rule, &err); /* see signals from the given interface */
	dbus_connection_flush(g_conn);

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_TTSD, "[Dbus ERROR] dbus_bus_add_match() : %s \n", err.message);
		return -1; 
	}

	int fd = 0;
	dbus_connection_get_unix_fd(g_conn, &fd);

	Ecore_Fd_Handler* fd_handler;
	fd_handler = ecore_main_fd_handler_add(fd, ECORE_FD_READ , (Ecore_Fd_Cb)listener_event_callback, g_conn, NULL, NULL);

	if (NULL == fd_handler) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Dbus ERROR] ecore_main_fd_handler_add() : fail to get fd handler \n");
		return -1;
	}

	SLOG(LOG_DEBUG, TAG_TTSD, "[Dbus SUCCESS] Open connection. ");
	return 0;
}

int ttsd_dbus_close_connection()
{
	DBusError err;
	dbus_error_init(&err);

	dbus_bus_release_name (g_conn, TTS_SERVER_SERVICE_NAME, &err);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Dbus ERROR] dbus_bus_release_name() : %s\n", err.message); 
		dbus_error_free(&err); 
		return -1;
	}

	SLOG(LOG_DEBUG, TAG_TTSD, "[Dbus SUCCESS] Close connection. ");

	return 0;
}

int ttsd_send_start_next_play_message(int uid)
{
	DBusMessage* msg;

	msg = dbus_message_new_method_call(
		TTS_SERVER_SERVICE_NAME, 
		TTS_SERVER_SERVICE_OBJECT_PATH, 
		TTS_SERVER_SERVICE_INTERFACE, 
		TTS_METHOD_NEXT_PLAY);

	if (NULL == msg) { 
		SLOG(LOG_ERROR, TAG_TTSD, "[Dbus ERROR] >>>> Fail to make message for 'start next play'"); 
		return -1;
	} 

	dbus_message_append_args(msg, DBUS_TYPE_INT32, &uid, DBUS_TYPE_INVALID);	

	if (!dbus_connection_send(g_conn, msg, NULL)) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Dbus ERROR] >>>> Fail to send message for 'start next play'\n"); 
		return -1;
	}

	dbus_connection_flush(g_conn);
	dbus_message_unref(msg);

	return 0;
}

int ttsd_send_start_next_synthesis_message(int uid)
{
	DBusMessage* msg;

	msg = dbus_message_new_method_call(
		TTS_SERVER_SERVICE_NAME, 
		TTS_SERVER_SERVICE_OBJECT_PATH, 
		TTS_SERVER_SERVICE_INTERFACE, 
		TTS_METHOD_NEXT_SYNTHESIS);

	if (NULL == msg) { 
		SLOG(LOG_ERROR, TAG_TTSD, "[Dbus ERROR] >>>> Fail to make message for 'start next synthesis'\n"); 
		return -1;
	} 
	
	dbus_message_append_args(msg, DBUS_TYPE_INT32, &uid, DBUS_TYPE_INVALID);	
	
	if (!dbus_connection_send(g_conn, msg, NULL)) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Dbus ERROR] >>>> Fail to send message for 'start next synthesis'\n"); 
		return -1;
	}

	dbus_connection_flush(g_conn);
	dbus_message_unref(msg);

	return 0;
}
