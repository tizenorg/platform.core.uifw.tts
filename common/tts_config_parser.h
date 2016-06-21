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


#ifndef __TTS_CONFIG_PARSER_H_
#define __TTS_CONFIG_PARSER_H_

#include <glib.h>
#include <libxml/parser.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	TTS_CONFIG_VOICE_TYPE_MALE = 1,		/**< Male */
	TTS_CONFIG_VOICE_TYPE_FEMALE = 2,	/**< Female */
	TTS_CONFIG_VOICE_TYPE_CHILD = 3,	/**< Child */
	TTS_CONFIG_VOICE_TYPE_USER_DEFINED = 4	/**< Engine defined */
} tts_config_voice_type_e;

typedef struct {
	char*	language;
	int	type;
} tts_config_voice_s;

typedef struct {
	char*	name;
	char*	uuid;
	char*	setting;
	GSList*	voices;
	bool	pitch_support;
} tts_engine_info_s;

typedef struct {
	char*	engine_id;
	char*	setting;
	bool	auto_voice;
	char*	language;
	int	type;
	int	speech_rate;
	int	pitch;
} tts_config_s;


/* Get engine information */
int tts_parser_get_engine_info(const char* path, tts_engine_info_s** engine_info);

int tts_parser_free_engine_info(tts_engine_info_s* engine_info);


int tts_parser_load_config(tts_config_s** config_info);

int tts_parser_unload_config(tts_config_s* config_info);

int tts_parser_set_engine(const char* engine_id, const char* setting, const char* language, int type);

int tts_parser_set_voice(const char* language, int type);

int tts_parser_set_auto_voice(bool value);

int tts_parser_set_speech_rate(int value);

int tts_parser_set_pitch(int value);

int tts_parser_find_config_changed(char** engine, char**setting, bool* auto_voice, char** language, int* voice_type, 
				   int* speech_rate, int* pitch);

int tts_parser_copy_xml(const char* original, const char* destination);

#ifdef __cplusplus
}
#endif

#endif /* __TTS_CONFIG_PARSER_H_ */
