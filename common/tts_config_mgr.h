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


#ifndef __TTS_CONFIG_MANAGER_H_
#define __TTS_CONFIG_MANAGER_H_

#include <errno.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	TTS_CONFIG_ERROR_NONE			= 0,		/**< Success, No error */
	TTS_CONFIG_ERROR_OUT_OF_MEMORY		= -ENOMEM,	/**< Out of Memory */
	TTS_CONFIG_ERROR_IO_ERROR		= -EIO,		/**< I/O error */
	TTS_CONFIG_ERROR_INVALID_PARAMETER	= -EINVAL,	/**< Invalid parameter */
	TTS_CONFIG_ERROR_INVALID_STATE		= -0x0100021,	/**< Invalid state */
	TTS_CONFIG_ERROR_INVALID_VOICE		= -0x0100022,	/**< Invalid voice */
	TTS_CONFIG_ERROR_ENGINE_NOT_FOUND	= -0x0100023,	/**< No available TTS-engine  */
	TTS_CONFIG_ERROR_OPERATION_FAILED	= -0x0100024,	/**< Operation failed  */
	TTS_CONFIG_ERROR_NOT_SUPPORTED_FEATURE	= -0x0100025	/**< Not supported feature of current engine */
} tts_config_error_e;

typedef enum {
	TTS_CONFIG_SPEED_AUTO		= 0,	/**< Speed from settings */
	TTS_CONFIG_SPEED_MIN		= 1,	/**< Min value */
	TTS_CONFIG_SPEED_VERY_SLOW	= 2,	/**< Very slow */
	TTS_CONFIG_SPEED_SLOW		= 5,	/**< Slow */
	TTS_CONFIG_SPEED_NORMAL		= 8,	/**< Normal */
	TTS_CONFIG_SPEED_FAST		= 11,	/**< Fast */
	TTS_CONFIG_SPEED_VERY_FAST	= 14,	/**< Very fast */
	TTS_CONFIG_SPEED_MAX		= 15	/**< Max value */
} tts_config_speed_e;


typedef bool (*tts_config_supported_engine_cb)(const char* engine_id, const char* engine_name, const char* setting, void* user_data);

typedef bool (*tts_config_supported_voice_cb)(const char* engine_id, const char* language, int type, void* user_data);

typedef void (*tts_config_engine_changed_cb)(const char* engine_id, const char* setting, 
					     const char* language, int voice_type, bool auto_voice, void* user_data);

typedef void (*tts_config_voice_changed_cb)(const char* before_language, int before_voice_type, 
					    const char* current_language, int current_voice_type, 
					    bool auto_voice, void* user_data);

typedef void (*tts_config_speech_rate_changed_cb)(int value, void* user_data);

typedef void (*tts_config_pitch_changed_cb)(int value, void* user_data);

typedef void (*tts_config_screen_reader_changed_cb)(bool value);


int tts_config_mgr_initialize(int uid);

int tts_config_mgr_finalize(int uid);


int tts_config_mgr_set_callback(int uid, 
				tts_config_engine_changed_cb engine_cb, 
				tts_config_voice_changed_cb voice_cb, 
				tts_config_speech_rate_changed_cb speech_cb, 
				tts_config_pitch_changed_cb pitch_cb,
				void* user_data);

int tts_config_mgr_unset_callback(int uid);

/* Only for screen reader option */
int tts_config_set_screen_reader_callback(int uid, tts_config_screen_reader_changed_cb callback);

int tts_config_unset_screen_reader_callback(int uid);


int tts_config_mgr_get_engine_list(tts_config_supported_engine_cb callback, void* user_data);

int tts_config_mgr_get_engine(char** engine);

int tts_config_mgr_set_engine(const char* engine);

int tts_config_mgr_get_voice_list(const char* engine_id, tts_config_supported_voice_cb callback, void* user_data);

int tts_config_mgr_get_voice(char** language, int* type);

int tts_config_mgr_set_voice(const char* language, int type);

int tts_config_mgr_get_auto_voice(bool* value);

int tts_config_mgr_set_auto_voice(bool value);

int tts_config_mgr_get_speech_rate(int* value);

int tts_config_mgr_set_speech_rate(int value);

int tts_config_mgr_get_pitch(int* value);

int tts_config_mgr_set_pitch(int value);

bool tts_config_check_default_engine_is_valid(const char* engine);

bool tts_config_check_default_voice_is_valid(const char* language, int type);

char* tts_config_get_message_path(int mode, int pid);


#ifdef __cplusplus
}
#endif

#endif /* __TTS_CONFIG_MANAGER_H_ */
