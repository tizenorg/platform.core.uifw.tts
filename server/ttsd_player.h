/*
*  Copyright (c) 2012, 2013 Samsung Electronics Co., Ltd All Rights Reserved 
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


#ifndef __TTSD_PLAYER_H_
#define __TTSD_PLAYER_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	PLAYER_END_OF_PLAYING,
	PLAYER_EMPTY_SOUND_QUEUE,
	PLAYER_ERROR
}player_event_e;

typedef enum {
	TTSD_PLAYER_STATE_NULL,
	TTSD_PLAYER_STATE_PAUSED,
	TTSD_PLAYER_STATE_PLAYING
}ttsd_player_state_e;

typedef int (*player_result_callback_func)(player_event_e event, int uid, int utt_id);

/*
* TTSD Player Interfaces 
*/

int ttsd_player_init(player_result_callback_func result_cb);

int ttsd_player_release(void);

int ttsd_player_create_instance(int uid);

int ttsd_player_destroy_instance(int uid);

int ttsd_player_play(int uid);

int ttsd_player_stop(int uid);

int ttsd_player_pause(int uid);

int ttsd_player_resume(int uid);

int ttsd_player_get_state(int uid, ttsd_player_state_e* state);

int ttsd_player_get_current_client();

int ttsd_player_get_current_utterance_id(int uid);

int ttsd_player_all_stop();

#ifdef __cplusplus
}
#endif

#endif /* __TTSD_PLAYER_H_ */
