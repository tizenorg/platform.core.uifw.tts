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

 
#ifndef __TTS_SETTING_DBUS_H_
#define __TTS_SETTING_DBUS_H_

#include "tts_setting.h"

#ifdef __cplusplus
extern "C" 
{
#endif

int tts_setting_dbus_open_connection();

int tts_setting_dbus_close_connection();

int tts_setting_dbus_request_hello();

int tts_setting_dbus_request_initialize();

int tts_setting_dbus_request_finalilze();

int tts_setting_dbus_request_get_engine_list(tts_setting_supported_engine_cb callback, void* user_data);

int tts_setting_dbus_request_get_engine(char** engine_id);

int tts_setting_dbus_request_set_engine(const char* engine_id );

int tts_setting_dbus_request_get_voice_list(tts_setting_supported_voice_cb callback, void* user_data);

int tts_setting_dbus_request_get_default_voice(char** language, tts_setting_voice_type_e* voice_type);

int tts_setting_dbus_request_set_default_voice(const char* language, const int voicetype );

int tts_setting_dbus_request_get_default_speed(int* speed);

int tts_setting_dbus_request_set_default_speed(int speed);

int tts_setting_dbus_request_get_engine_setting(tts_setting_engine_setting_cb callback, void* user_data);

int tts_setting_dbus_request_set_engine_setting(const char* key, const char* value);

#ifdef __cplusplus
}
#endif

#endif /* __TTS_SETTING_DBUS_H_ */
