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


/* For multi-user support */
#include <tzplatform_config.h>

#include "tts_config_mgr.h"
#include "ttsd_config.h"
#include "ttsd_main.h"


static ttsd_config_changed_cb g_callback;

static ttsd_config_screen_reader_changed_cb g_sr_callback;

void __ttsd_config_engine_changed_cb(const char* engine_id, const char* setting, const char* language, int voice_type, bool auto_voice, bool need_credential, void* user_data)
{
	/* Need to check engine is valid */
	if (false == tts_config_check_default_engine_is_valid(engine_id)) {
		SLOG(LOG_ERROR, get_tag(), "Engine id is NOT valid : %s", engine_id);
		return;
	}

	if (NULL != g_callback) {
		SECURE_SLOG(LOG_DEBUG, get_tag(), "Call the engine reload callback : engine id(%s)", engine_id);
		g_callback(TTS_CONFIG_TYPE_ENGINE, engine_id, 0);
		SECURE_SLOG(LOG_DEBUG, get_tag(), "Call the voice changed callback : lang(%s), type(%d)", language, voice_type);
		g_callback(TTS_CONFIG_TYPE_VOICE, language, voice_type);
	} else {
		SLOG(LOG_ERROR, get_tag(), "Config changed callback is NULL");
	}
}

void __ttsd_config_voice_changed_cb(const char* before_language, int before_type, const char* language, int type, bool auto_voice, void* user_data)
{
	/* Need to check voice is valid */
	if (false == tts_config_check_default_voice_is_valid(language, type)) {
		SLOG(LOG_ERROR, get_tag(), "Lang(%s) type(%d) is NOT valid : %s", language, type);
		return;
	}

	if (NULL != g_callback) {
		g_callback(TTS_CONFIG_TYPE_VOICE, language, type);
	} else {
		SLOG(LOG_ERROR, get_tag(), "Config changed callback is NULL");
	}
}

void __ttsd_config_speech_rate_changed_cb(int value, void* user_data)
{
	if (NULL != g_callback) {
		g_callback(TTS_CONFIG_TYPE_SPEED, NULL, value);
	} else {
		SLOG(LOG_ERROR, get_tag(), "Config changed callback is NULL");
	}
}

void __ttsd_config_pitch_changed_cb(int value, void* user_data)
{
	if (NULL != g_callback) {
		g_callback(TTS_CONFIG_TYPE_PITCH, NULL, value);
	} else {
		SLOG(LOG_ERROR, get_tag(), "Config changed callback is NULL");
	}
}

void __ttsd_config_screen_reader_changed_cb(bool value)
{
	if (NULL != g_sr_callback) {
		g_sr_callback(value);
	}
}

int ttsd_config_initialize(ttsd_config_changed_cb config_cb)
{
	if (NULL == config_cb) {
		SLOG(LOG_ERROR, get_tag(), "[Config] Invalid parameter");
		return -1;
	}

	g_callback = config_cb;
	g_sr_callback = NULL;

	int ret = -1;
	ret = tts_config_mgr_initialize(getpid());
	if (0 != ret) {
		SLOG(LOG_ERROR, get_tag(), "[Config] Fail to initialize config manager");
		return -1;
	}

	ret = tts_config_mgr_set_callback(getpid(), __ttsd_config_engine_changed_cb, __ttsd_config_voice_changed_cb, 
		__ttsd_config_speech_rate_changed_cb, __ttsd_config_pitch_changed_cb, NULL);
	if (0 != ret) {
		SLOG(LOG_ERROR, get_tag(), "[ERROR] Fail to set config changed : %d", ret);
		return -1;
	}

	return 0;
}

int ttsd_config_finalize()
{
	tts_config_unset_screen_reader_callback(getpid());

	tts_config_mgr_finalize(getpid());

	return 0;
}

int ttsd_config_set_screen_reader_callback(ttsd_config_screen_reader_changed_cb sr_cb)
{
	if (NULL == sr_cb) {
		SLOG(LOG_ERROR, get_tag(), "[Config] Invalid parameter");
		return -1;
	}

	g_sr_callback = sr_cb;

	int ret = tts_config_set_screen_reader_callback(getpid(), __ttsd_config_screen_reader_changed_cb) ;
	if (0 != ret) {
		SLOG(LOG_ERROR, get_tag(), "[Config] Fail to set screen reader callback");
		return -1;
	}
	return 0;
}

int ttsd_config_get_default_engine(char** engine_id)
{
	if (NULL == engine_id)
		return -1;

	if (0 != tts_config_mgr_get_engine(engine_id)) {
		SLOG(LOG_ERROR, get_tag(), "[Config ERROR] Fail to get engine id");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	return 0;
}

int ttsd_config_set_default_engine(const char* engine_id)
{
	if (NULL == engine_id)
		return -1;

	if (true == tts_config_check_default_engine_is_valid(engine_id)) {
		if (0 != tts_config_mgr_set_engine(engine_id)) {
			SLOG(LOG_ERROR, get_tag(), "[Config ERROR] Fail to set engine id");
		}
	}

	return 0;
}

int ttsd_config_get_default_voice(char** language, int* type)
{
	if (NULL == language || NULL == type)
		return -1;

	if (0 != tts_config_mgr_get_voice(language, type)) {
		SLOG(LOG_ERROR, get_tag(), "[Config ERROR] Fail to get default voice");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	return 0;
}

int ttsd_config_get_default_speed(int* speed)
{
	if (NULL == speed)
		return -1;

	if (0 != tts_config_mgr_get_speech_rate(speed)) {
		SLOG(LOG_ERROR, get_tag(), "[Config ERROR] Fail to get default speech rate");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	return 0;
}

int ttsd_config_get_default_pitch(int* pitch)
{
	if (NULL == pitch)
		return -1;

	if (0 != tts_config_mgr_get_pitch(pitch)) {
		SLOG(LOG_ERROR, get_tag(), "[Config ERROR] Fail to get default pitch");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	return 0;
}

int ttsd_config_save_error(int uid, int uttid, const char* lang, int vctype, const char* text, 
			   const char* func, int line, const char* message)
{
	SLOG(LOG_ERROR, get_tag(), "=============== TTS ERROR LOG ====================");

	SLOG(LOG_ERROR, get_tag(), "uid(%d) uttid(%d)", uid, uttid);
	
	if (NULL != func)	SLOG(LOG_ERROR, get_tag(), "Function(%s) Line(%d)", func, line);
	if (NULL != message)	SLOG(LOG_ERROR, get_tag(), "Message(%s)", message);
	if (NULL != lang)	SLOG(LOG_ERROR, get_tag(), "Lang(%s), type(%d)", lang, vctype);
	if (NULL != text)	SLOG(LOG_ERROR, get_tag(), "Text(%s)", text);

	SLOG(LOG_ERROR, get_tag(), "==================================================");

	return 0;
}