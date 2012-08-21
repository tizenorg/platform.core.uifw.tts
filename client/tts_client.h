/*
*  Copyright (c) 2012 Samsung Electronics Co., Ltd All Rights Reserved 
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

 
#ifndef __TTS_CLIENT_H_
#define __TTS_CLIENT_H_

#include <pthread.h>
#include "tts.h"
#include "tts_main.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	/* base info */
	tts_h	tts;
	int	pid; 
	int	uid;			/*<< unique id = pid + handle */
	int	current_utt_id;

	/* callback info */
	tts_state_changed_cb		state_changed_cb;
	void*				state_changed_user_data;
	tts_utterance_started_cb	utt_started_cb;
	void*				utt_started_user_data;
	tts_utterance_completed_cb	utt_completeted_cb;
	void*				utt_completed_user_data;
	tts_error_cb			error_cb;
	void*				error_user_data;

	/* state */
	tts_state_e	before_state;
	tts_state_e	current_state;

	/* semaphore */
	int		cb_ref_count;

	/* callback data */
	int		utt_id;
	int		reason;
}tts_client_s;

int tts_client_new(tts_h* tts);

int tts_client_destroy(tts_h tts);

tts_client_s* tts_client_get(tts_h tts);

tts_client_s* tts_client_get_by_uid(const int uid);

int tts_client_get_size();

int tts_client_use_callback(tts_client_s* client);

int tts_client_not_use_callback(tts_client_s* client);

#ifdef __cplusplus
}
#endif

#endif /* __TTS_CLIENT_H_ */
