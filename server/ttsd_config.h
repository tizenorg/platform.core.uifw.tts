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


#ifndef __TTSD_CONFIG_H_
#define __TTSD_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

int ttsd_config_initialize();

int ttsd_config_finalize();

int ttsd_config_get_default_engine(char** engine_id);

int ttsd_config_set_default_engine(const char* engine_id);

int ttsd_config_get_default_voice(char** language, int* type);

int ttsd_config_set_default_voice(const char* langauge, int type);

int ttsd_config_get_default_speed(int* speed);

int ttsd_config_set_default_speed(int speed);

#ifdef __cplusplus
}
#endif

#endif /* __TTSD_CONFIG_H_ */
