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


#include "tts_setting.h"
#include "tts_main.h"
#include "tts_setting_dbus.h"


static DBusConnection* g_conn = NULL;

int tts_setting_dbus_open_connection()
{
	if (NULL != g_conn) {
		SLOG(LOG_WARN, TAG_TTSC, "Already existed connection");
		return 0;
	}

	DBusError err;
	int ret;

	/* initialise the error value */
	dbus_error_init(&err);

	/* connect to the DBUS system bus, and check for errors */
	g_conn = dbus_bus_get_private(DBUS_BUS_SYSTEM, &err);

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_TTSC, "Dbus Connection Error (%s)\n", err.message); 
		dbus_error_free(&err); 
	}

	if (NULL == g_conn) {
		SLOG(LOG_ERROR, TAG_TTSC, "Fail to get dbus connection \n");
		return TTS_SETTING_ERROR_OPERATION_FAILED; 
	}

	int pid = getpid();

	char service_name[64];
	memset(service_name, 0, 64);
	snprintf(service_name, 64, "%s%d", TTS_SETTING_SERVICE_NAME, pid);

	SLOG(LOG_DEBUG, TAG_TTSC, "service name is %s\n", service_name);

	/* register our name on the bus, and check for errors */
	ret = dbus_bus_request_name(g_conn, service_name, DBUS_NAME_FLAG_REPLACE_EXISTING , &err);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_TTSC, "Name Error (%s)\n", err.message); 
		dbus_error_free(&err); 
	}

	if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) {
		printf("fail dbus_bus_request_name()\n");
		return TTS_SETTING_ERROR_OPERATION_FAILED;
	}

	return 0;
}

int tts_setting_dbus_close_connection()
{
	DBusError err;
	dbus_error_init(&err);

	int pid = getpid();

	char service_name[64];
	memset(service_name, 0, 64);
	snprintf(service_name, 64, "%s%d", TTS_SETTING_SERVICE_NAME, pid);

	dbus_bus_release_name(g_conn, service_name, &err);

	dbus_connection_close(g_conn);

	g_conn = NULL;

	return 0;
}

int tts_setting_dbus_request_hello()
{
	DBusMessage* msg;

	msg = dbus_message_new_method_call(
		TTS_SERVER_SERVICE_NAME, 
		TTS_SERVER_SERVICE_OBJECT_PATH, 
		TTS_SERVER_SERVICE_INTERFACE, 
		TTS_SETTING_METHOD_HELLO);

	if (NULL == msg) { 
		SLOG(LOG_ERROR, TAG_TTSC, ">>>> Request setting hello : Fail to make message \n"); 
		return TTS_SETTING_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_TTSC, ">>>> Request setting hello");
	}

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg = NULL;
	int result = 0;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn, msg, 500, &err);

	dbus_message_unref(msg);

	if (NULL != result_msg) {
		dbus_message_unref(result_msg);

		SLOG(LOG_DEBUG, TAG_TTSC, "<<<< setting hello");
		result = 0;
	} else {
		SLOG(LOG_ERROR, TAG_TTSC, "<<<< setting hello : no response");
		result = -1;
	}

	return result;
}

int tts_setting_dbus_request_initialize()
{
	DBusMessage* msg;

	msg = dbus_message_new_method_call(
		TTS_SERVER_SERVICE_NAME, 
		TTS_SERVER_SERVICE_OBJECT_PATH, 
		TTS_SERVER_SERVICE_INTERFACE, 
		TTS_SETTING_METHOD_INITIALIZE);

	if (NULL == msg) { 
		SLOG(LOG_ERROR, TAG_TTSC, ">>>> Request setting initialize : Fail to make message \n"); 
		return TTS_SETTING_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_TTSC, ">>>> Request setting initialize");
	}

	int pid = getpid();

	dbus_message_append_args( msg, 
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = TTS_SETTING_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn, msg, 3000, &err);

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err, DBUS_TYPE_INT32, &result, DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) { 
			SLOG(LOG_ERROR, TAG_TTSC, "Get arguments error (%s)\n", err.message);
			dbus_error_free(&err); 
			result = TTS_SETTING_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);
	}

	if (0 == result) {
		SLOG(LOG_DEBUG, TAG_TTSC, "<<<< setting initialize : result = %d \n", result);
	} else {
		SLOG(LOG_ERROR, TAG_TTSC, "<<<< setting initialize : result = %d \n", result);
	}

	dbus_message_unref(msg);

	return result;
}

int tts_setting_dbus_request_finalilze()
{
	DBusMessage* msg;

	msg = dbus_message_new_method_call(
		TTS_SERVER_SERVICE_NAME, 
		TTS_SERVER_SERVICE_OBJECT_PATH, 
		TTS_SERVER_SERVICE_INTERFACE, 
		TTS_SETTING_METHOD_FINALIZE);

	if (NULL == msg) { 
		SLOG(LOG_ERROR, TAG_TTSC, ">>>> Request setting finalize : Fail to make message \n"); 
		return TTS_SETTING_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_TTSC, ">>>> Request setting finalize");
	}

	int pid = getpid();

	dbus_message_append_args(msg, DBUS_TYPE_INT32, &pid, DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = 0;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn, msg, 3000, &err);

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err,
			DBUS_TYPE_INT32, &result,
			DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) { 
			SLOG(LOG_ERROR, TAG_TTSC, "Get arguments error (%s)\n", err.message);
			dbus_error_free(&err); 
			result = TTS_SETTING_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);
	}

	if (0 == result) {
		SLOG(LOG_DEBUG, TAG_TTSC, "<<<< setting finalize : result = %d \n", result);
	} else {
		SLOG(LOG_ERROR, TAG_TTSC, "<<<< setting finalize : result = %d \n", result);
	}

	dbus_message_unref(msg);

	return result;
}

int tts_setting_dbus_request_get_engine_list(tts_setting_supported_engine_cb callback, void* user_data)
{
	DBusMessage* msg;

	msg = dbus_message_new_method_call(
		TTS_SERVER_SERVICE_NAME, 
		TTS_SERVER_SERVICE_OBJECT_PATH, 
		TTS_SERVER_SERVICE_INTERFACE, 
		TTS_SETTING_METHOD_GET_ENGINE_LIST);

	if (NULL == msg) { 
		SLOG(LOG_ERROR, TAG_TTSC, ">>>> Request setting get engine list : Fail to make message \n"); 
		return TTS_SETTING_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_TTSC, ">>>> Request setting get engine list");
	}

	int pid = getpid();

	dbus_message_append_args(msg, DBUS_TYPE_INT32, &pid, DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = TTS_SETTING_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn, msg, 3000, &err);
	
	if (NULL != result_msg) {
		DBusMessageIter args;
		
		if (dbus_message_iter_init(result_msg, &args)) {
			/* Get result */
			if (DBUS_TYPE_INT32 == dbus_message_iter_get_arg_type(&args)) {
				dbus_message_iter_get_basic(&args, &result);
				dbus_message_iter_next(&args);
			} 

			if (0 == result) {
				SLOG(LOG_DEBUG, TAG_TTSC, "<<<< setting get engine list : result = %d \n", result);

				int size ; 
				char* temp_id;
				char* temp_name;
				char* temp_path;

				/* Get engine count */
				if (DBUS_TYPE_INT32 == dbus_message_iter_get_arg_type(&args)) {
					dbus_message_iter_get_basic(&args, &size);
					dbus_message_iter_next(&args);
				}

				int i=0;
				for (i=0 ; i<size ; i++) {
					dbus_message_iter_get_basic(&args, &(temp_id));
					dbus_message_iter_next(&args);
				
					dbus_message_iter_get_basic(&args, &(temp_name));
					dbus_message_iter_next(&args);
				
					dbus_message_iter_get_basic(&args, &(temp_path));
					dbus_message_iter_next(&args);
				
					if (true != callback(temp_id, temp_name, temp_path, user_data)) {
						break;
					}
				}
			}  else {
				SLOG(LOG_ERROR, TAG_TTSC, "<<<< setting get engine list : result = %d \n", result);
			}
		} else {
			SLOG(LOG_ERROR, TAG_TTSC, "<<<< setting get engine list : invalid message \n");
		}

		dbus_message_unref(result_msg);
	} else {
		SLOG(LOG_ERROR, TAG_TTSC, "<<<< setting get engine list : result message is NULL!! \n");
	}

	dbus_message_unref(msg);

	return result;
}

int tts_setting_dbus_request_get_engine(char** engine_id)
{
	if (NULL == engine_id)	{
		SLOG(LOG_ERROR, TAG_TTSC, "Input parameter is NULL");
		return TTS_SETTING_ERROR_INVALID_PARAMETER;
	}

	DBusMessage* msg;

	msg = dbus_message_new_method_call(
		TTS_SERVER_SERVICE_NAME, 
		TTS_SERVER_SERVICE_OBJECT_PATH, 
		TTS_SERVER_SERVICE_INTERFACE, 
		TTS_SETTING_METHOD_GET_ENGINE);

	if (NULL == msg) { 
		SLOG(LOG_ERROR, TAG_TTSC, ">>>> Request setting get engine : Fail to make message \n"); 
		return TTS_SETTING_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_TTSC, ">>>> Request setting get engine ");
	}

	int pid = getpid();

	dbus_message_append_args( msg, 
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = TTS_SETTING_ERROR_OPERATION_FAILED;
	char* temp;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn, msg, 3000, &err);

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err, DBUS_TYPE_INT32, &result, DBUS_TYPE_STRING, &temp, DBUS_TYPE_INVALID);
	
		if (dbus_error_is_set(&err)) { 
			SLOG(LOG_ERROR, TAG_TTSC, "Get arguments error (%s)\n", err.message);
			dbus_error_free(&err); 
			result = TTS_SETTING_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);
	}

	if (0 == result) {
		*engine_id = strdup(temp);

		if (NULL == *engine_id) {
			SLOG(LOG_ERROR, TAG_TTSC, "<<<< setting get engine : Out of memory \n");
			result = TTS_SETTING_ERROR_OUT_OF_MEMORY;
		} else {
			SLOG(LOG_DEBUG, TAG_TTSC, "<<<< setting get engine : result(%d), engine id(%s)\n", result, *engine_id);
		}
	} else {
		SLOG(LOG_ERROR, TAG_TTSC, "<<<< setting get engine : result(%d) \n", result);
	}

	dbus_message_unref(msg);

	return result;
}


int tts_setting_dbus_request_set_engine(const char* engine_id)
{
	if (NULL == engine_id)	{
		SLOG(LOG_ERROR, TAG_TTSC, "Input parameter is NULL");
		return TTS_SETTING_ERROR_INVALID_PARAMETER;
	}

	DBusMessage* msg;

	msg = dbus_message_new_method_call(
		TTS_SERVER_SERVICE_NAME, 
		TTS_SERVER_SERVICE_OBJECT_PATH, 
		TTS_SERVER_SERVICE_INTERFACE, 
		TTS_SETTING_METHOD_SET_ENGINE);

	if (NULL == msg) { 
		SLOG(LOG_ERROR, TAG_TTSC, ">>>> Request setting set engine : Fail to make message \n"); 
		return TTS_SETTING_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_TTSC, ">>>> Request setting set engine : engine id(%s)", engine_id);
	}

	int pid = getpid();

	dbus_message_append_args( msg, 
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_STRING, &engine_id,
		DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = TTS_SETTING_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block  ( g_conn, msg, 3000, &err);

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err, DBUS_TYPE_INT32, &result, DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) { 
			SLOG(LOG_ERROR, TAG_TTSC, "Get arguments error (%s)\n", err.message);
			dbus_error_free(&err); 
			result = TTS_SETTING_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);
	}

	if (0 == result) {
		SLOG(LOG_DEBUG, TAG_TTSC, "<<<< setting set engine : result(%d) \n", result);
	} else {
		SLOG(LOG_ERROR, TAG_TTSC, "<<<< setting set engine : result(%d) \n", result);
	}

	dbus_message_unref(msg);

	return result;
}

int tts_setting_dbus_request_get_voice_list(tts_setting_supported_voice_cb callback, void* user_data)
{
	if (NULL == callback) {
		SLOG(LOG_ERROR, TAG_TTSC, "Input parameter is NULL");
		return TTS_SETTING_ERROR_INVALID_PARAMETER;
	}

	DBusMessage* msg;

	msg = dbus_message_new_method_call(
		TTS_SERVER_SERVICE_NAME, 
		TTS_SERVER_SERVICE_OBJECT_PATH, 
		TTS_SERVER_SERVICE_INTERFACE, 
		TTS_SETTING_METHOD_GET_VOICE_LIST);

	if (NULL == msg) { 
		SLOG(LOG_ERROR, TAG_TTSC, ">>>> Request setting get voice list : Fail to make message \n"); 
		return TTS_SETTING_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_TTSC, ">>>> Request setting get voice list");
	}

	int pid = getpid();

	dbus_message_append_args( msg, 
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = TTS_SETTING_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn, msg, 3000, &err);

	DBusMessageIter args;

	if (NULL != result_msg) {
		if (dbus_message_iter_init(result_msg, &args)) {
			/* Get result */
			if (DBUS_TYPE_INT32 == dbus_message_iter_get_arg_type(&args)) {
				dbus_message_iter_get_basic(&args, &result);
				dbus_message_iter_next(&args);
			} 

			if (0 == result) {
				SLOG(LOG_DEBUG, TAG_TTSC, "<<<< setting get voice list : result = %d \n", result);

				int size = 0; 
				char* temp_id = NULL;
				char* temp_lang = NULL;
				int temp_type;

				/* Get current voice */
				dbus_message_iter_get_basic(&args, &temp_id);
				dbus_message_iter_next(&args);

				if (NULL != temp_id) {
					/* Get voice count */
					if (DBUS_TYPE_INT32 == dbus_message_iter_get_arg_type(&args)) {
						dbus_message_iter_get_basic(&args, &size);
						dbus_message_iter_next(&args);
					}
					
					int i=0;
					for (i=0 ; i<size ; i++) {
						dbus_message_iter_get_basic(&args, &(temp_lang) );
						dbus_message_iter_next(&args);

						dbus_message_iter_get_basic(&args, &(temp_type) );
						dbus_message_iter_next(&args);
				
						if (true != callback(temp_id, temp_lang, temp_type, user_data)) {
							break;
						}
					}
				} else {
					SLOG(LOG_ERROR, TAG_TTSC, "Engine ID is NULL \n");
					result = TTS_SETTING_ERROR_OPERATION_FAILED;
				}

			} else {
				SLOG(LOG_ERROR, TAG_TTSC, "<<<< setting get voice list : result = %d \n", result);
			}
		} 

		dbus_message_unref(result_msg);
	} else {
		SLOG(LOG_ERROR, TAG_TTSC, "<<<< setting get voice list : Result message is NULL!!");
	}

	dbus_message_unref(msg);

	return result;
}

int tts_setting_dbus_request_get_default_voice(char** language, tts_setting_voice_type_e* voice_type)
{
	if (NULL == language || NULL == voice_type) {
		SLOG(LOG_ERROR, TAG_TTSC, "Input Parameter is NULL");
		return TTS_SETTING_ERROR_INVALID_PARAMETER;
	}

	DBusMessage* msg;

	msg = dbus_message_new_method_call(
		TTS_SERVER_SERVICE_NAME, 
		TTS_SERVER_SERVICE_OBJECT_PATH, 
		TTS_SERVER_SERVICE_INTERFACE, 
		TTS_SETTING_METHOD_GET_DEFAULT_VOICE);

	if (NULL == msg) { 
		SLOG(LOG_ERROR, TAG_TTSC, ">>>> Request setting get default voice : Fail to make message \n"); 
		return TTS_SETTING_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_TTSC, ">>>> Request setting get default voice");
	}

	int pid = getpid();

	dbus_message_append_args( msg, 
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = TTS_SETTING_ERROR_OPERATION_FAILED;
	char* temp_char;
	int temp_int;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn, msg, 3000, &err);

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err, 
			DBUS_TYPE_INT32, &result, 
			DBUS_TYPE_STRING, &temp_char,
			DBUS_TYPE_INT32, &temp_int,
			DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) { 
			SLOG(LOG_ERROR, TAG_TTSC, "Get arguments error (%s)\n", err.message);
			dbus_error_free(&err); 
			result = TTS_SETTING_ERROR_OPERATION_FAILED;
		}

		dbus_message_unref(result_msg);
	}

	if (0 == result) {
		*language = strdup(temp_char);
		*voice_type = (tts_setting_voice_type_e)temp_int;

		if (NULL == *language) {
			SLOG(LOG_ERROR, TAG_TTSC, "<<<< setting get default voice : Out of memory \n");
			result = TTS_SETTING_ERROR_OUT_OF_MEMORY;
		} else {
			SLOG(LOG_DEBUG, TAG_TTSC, "<<<< setting get default voice : result(%d), lang(%s), vctype(%d) \n", result, *language, *voice_type);
		}
	} else {
		SLOG(LOG_ERROR, TAG_TTSC, "<<<< setting get default voice : result(%d) \n", result);
	}

	dbus_message_unref(msg);

	return result;
}

int tts_setting_dbus_request_set_default_voice(const char* language, const int voicetype )
{
	if (NULL == language) {
		SLOG(LOG_ERROR, TAG_TTSC, "Input Parameter is NULL");
		return TTS_SETTING_ERROR_INVALID_PARAMETER;
	}

	DBusMessage* msg;

	msg = dbus_message_new_method_call(
		TTS_SERVER_SERVICE_NAME, 
		TTS_SERVER_SERVICE_OBJECT_PATH, 
		TTS_SERVER_SERVICE_INTERFACE, 
		TTS_SETTING_METHOD_SET_DEFAULT_VOICE);

	if (NULL == msg) { 
		SLOG(LOG_ERROR, TAG_TTSC, ">>>> Request setting set default voice : Fail to make message \n"); 
		return TTS_SETTING_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_TTSC, ">>>> Request setting set default voice : language(%s), type(%d)", language, voicetype);
	}

	int pid = getpid();

	dbus_message_append_args( msg, 
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_STRING, &language,
		DBUS_TYPE_INT32, &voicetype,
		DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = TTS_SETTING_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn, msg, 3000, &err);

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err, DBUS_TYPE_INT32, &result, DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) { 
			SLOG(LOG_ERROR, TAG_TTSC, "Get arguments error (%s)\n", err.message);
			dbus_error_free(&err); 
			result = TTS_SETTING_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);
	}

	if (0 == result) {
		SLOG(LOG_DEBUG, TAG_TTSC, "<<<< setting set default voice : result(%d)", result);
	} else {
		SLOG(LOG_ERROR, TAG_TTSC, "<<<< setting set default voice : result(%d)", result);
	}

	dbus_message_unref(msg);

	return result;
}

int tts_setting_dbus_request_get_default_speed(int* speed)
{
	if (NULL == speed) {
		SLOG(LOG_ERROR, TAG_TTSC, "Input Parameter is NULL");
		return TTS_SETTING_ERROR_INVALID_PARAMETER;
	}

	DBusMessage* msg;

	msg = dbus_message_new_method_call(
		TTS_SERVER_SERVICE_NAME, 
		TTS_SERVER_SERVICE_OBJECT_PATH, 
		TTS_SERVER_SERVICE_INTERFACE, 
		TTS_SETTING_METHOD_GET_DEFAULT_SPEED);

	if (NULL == msg) { 
		SLOG(LOG_ERROR, TAG_TTSC, ">>>> Request setting get default speed : Fail to make message \n"); 
		return TTS_SETTING_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_TTSC, ">>>> Request setting get default speed ");
	}

	int pid = getpid();

	dbus_message_append_args( msg, 
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int temp_int;
	int result = TTS_SETTING_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn, msg, 3000, &err);

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err,
			DBUS_TYPE_INT32, &result,
			DBUS_TYPE_INT32, &temp_int,
			DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) { 
			SLOG(LOG_ERROR, TAG_TTSC, "Get arguments error (%s)\n", err.message);
			dbus_error_free(&err); 
			result = TTS_SETTING_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);
	}

	if (0 == result) {
		*speed = temp_int;
		SLOG(LOG_DEBUG, TAG_TTSC, "<<<< setting get default speed : result(%d), speed(%d)", result, *speed);
	} else {
		SLOG(LOG_ERROR, TAG_TTSC, "<<<< setting get default speed : result(%d)", result);
	}

	dbus_message_unref(msg);

	return result;
}

int tts_setting_dbus_request_set_default_speed(const int speed)
{
	DBusMessage* msg;

	msg = dbus_message_new_method_call(
		TTS_SERVER_SERVICE_NAME, 
		TTS_SERVER_SERVICE_OBJECT_PATH, 
		TTS_SERVER_SERVICE_INTERFACE, 
		TTS_SETTING_METHOD_SET_DEFAULT_SPEED);

	if (NULL == msg) { 
		SLOG(LOG_ERROR, TAG_TTSC, ">>>> Request setting set default speed : Fail to make message \n"); 
		return TTS_SETTING_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_TTSC, ">>>> Request setting set default speed : speed(%d)", speed);
	}

	int pid = getpid();

	dbus_message_append_args(msg, 
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_INT32, &speed,
		DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = TTS_SETTING_ERROR_OPERATION_FAILED;
	result_msg = dbus_connection_send_with_reply_and_block(g_conn, msg, 3000, &err);

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err, DBUS_TYPE_INT32, &result, DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) { 
			SLOG(LOG_ERROR, TAG_TTSC, "Get arguments error (%s)\n", err.message);
			dbus_error_free(&err); 
			result = TTS_SETTING_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);
	}

	if (0 == result) {
		SLOG(LOG_DEBUG, TAG_TTSC, "<<<< setting set default speed : result(%d)", result);
	} else {
		SLOG(LOG_ERROR, TAG_TTSC, "<<<< setting set default speed : result(%d)", result);
	}

	dbus_message_unref(msg);

	return result;
}

int tts_setting_dbus_request_get_engine_setting(tts_setting_engine_setting_cb callback, void* user_data)
{
	if (NULL == callback) {
		SLOG(LOG_ERROR, TAG_TTSC, "Input Parameter is NULL");
		return TTS_SETTING_ERROR_INVALID_PARAMETER;
	}

	DBusMessage* msg;

	msg = dbus_message_new_method_call(
		   TTS_SERVER_SERVICE_NAME, 
		   TTS_SERVER_SERVICE_OBJECT_PATH, 
		   TTS_SERVER_SERVICE_INTERFACE, 
		   TTS_SETTING_METHOD_GET_ENGINE_SETTING);

	if (NULL == msg) { 
		SLOG(LOG_ERROR, TAG_TTSC, ">>>> Request setting get engine setting : Fail to make message \n"); 
		return TTS_SETTING_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_TTSC, ">>>> Request setting get engine setting");
	}

	int pid = getpid();

	dbus_message_append_args( msg, 
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = TTS_SETTING_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn, msg, 3000, &err);

	if (NULL != result_msg) {
		DBusMessageIter args;

		if (dbus_message_iter_init(result_msg, &args)) {
			/* Get result */
			if (DBUS_TYPE_INT32 == dbus_message_iter_get_arg_type(&args)) {
				dbus_message_iter_get_basic(&args, &result);
				dbus_message_iter_next(&args);
			} 

			if (0 == result) {
				SLOG(LOG_DEBUG, TAG_TTSC, "<<<< get engine setting : result = %d \n", result);
				int size; 
				char* temp_id = NULL;
				char* temp_key;
				char* temp_value;
				
				/* Get engine id */
				dbus_message_iter_get_basic(&args, &temp_id);
				dbus_message_iter_next(&args);
		
				if (NULL != temp_id) {
					/* Get setting count */
					if (DBUS_TYPE_INT32 == dbus_message_iter_get_arg_type(&args)) {
						dbus_message_iter_get_basic(&args, &size);
						dbus_message_iter_next(&args);
					}

					int i=0;
					for (i=0 ; i<size ; i++) {
						dbus_message_iter_get_basic(&args, &(temp_key) );
						dbus_message_iter_next(&args);

						dbus_message_iter_get_basic(&args, &(temp_value) );
						dbus_message_iter_next(&args);

						if (true != callback(temp_id, temp_key, temp_value, user_data)) {
							break;
						}
					} 
				} else {
					SLOG(LOG_ERROR, TAG_TTSC, "<<<< get engine setting : result message is invalid \n");
					result = TTS_SETTING_ERROR_OPERATION_FAILED;
				}
			} 
		} else {
			SLOG(LOG_ERROR, TAG_TTSC, "<<<< get engine setting : result message is invalid \n");
			result = TTS_SETTING_ERROR_OPERATION_FAILED;
		}

		dbus_message_unref(result_msg);
	} else {
		SLOG(LOG_ERROR, TAG_TTSC, "<<<< get engine setting : Result message is NULL!! \n");
	}	

	dbus_message_unref(msg);

	return result;
}

int tts_setting_dbus_request_set_engine_setting(const char* key, const char* value)
{
	if (NULL == key || NULL == value) {
		SLOG(LOG_ERROR, TAG_TTSC, "Input Parameter is NULL");
		return TTS_SETTING_ERROR_INVALID_PARAMETER;
	}

	DBusMessage* msg;

	msg = dbus_message_new_method_call(
		TTS_SERVER_SERVICE_NAME, 
		TTS_SERVER_SERVICE_OBJECT_PATH, 
		TTS_SERVER_SERVICE_INTERFACE, 
		TTS_SETTING_METHOD_SET_ENGINE_SETTING);

	if (NULL == msg) { 
		SLOG(LOG_ERROR, TAG_TTSC, ">>>> Request setting set engine setting : Fail to make message \n"); 
		return TTS_SETTING_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_TTSC, ">>>> Request setting set engine setting : key(%s), value(%s)", key, value);
	}

	int pid = getpid();

	dbus_message_append_args( msg, 
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_STRING, &key,
		DBUS_TYPE_STRING, &value, 
		DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = TTS_SETTING_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn, msg, 3000, &err);

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err, DBUS_TYPE_INT32, &result, DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) { 
			SLOG(LOG_ERROR, TAG_TTSC, "<<<< Get arguments error (%s)\n", err.message);
			dbus_error_free(&err); 
			result = -1;
		}
		dbus_message_unref(result_msg);
	}

	if (0 == result) {
		SLOG(LOG_DEBUG, TAG_TTSC, "<<<< setting set engine setting : result(%d)", result);
	} else {
		SLOG(LOG_ERROR, TAG_TTSC, "<<<< setting set engine setting : result(%d)", result);
	}

	dbus_message_unref(msg);

	return result;
}

