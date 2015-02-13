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

#include <vector>
#include "ttsd_main.h"
#include "ttsd_data.h"

using namespace std;

typedef struct 
{
	char*	lang;
	int	vctype;
}used_voice_s;

typedef struct 
{
	int		pid;
	int		uid;
	int		utt_id_stopped;
	app_state_e	state;

	std::vector<speak_data_s> m_speak_data;
	std::vector<sound_data_s> m_wav_data;

	std::vector<used_voice_s> m_used_voice;	
}app_data_s;

static vector<app_data_s> g_app_list;


/*
* functions for debug
*/

int __data_show_list()
{
	int vsize = g_app_list.size();

	SLOG(LOG_DEBUG, get_tag(), "----- client list -----");

	for (int i=0; i<vsize; i++) {
		SECURE_SLOG(LOG_DEBUG, get_tag(), "[%dth] pid(%d), uid(%d), state(%d)", i, g_app_list[i].pid, g_app_list[i].uid, g_app_list[i].state);
	}

	if (0 == vsize) {
		SLOG(LOG_DEBUG, get_tag(), "No Client");
	}

	SLOG(LOG_DEBUG, get_tag(), "-----------------------");

	return TTSD_ERROR_NONE;
}

int __data_show_sound_list(int index)
{
	SLOG(LOG_DEBUG, get_tag(), "----- Sound list -----");
	
	unsigned int i;
	for (i=0 ; i < g_app_list[index].m_wav_data.size() ; i++) {
		SECURE_SLOG(LOG_DEBUG, get_tag(), "[%dth] data size(%ld), uttid(%d), type(%d)", 
			i+1, g_app_list[index].m_wav_data[i].data_size, g_app_list[index].m_wav_data[i].utt_id, g_app_list[index].m_wav_data[i].audio_type);
	}

	if (i == 0) {
		SLOG(LOG_DEBUG, get_tag(), "No Sound Data");
	}

	SLOG(LOG_DEBUG, get_tag(), "----------------------");
	return TTSD_ERROR_NONE;
}

int __data_show_text_list(int index)
{
	SLOG(LOG_DEBUG, get_tag(), "----- Text list -----");

	unsigned int i;
	for (i=0 ; i< g_app_list[index].m_speak_data.size() ; i++) {
		SECURE_SLOG(LOG_DEBUG, get_tag(), "[%dth] lang(%s), vctype(%d), speed(%d), uttid(%d), text(%s)", 
				i+1, g_app_list[index].m_speak_data[i].lang, g_app_list[index].m_speak_data[i].vctype, g_app_list[index].m_speak_data[i].speed,
				g_app_list[index].m_speak_data[i].utt_id, g_app_list[index].m_speak_data[i].text);	
	}

	if (0 == i) {
		SLOG(LOG_DEBUG, get_tag(), "No Text Data");
	}

	SLOG(LOG_DEBUG, get_tag(), "---------------------");
	return TTSD_ERROR_NONE;
}

int __data_show_used_voice_list(int index)
{
	SLOG(LOG_DEBUG, get_tag(), "----- Voice list -----");

	unsigned int i;
	for (i=0 ; i< g_app_list[index].m_speak_data.size() ; i++) {
		SECURE_SLOG(LOG_DEBUG, get_tag(), "[%dth] lang(%s), vctype(%d)", 
				i+1, g_app_list[index].m_used_voice[i].lang, g_app_list[index].m_used_voice[i].vctype);	
	}

	if (0 == i) {
		SLOG(LOG_DEBUG, get_tag(), "No Voice Data");
	}

	SLOG(LOG_DEBUG, get_tag(), "---------------------");
	return TTSD_ERROR_NONE;
}

/*
* ttsd data functions
*/

int ttsd_data_new_client(int pid, int uid)
{
	if( -1 != ttsd_data_is_client(uid) ) {
		SECURE_SLOG(LOG_ERROR, get_tag(), "[DATA ERROR] uid is not valid (%d)", uid);	
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	app_data_s app;
	app.pid = pid;
	app.uid = uid;
	app.utt_id_stopped = 0;
	app.state = APP_STATE_READY;

	g_app_list.insert( g_app_list.end(), app);

#ifdef DATA_DEBUG
	__data_show_list();
#endif 
	return TTSD_ERROR_NONE;
}

int ttsd_data_delete_client(int uid)
{
	int index = 0;

	index = ttsd_data_is_client(uid);
	
	if (index < 0) {
		SECURE_SLOG(LOG_ERROR, get_tag(), "[DATA ERROR] uid is not valid (%d)", uid);	
		return -1;
	}

	if (0 != ttsd_data_clear_data(uid)) {
		SLOG(LOG_ERROR, get_tag(), "[DATA ERROR] Fail to clear data");
		return -1;
	}

	g_app_list.erase(g_app_list.begin()+index);

#ifdef DATA_DEBUG
	__data_show_list();
#endif 
	return TTSD_ERROR_NONE;
}

int ttsd_data_is_client(int uid)
{
	int vsize = g_app_list.size();

	for (int i=0; i<vsize; i++) {
		if(g_app_list[i].uid == uid) {
			return i;		
		}
	}

	return -1;
}

int ttsd_data_get_client_count()
{
	return g_app_list.size();
}

int ttsd_data_get_pid(int uid)
{
	int index;

	index = ttsd_data_is_client(uid);
	
	if (index < 0)	{
		SECURE_SLOG(LOG_ERROR, get_tag(), "[DATA ERROR] uid is not valid (%d)", uid);	
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	return g_app_list[index].pid;
}

int ttsd_data_get_speak_data_size(int uid)
{
	int index = 0;
	index = ttsd_data_is_client(uid);
	
	if (index < 0) {
		SECURE_SLOG(LOG_ERROR, get_tag(), "[DATA ERROR] uid is not valid (%d)", uid);
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	int size = g_app_list[index].m_speak_data.size();
	return size;
}

int ttsd_data_set_used_voice(int uid, const char* lang, int type)
{
	int index = 0;
	index = ttsd_data_is_client(uid);

	if (index < 0) {
		SECURE_SLOG(LOG_ERROR, get_tag(), "[DATA ERROR] uid is not valid (%d)", uid);	
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	/* Find voice */
	int vsize = g_app_list[index].m_used_voice.size();
	int i = 0;

	for (i = 0;i < vsize;i++) {
		if (0 == strcmp(lang, g_app_list[index].m_used_voice[i].lang) && 
			type == g_app_list[index].m_used_voice[i].vctype) {
			SLOG(LOG_DEBUG, get_tag(), "[DATA] The voice is already registered (%s)(%d)", lang, type);
			return 0;
		}
	}

	/* Add voice */
	used_voice_s used_voice;
	used_voice.lang = strdup(lang);
	used_voice.vctype = type;

	g_app_list[index].m_used_voice.insert(g_app_list[index].m_used_voice.end(), used_voice);

#ifdef DATA_DEBUG
	__data_show_used_voice_list(index);
#endif

	return -1;	/* Need to load voice*/
}

int ttsd_data_reset_used_voice(int uid, ttsd_used_voice_cb callback)
{
	int index = 0;
	index = ttsd_data_is_client(uid);

	if (index < 0) {
		SECURE_SLOG(LOG_ERROR, get_tag(), "[DATA ERROR] uid is not valid (%d)", uid);
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	/* Find voice */
	int vsize = g_app_list[index].m_used_voice.size();
	int i = 0;

	for (i = 0;i < vsize;i++) {
		if (NULL != callback) {
			callback(g_app_list[index].m_used_voice[i].lang, g_app_list[index].m_used_voice[i].vctype);
		} else {
			SECURE_SLOG(LOG_WARN, get_tag(), "[DATA WARNING] Used voice callback is NULL");
		}

		if (NULL != g_app_list[index].m_used_voice[i].lang) {
			free(g_app_list[index].m_used_voice[i].lang);
		}
	} 

	g_app_list[index].m_used_voice.clear();

	return TTSD_ERROR_NONE;
}

int ttsd_data_add_speak_data(int uid, speak_data_s data)
{
	int index = 0;
	index = ttsd_data_is_client(uid);

	if (index < 0) {
		SECURE_SLOG(LOG_ERROR, get_tag(), "[DATA ERROR] uid is not valid (%d)", uid);	
		return TTSD_ERROR_INVALID_PARAMETER;
	}
	
	g_app_list[index].m_speak_data.insert(g_app_list[index].m_speak_data.end(), data);

	if (1 == data.utt_id)
		g_app_list[index].utt_id_stopped = 0;

#ifdef DATA_DEBUG
	__data_show_text_list(index);
#endif 
	return TTSD_ERROR_NONE;
}

int ttsd_data_get_speak_data(int uid, speak_data_s* data)
{
	int index = 0;
	index = ttsd_data_is_client(uid);

	if (index < 0) {
		SECURE_SLOG(LOG_ERROR, get_tag(), "[DATA ERROR] uid is not valid(%d)", uid);	
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	if (0 == g_app_list[index].m_speak_data.size()) {
#ifdef DATA_DEBUG
		SLOG(LOG_WARN, get_tag(), "[DATA WARNING] There is no speak data"); 
#endif 
		return -1;
	}

	data->lang = g_strdup(g_app_list[index].m_speak_data[0].lang);
	data->vctype = g_app_list[index].m_speak_data[0].vctype;
	data->speed = g_app_list[index].m_speak_data[0].speed;

	data->text = g_app_list[index].m_speak_data[0].text;
	data->utt_id = g_app_list[index].m_speak_data[0].utt_id;

	g_app_list[index].m_speak_data.erase(g_app_list[index].m_speak_data.begin());

#ifdef DATA_DEBUG
	__data_show_text_list(index);
#endif 
	return TTSD_ERROR_NONE;
}

int ttsd_data_add_sound_data(int uid, sound_data_s data)
{
	int index = 0;
	index = ttsd_data_is_client(uid);

	if(index < 0) {
		SECURE_SLOG(LOG_ERROR, get_tag(), "[DATA ERROR] uid is not valid (%d)", uid);	
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	g_app_list[index].m_wav_data.insert(g_app_list[index].m_wav_data.end(), data);

#ifdef DATA_DEBUG
	__data_show_sound_list(index);
#endif 
	return TTSD_ERROR_NONE;
}

int ttsd_data_get_sound_data(int uid, sound_data_s* data)
{
	int index = 0;
	index = ttsd_data_is_client(uid);

	if (index < 0)	{
		SECURE_SLOG(LOG_ERROR, get_tag(), "[DATA ERROR] uid is not valid (%d)", uid);	
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	if (0 == g_app_list[index].m_wav_data.size()) {
#ifdef DATA_DEBUG
		SLOG(LOG_DEBUG, get_tag(), "[DATA] There is no wav data");
#endif
		return -1;
	}

	data->data = g_app_list[index].m_wav_data[0].data;
	data->data_size = g_app_list[index].m_wav_data[0].data_size;
	data->utt_id = g_app_list[index].m_wav_data[0].utt_id;
	data->audio_type = g_app_list[index].m_wav_data[0].audio_type;
	data->rate = g_app_list[index].m_wav_data[0].rate;
	data->channels = g_app_list[index].m_wav_data[0].channels;
	data->event = g_app_list[index].m_wav_data[0].event;

	g_app_list[index].m_wav_data.erase(g_app_list[index].m_wav_data.begin());

#ifdef DATA_DEBUG
	__data_show_sound_list(index);
#endif 
	return TTSD_ERROR_NONE;
}

int ttsd_data_get_sound_data_size(int uid)
{
	int index = 0;
	index = ttsd_data_is_client(uid);

	if (index < 0)	{
		SECURE_SLOG(LOG_ERROR, get_tag(), "[DATA ERROR] uid is not valid (%d)", uid);	
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	return g_app_list[index].m_wav_data.size();
}

int ttsd_data_clear_data(int uid)
{
	int index = 0;

	index = ttsd_data_is_client(uid);
	if (index < 0) {
		SECURE_SLOG(LOG_ERROR, get_tag(), "[DATA ERROR] uid is not valid (%d)", uid);	
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	int removed_last_uttid = -1;
	/* free allocated data */
	while(1) {
		speak_data_s temp;
		if (0 != ttsd_data_get_speak_data(uid, &temp)) {
			break;
		}

		if (NULL != temp.text)	free(temp.text);
		if (NULL != temp.lang)	free(temp.lang);

		removed_last_uttid = temp.utt_id;
	}

	if (-1 != removed_last_uttid) {
		g_app_list[index].utt_id_stopped = removed_last_uttid;
	}

	while(1) {
		sound_data_s temp;
		if (0 != ttsd_data_get_sound_data(uid, &temp)) {
			break;
		}

		if (0 < temp.data_size) {
			if (NULL != temp.data)	free(temp.data);
		} else {
			SLOG(LOG_ERROR, get_tag(), "Sound data is NULL");
		}
	}

	g_app_list[index].m_speak_data.clear();
	g_app_list[index].m_wav_data.clear();

	return TTSD_ERROR_NONE;
}

int ttsd_data_get_client_state(int uid, app_state_e* state)
{
	int index = 0;

	index = ttsd_data_is_client(uid);
	if (index < 0)	{
		SECURE_SLOG(LOG_ERROR, get_tag(), "[DATA ERROR] uid is not valid (%d)", uid);	
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	*state = g_app_list[index].state;

	return TTSD_ERROR_NONE;
}

int ttsd_data_set_client_state(int uid, app_state_e state)
{
	int index = 0;

	index = ttsd_data_is_client(uid);
	if (index < 0)	{
		SECURE_SLOG(LOG_ERROR, get_tag(), "[DATA ERROR] uid is not valid (%d)", uid);	
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	/* The client of playing state of all clients is only one. need to check state. */
	if (APP_STATE_PLAYING == state) {
		int vsize = g_app_list.size();
		for (int i=0 ; i<vsize ; i++) {
			if(g_app_list[i].state == APP_STATE_PLAYING) {
				SLOG(LOG_ERROR, get_tag(), "[DATA ERROR] A playing client has already existed.");	
				return -1;
			}
		}
	}

	g_app_list[index].state = state;

	return TTSD_ERROR_NONE;
}

int ttsd_data_get_current_playing()
{
	int vsize = g_app_list.size();

	for (int i=0; i<vsize; i++) {
		if (APP_STATE_PLAYING == g_app_list[i].state) {
			SLOG(LOG_DEBUG, get_tag(), "[DATA] uid(%d) is playing", g_app_list[i].uid);
			return g_app_list[i].uid;
		}
	}

	return -1;
}

int ttsd_data_foreach_clients(ttsd_data_get_client_cb callback, void* user_data)
{
	if (NULL == callback) {
		SLOG(LOG_ERROR, get_tag(), "[DATA ERROR] input data is NULL!!");
		return -1;
	}

#ifdef DATA_DEBUG
	__data_show_list();
#endif 
	
	/* Copy app info */
	vector<app_data_s> temp_app_list;
	int vsize = g_app_list.size();

	int i = 0;
	for (i = 0;i < vsize;i++) {
		app_data_s app;
		app.pid = g_app_list[i].pid;
		app.uid = g_app_list[i].uid;
		app.utt_id_stopped = 0;
		app.state = g_app_list[i].state;

		temp_app_list.insert(temp_app_list.end(), app);
	}

	for (i = 0;i < vsize;i++) {
		SECURE_SLOG(LOG_DEBUG, get_tag(), "[%dth] pid(%d), uid(%d), state(%d)", i, temp_app_list[i].pid, temp_app_list[i].uid, temp_app_list[i].state);
		if (false == callback(temp_app_list[i].pid, temp_app_list[i].uid, temp_app_list[i].state, user_data)) {
			break;
		}
	}

	for (i = 0;i < vsize;i++) {
		temp_app_list.erase(temp_app_list.begin());
	}
	
	return 0;
}

bool ttsd_data_is_uttid_valid(int uid, int uttid)
{
	int index = 0;

	index = ttsd_data_is_client(uid);
	if (index < 0)	{
		SECURE_SLOG(LOG_ERROR, get_tag(), "[DATA ERROR] uid is not valid (%d)", uid);	
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	if (uttid < g_app_list[index].utt_id_stopped)
		return false;

	return true;
}

int ttsd_data_is_current_playing()
{
	int vsize = g_app_list.size();

	for (int i=0; i<vsize; i++) {
		if(g_app_list[i].state == APP_STATE_PLAYING) {
			return g_app_list[i].uid;		
		}
	}

	return -1;
}

int ttsd_data_get_same_pid_client_count(int pid)
{
	int vsize = g_app_list.size();
	int number = 0;

	for (int i = 0;i < vsize;i++) {
		if(g_app_list[i].pid == pid) {
			number++;		
		}
	}

	return number;
}

int ttsd_data_save_error_log(int uid, FILE* fp)
{
	int ret;
	int pid;
	/* pid */
	pid = ttsd_data_get_pid(uid);
	if (0 > pid) {
		SLOG(LOG_ERROR, get_tag(), "[ERROR] Fail to get pid");
	} else {
		fprintf(fp, "pid - %d", pid);
	}
	/* app state */
	app_state_e state;
	ret = ttsd_data_get_client_state(uid, &state);
	if (0 != ret) {
		SLOG(LOG_ERROR, get_tag(), "[ERROR] Fail to get app state");
	} else {
		fprintf(fp, "app state - %d", state);
	}

	int index = 0;
	unsigned int i;

	index = ttsd_data_is_client(uid);

	/* get sound data */
	fprintf(fp, "----- Sound list -----");
	
	for (i=0 ; i < g_app_list[index].m_wav_data.size() ; i++) {
		fprintf(fp, "[%dth] data size(%d), uttid(%d), type(%d)", 
			i+1, g_app_list[index].m_wav_data[i].data_size, g_app_list[index].m_wav_data[i].utt_id, g_app_list[index].m_wav_data[i].audio_type );
	}
	fprintf(fp, "----------------------");
	
	/* get speck data */
	fprintf(fp, "----- Text list -----");

	for (i=0 ; i< g_app_list[index].m_speak_data.size() ; i++) {
		fprintf(fp, "[%dth] lang(%s), vctype(%d), speed(%d), uttid(%d), text(%s)", 
				i+1, g_app_list[index].m_speak_data[i].lang, g_app_list[index].m_speak_data[i].vctype, g_app_list[index].m_speak_data[i].speed,
				g_app_list[index].m_speak_data[i].utt_id, g_app_list[index].m_speak_data[i].text );	
	}
	fprintf(fp, "---------------------");
	
	return 0;
}
