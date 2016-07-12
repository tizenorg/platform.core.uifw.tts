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


#ifndef __TTSD_MAIN_H_
#define __TTSD_MAIN_H_

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <tizen.h>
#include <unistd.h>
#include <string.h>
#include <glib.h>
#include <dlog.h>
#include <errno.h>

#include "ttse.h"
#include "tts_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* for debug message */
#define DATA_DEBUG
#define ENGINE_AGENT_DEBUG

typedef enum {
	TTSD_ERROR_NONE				= TIZEN_ERROR_NONE,		/**< Successful */
	TTSD_ERROR_OUT_OF_MEMORY		= TIZEN_ERROR_OUT_OF_MEMORY,	/**< Out of Memory */
	TTSD_ERROR_IO_ERROR			= TIZEN_ERROR_IO_ERROR,		/**< I/O error */
	TTSD_ERROR_INVALID_PARAMETER		= TIZEN_ERROR_INVALID_PARAMETER,/**< Invalid parameter */
	TTSD_ERROR_OUT_OF_NETWORK		= TIZEN_ERROR_NETWORK_DOWN,	/**< Out of network */
	TTSD_ERROR_TIMED_OUT			= TIZEN_ERROR_TIMED_OUT,	/**< No answer from the daemon */
	TTSD_ERROR_PERMISSION_DENIED		= TIZEN_ERROR_PERMISSION_DENIED,/**< Permission denied */
	TTSD_ERROR_NOT_SUPPORTED		= TIZEN_ERROR_NOT_SUPPORTED,	/**< TTS NOT supported */
	TTSD_ERROR_INVALID_STATE		= TIZEN_ERROR_TTS | 0x01,	/**< Invalid state */
	TTSD_ERROR_INVALID_VOICE		= TIZEN_ERROR_TTS | 0x02,	/**< Invalid voice */
	TTSD_ERROR_ENGINE_NOT_FOUND		= TIZEN_ERROR_TTS | 0x03,	/**< No available engine */
	TTSD_ERROR_OPERATION_FAILED		= TIZEN_ERROR_TTS | 0x04,	/**< Operation failed */
	TTSD_ERROR_AUDIO_POLICY_BLOCKED		= TIZEN_ERROR_TTS | 0x05,	/**< Audio policy blocked */
	TTSD_ERROR_NOT_SUPPORTED_FEATURE	= TIZEN_ERROR_TTS | 0x06,	/**< Not supported feature of current engine*/
	TTSD_ERROR_SERVICE_RESET		= TIZEN_ERROR_TTS | 0x07	/**< Service reset */
} ttsd_error_e;

typedef enum {
	TTSD_MODE_DEFAULT = 0,		/**< Default mode for normal application */
	TTSD_MODE_NOTIFICATION,		/**< Notification mode */
	TTSD_MODE_SCREEN_READER		/**< Screen reader mode */
} ttsd_mode_e;

typedef enum {
	TTSD_INTERRUPTED_PAUSED = 0,	/**< Current state change 'Pause' */
	TTSD_INTERRUPTED_STOPPED	/**< Current state change 'Ready' */
} ttsd_interrupted_code_e;

typedef struct {
	char* engine_id;
	char* engine_name;
	char* ug_name;
} engine_s;

typedef struct {
	char* language;
	int type;
}voice_s;

typedef struct {
	int size;
	int version;

	ttse_get_info_cb			get_info;

	ttse_initialize_cb			initialize;
	ttse_deinitialize_cb			deinitialize;

	ttse_foreach_supported_voices_cb	foreach_voices;
	ttse_is_valid_voice_cb			is_valid_voice;
	ttse_set_pitch_cb			set_pitch;

	ttse_load_voice_cb			load_voice;
	ttse_unload_voice_cb			unload_voice;

	ttse_start_synthesis_cb 		start_synth;
	ttse_cancel_synthesis_cb		cancel_synth;

	ttse_check_app_agreed_cb		check_app_agreed;
	ttse_need_app_credential_cb		need_app_credential;
	ttse_private_data_set_cb		private_data_set;
	ttse_private_data_requested_cb		private_data_requested;
	
} tts_engine_callback_s;


/* get daemon mode : default, notification or screen reader */
ttsd_mode_e ttsd_get_mode();

/* Get log tag : default, notification, screen reader */
//const char* get_tag();
const char* tts_tag();

#ifdef __cplusplus
}
#endif

#endif	/* __TTSD_MAIN_H_ */
