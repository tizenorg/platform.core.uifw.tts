/*
*  Copyright (c) 2011 Samsung Electronics Co., Ltd All Rights Reserved 
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
#include <unistd.h>
#include <string.h>
#include <glib.h>
#include <dlog.h>
#include <errno.h>

#include "ttsp.h"
#include "tts_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TTS Daemon Define */ 
#define TAG_TTSD "ttsd"

#define BASE_DIRECTORY_DEFAULT			"/usr/lib/voice/tts/1.0"
#define ENGINE_DIRECTORY_DEFAULT		"/usr/lib/voice/tts/1.0/engine"
#define ENGINE_DIRECTORY_DEFAULT_SETTING	"/usr/lib/voice/tts/1.0/setting"

#define CONFIG_DIRECTORY			"/opt/home/app/.voice"
	
#define ENGINE_DIRECTORY_DOWNLOAD		"/opt/usr/voice/tts/1.0/engine"
#define ENGINE_DIRECTORY_DOWNLOAD_SETTING	"/opt/usr/voice/tts/1.0/setting"


/* for debug message */
#define DATA_DEBUG

typedef enum {
	TTSD_ERROR_NONE			= 0,			/**< Successful */
	TTSD_ERROR_OUT_OF_MEMORY	= -ENOMEM,		/**< Out of Memory */
	TTSD_ERROR_IO_ERROR		= -EIO,			/**< I/O error */
	TTSD_ERROR_INVALID_PARAMETER	= -EINVAL,		/**< Invalid parameter */
	TTSD_ERROR_OUT_OF_NETWORK	= -ENETDOWN,		/**< Out of network */
	TTSD_ERROR_INVALID_STATE	= -0x0100000 | 0x21,	/**< Invalid state */
	TTSD_ERROR_INVALID_VOICE	= -0x0100000 | 0x22,	/**< Invalid voice */
	TTSD_ERROR_ENGINE_NOT_FOUND	= -0x0100000 | 0x23,	/**< No available engine  */
	TTSD_ERROR_TIMED_OUT		= -0x0100000 | 0x24,	/**< No answer from the daemon */
	TTSD_ERROR_OPERATION_FAILED	= -0x0100000 | 0x25	/**< Operation failed  */
}ttsd_error_e;


typedef enum {
	TTSD_INTERRUPTED_PAUSED = 0,	/**< Current state change 'Pause' */
	TTSD_INTERRUPTED_STOPPED	/**< Current state change 'Ready' */
}ttsd_interrupted_code_e;


typedef struct {
	char* engine_id;
	char* engine_name;
	char* ug_name;
}engine_s;

typedef struct {
	char* language;
	ttsp_voice_type_e type;
}voice_s;

typedef struct {
	char* key;
	char* value;
}engine_setting_s;

#ifdef __cplusplus
}
#endif

#endif	/* __TTSD_MAIN_H_ */
