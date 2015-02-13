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


#ifndef __TTSD_ENGINE_AGENT_H_
#define __TTSD_ENGINE_AGENT_H_

#include "ttsd_main.h"
#include "ttsd_player.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*synth_result_callback)(ttsp_result_event_e event, const void* data, unsigned int data_size, 
				     ttsp_audio_type_e audio_type, int rate, void *user_data);

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

/** Get state of current engine to need network */
bool ttsd_engine_agent_need_network();

bool ttsd_engine_select_valid_voice(const char* lang, int type, char** out_lang, int* out_type);

bool ttsd_engine_agent_is_same_engine(const char* engine_id);

/** Get/Set app option */
int ttsd_engine_agent_set_default_engine(const char* engine_id);

int ttsd_engine_agent_set_default_voice(const char* language, int vctype);

int ttsd_engine_agent_set_default_speed(int speed);

int ttsd_engine_agent_set_default_pitch(int pitch);

/*
* TTS Engine Interfaces for client
*/

int ttsd_engine_load_voice(const char* lang, int vctype);

int ttsd_engine_unload_voice(const char* lang, int vctype);

int ttsd_engine_start_synthesis(const char* lang, int vctype, const char* text, int speed, void* user_param);

int ttsd_engine_cancel_synthesis();

int ttsd_engine_get_voice_list(GList** voice_list);

int ttsd_engine_get_default_voice(char** lang, int* vctype);



#ifdef __cplusplus
}
#endif

#endif /* __TTSD_ENGINE_AGENT_H_ */
