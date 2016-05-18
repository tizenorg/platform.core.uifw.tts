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
		SLOG(LOG_ERROR, get_tag(), "[IN ERROR] tts initialize : get arguments error (%s)", err.message);
		dbus_error_free(&err);
		ret = TTSD_ERROR_OPERATION_FAILED;
	} else {

		SECURE_SLOG(LOG_DEBUG, get_tag(), "[IN] tts initialize : pid(%d), uid(%d)", pid , uid);
		ret =  ttsd_server_initialize(pid, uid);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);

		if (0 == ret) {
			SLOG(LOG_DEBUG, get_tag(), "[OUT] tts initialize : result(%d)", ret);
		} else {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] tts initialize : result(%d)", ret);
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] tts initialize : Out Of Memory!");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] tts initialize : Fail to create reply message!!");
	}

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
		SLOG(LOG_ERROR, get_tag(), "[IN ERROR] tts finalize : get arguments error (%s)", err.message);
		dbus_error_free(&err);
		ret = TTSD_ERROR_OPERATION_FAILED;
	} else {
		SECURE_SLOG(LOG_DEBUG, get_tag(), "[IN] tts finalize : uid(%d)", uid);
		ret =  ttsd_server_finalize(uid);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);

		if (0 == ret) {
			SLOG(LOG_DEBUG, get_tag(), "[OUT] tts finalize : result(%d)", ret);
		} else {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] tts finalize : result(%d)", ret);
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] tts finalize : Out Of Memory!");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] tts finalize : Fail to create reply message!!");
	}

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
		SLOG(LOG_ERROR, get_tag(), "[IN ERROR] tts get supported voices : get arguments error (%s)", err.message);
		dbus_error_free(&err);
		ret = TTSD_ERROR_OPERATION_FAILED;
	} else {
		SECURE_SLOG(LOG_DEBUG, get_tag(), "[IN] get supported voices : uid(%d)", uid);
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
				SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] tts supported voices : Fail to append type");
				ret = TTSD_ERROR_OPERATION_FAILED;
			} else {

				GList *iter = NULL;
				voice_s* voice;

				iter = g_list_first(voice_list);

				while (NULL != iter) {
					voice = iter->data;

					if (NULL != voice) {
						dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &(voice->language));
						dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &(voice->type));

						if (NULL != voice->language)
							g_free(voice->language);

						g_free(voice);
					}

					voice_list = g_list_remove_link(voice_list, iter);
					g_list_free(iter);
					iter = g_list_first(voice_list);
				}
			}
			SLOG(LOG_DEBUG, get_tag(), "[OUT] tts supported voices : result(%d)", ret);
		} else {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] tts supported voices : result(%d)", ret);
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] error : Out Of Memory!");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] tts supported voices : Fail to create reply message!!");
	}

	return 0;
}

int ttsd_dbus_server_get_current_voice(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int uid;
	char* lang = NULL;
	int voice_type;
	int ret;

	dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &uid, DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, get_tag(), ">>>>> TTS GET DEFAULT VOICE");

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, get_tag(), "[IN ERROR] tts get default voice : Get arguments error (%s)", err.message);
		dbus_error_free(&err);
		ret = TTSD_ERROR_OPERATION_FAILED;
	} else {
		SECURE_SLOG(LOG_DEBUG, get_tag(), "[IN] tts get default voice : uid(%d)", uid);
		ret = ttsd_server_get_current_voice(uid, &lang, &voice_type);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		if (0 == ret) {
			/* Append result and voice */
			dbus_message_append_args(reply,
				DBUS_TYPE_INT32, &ret,
				DBUS_TYPE_STRING, &lang,
				DBUS_TYPE_INT32, &voice_type,
				DBUS_TYPE_INVALID);
			SECURE_SLOG(LOG_DEBUG, get_tag(), "[OUT] tts default voice : lang(%s), vctype(%d)", lang, voice_type);
		} else {
			dbus_message_append_args(reply,
				DBUS_TYPE_INT32, &ret,
				DBUS_TYPE_INVALID);
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] tts default voice : result(%d)", ret); 
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] tts default voice : Out Of Memory!");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] tts default voice : Fail to create reply message!!");
	}

	if (NULL != lang)	free(lang);
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
		SLOG(LOG_ERROR, get_tag(), "[IN ERROR] tts add text : Get arguments error (%s)", err.message);
		dbus_error_free(&err);
		ret = TTSD_ERROR_OPERATION_FAILED;
	} else {
		
		SECURE_SLOG(LOG_DEBUG, get_tag(), "[IN] tts add text : uid(%d), text(%s), lang(%s), type(%d), speed(%d), uttid(%d)", 
			uid, text, lang, voicetype, speed, uttid); 
		ret =  ttsd_server_add_queue(uid, text, lang, voicetype, speed, uttid);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);

		if (0 == ret) {
			SLOG(LOG_DEBUG, get_tag(), "[OUT] tts add text : result(%d)", ret);
		} else {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] tts add text : result(%d)", ret);
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] tts add text : Out Of Memory!");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] tts add text : Fail to create reply message!!");
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
		SLOG(LOG_ERROR, get_tag(), "[IN ERROR] tts play : Get arguments error (%s)", err.message);
		dbus_error_free(&err);
		ret = TTSD_ERROR_OPERATION_FAILED;
	} else {
		SECURE_SLOG(LOG_DEBUG, get_tag(), "[IN] tts play : uid(%d)", uid);
		ret =  ttsd_server_play(uid);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);

		if (0 == ret) {
			SLOG(LOG_DEBUG, get_tag(), "[OUT] tts play : result(%d)", ret);
		} else {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] tts play : result(%d)", ret);
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] tts play : Out Of Memory!");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] tts play : Fail to create reply message!!");
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
		SLOG(LOG_ERROR, get_tag(), "[IN ERROR] tts stop : Get arguments error (%s)", err.message);
		dbus_error_free(&err);
		ret = TTSD_ERROR_OPERATION_FAILED;
	} else {
		SECURE_SLOG(LOG_DEBUG, get_tag(), "[IN] tts stop : uid(%d)", uid);
		ret = ttsd_server_stop(uid);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		dbus_message_append_args(reply, 
			DBUS_TYPE_INT32, &ret, 
			DBUS_TYPE_INVALID);

		if (0 == ret) {
			SLOG(LOG_DEBUG, get_tag(), "[OUT] tts stop : result(%d)", ret);
		} else {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] tts stop : result(%d)", ret);
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] tts stop : Out Of Memory!");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] tts stop : Fail to create reply message!!");
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
		SLOG(LOG_ERROR, get_tag(), "[IN ERROR] tts pause : Get arguments error (%s)", err.message);
		dbus_error_free(&err);
		ret = TTSD_ERROR_OPERATION_FAILED;
	} else {
		SECURE_SLOG(LOG_DEBUG, get_tag(), "[IN] tts pause : uid(%d)", uid);
		ret = ttsd_server_pause(uid, &uttid);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		dbus_message_append_args(reply, 
			DBUS_TYPE_INT32, &ret, 
			DBUS_TYPE_INVALID);

		if (0 == ret) {
			SLOG(LOG_DEBUG, get_tag(), "[OUT] tts pause : result(%d)", ret);
		} else {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] tts pause : result(%d)", ret);
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] tts pause : Out Of Memory!");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] tts pause : Fail to create reply message!!");
	}

	SLOG(LOG_DEBUG, get_tag(), "<<<<<");
	SLOG(LOG_DEBUG, get_tag(), "  ");

	return 0;
}

int ttsd_dbus_server_set_private_data(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int uid;
	char* key;
	char* data;
	int ret = 0;
	dbus_message_get_args(msg, &err,
		DBUS_TYPE_INT32, &uid,
		DBUS_TYPE_STRING, &key,
		DBUS_TYPE_STRING, &data,
		DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, get_tag(), ">>>>> TTS set private data");

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, get_tag(), "[IN ERROR] Fail to get arguments (%s)", err.message);
		dbus_error_free(&err);
		ret = TTSD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, get_tag(), "[IN] tts set private data(%d)", uid);
		ret = ttsd_server_set_private_data(uid, key, data);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		dbus_message_append_args(reply,
			DBUS_TYPE_INT32, &ret,
			DBUS_TYPE_INVALID);

		if (0 == ret) {
			SLOG(LOG_DEBUG, get_tag(), "[OUT] tts set private data : (%d)", ret);
		} else {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] tts set private data : (%d)", ret);
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] Fail to send reply");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] Fail to create reply message");
	}

	SLOG(LOG_DEBUG, get_tag(), "<<<<<");
	SLOG(LOG_DEBUG, get_tag(), "");

	return 0;
}

int ttsd_dbus_server_get_private_data(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int uid;
	char* key;
	char* data;
	int ret = 0;
	dbus_message_get_args(msg, &err,
		DBUS_TYPE_INT32, &uid,
		DBUS_TYPE_STRING, &key,
		DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, get_tag(), ">>>>> TTS get private data");

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, get_tag(), "[IN ERROR] Fail to get arguments (%s)", err.message);
		dbus_error_free(&err);
		ret = TTSD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, get_tag(), "[IN] tts get private data(%d)", uid);
		ret = ttsd_server_get_private_data(uid, key, &data);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		dbus_message_append_args(reply,
			DBUS_TYPE_INT32, &ret,
			DBUS_TYPE_INVALID);

		if (0 == ret) {
			SLOG(LOG_DEBUG, get_tag(), "[OUT] tts get private data : (%d)", ret);
		} else {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] tts get private data : (%d)", ret);
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] Fail to send reply");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, get_tag(), "[OUT ERROR] Fail to create reply message");
	}

	SLOG(LOG_DEBUG, get_tag(), "<<<<<");
	SLOG(LOG_DEBUG, get_tag(), "");

	return 0;
}
