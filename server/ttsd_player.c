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
	sound_data_s		paused_data;
} player_s;

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

void __player_audio_io_interrupted_cb(audio_io_interrupted_code_e code, void *user_data)
{
	SLOG(LOG_DEBUG, get_tag(), "===== INTERRUPTED CALLBACK");

	SLOG(LOG_WARN, get_tag(), "[Player] code : %d", (int)code);

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

	SLOG(LOG_DEBUG, get_tag(), "=====");
	SLOG(LOG_DEBUG, get_tag(), "  ");

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

	ret = audio_out_create(rate, AUDIO_CHANNEL_MONO, sample_type, SOUND_TYPE_MEDIA, &g_audio_h);
	if (AUDIO_IO_ERROR_NONE != ret) {
		g_audio_state = AUDIO_STATE_NONE;
		g_audio_h = NULL;
		SLOG(LOG_ERROR, get_tag(), "[Player ERROR] Fail to create audio");
		return -1;
	} else {
		SLOG(LOG_DEBUG, get_tag(), "[Player SUCCESS] Create audio");
	}

	if (TTSD_MODE_DEFAULT != ttsd_get_mode()) {
		ret = audio_out_ignore_session(g_audio_h);
		if (AUDIO_IO_ERROR_NONE != ret) {
			g_audio_state = AUDIO_STATE_NONE;
			g_audio_h = NULL;
			SLOG(LOG_ERROR, get_tag(), "[Player ERROR] Fail to set ignore session");
			return -1;
		}
	}

	ret = audio_out_set_interrupted_cb(g_audio_h, __player_audio_io_interrupted_cb, NULL);
	if (AUDIO_IO_ERROR_NONE != ret) {
		g_audio_state = AUDIO_STATE_NONE;
		g_audio_h = NULL;
		SLOG(LOG_ERROR, get_tag(), "[Player ERROR] Fail to set callback function");
		return -1;
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
	SLOG(LOG_DEBUG, get_tag(), "===== End thread");
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
	sound_data_s wdata;

	int ret = -1;
	int len = SOUND_BUFFER_LENGTH;
	int idx = 0;

	while (1) {
		if (true == player->is_paused_data) {
			/* Resume player */
			wdata.data = player->paused_data.data;
			wdata.data_size = player->paused_data.data_size;
			wdata.utt_id = player->paused_data.utt_id;
			wdata.audio_type = player->paused_data.audio_type;
			wdata.rate = player->paused_data.rate;
			wdata.channels = player->paused_data.channels;
			wdata.event = player->paused_data.event;

			idx = player->idx;

			player->is_paused_data = false;
			player->idx = 0;
		} else {
			if (0 != ttsd_data_get_sound_data(player->uid, &wdata)) {
				g_playing_info = NULL;
				SLOG(LOG_DEBUG, get_tag(), "[Player] No sound data. Finish thread");

				/* Request unprepare */
				ret = audio_out_unprepare(g_audio_h);
				if (AUDIO_IO_ERROR_NONE != ret) {
					SLOG(LOG_ERROR, get_tag(), "[Player ERROR] Fail to unprepare audio : %d", ret);
				} else {
					SLOG(LOG_DEBUG, get_tag(), "[Player SUCCESS] Unprepare audio");
				}

				g_audio_state = AUDIO_STATE_READY;
				return;
			}

			/* If wdata's event is 'start', current wdata is first data of engine for synthesis.
			 * If wdata's event is 'finish', player should check previous event to know whether this wdata is first or not.
			 * When previous wdata's event is 'finish' and current wdata's event is 'finish',
			 * the player should send utt started event.
			 */
			if (TTSP_RESULT_EVENT_START == wdata.event ||
			   (TTSP_RESULT_EVENT_FINISH == player->event && TTSP_RESULT_EVENT_FINISH == wdata.event)) {
				int pid = ttsd_data_get_pid(player->uid);

				if (pid <= 0) {
					SLOG(LOG_WARN, get_tag(), "[Send WARNIING] Current player is not valid");
					return;
				}

				if (0 != ttsdc_send_utt_start_message(pid, player->uid, wdata.utt_id)) {
					SLOG(LOG_ERROR, get_tag(), "[Send ERROR] Fail to send Utterance Start Signal : pid(%d), uid(%d), uttid(%d)", 
						pid, player->uid, wdata.utt_id);
				}
				SLOG(LOG_DEBUG, get_tag(), "[Player] Start utterance : uid(%d), uttid(%d)", player->uid, wdata.utt_id);
			}

			/* Save last event to check utterance start */
			player->event = wdata.event;
			idx = 0;

			if (NULL == wdata.data || 0 >= wdata.data_size) {
				if (TTSP_RESULT_EVENT_FINISH == wdata.event) {
					SLOG(LOG_DEBUG, get_tag(), "No sound data");
					/* send utterence finish signal */
					int pid = ttsd_data_get_pid(player->uid);

					if (pid <= 0) {
						SLOG(LOG_WARN, get_tag(), "[Send WARNIING] Current player is not valid");
						return;
					}
					if (0 != ttsdc_send_utt_finish_message(pid, player->uid, wdata.utt_id)) {
						SLOG(LOG_ERROR, get_tag(), "[Send ERROR] Fail to send Utterance Completed Signal : pid(%d), uid(%d), uttid(%d)", pid, player->uid, wdata.utt_id);
					}
				}
				SLOG(LOG_DEBUG, get_tag(), "[Player] Finish utterance : uid(%d), uttid(%d)", player->uid, wdata.utt_id);
				continue;
			}
		}

		SLOG(LOG_DEBUG, get_tag(), "[Player] Sound info : id(%d) size(%d) audiotype(%d) rate(%d) event(%d)", 
			wdata.utt_id, wdata.data_size, wdata.audio_type, wdata.rate, wdata.event);

		if (g_sampling_rate != wdata.rate || g_audio_type != wdata.audio_type) {
			SLOG(LOG_DEBUG, get_tag(), "[Player] Change audio handle : org type(%d) org rate(%d)", g_audio_type, g_sampling_rate);
			if (NULL != g_audio_h) {
				__destroy_audio_out();
			}

			if (0 > __create_audio_out(wdata.audio_type, wdata.rate)) {
				SLOG(LOG_ERROR, get_tag(), "[Player ERROR] Fail to create audio out");
				return;
			}
		}

		while (APP_STATE_PLAYING == player->state || APP_STATE_PAUSED == player->state) {
			if ((unsigned int)idx >= wdata.data_size)
				break;

			if ((unsigned int)idx + SOUND_BUFFER_LENGTH > wdata.data_size) {
				len = wdata.data_size - idx;
			} else {
				len = SOUND_BUFFER_LENGTH;
			}

			if (AUDIO_STATE_READY == g_audio_state) {
				/* Request prepare */
				ret = audio_out_prepare(g_audio_h);
				if (AUDIO_IO_ERROR_NONE != ret) {
					SLOG(LOG_ERROR, get_tag(), "[Player ERROR] Fail to prepare audio : %d", ret);
					g_playing_info = NULL;
					return;
				}
				SLOG(LOG_DEBUG, get_tag(), "[Player SUCCESS] Prepare audio");
				g_audio_state = AUDIO_STATE_PLAY;
			}

			char* temp_data = wdata.data;
			ret = audio_out_write(g_audio_h, &temp_data[idx], len);
			if (0 > ret) {
				SLOG(LOG_WARN, get_tag(), "[Player WARNING] Fail to audio write - %d", ret);
			} else {
				idx += len;
			}

			if (APP_STATE_PAUSED == player->state) {
				/* Save data */
				player->paused_data.data = wdata.data;
				player->paused_data.data_size = wdata.data_size;
				player->paused_data.utt_id = wdata.utt_id;
				player->paused_data.audio_type = wdata.audio_type;
				player->paused_data.rate = wdata.rate;
				player->paused_data.channels = wdata.channels;
				player->paused_data.event = wdata.event;

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
				return;
			}
		}

		if (NULL != wdata.data) {
			free(wdata.data);
			wdata.data = NULL;
		}

		if (APP_STATE_READY == player->state) {
			g_audio_state = AUDIO_STATE_READY;
			SLOG(LOG_DEBUG, get_tag(), "[Player] Stop player thread");

			/* Request prepare */
			ret = audio_out_unprepare(g_audio_h);
			if (AUDIO_IO_ERROR_NONE != ret) {
				SLOG(LOG_ERROR, get_tag(), "[Player ERROR] Fail to unprepare audio : %d", ret);
			} else {
				SLOG(LOG_DEBUG, get_tag(), "[Player SUCCESS] Unprepare audio");
			}
			return;
		}

		if (TTSP_RESULT_EVENT_FINISH == wdata.event) {
			/* send utterence finish signal */
			int pid = ttsd_data_get_pid(player->uid);

			if (pid <= 0) {
				SLOG(LOG_WARN, get_tag(), "[Send WARNIING] Current player is not valid");
				return;
			}

			if (0 != ttsdc_send_utt_finish_message(pid, player->uid, wdata.utt_id)) {
				SLOG(LOG_ERROR, get_tag(), "[Send ERROR] Fail to send Utterance Completed Signal : pid(%d), uid(%d), uttid(%d)", 
					pid, player->uid, wdata.utt_id);
				return;
			}

			SLOG(LOG_DEBUG, get_tag(), "[Player] Finish utterance : uid(%d), uttid(%d)", player->uid, wdata.utt_id);
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

	if (TTSD_MODE_DEFAULT == ttsd_get_mode()) {
		ret = sound_manager_set_session_type(SOUND_SESSION_TYPE_MEDIA);
		if (0 != ret) {
			SLOG(LOG_ERROR, get_tag(), "[Player ERROR] Fail to set session type");
		}

		ret = sound_manager_set_media_session_option(SOUND_SESSION_OPTION_PAUSE_OTHERS_WHEN_START, SOUND_SESSION_OPTION_INTERRUPTIBLE_DURING_PLAY);
		if (0 != ret) {
			SLOG(LOG_ERROR, get_tag(), "[Player ERROR] Fail set media session option");
		} else {
			SLOG(LOG_DEBUG, get_tag(), "[Player SUCCESS] set media session option");
		}
	}

	ecore_thread_max_set(1);

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

	ret = __destroy_audio_out();
	if (0 != ret)
		return -1;

	SLOG(LOG_DEBUG, get_tag(), "[Player DEBUG] ==========================");
	SLOG(LOG_DEBUG, get_tag(), "[Player DEBUG] Active thread count : %d", ecore_thread_active_get());
	SLOG(LOG_DEBUG, get_tag(), "[Player DEBUG] ==========================");

	/* The thread should be released */
	int thread_count = ecore_thread_active_get();
	int count = 0;
	while (0 < thread_count) {
		usleep(10);

		count++;
		if (100 == count) {
			SLOG(LOG_WARN, get_tag(), "[Player WARNING!!] Thread is blocked. Player release continue.");
			break;
		}

		thread_count = ecore_thread_active_get();
	}

	SLOG(LOG_DEBUG, get_tag(), "[Player DEBUG] Thread is released");

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

	new_client->uid = uid;
	new_client->event = TTSP_RESULT_EVENT_FINISH;
	new_client->state = APP_STATE_READY;
	new_client->is_paused_data = false;
	new_client->idx = 0;
	new_client->paused_data.data = NULL;

	SLOG(LOG_DEBUG, get_tag(), "[Player] Create player : uid(%d)", uid);

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
		if (NULL != current->paused_data.data) {
			free(current->paused_data.data);
			current->paused_data.data = NULL;
		}
	}

	current->event = TTSP_RESULT_EVENT_FINISH;
	current->state = APP_STATE_READY;
	current->is_paused_data = false;
	current->idx = 0;

	SLOG(LOG_DEBUG, get_tag(), "[Player SUCCESS] Stop player : uid(%d)", uid);

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
					if (NULL != data->paused_data.data) {
						free(data->paused_data.data);
						data->paused_data.data = NULL;
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
