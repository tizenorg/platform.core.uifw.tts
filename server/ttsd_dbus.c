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

#include <dbus/dbus.h>
#include <dirent.h>
#include <dlfcn.h>
#include <Ecore.h>

#include "tts_config_mgr.h"
#include "ttsd_main.h"
#include "ttsd_dbus_server.h"
#include "ttsd_dbus.h"
#include "ttsd_server.h"

static DBusConnection* g_conn_sender = NULL;
static DBusConnection* g_conn_listener = NULL;

static Ecore_Fd_Handler* g_dbus_fd_handler = NULL;

//static int g_waiting_time = 3000;

static char *g_service_name = NULL;
static char *g_service_object = NULL;
static char *g_service_interface = NULL;

const char* __ttsd_get_error_code(ttsd_error_e err)
{
	switch (err) {
	case TTSD_ERROR_NONE:			return "TTS_ERROR_NONE";
	case TTSD_ERROR_OUT_OF_MEMORY:		return "TTS_ERROR_OUT_OF_MEMORY";
	case TTSD_ERROR_IO_ERROR:		return "TTS_ERROR_IO_ERROR";
	case TTSD_ERROR_INVALID_PARAMETER:	return "TTS_ERROR_INVALID_PARAMETER";
	case TTSD_ERROR_OUT_OF_NETWORK:		return "TTS_ERROR_OUT_OF_NETWORK";
	case TTSD_ERROR_INVALID_STATE:		return "TTS_ERROR_INVALID_STATE";
	case TTSD_ERROR_INVALID_VOICE:		return "TTS_ERROR_INVALID_VOICE";
	case TTSD_ERROR_ENGINE_NOT_FOUND:	return "TTS_ERROR_ENGINE_NOT_FOUND";
	case TTSD_ERROR_TIMED_OUT:		return "TTS_ERROR_TIMED_OUT";
	case TTSD_ERROR_OPERATION_FAILED:	return "TTS_ERROR_OPERATION_FAILED";
	case TTSD_ERROR_AUDIO_POLICY_BLOCKED:	return "TTS_ERROR_AUDIO_POLICY_BLOCKED";
	case TTSD_ERROR_NOT_SUPPORTED_FEATURE:	return "TTSD_ERROR_NOT_SUPPORTED_FEATURE";
	default:
		return "Invalid error code";
	}

	return NULL;
}

int ttsdc_send_hello(int pid, int uid)
{
#if 0
	if (NULL == g_conn_sender) {
		SLOG(LOG_ERROR, get_tag(), "[Dbus ERROR] Dbus connection is not available");
		return -1;
	}

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
		TTSD_METHOD_HELLO);

	if (NULL == msg) {
		SLOG(LOG_ERROR, get_tag(), "<<<< [Dbus ERROR] Fail to create hello message : uid(%d)", uid);
		return -1;
	} else {
		SLOG(LOG_DEBUG, get_tag(), "<<<< [Dbus] Send hello message : uid(%d)", uid);
	}

	dbus_message_append_args(msg, DBUS_TYPE_INT32, &uid, DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = -1;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn_sender, msg, g_waiting_time, &err);
	dbus_message_unref(msg);
	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, get_tag(), "[ERROR] Send error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err, DBUS_TYPE_INT32, &result, DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) {
			SLOG(LOG_ERROR, get_tag(), ">>>> [Dbus] Get arguments error (%s)", err.message);
			dbus_error_free(&err);
			result = -1;
		}

		dbus_message_unref(result_msg);
	} else {
		SLOG(LOG_DEBUG, get_tag(), ">>>> [Dbus] Result message is NULL. Client is not available");
		result = 0;
	}

	return result;
#endif
	return 0;
}

int ttsdc_send_message(int pid, int uid, int data, const char *method)
{
	if (NULL == g_conn_sender) {
		SLOG(LOG_ERROR, get_tag(), "[Dbus ERROR] Dbus connection is not available");
		return -1;
	}

	DBusMessage* msg = NULL;

	/* create a message */
	msg = dbus_message_new_signal(
		TTS_CLIENT_SERVICE_OBJECT_PATH,	/* object name of the signal */
		TTS_CLIENT_SERVICE_INTERFACE,	/* interface name of the signal */
		method);			/* name of the signal */

	if (NULL == msg) {
		SLOG(LOG_ERROR, get_tag(), "<<<< [Dbus ERROR] Fail to create message : %s", method);
		return -1;
	} else {
		SLOG(LOG_DEBUG, get_tag(), "<<<< [Dbus] Send %s message : uid(%d) data(%d)", method, uid, data);
	}

	dbus_message_append_args(msg, DBUS_TYPE_INT32, &uid, DBUS_TYPE_INT32, &data, DBUS_TYPE_INVALID);

	if (1 != dbus_connection_send(g_conn_sender, msg, NULL)) {
		SLOG(LOG_ERROR, get_tag(), "[Dbus ERROR] Fail to Send");
		return -1;
	} else {
		SLOG(LOG_DEBUG, get_tag(), "[Dbus] SUCCESS Send");
		dbus_connection_flush(g_conn_sender);
	}

	dbus_message_unref(msg);

	return 0;
}

int ttsdc_send_utt_start_message(int pid, int uid, int uttid)
{
	return ttsdc_send_message(pid, uid, uttid, TTSD_METHOD_UTTERANCE_STARTED);
}

int ttsdc_send_utt_finish_message(int pid, int uid, int uttid)
{
	return ttsdc_send_message(pid, uid, uttid, TTSD_METHOD_UTTERANCE_COMPLETED);
}

int ttsdc_send_set_state_message(int pid, int uid, int state)
{
	return ttsdc_send_message(pid, uid, state, TTSD_METHOD_SET_STATE);
}

int ttsdc_send_error_message(int pid, int uid, int uttid, int reason, char* err_msg)
{
	if (NULL == g_conn_sender) {
		SLOG(LOG_ERROR, get_tag(), "[Dbus ERROR] Dbus connection is not available");
		return -1;
	}

	DBusMessage* msg = NULL;

	/* create a message */
	msg = dbus_message_new_signal(
		TTS_CLIENT_SERVICE_OBJECT_PATH,	/* object name of the signal */
		TTS_CLIENT_SERVICE_INTERFACE,	/* interface name of the signal */
		TTSD_METHOD_ERROR);		/* name of the signal */

	if (NULL == msg) {
		SLOG(LOG_ERROR, get_tag(), "[Dbus ERROR] Fail to create error message : uid(%d)", uid);
		return -1;
	}

	dbus_message_append_args(msg,
		DBUS_TYPE_INT32, &uid,
		DBUS_TYPE_INT32, &uttid,
		DBUS_TYPE_INT32, &reason,
		DBUS_TYPE_INT32, &err_msg,
		DBUS_TYPE_INVALID);

	dbus_message_set_no_reply(msg, TRUE);

	if (!dbus_connection_send(g_conn_sender, msg, NULL)) {
		SLOG(LOG_ERROR, get_tag(), "[Dbus ERROR] <<<< error message : Out Of Memory !");
	} else {
		SLOG(LOG_DEBUG, get_tag(), "<<<< Send error message : uid(%d), reason(%s), uttid(%d), err_msg(%d)",
			 uid, __ttsd_get_error_code(reason), uttid, (NULL == err_msg) ? "NULL" : err_msg);
		dbus_connection_flush(g_conn_sender);
	}

	dbus_message_unref(msg);

	return 0;
}

static Eina_Bool listener_event_callback(void* data, Ecore_Fd_Handler *fd_handler)
{
	if (NULL == g_conn_listener)	return ECORE_CALLBACK_RENEW;

	dbus_connection_read_write_dispatch(g_conn_listener, 50);

	DBusMessage* msg = NULL;
	msg = dbus_connection_pop_message(g_conn_listener);

	if (true != dbus_connection_get_is_connected(g_conn_listener)) {
		SLOG(LOG_ERROR, get_tag(), "[ERROR] Connection is disconnected");
		return ECORE_CALLBACK_RENEW;
	}

	/* loop again if we haven't read a message */
	if (NULL == msg) {
		return ECORE_CALLBACK_RENEW;
	}

	/* client event */
	if (dbus_message_is_method_call(msg, g_service_interface, TTS_METHOD_HELLO)) {
		ttsd_dbus_server_hello(g_conn_listener, msg);

	} else if (dbus_message_is_method_call(msg, g_service_interface, TTS_METHOD_INITIALIZE)) {
		ttsd_dbus_server_initialize(g_conn_listener, msg);

	} else if (dbus_message_is_method_call(msg, g_service_interface, TTS_METHOD_FINALIZE)) {
		ttsd_dbus_server_finalize(g_conn_listener, msg);

	} else if (dbus_message_is_method_call(msg, g_service_interface, TTS_METHOD_GET_SUPPORT_VOICES)) {
		ttsd_dbus_server_get_support_voices(g_conn_listener, msg);

	} else if (dbus_message_is_method_call(msg, g_service_interface, TTS_METHOD_GET_CURRENT_VOICE)) {
		ttsd_dbus_server_get_current_voice(g_conn_listener, msg);

	} else if (dbus_message_is_method_call(msg, g_service_interface, TTS_METHOD_ADD_QUEUE)) {
		ttsd_dbus_server_add_text(g_conn_listener, msg);

	} else if (dbus_message_is_method_call(msg, g_service_interface, TTS_METHOD_PLAY)) {
		ttsd_dbus_server_play(g_conn_listener, msg);

	} else if (dbus_message_is_method_call(msg, g_service_interface, TTS_METHOD_STOP)) {
		ttsd_dbus_server_stop(g_conn_listener, msg);

	} else if (dbus_message_is_method_call(msg, g_service_interface, TTS_METHOD_PAUSE)) {
		ttsd_dbus_server_pause(g_conn_listener, msg);

	} else if (dbus_message_is_method_call(msg, g_service_interface, TTS_METHOD_SET_PRIVATE_DATA)) {
		ttsd_dbus_server_set_private_data(g_conn_listener, msg);

	} else if (dbus_message_is_method_call(msg, g_service_interface, TTS_METHOD_GET_PRIVATE_DATA)) {
		ttsd_dbus_server_get_private_data(g_conn_listener, msg);

	} else {
		/* Invalid method */
	}

	/* free the message */
	dbus_message_unref(msg);

	return ECORE_CALLBACK_RENEW;
}

int ttsd_dbus_open_connection()
{
	DBusError err;
	dbus_error_init(&err);

	int ret;

	/* Create connection for sender */
	g_conn_sender = dbus_bus_get_private(DBUS_BUS_SESSION, &err);
	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, get_tag(), "[Dbus ERROR] Fail dbus_bus_get : %s", err.message);
		dbus_error_free(&err);
	}

	if (NULL == g_conn_sender) {
		SLOG(LOG_ERROR, get_tag(), "[Dbus ERROR] Fail to get dbus connection sender");
		return -1;
	}

	/* connect to the bus and check for errors */
	g_conn_listener = dbus_bus_get_private(DBUS_BUS_SESSION, &err);
	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, get_tag(), "[Dbus ERROR] Fail dbus_bus_get : %s", err.message);
		dbus_error_free(&err);
		return -1;
	}

	if (NULL == g_conn_listener) {
		SLOG(LOG_ERROR, get_tag(), "[Dbus ERROR] Fail to get dbus connection");
		return -1;
	}

	if (TTSD_MODE_SCREEN_READER == ttsd_get_mode()) {
		g_service_name = (char*)calloc(strlen(TTS_SR_SERVER_SERVICE_NAME) + 1, sizeof(char));
		g_service_object = (char*)calloc(strlen(TTS_SR_SERVER_SERVICE_OBJECT_PATH) + 1, sizeof(char));
		g_service_interface = (char*)calloc(strlen(TTS_SR_SERVER_SERVICE_INTERFACE) + 1, sizeof(char));

		snprintf(g_service_name, strlen(TTS_SR_SERVER_SERVICE_NAME) + 1, "%s", TTS_SR_SERVER_SERVICE_NAME);
		snprintf(g_service_object, strlen(TTS_SR_SERVER_SERVICE_OBJECT_PATH) + 1, "%s", TTS_SR_SERVER_SERVICE_OBJECT_PATH);
		snprintf(g_service_interface, strlen(TTS_SR_SERVER_SERVICE_INTERFACE) + 1, "%s", TTS_SR_SERVER_SERVICE_INTERFACE);
	} else if (TTSD_MODE_NOTIFICATION == ttsd_get_mode()) {
		g_service_name = (char*)calloc(strlen(TTS_NOTI_SERVER_SERVICE_NAME) + 1, sizeof(char));
		g_service_object = (char*)calloc(strlen(TTS_NOTI_SERVER_SERVICE_OBJECT_PATH) + 1, sizeof(char));
		g_service_interface = (char*)calloc(strlen(TTS_NOTI_SERVER_SERVICE_INTERFACE) + 1, sizeof(char));

		snprintf(g_service_name, strlen(TTS_NOTI_SERVER_SERVICE_NAME) + 1, "%s", TTS_NOTI_SERVER_SERVICE_NAME);
		snprintf(g_service_object, strlen(TTS_NOTI_SERVER_SERVICE_OBJECT_PATH) + 1, "%s", TTS_NOTI_SERVER_SERVICE_OBJECT_PATH);
		snprintf(g_service_interface, strlen(TTS_NOTI_SERVER_SERVICE_INTERFACE) + 1, "%s", TTS_NOTI_SERVER_SERVICE_INTERFACE);
	} else {
		g_service_name = (char*)calloc(strlen(TTS_SERVER_SERVICE_NAME) + 1, sizeof(char));
		g_service_object = (char*)calloc(strlen(TTS_SERVER_SERVICE_OBJECT_PATH) + 1, sizeof(char));
		g_service_interface = (char*)calloc(strlen(TTS_SERVER_SERVICE_INTERFACE) + 1, sizeof(char));

		snprintf(g_service_name, strlen(TTS_SERVER_SERVICE_NAME) + 1, "%s", TTS_SERVER_SERVICE_NAME);
		snprintf(g_service_object, strlen(TTS_SERVER_SERVICE_OBJECT_PATH) + 1, "%s", TTS_SERVER_SERVICE_OBJECT_PATH);
		snprintf(g_service_interface, strlen(TTS_SERVER_SERVICE_INTERFACE) + 1, "%s", TTS_SERVER_SERVICE_INTERFACE);
	}

	/* request our name on the bus and check for errors */
	ret = dbus_bus_request_name(g_conn_listener, g_service_name, DBUS_NAME_FLAG_REPLACE_EXISTING, &err);
	if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) {
		SLOG(LOG_ERROR, get_tag(), "[Dbus ERROR] Fail to be primary owner");
		return -1;
	}

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, get_tag(), "[Dbus ERROR] Fail to request dbus name : %s", err.message);
		dbus_error_free(&err);
		return -1;
	}

	/* add a rule for getting signal */
	char rule[128];
	snprintf(rule, 128, "type='method_call',interface='%s'", g_service_interface);

	/* add a rule for which messages we want to see */
	dbus_bus_add_match(g_conn_listener, rule, &err);/* see signals from the given interface */
	dbus_connection_flush(g_conn_listener);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, get_tag(), "[Dbus ERROR] dbus_bus_add_match() : %s", err.message);
		return -1;
	}

	int fd = 0;
	dbus_connection_get_unix_fd(g_conn_listener, &fd);

	g_dbus_fd_handler = ecore_main_fd_handler_add(fd, ECORE_FD_READ, (Ecore_Fd_Cb)listener_event_callback, g_conn_listener, NULL, NULL);
	if (NULL == g_dbus_fd_handler) {
		SLOG(LOG_ERROR, get_tag(), "[Dbus ERROR] Fail to get fd handler");
		return -1;
	}

	return 0;
}

int ttsd_dbus_close_connection()
{
	DBusError err;
	dbus_error_init(&err);

	if (NULL != g_dbus_fd_handler) {
		ecore_main_fd_handler_del(g_dbus_fd_handler);
		g_dbus_fd_handler = NULL;
	}

	dbus_bus_release_name(g_conn_listener, g_service_name, &err);
	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, get_tag(), "[Dbus ERROR] dbus_bus_release_name() : %s", err.message);
		dbus_error_free(&err);
	}

	dbus_connection_close(g_conn_sender);
	dbus_connection_close(g_conn_listener);

	dbus_connection_unref(g_conn_sender);
	dbus_connection_unref(g_conn_listener);

	g_conn_listener = NULL;
	g_conn_sender = NULL;

	if (NULL != g_service_name)	free(g_service_name);
	if (NULL != g_service_object)	free(g_service_object);
	if (NULL != g_service_interface)free(g_service_interface);

	return 0;
}
