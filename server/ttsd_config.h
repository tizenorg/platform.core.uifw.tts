/*
*  Copyright (c) 2011 Samsung Electronics Co., Ltd All Rights Reserved 
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

#define TTSD_CONFIG_PREFIX "db/ttsd/"

#define CONFIG_KEY_DEFAULT_ENGINE_ID	TTSD_CONFIG_PREFIX"engine"
#define CONFIG_KEY_DEFAULT_LANGUAGE	TTSD_CONFIG_PREFIX"language"
#define CONFIG_KEY_DEFAULT_VOICE_TYPE	TTSD_CONFIG_PREFIX"vctype"
#define CONFIG_KEY_DEFAULT_SPEED	TTSD_CONFIG_PREFIX"speed"

/*
* tts-daemon config
*/

int ttsd_config_get_char_type(const char* key, char** value);

int ttsd_config_set_char_type(const char* key, const char* value);

int ttsd_config_get_bool_type(const char* key, bool* value);

int ttsd_config_set_bool_type(const char* key, const bool value);

int ttsd_config_get_int_type(const char* key, int* value);

int ttsd_config_set_int_type(const char* key, const int value);

/*
* interface for engine plug-in
*/

int ttsd_config_set_persistent_data(const char* engine_id, const char* key, const char* value);

int ttsd_config_get_persistent_data(const char* engine_id, const char* key, char** value);

int ttsd_config_remove_persistent_data(const char* engine_id, const char* key);


#ifdef __cplusplus
}
#endif

#endif /* __TTSD_CONFIG_H_ */
