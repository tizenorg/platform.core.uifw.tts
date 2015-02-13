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

#include <dbus/dbus.h>
#include <dirent.h>
#include <dlfcn.h>
#include <Ecore.h>

#include "tts_config_mgr.h"
#include "ttsd_main.h"
#include "ttsd_dbus_server.h"
#include "ttsd_dbus.h"
#include "ttsd_server.h"


static DBusConnection* g_conn;

static int g_waiting_time = 3000;

static char *g_service_name;
static char *g_service_object;
static char *g_service_interface;

const char* __ttsd_get_error_code(ttsd_error_e err)
{
	switch(err) {
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
	default:
		return "Invalid error code";
	}

	return NULL;
}

int ttsdc_send_hello(int pid, int uid)
{
	if (NULL == g_conn) { 
		SLOG(LOG_ERROR, get_tag(), "[Dbus ERROR] Dbus connection is not available" );
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
		SECURE_SLOG(LOG_ERROR, get_tag(), "<<<< [Dbus ERROR] Fail to create hello message : uid(%d)", uid); 
		return -1;
	} else {
		SECURE_SLOG(LOG_DEBUG, get_tag(), "<<<< [Dbus] Send hello message : uid(%d)", uid);
	}

	dbus_message_append_args(msg, DBUS_TYPE_INT32, &uid, DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = -1;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn, msg, g_waiting_time, &err);
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
}

int ttsdc_send_message(int pid, int uid, int data, const char *method)
{
	char* send_filename;
	send_filename = tts_config_get_message_path((int)ttsd_get_mode(), pid);

	if (NULL == send_filename) {
		SLOG(LOG_ERROR, get_tag(), "[Message ERROR] Fail to get message file path");
		return -1;
	}

	FILE* fp;
	fp = fopen(send_filename, "a+");

	if (NULL != send_filename) {
		free(send_filename);
	}

	if (NULL == fp) {
		SLOG(LOG_ERROR, get_tag(), "[File message ERROR] Fail to open message file");
		return -1;
	}
	SECURE_SLOG(LOG_DEBUG, get_tag(), "[File message] Write send file - %s, uid=%d, send_data=%d", method, uid, data);

	fprintf(fp, "%s %d %d\n", method, uid, data);

	fclose(fp);

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

int ttsdc_send_error_message(int pid, int uid, int uttid, int reason)
{
	if (NULL == g_conn) { 
		SLOG(LOG_ERROR, get_tag(), "[Dbus ERROR] Dbus connection is not available" );
		return -1;
	}

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
		TTSD_METHOD_ERROR);

	if (NULL == msg) { 
		SECURE_SLOG(LOG_ERROR, get_tag(), "[Dbus ERROR] Fail to create error message : uid(%d)", uid); 
		return -1;
	}

	dbus_message_append_args( msg, 
		DBUS_TYPE_INT32, &uid, 
		DBUS_TYPE_INT32, &uttid, 
		DBUS_TYPE_INT32, &reason, 
		DBUS_TYPE_INVALID);
	
	if (!dbus_connection_send(g_conn, msg, NULL)) {
		SLOG(LOG_ERROR, get_tag(), "[Dbus ERROR] <<<< error message : Out Of Memory !"); 
	} else {
		SECURE_SLOG(LOG_DEBUG, get_tag(), "<<<< Send error signal : uid(%d), reason(%s), uttid(%d)", 
			uid, __ttsd_get_error_code(reason), uttid);
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

	if (true != dbus_connection_get_is_connected(conn)) {
		SLOG(LOG_ERROR, get_tag(), "[ERROR] Connection is disconnected");
		return ECORE_CALLBACK_RENEW;
	}

	/* loop again if we haven't read a message */
	if (NULL == msg) {
		return ECORE_CALLBACK_RENEW;
	}

	/* client event */
	if (dbus_message_is_method_call(msg, g_service_interface, TTS_METHOD_HELLO)) {
		ttsd_dbus_server_hello(conn, msg);

	} else if (dbus_message_is_method_call(msg, g_service_interface, TTS_METHOD_INITIALIZE)) {
		ttsd_dbus_server_initialize(conn, msg);
	
	} else if (dbus_message_is_method_call(msg, g_service_interface, TTS_METHOD_FINALIZE)) {
		ttsd_dbus_server_finalize(conn, msg);
	
	} else if (dbus_message_is_method_call(msg, g_service_interface, TTS_METHOD_GET_SUPPORT_VOICES)) {
		ttsd_dbus_server_get_support_voices(conn, msg);

	} else if (dbus_message_is_method_call(msg, g_service_interface, TTS_METHOD_GET_CURRENT_VOICE)) {
		ttsd_dbus_server_get_current_voice(conn, msg);

	} else if (dbus_message_is_method_call(msg, g_service_interface, TTS_METHOD_ADD_QUEUE)) {
		ttsd_dbus_server_add_text(conn, msg);

	} else if (dbus_message_is_method_call(msg, g_service_interface, TTS_METHOD_PLAY)) {
		ttsd_dbus_server_play(conn, msg);
	
	} else if (dbus_message_is_method_call(msg, g_service_interface, TTS_METHOD_STOP)) {
		ttsd_dbus_server_stop(conn, msg);

	} else if (dbus_message_is_method_call(msg, g_service_interface, TTS_METHOD_PAUSE)) {
		ttsd_dbus_server_pause(conn, msg);

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

	/* connect to the bus and check for errors */
	g_conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, get_tag(), "[Dbus ERROR] Fail dbus_bus_get : %s", err.message);
		dbus_error_free(&err); 
	}

	if (NULL == g_conn) { 
		SLOG(LOG_ERROR, get_tag(), "[Dbus ERROR] Fail to get dbus connection" );
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
		snprintf(g_service_interface, strlen(TTS_SERVER_SERVICE_INTERFACE)+ 1, "%s", TTS_SERVER_SERVICE_INTERFACE);
	}

	/* request our name on the bus and check for errors */
	ret = dbus_bus_request_name(g_conn, g_service_name, DBUS_NAME_FLAG_REPLACE_EXISTING, &err);

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
	snprintf(rule, 128, "type='signal',interface='%s'", g_service_interface);

	/* add a rule for which messages we want to see */
	dbus_bus_add_match(g_conn, rule, &err); /* see signals from the given interface */
	dbus_connection_flush(g_conn);

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, get_tag(), "[Dbus ERROR] dbus_bus_add_match() : %s", err.message);
		return -1; 
	}

	int fd = 0;
	dbus_connection_get_unix_fd(g_conn, &fd);

	Ecore_Fd_Handler* fd_handler = NULL;
	fd_handler = ecore_main_fd_handler_add(fd, ECORE_FD_READ, (Ecore_Fd_Cb)listener_event_callback, g_conn, NULL, NULL);
	if (NULL == fd_handler) {
		SLOG(LOG_ERROR, get_tag(), "[Dbus ERROR] Fail to get fd handler");
		return -1;
	}

	return 0;
}

int ttsd_dbus_close_connection()
{
	DBusError err;
	dbus_error_init(&err);

	dbus_bus_release_name (g_conn, g_service_name, &err);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, get_tag(), "[Dbus ERROR] dbus_bus_release_name() : %s", err.message); 
		dbus_error_free(&err); 
		return -1;
	}

	if (NULL != g_service_name)	free(g_service_name);
	if (NULL != g_service_object)	free(g_service_object);
	if (NULL != g_service_interface)free(g_service_interface);

	return 0;
}

int ttsd_file_msg_open_connection(int pid)
{
	/* Make file for Inotify */
	char* send_filename;
	send_filename = tts_config_get_message_path((int)ttsd_get_mode(), pid);

	if (NULL == send_filename) {
		SLOG(LOG_ERROR, get_tag(), "[Message ERROR] Fail to get message file path");
		return -1;
	}

	FILE* fp;
	fp = fopen(send_filename, "a+");
	if (NULL == fp) {
		SLOG(LOG_ERROR, get_tag(), "[File message ERROR] Fail to make message file");
		if (NULL != send_filename)	free(send_filename);
		return -1;
	}
	fclose(fp);

	if (NULL != send_filename)	free(send_filename);
	return 0;
}

int ttsd_file_msg_close_connection(int pid)
{
	/* delete inotify file */
	char* path = NULL;
	path = tts_config_get_message_path((int)ttsd_get_mode(), pid);

	if (NULL == path) {
		SLOG(LOG_ERROR, get_tag(), "[Message ERROR] Fail to get message file path");
		return -1;
	}

	if (0 != remove(path)) {
		SLOG(LOG_WARN, get_tag(), "[File message WARN] Fail to remove message file");
	}

	if (NULL != path)	free(path);
	return 0;
}

int ttsd_file_clean_up()
{
	SLOG(LOG_DEBUG, get_tag(), "== Old message file clean up == ");

	DIR *dp = NULL;
	int ret = -1;
	struct dirent entry;
	struct dirent *dirp = NULL;

	dp = opendir(MESSAGE_FILE_PATH_ROOT);
	if (dp == NULL) {
		SLOG(LOG_ERROR, get_tag(), "[File message WARN] Fail to open path : %s", MESSAGE_FILE_PATH_ROOT);
		return -1;
	}

	char prefix[36] = {0, };
	char remove_path[256] = {0, };

	switch(ttsd_get_mode()) {
	case TTSD_MODE_DEFAULT:		snprintf(prefix, 36, "%s", MESSAGE_FILE_PREFIX_DEFAULT);		break;
	case TTSD_MODE_NOTIFICATION:	snprintf(prefix, 36, "%s", MESSAGE_FILE_PREFIX_NOTIFICATION);	break;
	case TTSD_MODE_SCREEN_READER:	snprintf(prefix, 36, "%s", MESSAGE_FILE_PREFIX_SCREEN_READER);	break;
	default:
		SLOG(LOG_ERROR, get_tag(), "[File ERROR] Fail to get mode : %d", ttsd_get_mode());
		closedir(dp);
		return -1;
	}

	do {
		ret = readdir_r(dp, &entry, &dirp);
		if (0 != ret) {
			SLOG(LOG_ERROR, get_tag(), "[File ERROR] Fail to read directory");
			break;
		}

		if (NULL != dirp) {
			if (!strncmp(prefix, dirp->d_name, strlen(prefix))) {
				memset(remove_path, 0, 256);
				snprintf(remove_path, 256, "%s%s", MESSAGE_FILE_PATH_ROOT, dirp->d_name);

				/* Clean up code */
				if (0 != remove(remove_path)) {
					SLOG(LOG_WARN, get_tag(), "[File message WARN] Fail to remove message file : %s", remove_path);
				} else {
					SLOG(LOG_DEBUG, get_tag(), "[File message] Remove message file : %s", remove_path);
				}
			}
		}
	} while (NULL != dirp);

	closedir(dp);

	return 0;
}