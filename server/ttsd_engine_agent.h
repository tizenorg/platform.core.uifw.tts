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


#ifndef __TTSD_ENGINE_AGENT_H_
#define __TTSD_ENGINE_AGENT_H_

#include "ttsd_main.h"
#include "ttsd_player.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*synth_result_callback)(ttsp_result_event_e event, const void* data, unsigned int data_size, void *user_data);

/*
* tts Engine Agent Interfaces
*/

/** Init engine agent */
int ttsd_engine_agent_init(synth_result_callback result_cb);

/** Release engine agent */
int ttsd_engine_agent_release();

/** Set current engine */
int ttsd_engine_agent_initialize_current_engine();

/** load current engine */
int ttsd_engine_agent_load_current_engine();

/** Unload current engine */
int ttsd_engine_agent_unload_current_engine();

/** test for language list */
int ttsd_print_enginelist();

/** Get state of current engine to need network */
bool ttsd_engine_agent_need_network();

bool ttsd_engine_select_valid_voice(const char* lang, int type, char** out_lang, ttsp_voice_type_e* out_type);

bool ttsd_engine_agent_is_same_engine(const char* engine_id);

/*
* TTS Engine Interfaces for client
*/
int ttsd_engine_start_synthesis(const char* lang, const ttsp_voice_type_e vctype, const char* text, const int speed, void* user_param);

int ttsd_engine_cancel_synthesis();

int ttsd_engine_get_voice_list(GList** voice_list);

int ttsd_engine_get_default_voice(char** lang, ttsp_voice_type_e* vctype );

int ttsd_engine_get_audio_format(ttsp_audio_type_e* type, int* rate, int* channels);

/*
* TTS Engine Interfaces for setting
*/

/** Get engine list */
int ttsd_engine_setting_get_engine_list(GList** engine_list);

/** Get engine */
int ttsd_engine_setting_get_engine(char** engine_id);

/** Set engine */
int ttsd_engine_setting_set_engine(const char* engine_id);

/** Get voice list for engine setting */
int ttsd_engine_setting_get_voice_list(char** engine_id, GList** voice_list);

/** Get default voice */
int ttsd_engine_setting_get_default_voice(char** language, ttsp_voice_type_e* vctype);

/** Set voice for engine setting */
int ttsd_engine_setting_set_default_voice(const char* language, ttsp_voice_type_e vctype);

/** Get voice speed for engine setting */
int ttsd_engine_setting_get_default_speed(ttsp_speed_e* speed);

/** Set voice speed for engine setting */
int ttsd_engine_setting_set_default_speed(ttsp_speed_e speed);

/** Get setting info */
int ttsd_engine_setting_get_engine_setting_info(char** engine_id, GList** setting_list); 

/** Set setting info */
int ttsd_engine_setting_set_engine_setting(const char* key, const char* value);

#ifdef __cplusplus
}
#endif

#endif /* __TTSD_ENGINE_AGENT_H_ */
