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

#include <audio_io.h>
#include <Ecore.h>
#include <sound_manager.h>
#include <sound_manager_internal.h>

#include "ttsd_main.h"
#include "ttsd_player.h"
#include "ttsd_data.h"
#include "ttsd_dbus.h"

/*
* Internal data structure
*/

typedef enum {
	AUDIO_STATE_NONE = 0,
	AUDIO_STATE_READY,
	AUDIO_STATE_PLAY
} audio_state_e;

typedef struct {
	int			uid;	/** client id */
	app_state_e		state;	/** client state */

	/* Current utterance information */
	ttsp_result_event_e	event;	/** event of last utterance */

	bool			is_paused_data;
	int			idx;
	sound_data_s*		paused_data;
}player_s;

#define SOUND_BUFFER_LENGTH	2048

/** player init info */
static bool g_player_init = false;

/** Client list */
static GList *g_player_list;

/** current player information */
static player_s* g_playing_info;

/* player state */
static audio_state_e g_audio_state;

static ttsp_audio_type_e g_audio_type;

static int g_sampling_rate;

static audio_out_h g_audio_h;

static sound_stream_info_h g_stream_info_h;

/*
* Internal Interfaces
*/

player_s* __player_get_item(int uid)
{
	GList *iter = NULL;
	player_s *data = NULL;

	if (0 < g_list_length(g_player_list)) {
		/* Get a first item */
		iter = g_list_first(g_player_list);

		while (NULL != iter) {
			/* Get handle data from list */
			data = (player_s*)iter->data;

			/* compare uid */
			if (uid == data->uid)
				return data;

			/* Get next item */
			iter = g_list_next(iter);
		}
	}

	return NULL;
}

void __player_focus_state_cb(sound_stream_info_h stream_info, sound_stream_focus_change_reason_e reason_for_change, const char *extra_info, void *user_data)
{
	SLOG(LOG_DEBUG, get_tag(), "===== Focus state changed cb");

	if (stream_info != g_stream_info_h) {
		SLOG(LOG_ERROR, get_tag(), "[Player ERROR] Invalid stream info handle");
		return;
	}

	int ret;
	sound_stream_focus_state_e state_for_playback ;
	ret = sound_manager_get_focus_state(g_stream_info_h, &state_for_playback, NULL);
	if (SOUND_MANAGER_ERROR_NONE != ret) {
		SLOG(LOG_ERROR, get_tag(), "[Player ERROR] Fail to get focus state");
		return;
	}

	SLOG(LOG_WARN, get_tag(), "[Player] focus state changed to (%d) with reason(%d)", (int)state_for_playback, (int)reason_for_change);

	if (AUDIO_STATE_PLAY == g_audio_state && SOUND_STREAM_FOCUS_STATE_RELEASED == state_for_playback) {
		if (TTSD_MODE_DEFAULT == ttsd_get_mode()) {
			g_audio_state = AUDIO_STATE_READY;
	
			if (NULL == g_playing_info) {
				SLOG(LOG_WARN, get_tag(), "[Player WARNING] No current player");
				return;
			}

			if (APP_STATE_PLAYING == g_playing_info->state) {
				g_playing_info->state = APP_STATE_PAUSED;
	
				ttsd_data_set_client_state(g_playing_info->uid, APP_STATE_PAUSED);

				int pid = ttsd_data_get_pid(g_playing_info->uid);

				/* send message to client about changing state */
				ttsdc_send_set_state_message(pid, g_playing_info->uid, APP_STATE_PAUSED);
			}
		} else {
			SLOG(LOG_DEBUG, get_tag(), "[Player] Ignore focus state cb - mode(%d)", ttsd_get_mode());
		}
	}

	SLOG(LOG_DEBUG, get_tag(), "=====");
	SLOG(LOG_DEBUG, get_tag(), "");

	return;
}

static int __create_audio_out(ttsp_audio_type_e type, int rate)
{
	int ret = -1;
	audio_sample_type_e sample_type;

	if (TTSP_AUDIO_TYPE_RAW_S16 == type) {
		sample_type = AUDIO_SAMPLE_TYPE_S16_LE;
	} else {
		sample_type = AUDIO_SAMPLE_TYPE_U8;
	}

	ret = audio_out_create_new(rate, AUDIO_CHANNEL_MONO, sample_type, &g_audio_h);
	if (AUDIO_IO_ERROR_NONE != ret) {
		g_audio_state = AUDIO_STATE_NONE;
		g_audio_h = NULL;
		SLOG(LOG_ERROR, get_tag(), "[Player ERROR] Fail to create audio");
		return -1;
	} else {
		SLOG(LOG_DEBUG, get_tag(), "[Player SUCCESS] Create audio");
	}

	g_audio_type = type;
	g_sampling_rate = rate;

	g_audio_state = AUDIO_STATE_READY;

	return 0;
}

static int __destroy_audio_out()
{
	if (NULL == g_audio_h) {
		SLOG(LOG_ERROR, get_tag(), "[Player ERROR] Current handle is not valid");
		return -1;
	}

	int ret = -1;
	ret = audio_out_destroy(g_audio_h);
	if (AUDIO_IO_ERROR_NONE != ret) {
		SLOG(LOG_ERROR, get_tag(), "[Player ERROR] Fail to destroy audio");
		return -1;
	} else {
		SLOG(LOG_DEBUG, get_tag(), "[Player SUCCESS] Destroy audio");
	}

	g_audio_type = 0;
	g_sampling_rate = 0;

	g_audio_state = AUDIO_STATE_NONE;
	g_audio_h = NULL;

	return 0;
}

static void __end_play_thread(void *data, Ecore_Thread *thread)
{
	SLOG(LOG_ERROR, get_tag(), "===== End thread");
}

static void __set_policy_for_playing(int volume)
{
	/* Set stream info */
	int ret;
	if (TTSD_MODE_DEFAULT == ttsd_get_mode()) {
		ret = sound_manager_acquire_focus(g_stream_info_h, SOUND_STREAM_FOCUS_FOR_PLAYBACK, NULL);
		if (SOUND_MANAGER_ERROR_NONE != ret) {
			SLOG(LOG_WARN, get_tag(), "[Player WARNING] Fail to acquire focus");
		}
		ret = audio_out_set_stream_info(g_audio_h, g_stream_info_h);
		if (AUDIO_IO_ERROR_NONE != ret) {
			SLOG(LOG_WARN, get_tag(), "[Player WARNING] Fail to set stream info");
		}
	}

	/* Set volume policy */
/*
	SLOG(LOG_WARN, get_tag(), "[Player WARNING] set volume policy");
	ret = sound_manager_set_volume_voice_policy(volume);
	if (SOUND_MANAGER_ERROR_NONE != ret) {
		SLOG(LOG_WARN, get_tag(), "[Player WARNING] Fail to set volume policy");
	}
*/
	return;
}

static void __unset_policy_for_playing()
{
	int ret;
	/* Unset volume policy */
/*
	SLOG(LOG_WARN, get_tag(), "[Player WARNING] unset volume policy");
	ret = sound_manager_unset_volume_voice_policy();
	if (SOUND_MANAGER_ERROR_NONE != ret) {
		SLOG(LOG_WARN, get_tag(), "[Player WARNING] Fail to unset volume policy");
	}
*/
	/* Unset stream info */
	if (TTSD_MODE_DEFAULT == ttsd_get_mode()) {
		ret = sound_manager_release_focus(g_stream_info_h, SOUND_STREAM_FOCUS_FOR_PLAYBACK, NULL);
		if (SOUND_MANAGER_ERROR_NONE != ret) {
			SLOG(LOG_WARN, get_tag(), "[Player WARNING] Fail to release focus");
		}
	}

	return;
}

static void __play_thread(void *data, Ecore_Thread *thread)
{
	SLOG(LOG_DEBUG, get_tag(), "===== Start thread");

	if (NULL == g_playing_info) {
		SLOG(LOG_ERROR, get_tag(), "[Player ERROR] No current player");
		return;
	}

	player_s* player = g_playing_info;
	sound_data_s* sound_data = NULL;

	int ret = -1;
	int len = SOUND_BUFFER_LENGTH;
	int idx = 0;

	/* set volume policy as 40% */
	__set_policy_for_playing(40);
	while (1) {
		if (true == player->is_paused_data) {
			/* Resume player */
			sound_data = player->paused_data;
			player->paused_data = NULL;

			idx = player->idx;

			player->is_paused_data = false;
			player->idx = 0;

			if (NULL == sound_data) {
				/* Request unprepare */
				ret = audio_out_unprepare(g_audio_h);
				if (AUDIO_IO_ERROR_NONE != ret) {
					SLOG(LOG_ERROR, get_tag(), "[Player ERROR] Fail to unprepare audio : %d", ret);
				} else {
					SLOG(LOG_DEBUG, get_tag(), "[Player SUCCESS] Unprepare audio");
				}

				g_audio_state = AUDIO_STATE_READY;

				/* unset volume policy, volume will be 100% */
				__unset_policy_for_playing();
				return;
			}
			SLOG(LOG_INFO, get_tag(), "[Player] Sound info : id(%d) data(%p) size(%d) audiotype(%d) rate(%d) event(%d)", 
				sound_data->utt_id, sound_data->data, sound_data->data_size, sound_data->audio_type, sound_data->rate, sound_data->event);
		} else {
			sound_data = NULL;
			ret = ttsd_data_get_sound_data(player->uid, &sound_data);
			if (0 != ret || NULL == sound_data) {
				/* empty queue */
				SLOG(LOG_DEBUG, get_tag(), "[Player] No sound data. Waiting mode");
				/* release audio & recover session */
				ret = audio_out_unprepare(g_audio_h);
				if (AUDIO_IO_ERROR_NONE != ret) {
					SLOG(LOG_ERROR, get_tag(), "[Player ERROR] Fail to unprepare audio : %d", ret);
				} else {
					SLOG(LOG_DEBUG, get_tag(), "[Player SUCCESS] Unprepare audio");
				}
				g_audio_state = AUDIO_STATE_READY;

				/* unset volume policy, volume will be 100% */
				__unset_policy_for_playing();

				/* wait for new audio data come */
				while (1) {
					usleep(10000);
					if (NULL == g_playing_info) {
						/* current playing uid is replaced */
						SLOG(LOG_DEBUG, get_tag(), "[Player] Finish thread");
						return;
					} else if (0 < ttsd_data_get_sound_data_size(player->uid)) {
						/* new audio data come */
						SLOG(LOG_DEBUG, get_tag(), "[Player] Resume thread");
						break;
					}
				}

				/* set volume policy as 40%, when resume play thread*/
				__set_policy_for_playing(40);

				/* resume play thread */
				player->state = APP_STATE_PLAYING;
				continue;
			}

			/* If wdata's event is 'start', current wdata is first data of engine for synthesis.
			 * If wdata's event is 'finish', player should check previous event to know whether this wdata is first or not.
			 * When previous wdata's event is 'finish' and current wdata's event is 'finish',
			 * the player should send utt started event.
			 */
			if (TTSP_RESULT_EVENT_START == sound_data->event ||
			   (TTSP_RESULT_EVENT_FINISH == player->event && TTSP_RESULT_EVENT_FINISH == sound_data->event)) {
				int pid = ttsd_data_get_pid(player->uid);

				if (pid <= 0) {
					SLOG(LOG_WARN, get_tag(), "[Send WARNIING] Current player is not valid");
					/* unset volume policy, volume will be 100% */
					__unset_policy_for_playing();
					return;
				}

				if (0 != ttsdc_send_utt_start_message(pid, player->uid, sound_data->utt_id)) {
					SLOG(LOG_ERROR, get_tag(), "[Send ERROR] Fail to send Utterance Start Signal : pid(%d), uid(%d), uttid(%d)", 
						pid, player->uid, sound_data->utt_id);
				}
				SLOG(LOG_DEBUG, get_tag(), "[Player] Start utterance : uid(%d), uttid(%d)", player->uid, sound_data->utt_id);
			}

			/* Save last event to check utterance start */
			player->event = sound_data->event;
			idx = 0;

			if (NULL == sound_data->data || 0 >= sound_data->data_size) {
				if (TTSP_RESULT_EVENT_FINISH == sound_data->event) {
					SLOG(LOG_DEBUG, get_tag(), "No sound data");
					/* send utterence finish signal */
					int pid = ttsd_data_get_pid(player->uid);

					if (pid <= 0) {
						SLOG(LOG_WARN, get_tag(), "[Send WARNIING] Current player is not valid");
						/* unset volume policy, volume will be 100% */
						__unset_policy_for_playing();
						return;
					}
					if (0 != ttsdc_send_utt_finish_message(pid, player->uid, sound_data->utt_id)) {
						SLOG(LOG_ERROR, get_tag(), "[Send ERROR] Fail to send Utterance Completed Signal : pid(%d), uid(%d), uttid(%d)", 
							pid, player->uid, sound_data->utt_id);
					}
				}
				SLOG(LOG_DEBUG, get_tag(), "[Player] Finish utterance : uid(%d), uttid(%d)", player->uid, sound_data->utt_id);
				continue;
			}
		}

		if (g_sampling_rate != sound_data->rate || g_audio_type != sound_data->audio_type) {
			SLOG(LOG_DEBUG, get_tag(), "[Player] Change audio handle : org type(%d) org rate(%d)", g_audio_type, g_sampling_rate);
			if (NULL != g_audio_h) {
				__destroy_audio_out();
			}

			if (0 > __create_audio_out(sound_data->audio_type, sound_data->rate)) {
				SLOG(LOG_ERROR, get_tag(), "[Player ERROR] Fail to create audio out");
				/* unset volume policy, volume will be 100% */
				__unset_policy_for_playing();
				return;
			}
		}

		while (APP_STATE_PLAYING == player->state || APP_STATE_PAUSED == player->state) {
			if ((unsigned int)idx >= sound_data->data_size)
				break;

			if ((unsigned int)idx + SOUND_BUFFER_LENGTH > sound_data->data_size) {
				len = sound_data->data_size - idx;
			} else {
				len = SOUND_BUFFER_LENGTH;
			}

			if (AUDIO_STATE_READY == g_audio_state) {
				/* Request prepare */
				ret = audio_out_prepare(g_audio_h);
				if (AUDIO_IO_ERROR_NONE != ret) {
					SLOG(LOG_ERROR, get_tag(), "[Player ERROR] Fail to prepare audio : %d", ret);
					g_playing_info = NULL;
					/* unset volume policy, volume will be 100% */
					__unset_policy_for_playing();
					return;
				}
				SLOG(LOG_DEBUG, get_tag(), "[Player SUCCESS] Prepare audio");
				g_audio_state = AUDIO_STATE_PLAY;
			}

			char* temp_data = sound_data->data;
			ret = audio_out_write(g_audio_h, &temp_data[idx], len);
			if (0 > ret) {
				SLOG(LOG_WARN, get_tag(), "[Player WARNING] Fail to audio write - %d", ret);
			} else {
				idx += len;
			}

			if (NULL == g_playing_info && APP_STATE_PAUSED != player->state) {
				SLOG(LOG_ERROR, get_tag(), "[Player ERROR] Current player is NULL");
				g_audio_state = AUDIO_STATE_READY;
				ret = audio_out_unprepare(g_audio_h);
				if (AUDIO_IO_ERROR_NONE != ret) {
					SLOG(LOG_ERROR, get_tag(), "[Player ERROR] Fail to unprepare audio : %d", ret);
				}
				/* unset volume policy, volume will be 100% */
				__unset_policy_for_playing();

				if (NULL != sound_data) {
					if (NULL != sound_data->data) {
						free(sound_data->data);
						sound_data->data = NULL;
					}

					free(sound_data);
					sound_data = NULL;
				}

				return;
			}

			if (APP_STATE_PAUSED == player->state) {
				/* Save data */
				player->paused_data = sound_data;

				player->is_paused_data = true;
				player->idx = idx;

				g_audio_state = AUDIO_STATE_READY;
				SLOG(LOG_DEBUG, get_tag(), "[Player] Stop player thread by pause");

				/* Request prepare */
				ret = audio_out_unprepare(g_audio_h);
				if (AUDIO_IO_ERROR_NONE != ret) {
					SLOG(LOG_ERROR, get_tag(), "[Player ERROR] Fail to unprepare audio : %d", ret);
				} else {
					SLOG(LOG_DEBUG, get_tag(), "[Player SUCCESS] Unprepare audio");
				}
				/* unset volume policy, volume will be 100% */
				__unset_policy_for_playing();
				return;
			}
		}

		if (NULL == g_playing_info && APP_STATE_READY == player->state) {
			/* player_stop */ 
			g_audio_state = AUDIO_STATE_READY;
			SLOG(LOG_DEBUG, get_tag(), "[Player] Stop player thread");

			/* Request prepare */
			ret = audio_out_unprepare(g_audio_h);
			if (AUDIO_IO_ERROR_NONE != ret) {
				SLOG(LOG_ERROR, get_tag(), "[Player ERROR] Fail to unprepare audio : %d", ret);
			} else {
				SLOG(LOG_DEBUG, get_tag(), "[Player SUCCESS] Unprepare audio");
			}

			if (NULL != sound_data) {
				if (NULL != sound_data->data) {
					free(sound_data->data);
					sound_data->data = NULL;
				}

				free(sound_data);
				sound_data = NULL;
			}
			/* unset volume policy, volume will be 100% */
			__unset_policy_for_playing();
			return;
		}

		if ((APP_STATE_PLAYING == player->state || APP_STATE_PAUSED == player->state) &&
			(TTSP_RESULT_EVENT_FINISH == sound_data->event)) {
			/* send utterence finish signal */
			int pid = ttsd_data_get_pid(player->uid);

			if (pid <= 0) {
				SLOG(LOG_WARN, get_tag(), "[Send WARNIING] Current player is not valid");
				/* unset volume policy, volume will be 100% */
				__unset_policy_for_playing();
				return;
			}

			if (0 != ttsdc_send_utt_finish_message(pid, player->uid, sound_data->utt_id)) {
				SLOG(LOG_ERROR, get_tag(), "[Send ERROR] Fail to send Utterance Completed Signal : pid(%d), uid(%d), uttid(%d)", 
					pid, player->uid, sound_data->utt_id);
				/* unset volume policy, volume will be 100% */
				__unset_policy_for_playing();
				return;
			}

			SLOG(LOG_DEBUG, get_tag(), "[Player] Finish utterance : uid(%d), uttid(%d)", player->uid, sound_data->utt_id);
		}

		if (NULL != sound_data) {
			if (NULL != sound_data->data) {
				free(sound_data->data);
				sound_data->data = NULL;
			}

			free(sound_data);
			sound_data = NULL;
		}

		if (NULL == g_playing_info) {
			SLOG(LOG_ERROR, get_tag(), "[Player ERROR] Current player is NULL");
			g_audio_state = AUDIO_STATE_READY;
			ret = audio_out_unprepare(g_audio_h);
			if (AUDIO_IO_ERROR_NONE != ret) {
				SLOG(LOG_ERROR, get_tag(), "[Player ERROR] Fail to unprepare audio : %d", ret);
			}
			/* unset volume policy, volume will be 100% */
			__unset_policy_for_playing();

			return;
		}
	}
}

/*
* Player Interfaces
*/
int ttsd_player_init()
{
	g_playing_info = NULL;
	g_audio_state = AUDIO_STATE_NONE;
	g_audio_h = NULL;

	int ret;

	ecore_thread_max_set(1);

	ret = sound_manager_create_stream_information(SOUND_STREAM_TYPE_VOICE_INFORMATION, __player_focus_state_cb, NULL, &g_stream_info_h);
	if (SOUND_MANAGER_ERROR_NONE != ret) {
		SLOG(LOG_ERROR, get_tag(), "[Player ERROR] Fail to create stream info");
		return -1;
	} else {
		SLOG(LOG_DEBUG, get_tag(), "[Player SUCCESS] Create stream info");
	}

	ret = __create_audio_out(TTSP_AUDIO_TYPE_RAW_S16, 16000);
	if (0 != ret)
		return -1;

	g_player_init = true;

	return 0;
}

int ttsd_player_release(void)
{
	if (false == g_player_init) {
		SLOG(LOG_ERROR, get_tag(), "[Player ERROR] Not Initialized");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	int ret;

	SLOG(LOG_DEBUG, get_tag(), "[Player DEBUG] ==========================");
	SLOG(LOG_DEBUG, get_tag(), "[Player DEBUG] Active thread count : %d", ecore_thread_active_get());
	SLOG(LOG_DEBUG, get_tag(), "[Player DEBUG] ==========================");

	/* The thread should be released */
	int thread_count = ecore_thread_active_get();
	int count = 0;
	while (0 < thread_count) {
		usleep(10000);

		count++;
		if (20 == count) {
			SLOG(LOG_WARN, get_tag(), "[Player WARNING!!] Thread is blocked. Player release continue.");
			break;
		}

		thread_count = ecore_thread_active_get();
	}

	SLOG(LOG_DEBUG, get_tag(), "[Player DEBUG] Thread is released");

	ret = __destroy_audio_out();
	if (0 != ret)
		return -1;

	ret = sound_manager_destroy_stream_information(g_stream_info_h);
	if (SOUND_MANAGER_ERROR_NONE != ret) {
		SLOG(LOG_WARN, get_tag(), "[Player WARNING] Fail to destroy stream info");
	} else {
		SLOG(LOG_DEBUG, get_tag(), "[Player SUCCESS] Destroy stream info");
	}

	/* clear g_player_list */
	g_playing_info = NULL;
	g_player_init = false;

	return 0;
}

int ttsd_player_create_instance(int uid)
{
	if (false == g_player_init) {
		SLOG(LOG_ERROR, get_tag(), "[Player ERROR] Not Initialized");
		return -1;
	}

	/* Check uid is duplicated */
	if (NULL != __player_get_item(uid)) {
		SLOG(LOG_ERROR, get_tag(), "[Player ERROR] uid(%d) is already registered", uid);
		return -1;
	}

	player_s* new_client = (player_s*)calloc(1, sizeof(player_s));
	if (NULL == new_client) {
		SLOG(LOG_ERROR, get_tag(), "[Player ERROR] Fail to allocate memory");
		return TTSP_ERROR_OUT_OF_MEMORY;
	}

	new_client->uid = uid;
	new_client->event = TTSP_RESULT_EVENT_FINISH;
	new_client->state = APP_STATE_READY;
	new_client->is_paused_data = false;
	new_client->idx = 0;
	new_client->paused_data = NULL;
	
	SECURE_SLOG(LOG_DEBUG, get_tag(), "[Player] Create player : uid(%d)", uid);

	g_player_list = g_list_append(g_player_list, new_client);

	return 0;
}

int ttsd_player_destroy_instance(int uid)
{
	if (false == g_player_init) {
		SLOG(LOG_ERROR, get_tag(), "[Player ERROR] Not Initialized");
		return -1;
	}

	player_s* current;
	current = __player_get_item(uid);
	if (NULL == current) {
		SLOG(LOG_ERROR, get_tag(), "[Player ERROR] uid(%d) is not valid", uid);
		return -1;
	}

	if (NULL != g_playing_info) {
		if (uid == g_playing_info->uid) {
			g_playing_info = NULL;
		}
	}

	GList *iter = NULL;
	player_s *data = NULL;

	if (0 < g_list_length(g_player_list)) {
		/* Get a first item */
		iter = g_list_first(g_player_list);

		while (NULL != iter) {
			/* Get handle data from list */
			data = (player_s*)iter->data;

			if (NULL != data) {
				/* compare uid */
				if (uid == data->uid) {
					g_player_list = g_list_remove_link(g_player_list, iter);
					free(data);
					g_list_free(iter);		
					break;
				}
			}

			/* Get next item */
			iter = g_list_next(iter);
		}
	}

	SLOG(LOG_DEBUG, get_tag(), "[PLAYER Success] Destroy instance");

	return 0;
}

int ttsd_player_play(int uid)
{
	if (false == g_player_init) {
		SLOG(LOG_ERROR, get_tag(), "[Player ERROR] Not Initialized");
		return -1;
	}

	if (NULL != g_playing_info) {
		if (uid == g_playing_info->uid) {
			SLOG(LOG_DEBUG, get_tag(), "[Player] uid(%d) has already played", g_playing_info->uid);
			return 0;
		} else {
			SLOG(LOG_WARN, get_tag(), "[Player WARNING] stop old player (%d)", g_playing_info->uid);
			ttsd_player_stop(g_playing_info->uid);
		}
	}

	SLOG(LOG_DEBUG, get_tag(), "[Player] start play : uid(%d)", uid);

	/* Check sound queue size */
	if (0 == ttsd_data_get_sound_data_size(uid)) {
		SLOG(LOG_WARN, get_tag(), "[Player WARNING] A sound queue of current player(%d) is empty", uid);
		return -1;
	}

	/* Check uid */
	player_s* current;
	current = __player_get_item(uid);
	if (NULL == current) {
		SLOG(LOG_ERROR, get_tag(), "[Player ERROR] uid(%d) is not valid", uid);
		return -1;
	}

	current->state = APP_STATE_PLAYING;

	g_playing_info = current;

	SLOG(LOG_DEBUG, get_tag(), "[Player DEBUG] Active thread count : %d", ecore_thread_active_get());

	if (0 < ttsd_data_get_sound_data_size(current->uid)) {
		SLOG(LOG_DEBUG, get_tag(), "[Player] Run thread");
		ecore_thread_run(__play_thread, __end_play_thread, NULL, NULL);
	}

	return 0;
}

int ttsd_player_stop(int uid)
{
	if (false == g_player_init) {
		SLOG(LOG_ERROR, get_tag(), "[Player ERROR] Not Initialized");
		return -1;
	}

	/* Check uid */
	player_s* current;
	current = __player_get_item(uid);
	if (NULL == current) {
		SLOG(LOG_ERROR, get_tag(), "[Player ERROR] uid(%d) is not valid", uid);
		return -1;
	}

	/* check whether uid is current playing or not */
	if (NULL != g_playing_info) {
		if (uid == g_playing_info->uid) {
			/* release current playing info */
			g_playing_info = NULL;
		}
	} else {
		SLOG(LOG_DEBUG, get_tag(), "[Player] No current playing");
	}

	if (true == current->is_paused_data) {
		if (NULL != current->paused_data) {
			if (NULL != current->paused_data->data) {
				free(current->paused_data->data);
				current->paused_data->data = NULL;
			}

			free(current->paused_data);
			current->paused_data = NULL;
		}
	}

	current->event = TTSP_RESULT_EVENT_FINISH;
	current->state = APP_STATE_READY;
	current->is_paused_data = false;
	current->idx = 0;

	if (NULL == g_playing_info) {
		SLOG(LOG_DEBUG, get_tag(), "[Player] ==========================");
		SLOG(LOG_ERROR, get_tag(), "[Player] Active thread count : %d", ecore_thread_active_get());
		SLOG(LOG_DEBUG, get_tag(), "[Player] ==========================");

		/* The thread should be released */
		int thread_count = ecore_thread_active_get();
		int count = 0;
		while (0 < thread_count) {
			usleep(10000);

			count++;
			if (30 == count) {
				SLOG(LOG_WARN, get_tag(), "[Player WARNING!!] Thread is blocked. Player release continue.");
				break;
			}

			thread_count = ecore_thread_active_get();
		}

		SLOG(LOG_DEBUG, get_tag(), "[Player] ==========================");
		SLOG(LOG_ERROR, get_tag(), "[Player] Active thread count : %d", ecore_thread_active_get());
		SLOG(LOG_DEBUG, get_tag(), "[Player] ==========================");
	}

	SLOG(LOG_DEBUG, get_tag(), "[Player SUCCESS] Stop player : uid(%d)", uid);

	return 0;
}

int ttsd_player_clear(int uid)
{
	if (false == g_player_init) {
		SLOG(LOG_ERROR, get_tag(), "[Player ERROR] Not Initialized");
		return -1;
	}

	/* Check uid */
	player_s* current;
	current = __player_get_item(uid);
	if (NULL == current) {
		SECURE_SLOG(LOG_ERROR, get_tag(), "[Player ERROR] uid(%d) is not valid", uid); 
		return -1;
	}

	if (true == current->is_paused_data) {
		if (NULL != current->paused_data) {
			if (NULL != current->paused_data->data) {
				free(current->paused_data->data);
				current->paused_data->data = NULL;
			}

			free(current->paused_data);
			current->paused_data = NULL;
		}
	}

	current->event = TTSP_RESULT_EVENT_FINISH;
	current->state = APP_STATE_READY;
	current->is_paused_data = false;
	current->idx = 0;

	SLOG(LOG_DEBUG, get_tag(), "[Player SUCCESS] Clear player : uid(%d)", uid);

	return 0;
}

int ttsd_player_pause(int uid)
{
	SLOG(LOG_DEBUG, get_tag(), "[Player] pause player : uid(%d)", uid);

	if (false == g_player_init) {
		SLOG(LOG_ERROR, get_tag(), "[Player ERROR] Not Initialized");
		return -1;
	}

	/* Check uid */
	player_s* current;
	current = __player_get_item(uid);
	if (NULL == current) {
		SLOG(LOG_ERROR, get_tag(), "[Player ERROR] ttsd_player_pause() : uid(%d) is not valid", uid);
		return -1;
	}

	/* check whether uid is current playing or not */
	if (NULL != g_playing_info) {
		if (uid == g_playing_info->uid) {
			/* release current playing info */
			g_playing_info = NULL;
		} else {
			/* error case */
		}
	}

	current->state = APP_STATE_PAUSED;
	
	SLOG(LOG_DEBUG, get_tag(), "[Player] ==========================");
	SLOG(LOG_ERROR, get_tag(), "[Player] Active thread count : %d", ecore_thread_active_get());
	SLOG(LOG_DEBUG, get_tag(), "[Player] ==========================");

	/* The thread should be released */
	int thread_count = ecore_thread_active_get();
	int count = 0;
	while (0 < thread_count) {
		usleep(10000);

		count++;
		if (30 == count) {
			SLOG(LOG_WARN, get_tag(), "[Player WARNING!!] Thread is blocked. Player release continue.");
			break;
		}

		thread_count = ecore_thread_active_get();
	}

	SLOG(LOG_DEBUG, get_tag(), "[Player] ==========================");
	SLOG(LOG_ERROR, get_tag(), "[Player] Active thread count : %d", ecore_thread_active_get());
	SLOG(LOG_DEBUG, get_tag(), "[Player] ==========================");

	SLOG(LOG_DEBUG, get_tag(), "[Player SUCCESS] Pause player : uid(%d)", uid);

	return 0;
}

int ttsd_player_resume(int uid)
{
	SLOG(LOG_DEBUG, get_tag(), "[Player] Resume player : uid(%d)", uid);

	if (false == g_player_init) {
		SLOG(LOG_ERROR, get_tag(), "[Player ERROR] Not Initialized");
		return -1;
	}

	/* Check id */
	player_s* current;
	current = __player_get_item(uid);
	if (NULL == current) {
		SLOG(LOG_ERROR, get_tag(), "[Player ERROR] uid(%d) is not valid", uid);
		return -1;
	}

	/* check current player */
	if (NULL != g_playing_info)
		g_playing_info = NULL;

	current->state = APP_STATE_PLAYING;
	g_playing_info = current;

	SLOG(LOG_DEBUG, get_tag(), "[Player] Run thread");
	ecore_thread_run(__play_thread, __end_play_thread, NULL, NULL);

	return 0;
}

int ttsd_player_all_stop()
{
	if (false == g_player_init) {
		SLOG(LOG_ERROR, get_tag(), "[Player ERROR] Not Initialized");
		return -1;
	}

	g_playing_info = NULL;

	GList *iter = NULL;
	player_s *data = NULL;

	if (0 < g_list_length(g_player_list)) {
		/* Get a first item */
		iter = g_list_first(g_player_list);

		while (NULL != iter) {
			/* Get handle data from list */
			data = (player_s*)iter->data;

			app_state_e state;
			if (0 > ttsd_data_get_client_state(data->uid, &state)) {
				SLOG(LOG_ERROR, get_tag(), "[player ERROR] uid(%d) is not valid", data->uid);
				ttsd_player_destroy_instance(data->uid);
				iter = g_list_next(iter);
				continue;
			}

			if (APP_STATE_PLAYING == state || APP_STATE_PAUSED == state) {
				data->event = TTSP_RESULT_EVENT_FINISH;
				data->state = APP_STATE_READY;

				if (true == data->is_paused_data) {
					if (NULL != data->paused_data) {
						if (NULL != data->paused_data->data) {
							free(data->paused_data->data);
							data->paused_data->data = NULL;
						}

						free(data->paused_data);
						data->paused_data = NULL;
					}
				}

				data->is_paused_data = false;
				data->idx = 0;
			}

			/* Get next item */
			iter = g_list_next(iter);
		}
	}

	SLOG(LOG_DEBUG, get_tag(), "[Player SUCCESS] player all stop!!");

	return 0;
}
