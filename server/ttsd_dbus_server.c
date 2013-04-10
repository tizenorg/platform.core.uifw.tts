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


#include "ttsd_main.h"
#include "ttsd_dbus.h"
#include "ttsd_dbus_server.h"
#include "ttsd_server.h"

extern int ttsd_data_get_pid(const int uid);

/*
* Dbus Client-Daemon Server
*/ 

int ttsd_dbus_server_hello(DBusConnection* conn, DBusMessage* msg)
{
	SLOG(LOG_DEBUG, get_tag(), ">>>>> TTS Hello");

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] Out Of Memory!");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] Fail to create reply message!!"); 
	}

	SLOG(LOG_DEBUG, get_tag(), "<<<<<");
	SLOG(LOG_DEBUG, get_tag(), "  ");

	return 0;
}

int ttsd_dbus_server_initialize(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid, uid;
	int ret = 0; 

	dbus_message_get_args(msg, &err,
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_INT32, &uid,
		DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, get_tag(), ">>>>> TTS INITIALIZE");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, get_tag(), "[IN ERROR] tts initialize : get arguments error (%s)\n", err.message);
		dbus_error_free(&err); 
		ret = TTSD_ERROR_OPERATION_FAILED;
	} else {

		SLOG(LOG_DEBUG, get_tag(), "[IN] tts initialize : pid(%d), uid(%d) \n", pid , uid); 
		ret =  ttsd_server_initialize(pid, uid);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);

		if (0 == ret) {
			SLOG(LOG_DEBUG, get_tag(), "[OUT] tts initialize : result(%d) \n", ret); 
		} else {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] tts initialize : result(%d) \n", ret); 
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] tts initialize : Out Of Memory!\n");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] tts initialize : fail to create reply message!!"); 
	}

	SLOG(LOG_DEBUG, get_tag(), "<<<<<");
	SLOG(LOG_DEBUG, get_tag(), "  ");

	return 0;
}

int ttsd_dbus_server_finalize(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int uid;
	int ret = 0; 

	dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &uid, DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, get_tag(), ">>>>> TTS FINALIZE");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, get_tag(), "[IN ERROR] tts finalize : get arguments error (%s)\n", err.message);
		dbus_error_free(&err); 
		ret = TTSD_ERROR_OPERATION_FAILED;
	} else {
		
		SLOG(LOG_DEBUG, get_tag(), "[IN] tts finalize : uid(%d) \n", uid); 

		ret =  ttsd_server_finalize(uid);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);

		if (0 == ret) {
			SLOG(LOG_DEBUG, get_tag(), "[OUT] tts finalize : result(%d) \n", ret); 
		} else {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] tts finalize : result(%d) \n", ret); 
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] tts finalize : Out Of Memory!\n");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] tts finalize : fail to create reply message!!"); 
	}

	SLOG(LOG_DEBUG, get_tag(), "<<<<<");
	SLOG(LOG_DEBUG, get_tag(), "  ");

	return 0;
}

int ttsd_dbus_server_get_support_voices(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int uid;
	int ret = 0;
	GList* voice_list = NULL;

	dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &uid, DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, get_tag(), ">>>>> TTS GET VOICES");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, get_tag(), "[IN ERROR] tts get supported voices : get arguments error (%s)\n", err.message);
		dbus_error_free(&err); 
		ret = TTSD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, get_tag(), "[IN] get supported voices : uid(%d) \n", uid ); 
		ret = ttsd_server_get_support_voices(uid, &voice_list);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		DBusMessageIter args;
		dbus_message_iter_init_append(reply, &args);

		/* Append result*/
		dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &(ret));

		if (0 == ret) {
			/* Append voice size */
			int size = g_list_length(voice_list);
			
			if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &(size))) {
				SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] tts supported voices : Fail to append type \n"); 
				ret = TTSD_ERROR_OPERATION_FAILED;
			} else {

				GList *iter = NULL;
				voice_s* voice;

				iter = g_list_first(voice_list);

				while (NULL != iter) {
					voice = iter->data;

					if (NULL != voice) {
						dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &(voice->language) );
						dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &(voice->type) );

						if (NULL != voice->language)
							g_free(voice->language);
						
						g_free(voice);
					}
					
					voice_list = g_list_remove_link(voice_list, iter);

					iter = g_list_first(voice_list);
				} 
			}
			SLOG(LOG_DEBUG, get_tag(), "[OUT] tts supported voices : result(%d) \n", ret); 
		} else {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] tts supported voices : result(%d) \n", ret); 
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] error : Out Of Memory!\n");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] tts supported voices : fail to create reply message!!"); 		
	}

	SLOG(LOG_DEBUG, get_tag(), "<<<<<");
	SLOG(LOG_DEBUG, get_tag(), "  ");

	return 0;
}

int ttsd_dbus_server_get_current_voice(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int uid;
	char* lang;
	int voice_type;
	int ret;

	dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &uid, DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, get_tag(), ">>>>> TTS GET DEFAULT VOICE");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, get_tag(), "[IN ERROR] tts get default voice : Get arguments error (%s)\n", err.message);
		dbus_error_free(&err);
		ret = TTSD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, get_tag(), "[IN] tts get default voice : uid(%d) \n", uid); 
		ret = ttsd_server_get_current_voice(uid, &lang, &voice_type);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		if (0 == ret) { 
			/* Append result and voice */
			dbus_message_append_args( reply, 
				DBUS_TYPE_INT32, &ret,
				DBUS_TYPE_STRING, &lang,
				DBUS_TYPE_INT32, &voice_type,
				DBUS_TYPE_INVALID);
			SLOG(LOG_DEBUG, get_tag(), "[OUT] tts default voice : lang(%s), vctype(%d)\n", lang, voice_type );
		} else {
			dbus_message_append_args( reply, 
				DBUS_TYPE_INT32, &ret,
				DBUS_TYPE_INVALID);
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] tts default voice : result(%d) \n", ret); 
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] tts default voice : Out Of Memory!\n");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] tts default voice : fail to create reply message!!"); 
	}

	SLOG(LOG_DEBUG, get_tag(), "<<<<<");
	SLOG(LOG_DEBUG, get_tag(), "  ");

	return 0;
}

int ttsd_dbus_server_add_text(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int uid, voicetype, speed, uttid;
	char *text, *lang;
	int ret = 0;

	dbus_message_get_args(msg, &err,
		DBUS_TYPE_INT32, &uid,
		DBUS_TYPE_STRING, &text,
		DBUS_TYPE_STRING, &lang,
		DBUS_TYPE_INT32, &voicetype,
		DBUS_TYPE_INT32, &speed,
		DBUS_TYPE_INT32, &uttid,
		DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, get_tag(), ">>>>> TTS ADD TEXT");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, get_tag(), "[IN ERROR] tts add text : Get arguments error (%s)\n", err.message);
		dbus_error_free(&err); 
		ret = TTSD_ERROR_OPERATION_FAILED;
	} else {
		
		SLOG(LOG_DEBUG, get_tag(), "[IN] tts add text : uid(%d), text(%s), lang(%s), type(%d), speed(%d), uttid(%d) \n", 
			uid, text, lang, voicetype, speed, uttid); 
		ret =  ttsd_server_add_queue(uid, text, lang, voicetype, speed, uttid);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);

		if (0 == ret) {
			SLOG(LOG_DEBUG, get_tag(), "[OUT] tts add text : result(%d) \n", ret); 
		} else {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] tts add text : result(%d) \n", ret); 
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] tts add text : Out Of Memory!\n");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] tts add text : fail to create reply message!!"); 
	}

	SLOG(LOG_DEBUG, get_tag(), "<<<<<");
	SLOG(LOG_DEBUG, get_tag(), "  ");

	return 0;
}

int ttsd_dbus_server_play(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int uid;
	int ret = 0;

	dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &uid, DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, get_tag(), ">>>>> TTS PLAY");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, get_tag(), "[IN ERROR] tts play : Get arguments error (%s)\n", err.message);
		dbus_error_free(&err); 
		ret = TTSD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, get_tag(), "[IN] tts play : uid(%d) \n", uid ); 
		ret =  ttsd_server_play(uid);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);

		if (0 == ret) {
			SLOG(LOG_DEBUG, get_tag(), "[OUT] tts play : result(%d) \n", ret); 
		} else {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] tts play : result(%d) \n", ret); 
		}
	
		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] tts play : Out Of Memory!\n");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] tts play : fail to create reply message!!"); 
	}

	SLOG(LOG_DEBUG, get_tag(), "<<<<<");
	SLOG(LOG_DEBUG, get_tag(), "  ");

	return 0;
}

int ttsd_dbus_server_stop(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int uid;
	int ret = 0;
	dbus_message_get_args(msg, &err, 
		DBUS_TYPE_INT32, &uid, 
		DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, get_tag(), ">>>>> TTS STOP");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, get_tag(), "[IN ERROR] tts stop : Get arguments error (%s)\n", err.message);
		dbus_error_free(&err); 
		ret = TTSD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, get_tag(), "[IN] tts stop : uid(%d)\n", uid);
		ret = ttsd_server_stop(uid);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		dbus_message_append_args(reply, 
			DBUS_TYPE_INT32, &ret, 
			DBUS_TYPE_INVALID);

		if (0 == ret) {
			SLOG(LOG_DEBUG, get_tag(), "[OUT] tts stop : result(%d) \n", ret); 
		} else {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] tts stop : result(%d) \n", ret); 
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] tts stop : Out Of Memory!\n");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] tts stop : fail to create reply message!!"); 
	}

	SLOG(LOG_DEBUG, get_tag(), "<<<<<");
	SLOG(LOG_DEBUG, get_tag(), "  ");

	return 0;
}

int ttsd_dbus_server_pause(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int uid;
	int uttid;
	int ret = 0;
	dbus_message_get_args(msg, &err, 
		DBUS_TYPE_INT32, &uid, 
		DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, get_tag(), ">>>>> TTS PAUSE");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, get_tag(), "[IN ERROR] tts pause : Get arguments error (%s)\n", err.message);
		dbus_error_free(&err); 
		ret = TTSD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, get_tag(), "[IN] tts pause : uid(%d)\n", uid);
		ret = ttsd_server_pause(uid, &uttid);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		dbus_message_append_args(reply, 
			DBUS_TYPE_INT32, &ret, 
			DBUS_TYPE_INVALID);

		if (0 == ret) {
			SLOG(LOG_DEBUG, get_tag(), "[OUT] tts pause : result(%d) \n", ret); 
		} else {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] tts pause : result(%d) \n", ret); 
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] tts pause : Out Of Memory!\n");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] tts pause : fail to create reply message!!"); 
	}

	SLOG(LOG_DEBUG, get_tag(), "<<<<<");
	SLOG(LOG_DEBUG, get_tag(), "  ");

	return 0;
}

#if 0
/*
* Dbus Server functions for tts daemon internal
*/ 
int ttsd_dbus_server_start_next_synthesis()
{
	return ttsd_server_start_next_synthesis();
}
#endif

/*
* Dbus Setting-Daemon Server
*/ 

int ttsd_dbus_server_setting_initialize(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid;
	int ret = 0;

	dbus_message_get_args(msg, &err,
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, get_tag(), ">>>>> SETTING INITIALIZE");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, get_tag(), "[IN ERROR] Receivce setting initialize : Get arguments error (%s)\n", err.message);
		dbus_error_free(&err);
		ret = TTSD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, get_tag(), "[IN] Receivce setting initialize : pid(%d) \n", pid); 
		ret =  ttsd_server_setting_initialize(pid);
	}
	
	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);

		if (0 == ret) {
			SLOG(LOG_DEBUG, get_tag(), "[OUT] setting initialize : result(%d) \n", ret);
		} else {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] setting initialize : result(%d) \n", ret);
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] setting initialize : Out Of Memory!\n");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] setting initialize : fail to create reply message!!"); 
	}

	SLOG(LOG_DEBUG, get_tag(), "<<<<<");
	SLOG(LOG_DEBUG, get_tag(), "  ");

	return 0;
}

int ttsd_dbus_server_setting_finalize(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid;
	int ret = 0;

	dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &pid, DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, get_tag(), ">>>>> SETTING FINALIZE");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, get_tag(), "[IN ERROR] setting finalize : Get arguments error (%s)\n", err.message);
		dbus_error_free(&err); 
		ret = TTSD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, get_tag(), "[IN] setting finalize : pid(%d)\n", pid); 
		ret =  ttsd_server_setting_finalize(pid);	
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);

		if (0 == ret) {
			SLOG(LOG_DEBUG, get_tag(), "[OUT] setting finalize : result(%d) \n", ret);
		} else {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] setting finalize : result(%d) \n", ret);
		}
		
		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] setting finalize : Out Of Memory!\n");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] setting finalize : fail to create reply message!!"); 
	}

	SLOG(LOG_DEBUG, get_tag(), "<<<<<");
	SLOG(LOG_DEBUG, get_tag(), "  ");

	return 0;
}

int ttsd_dbus_server_setting_get_engine_list(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid;
	int ret = 0;
	GList* engine_list = NULL;
	
	dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &pid, DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, get_tag(), ">>>>> SETTING GET ENGINE LIST");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, get_tag(), "[IN ERROR] setting engine list : Get arguments error (%s)\n", err.message);
		dbus_error_free(&err); 
		ret = TTSD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, get_tag(), "[IN] setting engine list : pid(%d) \n", pid); 
		ret = ttsd_server_setting_get_engine_list(pid, &engine_list);	
	}
	
	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		DBusMessageIter args;
		dbus_message_iter_init_append(reply, &args);

		/* Append result*/
		dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &(ret) );

		if (0 == ret) {
			/* Append size */
			int size = g_list_length(engine_list);
			if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &(size))) {
				SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] setting engine list : fail to append engine size \n");
			} else {

				GList *iter = NULL;
				engine_s* engine;

				iter = g_list_first(engine_list);

				while (NULL != iter) {
					engine = iter->data;
					
					if (NULL != engine) {
						SLOG(LOG_DEBUG, get_tag(), "engine id : %s, engine name : %s, ug_name, : %s", 
							engine->engine_id, engine->engine_name, engine->ug_name);

						dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &(engine->engine_id) );
						dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &(engine->engine_name) );
						dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &(engine->ug_name) );

						if (NULL != engine->engine_id)
							g_free(engine->engine_id);
						if (NULL != engine->engine_name)
							g_free(engine->engine_name);
						if (NULL != engine->ug_name)
							g_free(engine->ug_name);
						
						g_free(engine);
					}
					
					engine_list = g_list_remove_link(engine_list, iter);

					iter = g_list_first(engine_list);
				} 
				SLOG(LOG_DEBUG, get_tag(), "[OUT] setting engine list : result(%d) \n", ret);
			}
		} else {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] setting engine list : result(%d) \n", ret);
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] setting engine list : Out Of Memory!\n");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] setting engine list : fail to create reply message!!"); 
	}

	SLOG(LOG_DEBUG, get_tag(), "<<<<<");
	SLOG(LOG_DEBUG, get_tag(), "  ");

	return 0;
}

int ttsd_dbus_server_setting_get_engine(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid;
	int ret = 0;
	char* engine_id = NULL;

	dbus_message_get_args(msg, &err,
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, get_tag(), ">>>>> SETTING GET ENGINE");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, get_tag(), "[IN ERROR] setting get engine : Get arguments error (%s)\n", err.message);
		dbus_error_free(&err); 
		ret = TTSD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, get_tag(), "[IN] setting get engine : pid(%d)\n", pid); 
		ret = ttsd_server_setting_get_current_engine(pid, &engine_id);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		if (0 == ret) {
			dbus_message_append_args( reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_STRING, &engine_id, DBUS_TYPE_INVALID);
			SLOG(LOG_DEBUG, get_tag(), "[OUT] setting get engine : result(%d), engine id(%s)\n", ret, engine_id);
		} else {
			dbus_message_append_args( reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] setting get engine : result(%d)", ret);
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] setting get engine : Out Of Memory!\n");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] setting get engine : fail to create reply message!!"); 
	}

	if (NULL != engine_id)
		free(engine_id);

	SLOG(LOG_DEBUG, get_tag(), "<<<<<");
	SLOG(LOG_DEBUG, get_tag(), "  ");

	return 0;
}

int ttsd_dbus_server_setting_set_engine(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid;
	char* engine_id;
	int ret = 0;

	dbus_message_get_args(msg, &err,
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_STRING, &engine_id,
		DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, get_tag(), ">>>>> SETTING SET ENGINE");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, get_tag(), "[IN ERROR] setting set engine : Get arguments error (%s)\n", err.message);
		dbus_error_free(&err); 
		ret = TTSD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, get_tag(), "[IN] setting set engine : pid(%d), engine id(%s)\n", pid,  engine_id); 
		ret = ttsd_server_setting_set_current_engine(pid, engine_id);
	}
	
	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);

		if (0 == ret) {
			SLOG(LOG_DEBUG, get_tag(), "[OUT] setting set engine : result(%d) \n", ret);
		} else {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] setting set engine : result(%d) \n", ret);
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] setting set engine : Out Of Memory!\n");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] setting set engine : fail to create reply message!!"); 
	}

	SLOG(LOG_DEBUG, get_tag(), "<<<<<");
	SLOG(LOG_DEBUG, get_tag(), "  ");

	return 0;
}

int ttsd_dbus_server_setting_get_voice_list(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid;
	char* engine_id = NULL;
	int ret = 0;
	GList* voice_list = NULL;

	dbus_message_get_args(msg, &err,
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, get_tag(), ">>>>> SETTING GET VOICE LIST");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, get_tag(), "[IN ERROR] setting voice list : Get arguments error (%s)\n", err.message);
		dbus_error_free(&err); 
		ret = TTSD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, get_tag(), "[IN] setting voice list : pid(%d)\n", pid); 
		ret = ttsd_server_setting_get_voice_list(pid, &engine_id, &voice_list);
	}
	
	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		DBusMessageIter args;

		dbus_message_iter_init_append(reply, &args);

		/* Append result*/
		dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &(ret) );

		if (0 == ret) {
			if (dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &(engine_id))) {
				/* Append voice size */
				int size = g_list_length(voice_list);
				if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &(size))) {
					SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] setting voice list : Fail to append type \n"); 
					ret = TTSD_ERROR_OPERATION_FAILED;
				} else {

					GList *iter = NULL;
					voice_s* voice;

					iter = g_list_first(voice_list);

					while (NULL != iter) {
						voice = iter->data;

						if (NULL != voice) {
							dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &(voice->language) );
							dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &(voice->type) );

							if (NULL != voice->language)
								g_free(voice->language);

							g_free(voice);
						}

						voice_list = g_list_remove_link(voice_list, iter);

						iter = g_list_first(voice_list);
					} 
				}
				SLOG(LOG_DEBUG, get_tag(), "[OUT] setting voice list : result(%d) \n", ret); 
			} else {
				SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] setting voice list : Fail to append engine_id \n"); 
			}

			if (NULL != engine_id)
				free(engine_id);
		} else {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] setting voice list : result(%d) \n", ret);
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] setting voice list : Out Of Memory!\n");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] setting voice list : fail to create reply message!!"); 
	}

	SLOG(LOG_DEBUG, get_tag(), "<<<<<");
	SLOG(LOG_DEBUG, get_tag(), "  ");

	return 0;
}

int ttsd_dbus_server_setting_get_default_voice(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid;
	int ret = 0;
	char* lang;
	ttsp_voice_type_e type;

	dbus_message_get_args(msg, &err,
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, get_tag(), ">>>>> SETTING GET DEFAULT VOICE");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, get_tag(), "[IN ERROR] setting get default voice : Get arguments error (%s)\n", err.message);
		dbus_error_free(&err); 
		ret = TTSD_ERROR_OPERATION_FAILED;
	} else {	
		SLOG(LOG_DEBUG, get_tag(), "[IN] setting get default voice : pid(%d)\n", pid); 
		ret = ttsd_server_setting_get_default_voice(pid, &lang, &type);
	}
	
	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		if (0 == ret) {
			dbus_message_append_args(reply, 
				DBUS_TYPE_INT32, &ret, 
				DBUS_TYPE_STRING, &lang, 
				DBUS_TYPE_INT32, &type, 
				DBUS_TYPE_INVALID);
			SLOG(LOG_DEBUG, get_tag(), "[OUT] setting get default voice : result(%d), language(%s), type(%d) \n", ret, lang, type);
		} else {
			dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] setting get default voice : result(%d) \n", ret);
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] setting get default voice : Out Of Memory!\n");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] setting get default voice : fail to create reply message!!"); 
	}

	SLOG(LOG_DEBUG, get_tag(), "<<<<<");
	SLOG(LOG_DEBUG, get_tag(), "  ");

	return 0;
}

int ttsd_dbus_server_setting_set_default_voice(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid;
	char* language;
	int voicetype;
	int ret = 0;

	dbus_message_get_args(msg, &err,
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_STRING, &language,
		DBUS_TYPE_INT32, &voicetype,
		DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, get_tag(), ">>>>> SET DEFAULT VOICE");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, get_tag(), "[IN ERROR] setting set default voice : Get arguments error (%s)\n", err.message);
		dbus_error_free(&err); 
		ret = TTSD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, get_tag(), "[IN] setting set default voice : pid(%d), lang(%s), type(%d)\n"
			, pid, language, voicetype); 
		ret =  ttsd_server_setting_set_default_voice(pid, language, voicetype);
	}
	
	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);

		if (0 == ret) {
			SLOG(LOG_DEBUG, get_tag(), "[OUT] setting set default voice : result(%d) \n", ret);
		} else {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] setting set default voice : result(%d) \n", ret);
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] setting set default voice : Out Of Memory!\n");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] setting set default voice : fail to create reply message!!"); 
	}

	SLOG(LOG_DEBUG, get_tag(), "<<<<<");
	SLOG(LOG_DEBUG, get_tag(), "  ");

	return 0;
}

int ttsd_dbus_server_setting_get_speed(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid;
	int ret = 0;
	int speed;

	dbus_message_get_args(msg, &err,
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, get_tag(), ">>>>> SETTING GET SPEED");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, get_tag(), "[IN ERROR] setting get speed : Get arguments error (%s)\n", err.message);
		dbus_error_free(&err); 
		ret = TTSD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, get_tag(), "[IN] setting get default speed : pid(%d)\n", pid); 
		ret =  ttsd_server_setting_get_default_speed(pid, &speed);
	}
	
	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		if (0 == ret) {
			dbus_message_append_args( reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INT32, &speed, DBUS_TYPE_INVALID);
			SLOG(LOG_DEBUG, get_tag(), "[OUT] setting get default speed : result(%d), speed(%d) \n", ret, speed);
		} else {
			dbus_message_append_args( reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] setting get default speed : result(%d) \n", ret);
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] setting get default speed : Out Of Memory!\n");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] setting get default speed : fail to create reply message!!"); 
	}

	SLOG(LOG_DEBUG, get_tag(), "<<<<<");
	SLOG(LOG_DEBUG, get_tag(), "  ");

	return 0;
}

int ttsd_dbus_server_setting_set_speed(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid;
	int speed;
	int ret = 0;

	dbus_message_get_args(msg, &err,
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_INT32, &speed,
		DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, get_tag(), ">>>>> SETTING GET SPEED");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, get_tag(), "[IN ERROR] setting set default speed : Get arguments error (%s)\n", err.message);
		dbus_error_free(&err); 
		ret = TTSD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, get_tag(), "[IN] setting set default speed : pid(%d), speed(%d)\n", pid, speed); 
		ret = ttsd_server_setting_set_default_speed(pid, speed);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);

		if (0 == ret) {
			SLOG(LOG_DEBUG, get_tag(), "[OUT] setting set default speed : result(%d)", ret);
		} else {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] setting set default speed : result(%d)", ret);
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] setting set default speed : Out Of Memory!\n");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] setting set default speed : fail to create reply message!!"); 
	}

	SLOG(LOG_DEBUG, get_tag(), "<<<<<");
	SLOG(LOG_DEBUG, get_tag(), "  ");

	return 0;
}

int ttsd_dbus_server_setting_get_engine_setting(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid;
	int ret = 0;
	char* engine_id;
	GList* engine_setting_list = NULL;

	dbus_message_get_args(msg, &err,
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, get_tag(), ">>>>> SETTING GET ENGINE SETTING");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, get_tag(), "[IN ERROR] setting get engine option : Get arguments error (%s)\n", err.message);
		dbus_error_free(&err); 
		ret = TTSD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, get_tag(), "[IN] setting get engine option : pid(%d)", pid); 
		ret = ttsd_server_setting_get_engine_setting(pid, &engine_id, &engine_setting_list);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		DBusMessageIter args;
		dbus_message_iter_init_append(reply, &args);

		/* Append result*/
		dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &(ret) );

		if (0 == ret) {
			if (dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &(engine_id))) {
				/* Append voice size */
				int size = g_list_length(engine_setting_list);
				if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &(size))) {
					SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] setting voice list : Fail to append type \n"); 
					ret = TTSD_ERROR_OPERATION_FAILED;
				} else {

					GList *iter = NULL;
					engine_setting_s* setting;

					iter = g_list_first(engine_setting_list);

					while (NULL != iter) {
						setting = iter->data;

						if (NULL != setting) {
							dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &(setting->key) );
							dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &(setting->value) );

							if (NULL != setting->key)
								g_free(setting->key);
							if (NULL != setting->value)
								g_free(setting->value);
							
							g_free(setting);
						}
						
						engine_setting_list = g_list_remove_link(engine_setting_list, iter);

						iter = g_list_first(engine_setting_list);
					} 
				}
				SLOG(LOG_DEBUG, get_tag(), "[OUT] setting engine setting list : result(%d) \n", ret); 
			} else {
				SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] setting voice list : Fail to append engine_id \n"); 
			}

			if (NULL != engine_id)
				free(engine_id);
		} else {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] setting get engine option : result(%d) \n", ret);
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] setting get engine option : Out Of Memory!\n");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] setting get engine option : fail to create reply message!!"); 
	}

	SLOG(LOG_DEBUG, get_tag(), "<<<<<");
	SLOG(LOG_DEBUG, get_tag(), "  ");

	return 0;
}

int ttsd_dbus_server_setting_set_engine_setting(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid;
	char* key;
	char* value;
	int ret = 0;

	dbus_message_get_args(msg, &err,
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_STRING, &key,
		DBUS_TYPE_STRING, &value,
		DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, get_tag(), ">>>>> SETTING SET ENGINE SETTING");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, get_tag(), "[IN ERROR] setting set engine option : Get arguments error (%s)\n", err.message);
		dbus_error_free(&err); 
		ret = TTSD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, get_tag(), "[IN] setting set engine option : pid(%d), key(%s), value(%s)\n", pid, key, value); 
		ret = ttsd_server_setting_set_engine_setting(pid, key, value);
	}
	
	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		dbus_message_append_args( reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);

		if (0 == ret) {
			SLOG(LOG_DEBUG, get_tag(), "[OUT] setting set engine option : result(%d)", ret);
		} else {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] setting set engine option : result(%d)", ret);
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] setting set engine option : Out Of Memory!\n");
		}
		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] setting set engine option : fail to create reply message!!"); 
	}

	SLOG(LOG_DEBUG, get_tag(), "<<<<<");
	SLOG(LOG_DEBUG, get_tag(), "  ");

	return 0;
}

