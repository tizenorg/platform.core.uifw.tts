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

 
#ifndef __TTS_MAIN_H_
#define __TTS_MAIN_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <glib.h>
#include <dbus/dbus.h>
#include <dlog.h>

#include "tts_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TAG_TTSC "ttsc"

/** 
* @brief A structure of handle for identification
*/
struct tts_s {
	int handle;
};

#ifdef __cplusplus
}
#endif

#endif /* __TTS_MAIN_H_ */
