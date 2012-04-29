/*
* Copyright (c) 2011 Samsung Electronics Co., Ltd All Rights Reserved 
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

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************************
* Definition for IPC
*******************************************************************************************/

#define TTS_CLIENT_SERVICE_NAME         "com.samsung.voice.ttsclient"
#define TTS_CLIENT_SERVICE_OBJECT_PATH	"/com/samsung/voice/ttsclient"
#define TTS_CLIENT_SERVICE_INTERFACE	"com.samsung.voice.ttsclient"

#define TTS_SETTING_SERVICE_NAME	"com.samsung.voice.ttssetting"
#define TTS_SETTING_SERVICE_OBJECT_PATH	"/com/samsung/voice/ttssetting"
#define TTS_SETTING_SERVICE_INTERFACE	"com.samsung.voice.ttssetting"

#define TTS_SERVER_SERVICE_NAME		"service.connect.ttsserver"
#define TTS_SERVER_SERVICE_OBJECT_PATH	"/com/samsung/voice/ttsserver"
#define TTS_SERVER_SERVICE_INTERFACE	"com.samsung.voice.ttsserver"


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

#define TTS_METHOD_INTERRUPT		"tts_method_interrupt"
#define TTS_METHOD_UTTERANCE_STARTED	"tts_method_utterance_started"
#define TTS_METHOD_UTTERANCE_COMPLETED	"tts_method_utterance_completed"
#define TTS_METHOD_ERROR		"tts_method_error"


/******************************************************************************************
* Message Definition for Setting
*******************************************************************************************/

#define TTS_SETTING_METHOD_HELLO		"tts_setting_method_hello"
#define TTS_SETTING_METHOD_INITIALIZE		"tts_setting_method_initialize"
#define TTS_SETTING_METHOD_FINALIZE		"tts_setting_method_finalilze"
#define TTS_SETTING_METHOD_GET_ENGINE_LIST	"tts_setting_method_get_engine_list"
#define TTS_SETTING_METHOD_GET_ENGINE		"tts_setting_method_get_engine"
#define TTS_SETTING_METHOD_SET_ENGINE		"tts_setting_method_set_engine"
#define TTS_SETTING_METHOD_GET_VOICE_LIST	"tts_setting_method_get_voice_list"
#define TTS_SETTING_METHOD_GET_DEFAULT_VOICE	"tts_setting_method_get_voice"
#define TTS_SETTING_METHOD_SET_DEFAULT_VOICE	"tts_setting_method_set_voice"
#define TTS_SETTING_METHOD_GET_DEFAULT_SPEED	"tts_setting_method_get_speed"
#define TTS_SETTING_METHOD_SET_DEFAULT_SPEED	"tts_setting_method_set_speed"
#define TTS_SETTING_METHOD_GET_ENGINE_SETTING	"tts_setting_method_get_engine_setting"
#define TTS_SETTING_METHOD_SET_ENGINE_SETTING	"tts_setting_method_set_engine_setting"

/******************************************************************************************
* Message Definition for tts-daemon internal
*******************************************************************************************/

#define TTS_METHOD_NEXT_PLAY		"tts_method_start_play"
#define TTS_METHOD_NEXT_SYNTHESIS	"tts_method_start_synthesis"


#ifdef __cplusplus
}
#endif

#endif	/* _TTS_DEFS_H__ */
