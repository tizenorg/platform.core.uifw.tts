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

int tts_dbus_set_sound_type(int uid, int type);


int tts_dbus_request_add_text(int uid, const char* text, const char* lang, int vctype, int speed, int uttid); 

int tts_dbus_request_play(int uid) ;

int tts_dbus_request_stop(int uid);

int tts_dbus_request_pause(int uid);

int tts_dbus_request_set_private_data(int uid, const char* key, const char* data);

int tts_dbus_request_get_private_data(int uid, const char* key, char** data);

#ifdef __cplusplus
}
#endif

#endif /* __TTS_DBUS_H_ */
