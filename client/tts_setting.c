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

#include <dirent.h>
#include <Ecore.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "tts_config_mgr.h"
#include "tts_main.h"
#include "tts_setting.h"

/** 
* @brief Enumerations of setting state.
*/
typedef enum {
	TTS_SETTING_STATE_NONE = 0,
	TTS_SETTING_STATE_READY
}tts_setting_state_e;


static tts_setting_state_e g_state = TTS_SETTING_STATE_NONE;

static tts_setting_supported_engine_cb g_engine_cb;

static tts_setting_engine_changed_cb g_engine_changed_cb;
static void* g_engine_changed_user_data;

static tts_setting_voice_changed_cb g_voice_changed_cb;
static void* g_voice_changed_user_data;

static tts_setting_speed_changed_cb g_speed_changed_cb;
static void* g_speed_changed_user_data;

static tts_setting_pitch_changed_cb g_pitch_changed_cb;
static void* g_pitch_changed_user_data;



const char* tts_tag()
{
	return "ttsc";
}

static int __setting_convert_config_error_code(tts_config_error_e code)
{
	if (code == TTS_CONFIG_ERROR_NONE)			return TTS_SETTING_ERROR_NONE;
	if (code == TTS_CONFIG_ERROR_OUT_OF_MEMORY)		return TTS_SETTING_ERROR_OUT_OF_MEMORY;
	if (code == TTS_CONFIG_ERROR_IO_ERROR)			return TTS_SETTING_ERROR_IO_ERROR;
	if (code == TTS_CONFIG_ERROR_INVALID_PARAMETER)		return TTS_SETTING_ERROR_INVALID_PARAMETER;
	if (code == TTS_CONFIG_ERROR_INVALID_STATE)		return TTS_SETTING_ERROR_INVALID_STATE;
	if (code == TTS_CONFIG_ERROR_INVALID_VOICE)		return TTS_SETTING_ERROR_INVALID_VOICE;
	if (code == TTS_CONFIG_ERROR_ENGINE_NOT_FOUND)		return TTS_SETTING_ERROR_ENGINE_NOT_FOUND;
	if (code == TTS_CONFIG_ERROR_OPERATION_FAILED)		return TTS_SETTING_ERROR_OPERATION_FAILED;
	if (code == TTS_CONFIG_ERROR_NOT_SUPPORTED_FEATURE)	return TTS_SETTING_ERROR_NOT_SUPPORTED_FEATURE;

	return TTS_SETTING_ERROR_NONE;
}

void __setting_config_engine_changed_cb(const char* engine_id, const char* setting, const char* language, int voice_type, bool auto_voice, void* user_data)
{
	SLOG(LOG_DEBUG, TAG_TTSC, "Engine chagned : engine(%s) setting(%s) lang(%s) type(%d)", 
		engine_id, setting, language, language, voice_type);

	if (NULL != g_engine_changed_cb)
		g_engine_changed_cb(engine_id, g_engine_changed_user_data);

	if (NULL != g_voice_changed_cb)
		g_voice_changed_cb(language, voice_type, auto_voice, g_voice_changed_user_data);
}

void __setting_config_voice_changed_cb(const char* before_language, int before_type, const char* language, int voice_type, bool auto_voice, void* user_data)
{
	SLOG(LOG_DEBUG, TAG_TTSC, "Voice changed : lang(%s) type(%d) auto(%s)", language, voice_type, auto_voice ? "on" : "off");

	if (NULL != g_voice_changed_cb)
		g_voice_changed_cb(language, voice_type, auto_voice, g_voice_changed_user_data);
}

void __setting_config_speech_rate_changed_cb(int value, void* user_data)
{
	SLOG(LOG_DEBUG, TAG_TTSC, "Speech rate : %d", value);

	if (NULL != g_speed_changed_cb)
		g_speed_changed_cb(value, g_speed_changed_user_data);
}

void __setting_config_pitch_changed_cb(int value, void* user_data)
{
	SLOG(LOG_DEBUG, TAG_TTSC, "Pitch : %d", value);

	if (NULL != g_pitch_changed_cb)
		g_pitch_changed_cb(value, g_pitch_changed_user_data);
}

int tts_setting_initialize()
{
	SLOG(LOG_DEBUG, TAG_TTSC, "===== Initialize TTS Setting");

	if (TTS_SETTING_STATE_READY == g_state) {
		SLOG(LOG_WARN, TAG_TTSC, "[WARNING] TTS Setting has already been initialized.");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_SETTING_ERROR_NONE;
	}

	int ret = tts_config_mgr_initialize(getpid());
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to initialize config manager : %d", ret);
		return __setting_convert_config_error_code(ret);
	}

	ret = tts_config_mgr_set_callback(getpid(), __setting_config_engine_changed_cb, __setting_config_voice_changed_cb, 
		__setting_config_speech_rate_changed_cb, __setting_config_pitch_changed_cb, NULL);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to set config changed : %d", ret);
		return __setting_convert_config_error_code(ret);
	}

	g_state = TTS_SETTING_STATE_READY;

	g_engine_changed_cb = NULL;
	g_engine_changed_user_data = NULL;

	g_voice_changed_cb = NULL;
	g_voice_changed_user_data = NULL;

	g_speed_changed_cb = NULL;
	g_speed_changed_user_data = NULL;

	g_pitch_changed_cb = NULL;
	g_pitch_changed_user_data = NULL;

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return TTS_SETTING_ERROR_NONE;
}

int tts_setting_finalize()
{
	SLOG(LOG_DEBUG, TAG_TTSC, "===== Finalize TTS Setting");

	tts_config_mgr_finalize(getpid());

	g_state = TTS_SETTING_STATE_NONE;

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return TTS_SETTING_ERROR_NONE;
}

bool __tts_config_mgr_get_engine_list(const char* engine_id, const char* engine_name, const char* setting, void* user_data)
{
	if (NULL != g_engine_cb) {
		return g_engine_cb(engine_id, engine_name, setting, user_data);
	}

	return false;
}

int tts_setting_foreach_supported_engines(tts_setting_supported_engine_cb callback, void* user_data)
{    
	SLOG(LOG_DEBUG, TAG_TTSC, "===== Foreach supported engines");

	if (TTS_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Not initialized");
		return TTS_SETTING_ERROR_INVALID_STATE;
	}

	if (NULL == callback) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Callback is NULL");
		return TTS_SETTING_ERROR_INVALID_PARAMETER;
	}

	g_engine_cb = callback;

	int ret = tts_config_mgr_get_engine_list(__tts_config_mgr_get_engine_list, user_data);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Result : %d", ret);
	}

	g_engine_cb = NULL;

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return __setting_convert_config_error_code(ret);
}

int tts_setting_get_engine(char** engine_id)
{
	SLOG(LOG_DEBUG, TAG_TTSC, "===== Get current engine");

	if (TTS_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Not initialized");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_SETTING_ERROR_INVALID_STATE;
	}

	if (NULL == engine_id) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Engine id is NULL");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_SETTING_ERROR_INVALID_PARAMETER;
	}

	int ret = tts_config_mgr_get_engine(engine_id);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Result : %d", ret);
	} else {
		SLOG(LOG_DEBUG, TAG_TTSC, "[SUCCESS] Get current engine");
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return __setting_convert_config_error_code(ret);
}

int tts_setting_set_engine(const char* engine_id)
{
	SLOG(LOG_DEBUG, TAG_TTSC, "===== Set current engine");

	if (TTS_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Not initialized");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_SETTING_ERROR_INVALID_STATE;
	}

	if (NULL == engine_id) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Engine id is NULL");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_SETTING_ERROR_INVALID_PARAMETER;
	}

	int ret = tts_config_mgr_set_engine(engine_id);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Result : %d", ret);
	} else {
		SECURE_SLOG(LOG_DEBUG, TAG_TTSC, "[SUCCESS] Set current engine : %s", engine_id);
	}
	
	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");
    
	return __setting_convert_config_error_code(ret);
}

int tts_setting_foreach_surpported_voices(tts_setting_supported_voice_cb callback, void* user_data)
{
	SLOG(LOG_DEBUG, TAG_TTSC, "===== Foreach supported voices");

	if (TTS_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Not initialized");
		return TTS_SETTING_ERROR_INVALID_STATE;
	}

	if (NULL == callback) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Param is NULL");
		return TTS_SETTING_ERROR_INVALID_PARAMETER;
	}

	char* current_engine = NULL;
	int ret = tts_config_mgr_get_engine(&current_engine);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to get current engine : %d", ret);
		return TTS_SETTING_ERROR_OPERATION_FAILED;
	}

	ret = tts_config_mgr_get_voice_list(current_engine, (tts_config_supported_voice_cb)callback, user_data);

	if (NULL != current_engine)
		free(current_engine);

	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Result : %d", ret);
	} else {
		SLOG(LOG_DEBUG, TAG_TTSC, "[SUCCESS] Foreach supported voices");
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return __setting_convert_config_error_code(ret);
}

int tts_setting_get_voice(char** language, int* voice_type)
{
	SLOG(LOG_DEBUG, TAG_TTSC, "===== Get default voice");

	if (TTS_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Not initialized");
		return TTS_SETTING_ERROR_INVALID_STATE;
	}

	if (NULL == language || NULL == voice_type) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Input parameter is NULL");
		return TTS_SETTING_ERROR_INVALID_PARAMETER;
	}

	int ret = tts_config_mgr_get_voice(language, (int*)voice_type);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Result : %d", ret);
	} else {
		SECURE_SLOG(LOG_DEBUG, TAG_TTSC, "[SUCCESS] Get default voices : lang(%s) type(%d)", *language, *voice_type);
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return __setting_convert_config_error_code(ret);
}

int tts_setting_set_voice(const char* language, int voice_type)
{
	SLOG(LOG_DEBUG, TAG_TTSC, "===== Set default voice");

	if (TTS_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Not initialized");
		return TTS_SETTING_ERROR_INVALID_STATE;
	}

	if (NULL == language) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Input parameter is NULL");
		return TTS_SETTING_ERROR_INVALID_PARAMETER;
	}

	if (voice_type < TTS_SETTING_VOICE_TYPE_MALE || TTS_SETTING_VOICE_TYPE_CHILD < voice_type) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Invalid voice type");
		return TTS_SETTING_ERROR_INVALID_PARAMETER;
	}

	int ret = tts_config_mgr_set_voice(language, (int)voice_type);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Result : %d", ret);
	} else {
		SECURE_SLOG(LOG_DEBUG, TAG_TTSC, "[SUCCESS] Set default voice : lang(%s) type(%d)",language, voice_type);
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");
    
	return __setting_convert_config_error_code(ret);
}

int tts_setting_set_auto_voice(bool value)
{
	SLOG(LOG_DEBUG, TAG_TTSC, "===== Set auto voice");

	if (TTS_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Not initialized");
		return TTS_SETTING_ERROR_INVALID_STATE;
	}

	if (value != true && value != false) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Invalid value");
		return TTS_SETTING_ERROR_INVALID_PARAMETER;
	}

	int ret = tts_config_mgr_set_auto_voice(value);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Result : %d", ret);
	} else {
		SECURE_SLOG(LOG_DEBUG, TAG_TTSC, "[SUCCESS] Set auto voice %s", value ? "on" : "off");
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return __setting_convert_config_error_code(ret);
}

int tts_setting_get_auto_voice(bool* value)
{
	SLOG(LOG_DEBUG, TAG_TTSC, "===== Get auto voice");

	if (TTS_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Not initialized");
		return TTS_SETTING_ERROR_INVALID_STATE;
	}

	if (NULL == value) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Param is NULL");
		return TTS_SETTING_ERROR_INVALID_PARAMETER;
	}

	int ret = tts_config_mgr_get_auto_voice(value);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Result : %d", ret);
	} else {
		SECURE_SLOG(LOG_DEBUG, TAG_TTSC, "[SUCCESS] Get auto voice : %d ", (int)*value);
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return __setting_convert_config_error_code(ret);
}

int tts_setting_get_speed_range(int* min, int* normal, int* max)
{
	SLOG(LOG_DEBUG, TAG_TTSC, "===== Get speed range");

	if (TTS_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Not initialized");
		return TTS_SETTING_ERROR_INVALID_STATE;
	}

	if (NULL == min || NULL == normal || NULL == max) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Input parameter is null");
		return TTS_SETTING_ERROR_INVALID_PARAMETER;
	}

	*min = TTS_SPEED_MIN;
	*normal = TTS_SPEED_NORMAL;
	*max = TTS_SPEED_MAX;

	SLOG(LOG_DEBUG, TAG_TTSC, "[SUCCESS] Get speed range : min(%d) normal(%d) max(%d)", *min, *normal, *max);

	return TTS_SETTING_ERROR_NONE;
}

int tts_setting_get_speed(int* speed)
{
	SLOG(LOG_DEBUG, TAG_TTSC, "===== Get default speed");

	if (TTS_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Not initialized");
		return TTS_SETTING_ERROR_INVALID_STATE;
	}

	if (NULL == speed) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Param is NULL");
		return TTS_SETTING_ERROR_INVALID_PARAMETER;
	}

	int temp;
	temp = 0;

	int ret = tts_config_mgr_get_speech_rate(&temp);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Result : %d", ret);
	} else {
		/* Copy value */
		*speed = temp;
		SECURE_SLOG(LOG_DEBUG, TAG_TTSC, "[SUCCESS] Get default speed : %d ", *speed);
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return __setting_convert_config_error_code(ret);
}

int tts_setting_set_speed(int speed)
{
	SLOG(LOG_DEBUG, TAG_TTSC, "===== Set default speed");

	if (TTS_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Not initialized");
		return TTS_SETTING_ERROR_INVALID_STATE;
	}

	if (TTS_SPEED_MIN > speed || speed > TTS_SPEED_MAX) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Invalid speed");
		return TTS_SETTING_ERROR_INVALID_PARAMETER;
	}

	int ret = tts_config_mgr_set_speech_rate(speed);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Result : %d", ret);
	} else {
		SECURE_SLOG(LOG_DEBUG, TAG_TTSC, "[SUCCESS] Set default speed, %d", speed);
	}
	
	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return __setting_convert_config_error_code(ret);
}

int tts_setting_get_pitch_range(int* min, int* normal, int* max)
{
	SLOG(LOG_DEBUG, TAG_TTSC, "===== Get speed range");

	if (TTS_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Not initialized");
		return TTS_SETTING_ERROR_INVALID_STATE;
	}

	if (NULL == min || NULL == normal || NULL == max) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Input parameter is null");
		return TTS_SETTING_ERROR_INVALID_PARAMETER;
	}

	*min = TTS_PITCH_MIN;
	*normal = TTS_PITCH_NORMAL;
	*max = TTS_PITCH_MAX;

	SLOG(LOG_DEBUG, TAG_TTSC, "[SUCCESS] Get pitch range : min(%d) normal(%d) max(%d)", *min, *normal, *max);

	return TTS_SETTING_ERROR_NONE;
}

int tts_setting_set_pitch(int pitch)
{
	SLOG(LOG_DEBUG, TAG_TTSC, "===== Set default pitch");

	if (TTS_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Not initialized");
		return TTS_SETTING_ERROR_INVALID_STATE;
	}

	if (TTS_PITCH_MIN > pitch || pitch > TTS_PITCH_MAX) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Invalid pitch");
		return TTS_SETTING_ERROR_INVALID_PARAMETER;
	}

	int ret = tts_config_mgr_set_pitch(pitch);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Result : %d", ret);
	} else {
		SECURE_SLOG(LOG_DEBUG, TAG_TTSC, "[SUCCESS] Set default pitch, %d", pitch);
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return __setting_convert_config_error_code(ret);
}

int tts_setting_get_pitch(int* pitch)
{
	SLOG(LOG_DEBUG, TAG_TTSC, "===== Get default pitch");

	if (TTS_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Not initialized");
		return TTS_SETTING_ERROR_INVALID_STATE;
	}

	if (NULL == pitch) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Input parameter is NULL");
		return TTS_SETTING_ERROR_INVALID_PARAMETER;
	}

	int temp;
	temp = 0;

	int ret = tts_config_mgr_get_pitch(&temp);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Result : %d", ret);
	} else {
		/* Copy value */
		*pitch = temp;
		SECURE_SLOG(LOG_DEBUG, TAG_TTSC, "[SUCCESS] Get default pitch : %d ", *pitch);
	}

	return __setting_convert_config_error_code(ret);
}

int tts_setting_set_engine_changed_cb(tts_setting_engine_changed_cb callback, void* user_data)
{
	if (NULL == callback) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Input param is NULL");
		return TTS_SETTING_ERROR_INVALID_PARAMETER;
	}

	g_engine_changed_cb = callback;
	g_engine_changed_user_data = user_data;

	return TTS_SETTING_ERROR_NONE;
}

int tts_setting_unset_engine_changed_cb()
{
	g_engine_changed_cb = NULL;
	g_engine_changed_user_data = NULL;

	return TTS_SETTING_ERROR_NONE;
}

int tts_setting_set_voice_changed_cb(tts_setting_voice_changed_cb callback, void* user_data)
{
	if (NULL == callback) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Input param is NULL");
		return TTS_SETTING_ERROR_INVALID_PARAMETER;
	}

	g_voice_changed_cb = callback;
	g_voice_changed_user_data = user_data;

	return TTS_SETTING_ERROR_NONE;
}

int tts_setting_unset_voice_changed_cb()
{
	g_voice_changed_cb = NULL;
	g_voice_changed_user_data = NULL;

	return TTS_SETTING_ERROR_NONE;
}

int tts_setting_set_speed_changed_cb(tts_setting_speed_changed_cb callback, void* user_data)
{
	if (NULL == callback) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Input param is NULL");
		return TTS_SETTING_ERROR_INVALID_PARAMETER;
	}

	g_speed_changed_cb = callback;
	g_speed_changed_user_data = user_data;

	return TTS_SETTING_ERROR_NONE;
}

int tts_setting_unset_speed_changed_cb()
{
	g_speed_changed_cb = NULL;
	g_speed_changed_user_data = NULL;

	return TTS_SETTING_ERROR_NONE;
}


int tts_setting_set_pitch_changed_cb(tts_setting_pitch_changed_cb callback, void* user_data)
{
	if (NULL == callback) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Input param is NULL");
		return TTS_SETTING_ERROR_INVALID_PARAMETER;
	}

	g_pitch_changed_cb = callback;
	g_pitch_changed_user_data = user_data;

	return TTS_SETTING_ERROR_NONE;
}

int tts_setting_unset_pitch_changed_cb()
{
	g_pitch_changed_cb = NULL;
	g_pitch_changed_user_data = NULL;

	return TTS_SETTING_ERROR_NONE;
}
