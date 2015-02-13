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


#ifndef _TTS_DEFS_H__
#define _TTS_DEFS_H__

/* For multi-user support */
#include <tzplatform_config.h>
#include <vconf.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************************
* Definition for IPC
*******************************************************************************************/

#define TTS_CLIENT_SERVICE_NAME         "org.tizen.voice.ttsclient"
#define TTS_CLIENT_SERVICE_OBJECT_PATH	"/org/tizen/voice/ttsclient"
#define TTS_CLIENT_SERVICE_INTERFACE	"org.tizen.voice.ttsclient"

#define TTS_SERVER_SERVICE_NAME		"service.connect.ttsserver"
#define TTS_SERVER_SERVICE_OBJECT_PATH	"/org/tizen/voice/ttsserver"
#define TTS_SERVER_SERVICE_INTERFACE	"org.tizen.voice.ttsserver"

#define TTS_NOTI_SERVER_SERVICE_NAME		"service.connect.ttsnotiserver"
#define TTS_NOTI_SERVER_SERVICE_OBJECT_PATH	"/org/tizen/voice/ttsnotiserver"
#define TTS_NOTI_SERVER_SERVICE_INTERFACE	"org.tizen.voice.ttsnotiserver"

#define TTS_SR_SERVER_SERVICE_NAME		"service.connect.ttssrserver"
#define TTS_SR_SERVER_SERVICE_OBJECT_PATH	"/org/tizen/voice/ttssrserver"
#define TTS_SR_SERVER_SERVICE_INTERFACE		"org.tizen.voice.ttssrserver"

/******************************************************************************************
* Message Definition for APIs
*******************************************************************************************/

#define TTS_METHOD_HELLO		"tts_method_hello"
#define TTS_METHOD_INITIALIZE		"tts_method_initialize"
#define TTS_METHOD_FINALIZE		"tts_method_finalilze"
#define TTS_METHOD_GET_SUPPORT_VOICES	"tts_method_get_support_voices"
#define TTS_METHOD_GET_CURRENT_VOICE	"tts_method_get_current_voice"
#define TTS_METHOD_ADD_QUEUE		"tts_method_add_queue"
#define TTS_METHOD_PLAY			"tts_method_play"
#define TTS_METHOD_STOP			"tts_method_stop"
#define TTS_METHOD_PAUSE		"tts_method_pause"

#define TTSD_METHOD_HELLO		"ttsd_method_hello"
#define TTSD_METHOD_UTTERANCE_STARTED	"ttsd_method_utterance_started"
#define TTSD_METHOD_UTTERANCE_COMPLETED	"ttsd_method_utterance_completed"
#define TTSD_METHOD_ERROR		"ttsd_method_error"
#define TTSD_METHOD_SET_STATE		"ttsd_method_set_state"
#define TTSD_METHOD_GET_STATE		"ttsd_method_get_state"


/******************************************************************************************
* Defines for configuration
*******************************************************************************************/

#define TTS_USR_BASE			"/usr/lib/voice/tts/1.0"
#define TTS_OPT_BASE			"/opt/usr/data/voice/tts/1.0"

#define TTS_DEFAULT_CONFIG		TTS_USR_BASE"/tts-config.xml"
#define TTS_CONFIG			tzplatform_mkpath(TZ_USER_HOME, ".voice/tts-config.xml")

#define MESSAGE_FILE_PATH_ROOT		tzplatform_mkpath(TZ_USER_HOME, ".voice/")

#define MESSAGE_FILE_PREFIX_DEFAULT		"ttsd_msg"
#define MESSAGE_FILE_PREFIX_NOTIFICATION	"ttsdnoti_msg"
#define MESSAGE_FILE_PREFIX_SCREEN_READER	"ttsdsr_msg"

#define TTS_DEFAULT_ENGINE		TTS_USR_BASE"/engine"
#define TTS_DOWNLOAD_ENGINE		TTS_OPT_BASE"/engine"

#define TTS_DEFAULT_ENGINE_INFO		TTS_USR_BASE"/engine-info"
#define TTS_DOWNLOAD_ENGINE_INFO	TTS_OPT_BASE"/engine-info"

#define TTS_DEFAULT_ENGINE_SETTING	TTS_USR_BASE"/engine-setting"
#define TTS_DOWNLOAD_ENGINE_SETTING	TTS_OPT_BASE"/engine-setting"

#define TTS_BASE_LANGUAGE		"en_US"

#define TTS_SPEED_MIN		1
#define TTS_SPEED_NORMAL	8
#define TTS_SPEED_MAX		15

#define TTS_PITCH_MIN		1
#define TTS_PITCH_NORMAL	8
#define TTS_PITCH_MAX		15

#define TTS_MAX_TEXT_SIZE	2000

#define TTS_FEATURE_PATH	"tizen.org/feature/speech.synthesis"

/******************************************************************************************
* Defines for vconf key
*******************************************************************************************/

#define TTS_ACCESSIBILITY_KEY		VCONFKEY_SETAPPL_ACCESSIBILITY_TTS

#define TTS_ACCESSIBILITY_SPEED_KEY	VCONFKEY_SETAPPL_ACCESSIBILITY_SPEECH_RATE

#define TTS_LANGSET_KEY			VCONFKEY_LANGSET


#ifdef __cplusplus
}
#endif

#endif	/* _TTS_DEFS_H__ */
