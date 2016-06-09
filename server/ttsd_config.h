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


#ifndef __TTSD_CONFIG_H_
#define __TTSD_CONFIG_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	TTS_CONFIG_TYPE_ENGINE,
	TTS_CONFIG_TYPE_VOICE,
	TTS_CONFIG_TYPE_SPEED,
	TTS_CONFIG_TYPE_PITCH
} tts_config_type_e;

typedef void (*ttsd_config_changed_cb)(tts_config_type_e type, const char* str_param, int int_param);

typedef void (*ttsd_config_screen_reader_changed_cb)(bool value);

int ttsd_config_initialize(ttsd_config_changed_cb config_cb);

int ttsd_config_finalize();

int ttsd_config_set_screen_reader_callback(ttsd_config_screen_reader_changed_cb sr_cb);

int ttsd_config_get_default_engine(char** engine_id);

int ttsd_config_set_default_engine(const char* engine_id);

int ttsd_config_get_default_voice(char** language, int* type);

int ttsd_config_get_default_speed(int* speed);

int ttsd_config_get_default_pitch(int* pitch);

int ttsd_config_save_error(int uid, int uttid, const char* lang, int vctype, const char* text, 
			   const char* func, int line, const char* message);

#ifdef __cplusplus
}
#endif

#endif /* __TTSD_CONFIG_H_ */
