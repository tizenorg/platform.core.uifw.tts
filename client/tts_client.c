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


#include "tts_client.h"

/* Max number of handle */
static const int g_max_handle = 999;
/* allocated handle */
static int g_allocated_handle = 0;
/* client list */
static GList *g_client_list = NULL;

/* private functions */
static int __client_generate_uid(int pid)
{
	g_allocated_handle++;

	if (g_allocated_handle > g_max_handle) {
		g_allocated_handle = 1;
	}

	/* generate uid, handle number should be smaller than 1000 */
	return pid * 1000 + g_allocated_handle;
}

int tts_client_new(tts_h* tts)
{
	tts_client_s* client = NULL;
	client = (tts_client_s*)calloc(1, sizeof(tts_client_s));
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to allocate memory");
		return TTS_ERROR_OUT_OF_MEMORY;
	}
	tts_h temp = (tts_h)calloc(1, sizeof(struct tts_s));
	if (NULL == temp) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to allocate memory");
		free(client);
		return TTS_ERROR_OUT_OF_MEMORY;
	}
	temp->handle = __client_generate_uid(getpid()); 

	/* initialize client data */
	client->tts = temp;
	client->pid = getpid(); 
	client->uid = temp->handle;
	client->current_utt_id = 0;

	client->state_changed_cb = NULL;
	client->state_changed_user_data = NULL;

	client->utt_started_cb = NULL;
	client->utt_started_user_data = NULL;
	client->utt_completeted_cb = NULL;
	client->utt_completed_user_data = NULL;

	client->error_cb = NULL;
	client->error_user_data = NULL;
	client->default_voice_changed_cb = NULL;
	client->default_voice_changed_user_data = NULL;
	client->supported_voice_cb = NULL;
	client->supported_voice_user_data = NULL;

	client->mode = TTS_MODE_DEFAULT;
	client->before_state = TTS_STATE_CREATED; 
	client->current_state = TTS_STATE_CREATED; 

	client->cb_ref_count = 0;

	client->utt_id = 0;
	client->reason = 0;
	client->err_msg = NULL;

	client->conn_timer = NULL;

	client->credential = NULL;
	client->credential_needed = false;

	g_client_list = g_list_append(g_client_list, client);

	*tts = temp;

	SLOG(LOG_DEBUG, TAG_TTSC, "[Success] Create client object : uid(%d)", client->uid); 

	return TTS_ERROR_NONE;
}

int tts_client_destroy(tts_h tts)
{
	if (tts == NULL) {
		SLOG(LOG_ERROR, TAG_TTSC, "Input parameter is NULL");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	GList *iter = NULL;
	tts_client_s *data = NULL;

	/* if list have item */
	if (g_list_length(g_client_list) > 0) {
		/* Get a first item */
		iter = g_list_first(g_client_list);

		while (NULL != iter) {
			data = iter->data;
			if (tts->handle == data->tts->handle) {
				g_client_list = g_list_remove_link(g_client_list, iter);

				while (0 != data->cb_ref_count) {
					/* wait for release callback function */
				}

				if (NULL != data->err_msg) {
					free(data->err_msg);
					data->err_msg = NULL;
				}

				if (NULL != data->credential) {
					free(data->credential);
					data->credential = NULL;
				}

				free(data);
				free(tts);

				data = NULL;
				tts = NULL;

				SLOG(LOG_DEBUG, TAG_TTSC, "Client destroy");
				g_list_free(iter);

				return TTS_ERROR_NONE;
			}

			/* Next item */
			iter = g_list_next(iter);
		}
	}
	SLOG(LOG_ERROR, TAG_TTSC, "Fail to destroy client : handle is not valid");

	return TTS_ERROR_INVALID_PARAMETER;
}

tts_client_s* tts_client_get(tts_h tts)
{
	if (tts == NULL) {
		SLOG(LOG_ERROR, TAG_TTSC, "Input parameter is NULL");
		return NULL;
	}

	GList *iter = NULL;
	tts_client_s *data = NULL;

	if (g_list_length(g_client_list) > 0) {
		/* Get a first item */
		iter = g_list_first(g_client_list);

		while (NULL != iter) {
			data = iter->data;

			if (tts->handle == data->tts->handle) 
				return data;

			/* Next item */
			iter = g_list_next(iter);
		}
	}

	SLOG(LOG_ERROR, TAG_TTSC, "handle is not valid");

	return NULL;
}

tts_client_s* tts_client_get_by_uid(const int uid)
{
	if (uid < 0) {
		SLOG(LOG_ERROR, TAG_TTSC, "out of range : handle");
		return NULL;
	}

	GList *iter = NULL;
	tts_client_s *data = NULL;

	if (g_list_length(g_client_list) > 0) {
		/* Get a first item */
		iter = g_list_first(g_client_list);

		while (NULL != iter) {
			data = iter->data;
			if (uid == data->uid) {
				return data;
			}

			/* Next item */
			iter = g_list_next(iter);
		}
	}

	SLOG(LOG_WARN, TAG_TTSC, "uid is not valid");

	return NULL;
}

int tts_client_get_size()
{
	return g_list_length(g_client_list);
}

int tts_client_use_callback(tts_client_s* client)
{
	client->cb_ref_count++;
	return 0;
}

int tts_client_not_use_callback(tts_client_s* client)
{
	client->cb_ref_count--;
	return 0;
}

int tts_client_get_use_callback(tts_client_s* client)
{
	return client->cb_ref_count;
}

int tts_client_get_connected_client_count()
{
	GList *iter = NULL;
	tts_client_s *data = NULL;
	int number = 0;

	if (g_list_length(g_client_list) > 0) {
		/* Get a first item */
		iter = g_list_first(g_client_list);

		while (NULL != iter) {
			data = iter->data;
			if (0 < data->current_state) {
				number++;
			}

			/* Next item */
			iter = g_list_next(iter);
		}
	}
	return number;
}

int tts_client_get_mode_client_count(tts_mode_e mode)
{
	GList *iter = NULL;
	tts_client_s *data = NULL;
	int number = 0;

	if (g_list_length(g_client_list) > 0) {
		/* Get a first item */
		iter = g_list_first(g_client_list);

		while (NULL != iter) {
			data = iter->data;
			if (0 < data->current_state && data->mode == mode) {
				number++;
			}

			/* Next item */
			iter = g_list_next(iter);
		}
	}
	return number;
}

GList* tts_client_get_client_list()
{
	return g_client_list;
}