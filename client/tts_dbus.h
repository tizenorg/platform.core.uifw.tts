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

 
#ifndef __TTS_DBUS_H_
#define __TTS_DBUS_H_

#include "tts.h"

#ifdef __cplusplus
extern "C" {
#endif

int tts_dbus_open_connection();

int tts_dbus_close_connection();

int tts_dbus_request_hello(int uid);

int tts_dbus_request_initialize(int uid);

int tts_dbus_request_finalize(int uid);

int tts_dbus_request_get_support_voice(int uid, tts_h tts, tts_supported_voice_cb callback, void* user_data);

int tts_dbus_request_get_default_voice(int uid , char** lang, tts_voice_type_e* vctype);

int tts_dbus_request_add_text(int uid, const char* text, const char* lang, int vctype, int speed, int uttid); 

int tts_dbus_request_remove_all_text(int uid); 


int tts_dbus_request_play(int uid) ;

int tts_dbus_request_stop(int uid);

int tts_dbus_request_pause(int uid);


int tts_file_msg_open_connection();

int tts_file_msg_close_connection();

#ifdef __cplusplus
}
#endif

#endif /* __TTS_DBUS_H_ */
