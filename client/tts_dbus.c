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

#include <Ecore.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/inotify.h>

#include "tts_client.h"
#include "tts_config_mgr.h"
#include "tts_dbus.h"
#include "tts_defs.h"
#include "tts_main.h"


#define HELLO_WAITING_TIME 500
#define WAITING_TIME 5000

static DBusConnection* g_conn_sender = NULL;
static DBusConnection* g_conn_listener = NULL;

static Ecore_Fd_Handler* g_dbus_fd_handler = NULL;


extern int __tts_cb_error(int uid, tts_error_e reason, int utt_id);

extern int __tts_cb_set_state(int uid, int state);

extern int __tts_cb_utt_started(int uid, int utt_id);

extern int __tts_cb_utt_completed(int uid, int utt_id);


static Eina_Bool listener_event_callback(void* data, Ecore_Fd_Handler *fd_handler)
{
	if (NULL == g_conn_listener)	return ECORE_CALLBACK_RENEW;

	dbus_connection_read_write_dispatch(g_conn_listener, 50);

	DBusMessage* msg = NULL;
	msg = dbus_connection_pop_message(g_conn_listener);

	if (true != dbus_connection_get_is_connected(g_conn_listener)) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Connection is disconnected");
		return ECORE_CALLBACK_RENEW;
	}

	/* loop again if we haven't read a message */
	if (NULL == msg) { 
		return ECORE_CALLBACK_RENEW;
	}

	DBusError err;
	dbus_error_init(&err);

	DBusMessage *reply = NULL; 
	
	char if_name[64];
	//snprintf(if_name, 64, "%s%d", TTS_CLIENT_SERVICE_INTERFACE, getpid());
	snprintf(if_name, 64, "%s", TTS_CLIENT_SERVICE_INTERFACE);

	/* check if the message is a signal from the correct interface and with the correct name */
	if (dbus_message_is_method_call(msg, if_name, TTSD_METHOD_HELLO)) {
		SLOG(LOG_DEBUG, TAG_TTSC, "===== Get Hello");
		int uid = 0;
		int response = -1;

		dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &uid, DBUS_TYPE_INVALID);
		if (dbus_error_is_set(&err)) {
			SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Get arguments error (%s)", err.message);
			dbus_error_free(&err);
		}

		if (uid > 0) {
			SLOG(LOG_DEBUG, TAG_TTSC, "<<<< tts get hello : uid(%d)", uid);

			/* check uid */
			tts_client_s* client = tts_client_get_by_uid(uid);
			if (NULL != client) 
				response = 1;
			else 
				response = 0;
		} else {
			SLOG(LOG_ERROR, TAG_TTSC, "<<<< tts get hello : invalid uid");
		}

		reply = dbus_message_new_method_return(msg);

		if (NULL != reply) {
			dbus_message_append_args(reply, DBUS_TYPE_INT32, &response, DBUS_TYPE_INVALID);

			if (!dbus_connection_send(g_conn_listener, reply, NULL))
				SLOG(LOG_ERROR, TAG_TTSC, ">>>> tts get hello : fail to send reply");
			else 
				SLOG(LOG_DEBUG, TAG_TTSC, ">>>> tts get hello : result(%d)", response);

			dbus_connection_flush(g_conn_listener);
			dbus_message_unref(reply); 
		} else {
			SLOG(LOG_ERROR, TAG_TTSC, ">>>> tts get hello : fail to create reply message");
		}

		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
	} /* TTSD_METHOD_HELLO */

	else if (dbus_message_is_method_call(msg, if_name, TTSD_METHOD_UTTERANCE_STARTED)) {
		SLOG(LOG_DEBUG, TAG_TTSC, "===== Get utterance started");
		int uid = 0;
		int uttid = 0;

		dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &uid, DBUS_TYPE_INT32, &uttid, DBUS_TYPE_INVALID);
		if (dbus_error_is_set(&err)) {
			SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Get arguments error (%s)", err.message);
			dbus_error_free(&err);
		}

		if (uid > 0) {
			SLOG(LOG_DEBUG, TAG_TTSC, "<<<< tts utterance started : uid(%d) uttid(%d)", uid, uttid);
			__tts_cb_utt_started(uid, uttid);
		} else {
			SLOG(LOG_ERROR, TAG_TTSC, "<<<< tts utterance started : invalid uid");
		}

		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
	} /* TTSD_METHOD_UTTERANCE_STARTED */

	else if (dbus_message_is_method_call(msg, if_name, TTSD_METHOD_UTTERANCE_COMPLETED)) {
		SLOG(LOG_DEBUG, TAG_TTSC, "===== Get utterance completed");
		int uid = 0;
		int uttid = 0;

		dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &uid, DBUS_TYPE_INT32, &uttid, DBUS_TYPE_INVALID);
		if (dbus_error_is_set(&err)) {
			SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Get arguments error (%s)", err.message);
			dbus_error_free(&err);
		}

		if (uid > 0) {
			SLOG(LOG_DEBUG, TAG_TTSC, "<<<< tts utterance completed : uid(%d) uttid(%d)", uid, uttid);
			__tts_cb_utt_completed(uid, uttid);
		} else {
			SLOG(LOG_ERROR, TAG_TTSC, "<<<< tts utterance completed : invalid uid");
		}

		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
	} /* TTSD_METHOD_UTTERANCE_COMPLETED */

	else if (dbus_message_is_method_call(msg, if_name, TTSD_METHOD_SET_STATE)) {
		SLOG(LOG_DEBUG, TAG_TTSC, "===== Set state changed");
		int uid = 0;
		int state = 0;

		dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &uid, DBUS_TYPE_INT32, &state, DBUS_TYPE_INVALID);
		if (dbus_error_is_set(&err)) {
			SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Get arguments error (%s)", err.message);
			dbus_error_free(&err);
		}

		if (uid > 0) {
			SLOG(LOG_DEBUG, TAG_TTSC, "<<<< tts state changed : uid(%d) state(%d)", uid, state);
			__tts_cb_set_state(uid, state);
		} else {
			SLOG(LOG_ERROR, TAG_TTSC, "<<<< tts state changed : invalid uid");
		}

		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
	} /* TTSD_METHOD_SET_STATE */

	else if (dbus_message_is_method_call(msg, if_name, TTSD_METHOD_ERROR)) {
		SLOG(LOG_DEBUG, TAG_TTSC, "===== Get error message");

		int uid;
		int uttid;
		int reason;

		dbus_message_get_args(msg, &err,
			DBUS_TYPE_INT32, &uid,
			DBUS_TYPE_INT32, &uttid,
			DBUS_TYPE_INT32, &reason,
			DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) { 
			SLOG(LOG_ERROR, TAG_TTSC, "<<<< Get Error message - Get arguments error (%s)", err.message);
			dbus_error_free(&err); 
		} else {
			SLOG(LOG_ERROR, TAG_TTSC, "<<<< Get Error message : uid(%d), error(%d), uttid(%d)", uid, reason, uttid);
			__tts_cb_error(uid, reason, uttid);
		}

		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
	}/* TTS_SIGNAL_ERROR */

	/* free the message */
	dbus_message_unref(msg);

	return ECORE_CALLBACK_PASS_ON;
}

int tts_dbus_open_connection()
{
	if (NULL != g_conn_sender && NULL != g_conn_listener) {
		SLOG(LOG_WARN, TAG_TTSC, "already existed connection ");
		return 0;
	}

	DBusError err;

	/* initialise the error value */
	dbus_error_init(&err);

	/* connect to the DBUS system bus, and check for errors */
	g_conn_sender = dbus_bus_get_private(DBUS_BUS_SYSTEM, &err);
	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Dbus Connection Error (%s)", err.message); 
		dbus_error_free(&err); 
	}

	if (NULL == g_conn_sender) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] fail to get dbus connection");
		return TTS_ERROR_OPERATION_FAILED; 
	}

	dbus_connection_set_exit_on_disconnect(g_conn_sender, false);

	g_conn_listener = dbus_bus_get_private(DBUS_BUS_SYSTEM, &err);
	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Dbus Connection Error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL == g_conn_listener) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] fail to get dbus connection for listener");
		return TTS_ERROR_OPERATION_FAILED;
	}

	int pid = getpid();

	char service_name[64];
	memset(service_name, 0, 64);
	//snprintf(service_name, 64, "%s%d", TTS_CLIENT_SERVICE_NAME, pid);
	snprintf(service_name, 64, "%s", TTS_CLIENT_SERVICE_NAME);

	SLOG(LOG_DEBUG, TAG_TTSC, "Service name is %s", service_name);

	/* register our name on the bus, and check for errors */
	dbus_bus_request_name(g_conn_listener, service_name, DBUS_NAME_FLAG_REPLACE_EXISTING, &err);
	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Name Error (%s)", err.message);
		dbus_error_free(&err);
	}

	char rule[128];
	//snprintf(rule, 128, "type='method_call',interface='%s%d'", TTS_CLIENT_SERVICE_INTERFACE, pid);
	snprintf(rule, 128, "type='method_call',interface='%s'", TTS_CLIENT_SERVICE_INTERFACE);

	SLOG(LOG_DEBUG, TAG_TTSC, "rule is %s", rule);

	/* add a rule for which messages we want to see */
	dbus_bus_add_match(g_conn_listener, rule, &err);
	dbus_connection_flush(g_conn_listener);

	int fd = 0;
	if (true != dbus_connection_get_unix_fd(g_conn_listener, &fd)) {
		SLOG(LOG_ERROR, TAG_TTSC, "Fail to get fd from dbus");
		return TTS_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_TTSC, "Get fd from dbus : %d", fd);
	}

	g_dbus_fd_handler = ecore_main_fd_handler_add(fd, ECORE_FD_READ, (Ecore_Fd_Cb)listener_event_callback, NULL, NULL, NULL);
	if (NULL == g_dbus_fd_handler) { 
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to get fd handler from ecore");
		return TTS_ERROR_OPERATION_FAILED;
	}

	return 0;
}

int tts_dbus_close_connection()
{
	DBusError err;
	dbus_error_init(&err);

	if (NULL != g_dbus_fd_handler) {
		ecore_main_fd_handler_del(g_dbus_fd_handler);
		g_dbus_fd_handler = NULL;
	}

	int pid = getpid();

	char service_name[64];
	memset(service_name, 0, 64);
	//snprintf(service_name, 64, "%s%d", TTS_CLIENT_SERVICE_NAME, pid);
	snprintf(service_name, 64, "%s", TTS_CLIENT_SERVICE_NAME);

	dbus_bus_release_name(g_conn_listener, service_name, &err);
	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Release name error (%s)", err.message);
		dbus_error_free(&err);
	}

	g_conn_sender = NULL;
	g_conn_listener = NULL;

	return 0;
}

int tts_dbus_reconnect()
{
	bool sender_connected = dbus_connection_get_is_connected(g_conn_sender);
	bool listener_connected = dbus_connection_get_is_connected(g_conn_listener);
	SLOG(LOG_DEBUG, TAG_TTSC, "[DBUS] Sender(%s) Listener(%s)", 
		sender_connected ? "Connected" : "Not connected", listener_connected ? "Connected" : "Not connected");

	if (false == sender_connected || false == listener_connected) {
		tts_dbus_close_connection();

		if(0 != tts_dbus_open_connection()) {
			SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to reconnect");
			return -1;
		} 

		SLOG(LOG_DEBUG, TAG_TTSC, "[DBUS] Reconnect");
	}
	
	return 0;
}

DBusMessage* __tts_dbus_make_message(int uid, const char* method)
{
	if (NULL == method) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Input method is NULL"); 
		return NULL;
	}

	tts_client_s* client = tts_client_get_by_uid(uid);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] uid is not available");
		return NULL;
	}

	DBusMessage* msg;

	if (TTS_MODE_DEFAULT == client->mode) {
		msg = dbus_message_new_method_call(
			TTS_SERVER_SERVICE_NAME, 
			TTS_SERVER_SERVICE_OBJECT_PATH, 
			TTS_SERVER_SERVICE_INTERFACE, 
			method);

		SLOG(LOG_DEBUG, TAG_TTSC, ">>>> Service name : %s, method : %s", TTS_SERVER_SERVICE_NAME, method);
	} else if (TTS_MODE_NOTIFICATION == client->mode) {
		msg = dbus_message_new_method_call(
			TTS_NOTI_SERVER_SERVICE_NAME, 
			TTS_NOTI_SERVER_SERVICE_OBJECT_PATH, 
			TTS_NOTI_SERVER_SERVICE_INTERFACE, 
			method);
	} else if (TTS_MODE_SCREEN_READER == client->mode) {
		msg = dbus_message_new_method_call(
			TTS_SR_SERVER_SERVICE_NAME, 
			TTS_SR_SERVER_SERVICE_OBJECT_PATH, 
			TTS_SR_SERVER_SERVICE_INTERFACE, 
			method);
	} else {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Input mode is not available"); 
		return NULL;
	}

	return msg;
}

int tts_dbus_request_hello(int uid)
{
	DBusMessage* msg;

	msg = __tts_dbus_make_message(uid, TTS_METHOD_HELLO);
	if (NULL == msg) { 
		SLOG(LOG_ERROR, TAG_TTSC, ">>>> Request tts hello : Fail to make message");
		return TTS_ERROR_OPERATION_FAILED;
	} 

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg = NULL;
	int result = 0;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn_sender, msg, HELLO_WAITING_TIME, &err);
	dbus_message_unref(msg);
	if (dbus_error_is_set(&err)) {
		SLOG(LOG_DEBUG, TAG_TTSC, ">>>> error : %s", err);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		dbus_message_unref(result_msg);

		SLOG(LOG_DEBUG, TAG_TTSC, "<<<< tts hello");
		result = 0;
	} else {
		result = TTS_ERROR_TIMED_OUT;
	}

	return result;
}

int tts_dbus_request_initialize(int uid)
{
	DBusMessage* msg;
	DBusError err;
	dbus_error_init(&err);

	msg = __tts_dbus_make_message(uid, TTS_METHOD_INITIALIZE);

	if (NULL == msg) { 
		SLOG(LOG_ERROR, TAG_TTSC, ">>>> Request tts initialize : Fail to make message");
		return TTS_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_TTSC, ">>>> Request tts initialize : uid(%d)", uid);
	}

	int pid = getpid();
	if (true != dbus_message_append_args(msg, DBUS_TYPE_INT32, &pid, DBUS_TYPE_INT32, &uid,	DBUS_TYPE_INVALID)) {
		dbus_message_unref(msg);
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to append args"); 

		return TTS_ERROR_OPERATION_FAILED;
	}

	DBusMessage* result_msg;
	int result = TTS_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn_sender, msg, WAITING_TIME, &err);
	dbus_message_unref(msg);
	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Send error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err,
				  DBUS_TYPE_INT32, &result,
				  DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) { 
			SLOG(LOG_ERROR, TAG_TTSC, "<<<< Get arguments error (%s)", err.message);
			dbus_error_free(&err); 
			result = TTS_ERROR_OPERATION_FAILED;
		}

		dbus_message_unref(result_msg);

		if (0 == result) {
			SLOG(LOG_DEBUG, TAG_TTSC, "<<<< tts initialize : result = %d", result);
		} else {
			SLOG(LOG_ERROR, TAG_TTSC, "<<<< tts initialize : result = %d", result);
		}
	} else {
		SLOG(LOG_ERROR, TAG_TTSC, "<<<< Result message is NULL ");
		tts_dbus_reconnect();
		result = TTS_ERROR_TIMED_OUT;
	}

	return result;
}


int tts_dbus_request_finalize(int uid)
{
	DBusMessage* msg;
	DBusError err;
	dbus_error_init(&err);

	msg = __tts_dbus_make_message(uid, TTS_METHOD_FINALIZE);

	if (NULL == msg) { 
		SLOG(LOG_ERROR, TAG_TTSC, ">>>> Request tts finalize : Fail to make message"); 
		return TTS_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_TTSC, ">>>> Request tts finalize : uid(%d)", uid);
	}

	if (true != dbus_message_append_args(msg, DBUS_TYPE_INT32, &uid, DBUS_TYPE_INVALID)) {
		dbus_message_unref(msg);
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to append args"); 

		return TTS_ERROR_OPERATION_FAILED;
	}

	DBusMessage* result_msg;
	int result = TTS_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn_sender, msg, WAITING_TIME, &err);
	dbus_message_unref(msg);
	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Send error %s", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err, DBUS_TYPE_INT32, &result, DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) { 
			SLOG(LOG_ERROR, TAG_TTSC, "<<<< Get arguments error (%s)", err.message);
			dbus_error_free(&err); 
			result = TTS_ERROR_OPERATION_FAILED;
		}

		dbus_message_unref(result_msg);

		if (0 == result) {
			SLOG(LOG_DEBUG, TAG_TTSC, "<<<< tts finalize : result = %d", result);
		} else {
			SLOG(LOG_ERROR, TAG_TTSC, "<<<< tts finalize : result = %d", result);
		}
	} else {
		SLOG(LOG_ERROR, TAG_TTSC, "<<<< Result message is NULL ");
		tts_dbus_reconnect();
		result = TTS_ERROR_TIMED_OUT;
	}

	return result;
}

int tts_dbus_request_add_text(int uid, const char* text, const char* lang, int vctype, int speed, int uttid)
{
	if (NULL == text || NULL == lang) {
		SLOG(LOG_ERROR, TAG_TTSC, "Input parameter is NULL");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	DBusMessage* msg;
	DBusError err;
	dbus_error_init(&err);

	msg = __tts_dbus_make_message(uid, TTS_METHOD_ADD_QUEUE);

	if (NULL == msg) { 
		SLOG(LOG_ERROR, TAG_TTSC, ">>>> Request tts add text : Fail to make message"); 
		return TTS_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_TTSC, ">>>> Request tts add text : uid(%d), text(%s), lang(%s), type(%d), speed(%d), id(%d)", 
			uid, text, lang, vctype, speed, uttid);
	}

	if (true != dbus_message_append_args( msg, 
		DBUS_TYPE_INT32, &uid,
		DBUS_TYPE_STRING, &text,
		DBUS_TYPE_STRING, &lang,
		DBUS_TYPE_INT32, &vctype,
		DBUS_TYPE_INT32, &speed,
		DBUS_TYPE_INT32, &uttid,
		DBUS_TYPE_INVALID)) {
		dbus_message_unref(msg);
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to append args"); 

		return TTS_ERROR_OPERATION_FAILED;
	}

	DBusMessage* result_msg;
	int result = TTS_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn_sender, msg, WAITING_TIME, &err);
	dbus_message_unref(msg);
	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Send error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err,
			DBUS_TYPE_INT32, &result,
			DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) { 
			SLOG(LOG_ERROR, TAG_TTSC, "<<<< tts add text : Get arguments error (%s)", err.message);
			dbus_error_free(&err); 
			result = TTS_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);
		
		if (0 == result) {
			SLOG(LOG_DEBUG, TAG_TTSC, "<<<< tts add text : result(%d)", result);
		} else {
			SLOG(LOG_ERROR, TAG_TTSC, "<<<< tts add text : result(%d)", result);
		}	
	} else {
		SLOG(LOG_ERROR, TAG_TTSC, "<<<< Result message is NULL ");
		tts_dbus_reconnect();
		result = TTS_ERROR_TIMED_OUT;
	}

	return result;
}

int tts_dbus_request_play(int uid) 
{
	DBusMessage* msg;
	DBusError err;
	dbus_error_init(&err);

	msg = __tts_dbus_make_message(uid, TTS_METHOD_PLAY);

	if (NULL == msg) { 
		SLOG(LOG_ERROR, TAG_TTSC, ">>>> Request tts play : Fail to make message"); 
		return TTS_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_TTSC, ">>>> Request tts play : uid(%d)", uid);
	}
	
	if (true != dbus_message_append_args(msg, DBUS_TYPE_INT32, &uid, DBUS_TYPE_INVALID)) {
		dbus_message_unref(msg);
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to append args"); 

		return TTS_ERROR_OPERATION_FAILED;
	}

	DBusMessage* result_msg;
	int result = TTS_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn_sender, msg, WAITING_TIME, &err);
	dbus_message_unref(msg);
	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Send error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err,
			DBUS_TYPE_INT32, &result,
			DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) { 
			SLOG(LOG_ERROR, TAG_TTSC, "<<<< tts play : Get arguments error (%s)", err.message);
			dbus_error_free(&err); 
			result = TTS_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);

		if (0 == result) {
			SLOG(LOG_DEBUG, TAG_TTSC, "<<<< tts play : result(%d)", result);
		} else {
			SLOG(LOG_ERROR, TAG_TTSC, "<<<< tts play : result(%d)", result);
		}
	} else {
		SLOG(LOG_ERROR, TAG_TTSC, "<<<< Result message is NULL ");
		tts_dbus_reconnect();
		result = TTS_ERROR_TIMED_OUT;
	}

	return result;
}

int tts_dbus_request_stop(int uid)
{
	DBusMessage* msg;
	DBusError err;
	dbus_error_init(&err);

	msg = __tts_dbus_make_message(uid, TTS_METHOD_STOP);

	if (NULL == msg) { 
		SLOG(LOG_ERROR, TAG_TTSC, ">>>> Request tts stop : Fail to make message"); 
		return TTS_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_TTSC, ">>>> Request tts stop : uid(%d)", uid);
	}

	DBusMessage* result_msg;
	int result = TTS_ERROR_OPERATION_FAILED;

	if (true != dbus_message_append_args(msg, DBUS_TYPE_INT32, &uid, DBUS_TYPE_INVALID)) {
		dbus_message_unref(msg);
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to append args"); 

		return TTS_ERROR_OPERATION_FAILED;
	}

	result_msg = dbus_connection_send_with_reply_and_block(g_conn_sender, msg, WAITING_TIME, &err);
	dbus_message_unref(msg);
	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Send error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err,
			DBUS_TYPE_INT32, &result,
			DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) { 
			SLOG(LOG_ERROR, TAG_TTSC, "<<<< tts stop : Get arguments error (%s)", err.message);
			dbus_error_free(&err); 
			result = TTS_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);

		if (0 == result) {
			SLOG(LOG_DEBUG, TAG_TTSC, "<<<< tts stop : result(%d)", result);
		} else {
			SLOG(LOG_ERROR, TAG_TTSC, "<<<< tts stop : result(%d)", result);
		}
	} else {
		SLOG(LOG_ERROR, TAG_TTSC, "<<<< Result message is NULL ");
		tts_dbus_reconnect();
		result = TTS_ERROR_TIMED_OUT;
	}

	return result;
}

int tts_dbus_request_pause(int uid)
{
	DBusMessage* msg;
	DBusError err;
	dbus_error_init(&err);

	msg = __tts_dbus_make_message(uid, TTS_METHOD_PAUSE);

	if (NULL == msg) { 
		SLOG(LOG_ERROR, TAG_TTSC, ">>>> Request tts pause : Fail to make message"); 
		return TTS_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_TTSC, ">>>> Request tts pause : uid(%d)", uid);
	}

	DBusMessage* result_msg;
	int result = TTS_ERROR_OPERATION_FAILED;

	if (true != dbus_message_append_args(msg, DBUS_TYPE_INT32, &uid, DBUS_TYPE_INVALID)) {
		dbus_message_unref(msg);
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to append args"); 

		return TTS_ERROR_OPERATION_FAILED;
	}

	result_msg = dbus_connection_send_with_reply_and_block(g_conn_sender, msg, WAITING_TIME, &err);
	dbus_message_unref(msg);
	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Send error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err,
			DBUS_TYPE_INT32, &result,
			DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) { 
			SLOG(LOG_ERROR, TAG_TTSC, "<<<< tts pause : Get arguments error (%s)", err.message);
			dbus_error_free(&err); 
			result = TTS_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);

		if (0 == result) {
			SLOG(LOG_DEBUG, TAG_TTSC, "<<<< tts pause : result(%d)", result);
		} else {
			SLOG(LOG_ERROR, TAG_TTSC, "<<<< tts pause : result(%d)", result);
		}
	} else {
		SLOG(LOG_ERROR, TAG_TTSC, "<<<< Result message is NULL ");
		tts_dbus_reconnect();
		result = TTS_ERROR_TIMED_OUT;
	}

	return result;
}
