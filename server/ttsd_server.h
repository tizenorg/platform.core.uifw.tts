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


#ifndef __TTSD_SERVER_CORE_H_
#define __TTSD_SERVER_CORE_H_

#include <glib.h>
#include <Ecore.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
* Daemon init
*/

/** Daemon initialize */
int ttsd_initialize();

Eina_Bool ttsd_cleanup_client(void *data);

/*
* Server API for client
*/

int ttsd_server_initialize(int pid, int uid);

int ttsd_server_finalize(int uid);

int ttsd_server_get_support_voices(int uid, GList** voice_list);

int ttsd_server_get_current_voice(int uid, char** language, int* voice_type);

int ttsd_server_add_queue(int uid, const char* text, const char* lang, int voice_type, int speed, int utt_id);

int ttsd_server_play(int uid);

int ttsd_server_stop(int uid);

int ttsd_server_pause(int uid, int* utt_id);

/*
* Server API for setting
*/

int ttsd_server_setting_initialize(int uid);

int ttsd_server_setting_finalize(int uid);

int ttsd_server_setting_get_engine_list(int uid, GList** engine_list);

int ttsd_server_setting_get_current_engine(int uid, char** engine_id);

int ttsd_server_setting_set_current_engine(int uid, const char* engine_id);

int ttsd_server_setting_get_voice_list(int uid, char** engine_id, GList** voice_list);

int ttsd_server_setting_get_default_voice(int uid, char** language, ttsp_voice_type_e* vctype);

int ttsd_server_setting_set_default_voice(int uid, const char* language, int vctype);

int ttsd_server_setting_get_default_speed(int uid, int* default_speed);

int ttsd_server_setting_set_default_speed(int uid, int default_speed);

int ttsd_server_setting_get_engine_setting(int uid, char** engine_id, GList** engine_setting_list);

int ttsd_server_setting_set_engine_setting(int uid, const char* key, const char* value);


#ifdef __cplusplus
}
#endif

#endif /* __TTSD_SERVER_CORE_H_ */
