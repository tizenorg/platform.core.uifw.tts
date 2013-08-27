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


#include <mm_types.h>
#include <mm_player.h>
#include <mm_player_internal.h>
#include <mm_session.h>
#include <mm_error.h>
#include <Ecore.h>

#include "ttsd_main.h"
#include "ttsd_player.h"
#include "ttsd_data.h"
#include "ttsd_dbus.h"


/*
* Internal data structure
*/

#define TEMP_FILE_MAX	36

typedef struct {
	char	riff[4];
	int	file_size;
	char	wave[4];
	char	fmt[4];
	int	header_size;
	short	sample_format;
	short	n_channels;
	int	sample_rate;
	int	bytes_per_second;
	short	block_align;
	short	bits_per_sample;
	char	data[4];
	int	data_size;
} WavHeader;

typedef struct {
	int		uid;		/** client id */
	MMHandleType	player_handle;	/** mm player handle */
	int		utt_id;		/** utt_id of next file */
	ttsp_result_event_e event;	/** event of callback */
	bool		pause_after_complete;	/** For resume when the 'Paused' state of player after end callback */
} player_s;

typedef struct {
	int  uid;
	int  utt_id;
	ttsp_result_event_e event;
	char filename[TEMP_FILE_MAX];
} user_data_s;


/*
* static data
*/

#define TEMP_FILE_PATH  "/tmp"
#define FILE_PATH_SIZE  256
#define DEFAULT_FILE_SIZE 10

/** player init info */
static bool g_player_init = false;

/** tts engine list */
static GList *g_player_list;

/** current player information */
static player_s* g_playing_info;

/** player callback function */
static player_result_callback_func g_result_callback;

/** numbering for temp file */
static unsigned int g_index;              

/*
* Internal Interfaces 
*/
player_s* __player_get_item(int uid);

int __save_file(int uid, int index, sound_data_s data, char** filename);

int __set_and_start(player_s* player);

int __init_wave_header(WavHeader* hdr, size_t nsamples, size_t sampling_rate, int channel);

static int msg_callback(int message, void *data, void *user_param) ;


/*
* Player Interfaces 
*/

int ttsd_player_init(player_result_callback_func result_cb)
{
	if (NULL == result_cb) {
		SLOG(LOG_ERROR, get_tag(), "[Player ERROR] invalid parameter");
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	g_result_callback = result_cb;

	g_playing_info = NULL;
	
	g_index = 1;
	g_player_init = true;

	if (TTSD_MODE_DEFAULT == ttsd_get_mode()) {
		if (MM_ERROR_NONE != mm_session_init(MM_SESSION_TYPE_EXCLUSIVE)) {
			SLOG(LOG_ERROR, get_tag(), "[Player ERROR] Fail mm_session_init(MM_SESSION_TYPE_EXCLUSIVE)");	
		} else {
			SLOG(LOG_ERROR, get_tag(), "[Player SUCCESS] mm_session_init(MM_SESSION_TYPE_EXCLUSIVE)");	
		}
	}

	return 0;
}

int ttsd_player_release(void)
{
	if (false == g_player_init) {
		SLOG(LOG_ERROR, get_tag(), "[Player ERROR] Not Initialized");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	/* clear g_player_list */
	g_playing_info = NULL;
	g_player_init = false;

	return 0;
}

int ttsd_player_create_instance(int uid)
{
	if (false == g_player_init) {
		SLOG(LOG_ERROR, get_tag(), "[Player ERROR] Not Initialized" );
		return -1;
	}
	
	/* Check uid is duplicated */
	if (NULL != __player_get_item(uid)) {
		SECURE_SLOG(LOG_ERROR, get_tag(), "[Player ERROR] uid(%d) is already registered", uid); 
		return -1;
	}

	int ret = MM_ERROR_NONE; 
	MMHandleType player_handle;
	
	ret = mm_player_create(&player_handle);
	if (ret != MM_ERROR_NONE || 0 == player_handle) {
		SLOG(LOG_ERROR, get_tag(), "[Player ERROR] fail mm_player_create() : %x", ret);
		return -2;
	}

	player_s* new_client = (player_s*)g_malloc0( sizeof(player_s) * 1);

	new_client->uid = uid;
	new_client->player_handle = player_handle;
	new_client->utt_id = -1;
	new_client->event = TTSP_RESULT_EVENT_FINISH;
	new_client->pause_after_complete = false;
	
	SECURE_SLOG(LOG_DEBUG, get_tag(), "[Player] Create player : uid(%d), handle(%d)", uid, player_handle );

	g_player_list = g_list_append(g_player_list, new_client);

	return 0;
}

int ttsd_player_destroy_instance(int uid)
{
	if (false == g_player_init) {
		SLOG(LOG_ERROR, get_tag(), "[Player ERROR] Not Initialized" );
		return -1;
	}

	player_s* current;
	current = __player_get_item(uid);
	if (NULL == current) {
		SECURE_SLOG(LOG_ERROR, get_tag(), "[Player ERROR] uid(%d) is not valid", uid); 
		return -1;
	}

	if (NULL != g_playing_info) {
		if (uid == g_playing_info->uid) {
			g_playing_info = NULL;
		}
	}

	MMPlayerStateType player_state;
	mm_player_get_state(current->player_handle, &player_state);

	SLOG(LOG_DEBUG, get_tag(), "[PLAYER] State changed : state(%d)", player_state);

	int ret = -1;
	/* destroy player */
	switch (player_state) {
		case MM_PLAYER_STATE_PLAYING:
		case MM_PLAYER_STATE_PAUSED:
		case MM_PLAYER_STATE_READY:
			ret = mm_player_unrealize(current->player_handle);
			if (MM_ERROR_NONE != ret) {
				SLOG(LOG_ERROR, get_tag(), "[Player ERROR] fail mm_player_unrealize() : %x", ret);
			} 
			/* NO break for destroy */

		case MM_PLAYER_STATE_NULL:
			ret = mm_player_destroy(current->player_handle);
			if (MM_ERROR_NONE != ret) {
				SLOG(LOG_ERROR, get_tag(), "[Player ERROR] fail mm_player_destroy() : %x", ret);
			} 
			break;

		default:
			break;
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
					g_free(data);
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
	SECURE_SLOG(LOG_DEBUG, get_tag(), "[Player] start play : uid(%d)", uid );

	if (false == g_player_init) {
		SLOG(LOG_ERROR, get_tag(), "[Player ERROR] Not Initialized" );
		return -1;
	}

	if (NULL != g_playing_info) {
		if (uid == g_playing_info->uid) {
			SECURE_SLOG(LOG_WARN, get_tag(), "[Player WARNING] uid(%d) has already played", g_playing_info->uid); 
			return 0;
		}
	}

	/* Check sound queue size */
	if (0 == ttsd_data_get_sound_data_size(uid)) {
		SECURE_SLOG(LOG_WARN, get_tag(), "[Player WARNING] A sound queue of current player(%d) is empty", uid); 
		return -1;
	}

	/* Check uid */
	player_s* current;
	current = __player_get_item(uid);
	if (NULL == current) {
		SECURE_SLOG(LOG_ERROR, get_tag(), "[Player ERROR] uid(%d) is not valid", uid); 
		return -1;
	}

	MMPlayerStateType player_state;
	mm_player_get_state(current->player_handle, &player_state);

	SLOG(LOG_DEBUG, get_tag(), "[PLAYER] State changed : state(%d)", player_state);

	switch (player_state) {
		case MM_PLAYER_STATE_PLAYING:
			SLOG(LOG_WARN, get_tag(), "[Player] Current player is playing. Do not start new sound.");
			return 0;

		case MM_PLAYER_STATE_PAUSED:
			SLOG(LOG_WARN, get_tag(), "[Player] Player is paused. Do not start new sound.");
			return -1;

		case MM_PLAYER_STATE_READY:
			SLOG(LOG_WARN, get_tag(), "[Player] Player is ready for next play. Do not start new sound.");
			return -1;

		case MM_PLAYER_STATE_NULL:
			break;

		case MM_PLAYER_STATE_NONE:
			SLOG(LOG_WARN, get_tag(), "[Player] Player is created. Do not start new sound.");
			return -1;

		default:
			return -1;
	}

	int ret;
	ret = __set_and_start(current);
	if (0 != ret) {
		SLOG(LOG_ERROR, get_tag(), "[Player ERROR] fail to set or start mm_player");
		return ret;
	}

	SECURE_SLOG(LOG_DEBUG, get_tag(), "[Player] Started play and wait for played callback : uid(%d)", uid);

	return 0;
}

int __ttsd_player_next_play(int uid)
{
	if (false == g_player_init) {
		SLOG(LOG_ERROR, get_tag(), "[Player ERROR] Not Initialized" );
		return -1;
	}

	/* Check uid */
	player_s* current;
	current = __player_get_item(uid);
	if (NULL == current) {
		SECURE_SLOG(LOG_ERROR, get_tag(), "[Player ERROR] uid(%d) is not valid", uid); 
		g_playing_info = NULL;
		return -1;
	}

	if (NULL != g_playing_info) {
		if (uid != g_playing_info->uid) {
			SECURE_SLOG(LOG_WARN, get_tag(), "[Player WARNING] Current player(%d) is NOT uid(%d)", g_playing_info->uid, uid); 
			return 0;
		}
	} else {
		SLOG(LOG_WARN, get_tag(), "[Player WARNING] Current player do NOT exist"); 
		return -1;
	}

	MMPlayerStateType player_state;
	mm_player_get_state(current->player_handle, &player_state);

	SLOG(LOG_DEBUG, get_tag(), "[PLAYER] State changed : state(%d)", player_state);

	int ret = -1;
	/* stop player */
	switch (player_state) {
		case MM_PLAYER_STATE_PLAYING:
		case MM_PLAYER_STATE_PAUSED:
		case MM_PLAYER_STATE_READY:
			ret = mm_player_unrealize(current->player_handle);
			if (MM_ERROR_NONE != ret) {
				SLOG(LOG_ERROR, get_tag(), "[Player ERROR] fail mm_player_unrealize() : %x", ret);
				return -1;
			} 
			break;

		case MM_PLAYER_STATE_NULL:
			break;

		default:
			break;
	}

	/* Check sound queue size */
	if (0 == ttsd_data_get_sound_data_size(uid)) {
		SECURE_SLOG(LOG_WARN, get_tag(), "[Player WARNING] A sound queue of current player(%d) is empty", uid); 
		g_playing_info = NULL;
		return -1;
	}

	ret = __set_and_start(current);
	if (0 != ret) {
		SLOG(LOG_ERROR, get_tag(), "[Player ERROR] fail to set or start mm_player");
		return ret;
	}

	SECURE_SLOG(LOG_DEBUG, get_tag(), "[Player] Started play and wait for played callback : uid(%d)", uid);

	return 0;
}


int ttsd_player_stop(int uid)
{
	SECURE_SLOG(LOG_DEBUG, get_tag(), "[Player] stop player : uid(%d)", uid );

	if (false == g_player_init) {
		SLOG(LOG_ERROR, get_tag(), "[Player ERROR] Not Initialized" );
		return -1;
	}

	/* Check uid */
	player_s* current;
	current = __player_get_item(uid);
	if (NULL == current) {
		SECURE_SLOG(LOG_ERROR, get_tag(), "[Player ERROR] uid(%d) is not valid", uid); 
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

	current->utt_id = -1;
	current->pause_after_complete = false;

	MMPlayerStateType player_state;
	mm_player_get_state(current->player_handle, &player_state);

	SLOG(LOG_DEBUG, get_tag(), "[PLAYER] Current state(%d)", player_state);

	int ret = -1;
	switch (player_state) {
		case MM_PLAYER_STATE_PLAYING:
		case MM_PLAYER_STATE_PAUSED:
		case MM_PLAYER_STATE_READY:
			ret = mm_player_unrealize(current->player_handle);
			if (MM_ERROR_NONE != ret) {
				SLOG(LOG_ERROR, get_tag(), "[Player ERROR] fail mm_player_unrealize() : %x", ret);
				return -1;
			} 
			break;

		case MM_PLAYER_STATE_NULL:
			break;
		
		default:
			break;
	}

	SECURE_SLOG(LOG_DEBUG, get_tag(), "[Player SUCCESS] Stop player : uid(%d)", uid);

	return 0;
}

int ttsd_player_pause(int uid)
{
	SECURE_SLOG(LOG_DEBUG, get_tag(), "[Player] pause player : uid(%d)", uid );

	if (false == g_player_init) {
		SLOG(LOG_ERROR, get_tag(), "[Player ERROR] Not Initialized" );
		return -1;
	}

	/* Check uid */
	player_s* current;
	current = __player_get_item(uid);
	if (NULL == current) {
		SECURE_SLOG(LOG_ERROR, get_tag(), "[Player ERROR] ttsd_player_pause() : uid(%d) is not valid", uid); 
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

	MMPlayerStateType player_state;
	mm_player_get_state(current->player_handle, &player_state);

	SLOG(LOG_DEBUG, get_tag(), "[PLAYER] Current state(%d)", player_state);

	int ret = 0;
	if (MM_PLAYER_STATE_PLAYING == player_state) {
		ret = mm_player_pause(current->player_handle);
		if (MM_ERROR_NONE != ret) {
			SLOG(LOG_ERROR, get_tag(), "[Player ERROR] fail mm_player_pause : %x ", ret);
		}
	} else {
		SLOG(LOG_WARN, get_tag(), "[Player WARNING] Current player is NOT 'playing'");
	}
	

	return 0;
}

int ttsd_player_resume(int uid)
{
	SECURE_SLOG(LOG_DEBUG, get_tag(), "[Player] Resume player : uid(%d)", uid );

	if (false == g_player_init) {
		SLOG(LOG_ERROR, get_tag(), "[Player ERROR] Not Initialized" );
		return -1;
	}

	/* Check id */
	player_s* current;
	current = __player_get_item(uid);
	if (NULL == current) {
		SECURE_SLOG(LOG_ERROR, get_tag(), "[Player ERROR] uid(%d) is not valid", uid); 
		return -1;
	}

	/* check current player */
	if (NULL != g_playing_info)
		g_playing_info = NULL;

	
	MMPlayerStateType player_state;
	mm_player_get_state(current->player_handle, &player_state);

	SLOG(LOG_DEBUG, get_tag(), "[PLAYER] Current state(%d)", player_state);

	int ret = -1;
	if (MM_PLAYER_STATE_PAUSED == player_state) {
		/* When the 'Paused' state of player after end of play */
		if (true == current->pause_after_complete) {
			g_playing_info = current;

			current->pause_after_complete = false;
			
			/* Current state need load and play */
			ret = __ttsd_player_next_play(uid);
			if (0 != ret) {
				SLOG(LOG_ERROR, get_tag(), "[player] Fail to next play in resume function");
			}
		} else {
			ret = mm_player_resume(current->player_handle);
			if (MM_ERROR_NONE != ret) {
				SLOG(LOG_ERROR, get_tag(), "[Player ERROR] fail mm_player_resume() : %d", ret);
				return -1;
			} else {
				SLOG(LOG_DEBUG, get_tag(), "[Player] Resume player");
			}

			g_playing_info = current;
		}
	} else {
		SLOG(LOG_WARN, get_tag(), "[Player WARNING] Current uid is NOT paused state.");
	}

	return 0;
}

int ttsd_player_get_state(int uid, ttsd_player_state_e* state)
{
	if (false == g_player_init) {
		SLOG(LOG_ERROR, get_tag(), "[Player ERROR] Not Initialized" );
		return -1;
	}

	player_s* current;
	current = __player_get_item(uid);
	if (NULL == current) {
		SECURE_SLOG(LOG_ERROR, get_tag(), "[Player ERROR] uid(%d) is not valid", uid); 
		return -1;
	}

	MMPlayerStateType player_state;
	mm_player_get_state(current->player_handle, &player_state);

	SLOG(LOG_DEBUG, get_tag(), "[PLAYER] State changed : state(%d)", player_state);

	/* destroy player */
	switch (player_state) {
		case MM_PLAYER_STATE_PLAYING:
			*state = TTSD_PLAYER_STATE_PLAYING;
			break;
		case MM_PLAYER_STATE_PAUSED:
			*state = TTSD_PLAYER_STATE_PAUSED;
			break;
		
		case MM_PLAYER_STATE_NULL:
			*state = TTSD_PLAYER_STATE_NULL;
			break;

		case MM_PLAYER_STATE_READY:
		default:
			SECURE_SLOG(LOG_ERROR, get_tag(), "[Player ERROR] player state of uid(%d) is not valid", uid); 
			return -1;
			break;
	}

	return 0;
}

int ttsd_player_get_current_client()
{
	if (false == g_player_init) {
		SLOG(LOG_ERROR, get_tag(), "[Player ERROR] Not Initialized" );
		return -1;
	}

	if (NULL != g_playing_info) 
		return g_playing_info->uid;

	SLOG(LOG_WARN, get_tag(), "[Player WARNING] No current player"); 

	return 0;
}

int ttsd_player_get_current_utterance_id(int uid)
{
	SECURE_SLOG(LOG_DEBUG, get_tag(), "[Player] get current utt id : uid(%d)", uid );

	if (false == g_player_init) {
		SLOG(LOG_ERROR, get_tag(), "[Player ERROR] Not Initialized" );
		return -1;
	}

	/* Check uid */
	player_s* current;
	current = __player_get_item(uid);
	if (NULL == current) {
		SECURE_SLOG(LOG_ERROR, get_tag(), "[Player ERROR] uid(%d) is not valid", uid); 
		return -1;
	}

	return current->utt_id;
}

int ttsd_player_all_stop()
{
	if (false == g_player_init) {
		SLOG(LOG_ERROR, get_tag(), "[Player ERROR] Not Initialized" );
		return -1;
	}

	g_playing_info = NULL;

	int ret = -1;
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
				SLOG(LOG_ERROR, get_tag(), "[player ERROR] ttsd_player_all_stop : uid is not valid ");
				ttsd_player_destroy_instance(data->uid);
				iter = g_list_next(iter);
				continue;
			}

			if (APP_STATE_PLAYING == state || APP_STATE_PAUSED == state) {
				/* unrealize player */
				ret = mm_player_unrealize(data->player_handle);
				if (MM_ERROR_NONE != ret) {
					SLOG(LOG_ERROR, get_tag(), "[Player ERROR] fail mm_player_unrealize() : %x", ret);
				} 

				data->utt_id = -1;
				data->event = TTSP_RESULT_EVENT_FINISH;
			}
			
			/* Get next item */
			iter = g_list_next(iter);
		}
	}

	SLOG(LOG_DEBUG, get_tag(), "[Player SUCCESS] player all stop!! ");

	return 0;
}

static Eina_Bool __player_next_play(void *data)
{
	SLOG(LOG_DEBUG, get_tag(), "===== PLAYER NEXT PLAY");

	int* uid = (int*)data;

	if (NULL == uid) {
		SLOG(LOG_ERROR, get_tag(), "[PLAYER ERROR] uid is NULL");
		SLOG(LOG_DEBUG, get_tag(), "=====");
		SLOG(LOG_DEBUG, get_tag(), "  ");
		return EINA_FALSE;
	}

	SECURE_SLOG(LOG_DEBUG, get_tag(), "[PLAYER] uid = %d", *uid);
	
	if (0 != __ttsd_player_next_play(*uid)) {
		SLOG(LOG_WARN, get_tag(), "[PLAYER WARNING] Fail to play next");
	}

	free(uid);

	SLOG(LOG_DEBUG, get_tag(), "=====");
	SLOG(LOG_DEBUG, get_tag(), "  ");

	return EINA_FALSE;
}

static int msg_callback(int message, void *data, void *user_param) 
{
	user_data_s* user_data = NULL;

	user_data = (user_data_s*)user_param;

	if (NULL == user_data) {
		SLOG(LOG_ERROR, get_tag(), "[PLAYER ERROR] user_param is NULL");
		return -1;
	}

	int uid = user_data->uid;
	int utt_id = user_data->utt_id;

	MMMessageParamType *msg = (MMMessageParamType*)data;

	switch (message) {
	case MM_MESSAGE_ERROR:
		{
			SLOG(LOG_DEBUG, get_tag(), "===== PLAYER ERROR CALLBACK");
			SECURE_SLOG(LOG_ERROR, get_tag(), "[PLAYER ERROR] Info : uid(%d), utt id(%d), error file(%s)", uid, utt_id, user_data->filename);

			/* send error info */
			g_result_callback(PLAYER_ERROR, uid, utt_id);

			player_s* current;
			current = __player_get_item(uid);
			if (NULL == current) {
				SECURE_SLOG(LOG_ERROR, get_tag(), "[PLAYER ERROR] uid(%d) is NOT valid ", uid); 
			} else {
				current->event = TTSP_RESULT_EVENT_FINISH;
			}

			if (NULL != user_data) 
				g_free(user_data);

			/* check current player */
			if (NULL != g_playing_info) {
				if (uid == g_playing_info->uid) {
					g_playing_info = NULL;
					SECURE_SLOG(LOG_WARN, get_tag(), "[PLAYER] Current Player is NOT uid(%d)", uid);
				}
			}

			SLOG(LOG_DEBUG, get_tag(), "=====");
			SLOG(LOG_DEBUG, get_tag(), "  ");
		}
		break;	/*MM_MESSAGE_ERROR*/

	case MM_MESSAGE_BEGIN_OF_STREAM:

		break;

	case MM_MESSAGE_END_OF_STREAM:
		{
			SLOG(LOG_DEBUG, get_tag(), "===== END OF STREAM CALLBACK");

			if (-1 == remove(user_data->filename)) {
				SECURE_SLOG(LOG_WARN, get_tag(), "[PLAYER WARNING] Fail to remove temp file", user_data->filename); 
			}

			/* Check uid */
			player_s* current;
			current = __player_get_item(uid);
			if (NULL == current) {
				SECURE_SLOG(LOG_ERROR, get_tag(), "[PLAYER ERROR] uid(%d) is NOT valid", uid); 
				if (NULL != g_playing_info) {
					if (uid == g_playing_info->uid) {
						g_playing_info = NULL;
						SECURE_SLOG(LOG_WARN, get_tag(), "[PLAYER] Current Player is NOT uid(%d)", uid);
					}
				}
				SLOG(LOG_DEBUG, get_tag(), "=====");
				SLOG(LOG_DEBUG, get_tag(), "  ");
				return -1;
			}

			g_free(user_data);

			int pid = ttsd_data_get_pid(uid);

			/* send utterence finish signal */
			if (TTSP_RESULT_EVENT_FINISH == current->event) {
				if (0 == ttsdc_send_utt_finish_message(pid, uid, utt_id))
					SECURE_SLOG(LOG_DEBUG, get_tag(), "[Send SUCCESS] Send Utterance Completed Signal : pid(%d), uid(%d), uttid(%d)", pid, uid, utt_id);
				else 
					SECURE_SLOG(LOG_ERROR, get_tag(), "[Send ERROR] Fail to send Utterance Completed Signal : pid(%d), uid(%d), uttid(%d)", pid, uid, utt_id);
			}

			/* for sync problem */
			MMPlayerStateType player_state;
			mm_player_get_state(current->player_handle, &player_state);
			
			if (MM_PLAYER_STATE_PAUSED == player_state) {
				/* The current state of player is 'Paused' */
				current->pause_after_complete = true;
			} else {
				/* play state */
				int* uid_data = (int*) g_malloc0(sizeof(int));
				*uid_data = uid;

				SECURE_SLOG(LOG_DEBUG, get_tag(), "[PLAYER] uid = %d", *uid_data);

				ecore_timer_add(0, __player_next_play, (void*)uid_data);
			}

			SLOG(LOG_DEBUG, get_tag(), "=====");
			SLOG(LOG_DEBUG, get_tag(), "  ");
		}
		break;	/*MM_MESSAGE_END_OF_STREAM*/

	case MM_MESSAGE_STATE_CHANGED:
		break;

	case MM_MESSAGE_STATE_INTERRUPTED:
		if (MM_PLAYER_STATE_PAUSED == msg->state.current) {

			SLOG(LOG_DEBUG, get_tag(), "===== INTERRUPTED CALLBACK");

			ttsd_data_set_client_state(uid, APP_STATE_PAUSED);

			int pid = ttsd_data_get_pid(uid);
			/* send message to client about changing state */
			ttsdc_send_set_state_message (pid, uid, APP_STATE_PAUSED);

			SLOG(LOG_DEBUG, get_tag(), "=====");
			SLOG(LOG_DEBUG, get_tag(), "  ");
		}
		break;

	default:
		break;
	}

	return TRUE;
}

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

int __save_file(int uid, int index, sound_data_s data, char** filename)
{
	char postfix[5];
	memset(postfix, '\0', 5);

	switch (data.audio_type) {
	case TTSP_AUDIO_TYPE_RAW:
	case TTSP_AUDIO_TYPE_WAV:
		strcpy(postfix, "wav");
		break;
	case TTSP_AUDIO_TYPE_MP3:
		strcpy(postfix, "mp3");
		break;
	case TTSP_AUDIO_TYPE_AMR:
		strcpy(postfix, "amr");
		break;
	default:
		SECURE_SLOG(LOG_ERROR, get_tag(), "[Player ERROR] Audio type(%d) is NOT valid", data.audio_type); 
		return -1;
	}

	/* make filename to save */
	char* temp;
	temp = (char*)g_malloc0(sizeof(char) * FILE_PATH_SIZE);
	if (NULL == temp) {
		SLOG(LOG_ERROR, get_tag(), "[Player Error] make buf is failed");
		return -1;
	}

	int ret = snprintf(temp, FILE_PATH_SIZE, "%s/ttstemp%d_%d.%s", TEMP_FILE_PATH, uid, index, postfix);

	if (0 >= ret) {
		if (NULL != temp)
			g_free(temp);
		return -1;
	}

	FILE* fp;
	fp = fopen(temp, "wb");

	if (fp == NULL) {
		SLOG(LOG_ERROR, get_tag(), "[Player ERROR] temp file open error");
		if (NULL != temp)
			g_free(temp);
		return -1;
	}

	if (data.audio_type == TTSP_AUDIO_TYPE_RAW) {
		WavHeader header;
		if (0 != __init_wave_header(&header, data.data_size, data.rate, data.channels)) {
			fclose(fp);
			if (NULL != temp)
				g_free(temp);
			return -1;
		}

		if (0 >= fwrite(&header, sizeof(WavHeader), 1, fp)) {
			SLOG(LOG_ERROR, get_tag(), "[Player ERROR] fail to write wav header to file");
			fclose(fp);
			if (NULL != temp)
				g_free(temp);
			return -1;
		}
	}

	int size = fwrite(data.data, data.data_size, 1,  fp);
	if (size <= 0) {
		size = fwrite("0000000000", DEFAULT_FILE_SIZE, 1,  fp);
		if (size <= 0) {
			SLOG(LOG_ERROR, get_tag(), "[Player ERROR] Fail to write date");
			fclose(fp);
			if (NULL != temp)
				g_free(temp);
			return -1;
		}	
	} 

	fclose(fp);
	*filename = temp;
	
	SLOG(LOG_DEBUG, get_tag(), " ");
	SECURE_SLOG(LOG_DEBUG, get_tag(), "Filepath : %s ", *filename);
	SECURE_SLOG(LOG_DEBUG, get_tag(), "Header : Data size(%d), Sample rate(%d), Channel(%d) ", data.data_size, data.rate, data.channels);

	return 0;
}

int __init_wave_header (WavHeader* hdr, size_t nsamples, size_t sampling_rate, int channel)
{
	if (hdr == NULL || sampling_rate <= 0 || channel <= 0) {
		SLOG(LOG_ERROR, get_tag(), "[Player ERROR] __init_wave_header : input parameter invalid");
		SECURE_SLOG(LOG_ERROR, get_tag(), "[Player ERROR] hdr : %p", hdr);
		SECURE_SLOG(LOG_ERROR, get_tag(), "[Player ERROR] nsample : %d", nsamples);
		SECURE_SLOG(LOG_ERROR, get_tag(), "[Player ERROR] sampling_rate : %", sampling_rate);
		SECURE_SLOG(LOG_ERROR, get_tag(), "[Player ERROR] channel : %", channel);
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	size_t bytesize = DEFAULT_FILE_SIZE;

	if (0 < nsamples) {
		bytesize = nsamples;	
	} 

	/* NOT include \0(NULL) */
	strncpy(hdr->riff, "RIFF", 4);	
	hdr->file_size = (int)(bytesize  + 36);
	strncpy(hdr->wave, "WAVE", 4);
	strncpy(hdr->fmt, "fmt ", 4);	/* fmt + space */
	hdr->header_size = 16;
	hdr->sample_format = 1;		/* WAVE_FORMAT_PCM */
	hdr->n_channels = channel;
	hdr->sample_rate = (int)(sampling_rate);
	hdr->bytes_per_second = (int)sampling_rate * sizeof(short);
	hdr->block_align =  sizeof(short);
	hdr->bits_per_sample = sizeof(short)*8;
	strncpy(hdr->data, "data", 4);	
	hdr->data_size = (int)bytesize;

	return 0;
}

int __set_and_start(player_s* player)
{
	/* get sound data */
	sound_data_s wdata;
	if (0 != ttsd_data_get_sound_data(player->uid, &wdata)) {
		SECURE_SLOG(LOG_WARN, get_tag(), "[Player WARNING] A sound queue of current player(%d) is empty", player->uid); 
		return -1;
	}

	g_index++;
	if (10000 <= g_index)	{
		g_index = 1;
	}

	int ret;

	/* make sound file for mmplayer */
	char* sound_file = NULL;
	ret = __save_file(player->uid, g_index, wdata, &sound_file);
	if (0 != ret || NULL == sound_file) {
		SLOG(LOG_ERROR, get_tag(), "[Player ERROR] fail to make sound file");
		return -1;
	}
	
	user_data_s* user_data = (user_data_s*)g_malloc0(sizeof(user_data_s));
	user_data->uid = player->uid;
	user_data->utt_id = wdata.utt_id;
	user_data->event = wdata.event;
	memset(user_data->filename, 0, TEMP_FILE_MAX); 
	strncpy( user_data->filename, sound_file, strlen(sound_file) );

	SECURE_SLOG(LOG_DEBUG, get_tag(), "Info : uid(%d), utt(%d), filename(%s) , event(%d)", 
		user_data->uid, user_data->utt_id, user_data->filename, user_data->event);
	SLOG(LOG_DEBUG, get_tag(), " ");

	
	/* set callback func */
	ret = mm_player_set_message_callback(player->player_handle, msg_callback, (void*)user_data);
	if (MM_ERROR_NONE != ret) {
		SLOG(LOG_ERROR, get_tag(), "[Player ERROR] Fail mm_player_set_message_callback() : %x ", ret);
		if (NULL != user_data)	g_free(user_data);
		return -1;
	}

	/* set playing info to mm player */
	char* err_attr_name = NULL;

	if (0 != access(sound_file, R_OK)) {
		SECURE_SLOG(LOG_ERROR, get_tag(), "[Player ERROR] Fail to read sound file (%s)", sound_file);
		if (NULL != user_data)	g_free(user_data);
		return -1;
	}

	ret = mm_player_set_attribute(player->player_handle, &err_attr_name,
		"profile_uri", sound_file , strlen(sound_file) + 1,
		"sound_volume_type", MM_SOUND_VOLUME_TYPE_MEDIA,
		"sound_route", MM_AUDIOROUTE_PLAYBACK_NORMAL,
		NULL );

	if (MM_ERROR_NONE != ret) {
		if (NULL != err_attr_name) {
			SLOG(LOG_ERROR, get_tag(), "[Player ERROR] Fail mm_player_set_attribute() : msg(%s), result(%x) ", err_attr_name, ret);
		}
		if (NULL != user_data)	g_free(user_data);
		return -1;
	}

	if (TTSD_MODE_DEFAULT != ttsd_get_mode()) {
		ret = mm_player_ignore_session(player->player_handle);
		if (MM_ERROR_NONE != ret) {
			SLOG(LOG_WARN, get_tag(), "[Player WARNING] fail mm_player_ignore_session() : %x", ret);
		}
	}
	
	/* realize and start mm player */ 
	ret = mm_player_realize(player->player_handle);
	if (MM_ERROR_NONE != ret) {
		SLOG(LOG_ERROR, get_tag(), "[Player ERROR] fail mm_player_realize() : %x", ret);

		if (NULL != user_data)	g_free(user_data);
		return -2;
	}

	ret = mm_player_start(player->player_handle);
	if (MM_ERROR_NONE != ret) {
		SLOG(LOG_ERROR, get_tag(), "[Player ERROR] fail mm_player_start() : %x", ret);
		mm_player_unrealize(player->player_handle);

		int reason;
		if (MM_ERROR_POLICY_BLOCKED == ret) {
			SLOG(LOG_ERROR, get_tag(), "[Player ERROR] Blocked to start player by policy : %x", ret);
			reason = TTSD_ERROR_AUDIO_POLICY_BLOCKED;
		} else {
			reason = TTSD_ERROR_OPERATION_FAILED;
		}

		if (NULL != user_data) {
			ttsd_data_set_error_data(user_data->uid, user_data->utt_id, reason);
			g_free(user_data);
		}
		return -3;
	}

	/* If wdata's event is 'start', current wdata is first data of engine for synthesis. 
	 * If wdata's event is 'finish', player should check previous event to know whether this wdata is first or not.
	 * When previous wdata's event is 'finish' and current wdata's event is 'finish', 
	 * the player should send utt started event. 
	 */
	if (TTSP_RESULT_EVENT_START == wdata.event ||
	    (TTSP_RESULT_EVENT_FINISH == player->event && TTSP_RESULT_EVENT_FINISH == wdata.event)) {
		int pid;
		pid = ttsd_data_get_pid(player->uid);

		/* send utterance start message */
		if (0 != ttsdc_send_utt_start_message(pid, player->uid, wdata.utt_id)) {
			SECURE_SLOG(LOG_ERROR, get_tag(), "[Send ERROR] Fail to send Utterance Start Signal : pid(%d), uid(%d), uttid(%d)", pid, player->uid, wdata.utt_id);
		}
	} else {
		SLOG(LOG_DEBUG, get_tag(), "[PLAYER] Don't need to send Utterance Start Signal");
	}

	g_playing_info = player;

	if (NULL != sound_file)	
		g_free(sound_file);
	if (NULL != wdata.data)	
		g_free(wdata.data);

	return 0;
}


