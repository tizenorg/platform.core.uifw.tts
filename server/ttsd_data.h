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


#ifndef __TTSD_DATA_H_
#define __TTSD_DATA_H_

#include <vector>
#include "ttsp.h"

using namespace std;

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	APP_STATE_READY = 0,
	APP_STATE_PLAYING,
	APP_STATE_PAUSED
}app_state_e;

typedef struct 
{
	int			utt_id;	
	char*			text;
	char*			lang;
	ttsp_voice_type_e	vctype;
	ttsp_speed_e		speed;
}speak_data_s;

typedef struct 
{
	int		utt_id;
	void*		data;
	unsigned int 	data_size;

	ttsp_result_event_e	event;
	ttsp_audio_type_e	audio_type;
	int			rate;
	int			channels;
}sound_data_s;

typedef struct 
{
	int		pid;
	int		uid;
	int		utt_id_stopped;
	app_state_e	state;
	
	std::vector<speak_data_s> m_speak_data;	
	std::vector<sound_data_s> m_wav_data;
}app_data_s;


int ttsd_data_new_client(const int pid, const int uid);

int ttsd_data_delete_client(const int uid);

int ttsd_data_is_client(const int uid);

int ttsd_data_get_client_count();

int ttsd_data_get_pid(const int uid);

int ttsd_data_add_speak_data(const int uid, const speak_data_s data);

int ttsd_data_get_speak_data(const int uid, speak_data_s* data);

int ttsd_data_get_speak_data_size(const int uid);

int ttsd_data_add_sound_data(const int uid, const sound_data_s data);

int ttsd_data_get_sound_data(const int uid, sound_data_s* data);

int ttsd_data_get_sound_data_size(const int uid);

int ttsd_data_clear_data(const int uid);

int ttsd_data_get_client_state(const int pid, app_state_e* state);

int ttsd_data_set_client_state(const int pid, const app_state_e state);

int ttsd_data_get_current_playing();


typedef bool(*ttsd_data_get_client_cb)(int pid, int uid, app_state_e state, void* user_data);

int ttsd_data_foreach_clients(ttsd_data_get_client_cb callback, void* user_data);

bool ttsd_data_is_uttid_valid(int uid, int uttid);

#ifdef __cplusplus
}
#endif

#endif /* __TTSD_DATA_H_ */
