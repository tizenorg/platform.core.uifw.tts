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


#ifndef __TTSD_SERVER_CORE_H_
#define __TTSD_SERVER_CORE_H_

#include <glib.h>
#include <Ecore.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
* Server APIs
*/
int ttsd_initialize();

int ttsd_finalize();

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


#ifdef __cplusplus
}
#endif

#endif /* __TTSD_SERVER_CORE_H_ */
