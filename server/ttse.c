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


#include "ttsd_main.h"
#include "ttsd_server.h"
#include "ttsd_dbus.h"
#include "ttsd_network.h"

#include <bundle.h>
#include <bundle_internal.h>
#include <dlog.h>
#include <Ecore.h>

#include "ttse.h"

#define CLIENT_CLEAN_UP_TIME 500

static Ecore_Timer* g_check_client_timer = NULL;

static ttsd_mode_e g_tts_mode = TTSD_MODE_DEFAULT;

const char* tts_tag()
{
	if (TTSD_MODE_NOTIFICATION == g_tts_mode) {
		return "ttsdnoti";
	} else if (TTSD_MODE_SCREEN_READER == g_tts_mode) {
		return "ttsdsr";
	} else {
		return "ttsd";
	}
}

ttsd_mode_e ttsd_get_mode()
{
	return g_tts_mode;
}

void ttsd_set_mode(ttsd_mode_e mode)
{
	g_tts_mode = mode;
	return;
}

int ttse_main(int argc, char** argv, ttse_request_callback_s *callback)
{
	bundle *b = NULL;
	ttsd_mode_e mode = TTSD_MODE_DEFAULT;

	b = bundle_import_from_argv(argc, argv);
	if (NULL != b) {
		char *val = NULL;
		if (0 == bundle_get_str(b, "mode", &val)) {
			if (NULL != val) {
				if (!strcmp("noti", val)) {
					mode = TTSD_MODE_NOTIFICATION;
				} else if (!strcmp("sr", val)) {
					mode = TTSD_MODE_SCREEN_READER;
				} else {
					SLOG(LOG_WARN, tts_tag(), "[WARNING] mode (%s)", val);
				}
			} else {
				SLOG(LOG_ERROR, tts_tag(), "[ERROR] NULL data");
			}
		} else {
			SLOG(LOG_ERROR, tts_tag(), "[ERROR] Fail to get data from bundle");
		}
		bundle_free(b);
	} else {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Fail to get bundle");
	}

	ttsd_set_mode(mode);

	SLOG(LOG_DEBUG, tts_tag(), "Start engine as [%d] mode", mode);

	if (!ecore_init()) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Fail to initialize Ecore");
		return -1;
	}

	if (0 != ttsd_dbus_open_connection()) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Fail to open dbus connection");
		return -1;
	}

	if (0 != ttsd_initialize(callback)) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Fail to initialize");
		return -1;
	}

	if (0 != ttsd_network_initialize()) {
		SLOG(LOG_WARN, tts_tag(), "[WARNING] Fail to initialize network");
	}

	g_check_client_timer = ecore_timer_add(CLIENT_CLEAN_UP_TIME, ttsd_cleanup_client, NULL);
	if (NULL == g_check_client_timer) {
		SLOG(LOG_WARN, tts_tag(), "[WARNING] Fail to create timer");
	}

	SLOG(LOG_DEBUG, tts_tag(), "====");
	SLOG(LOG_DEBUG, tts_tag(), "");

	return 0;
}

int ttse_get_speed_range(int* min, int* normal, int* max)
{
	if (NULL == min || NULL == normal || NULL == max) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Input parameter is null");
		return TTSE_ERROR_INVALID_PARAMETER;
	}

	*min = TTS_SPEED_MIN;
	*normal = TTS_SPEED_NORMAL;
	*max = TTS_SPEED_MAX;

	return 0;
}

int ttse_get_pitch_range(int* min, int* normal, int* max)
{
	if (NULL == min || NULL == normal || NULL == max) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Input parameter is null");
		return TTSE_ERROR_INVALID_PARAMETER;
	}

	*min = TTS_PITCH_MIN;
	*normal = TTS_PITCH_NORMAL;
	*max = TTS_PITCH_MAX;

	return 0;
}

int ttse_send_result(ttse_result_event_e event, const void* data, unsigned int data_size, ttse_audio_type_e audio_type, int rate, void* user_data)
{
	int ret;

	if (NULL == data) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Input parameter is null");
		return TTSE_ERROR_INVALID_PARAMETER;
	}

	ret = ttsd_send_result(event, data, data_size, audio_type, rate, user_data);

	if (0 != ret) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Fail to send result");
	}

	return ret;
}

int ttse_send_error(ttse_error_e error, const char* msg)
{
	int ret;

	if (NULL == msg) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Input parameter is null");
		return TTSE_ERROR_INVALID_PARAMETER;
	}

	ret = ttsd_send_error(error, msg);

	if (0 != ret) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Fail to send error");
	}

	return ret;
}

int ttse_set_private_data_set_cb(ttse_private_data_set_cb callback_func)
{
	int ret = ttsd_set_private_data_set_cb(callback_func);

	if (0 != ret) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Fail to set private data set cb");
	}

	return ret;
}

int ttse_set_private_data_requested_cb(ttse_private_data_requested_cb callback_func)
{
	int ret = ttsd_set_private_data_requested_cb(callback_func);

	if (0 != ret) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Fail to set private data requested cb");
	}

	return ret;
}
