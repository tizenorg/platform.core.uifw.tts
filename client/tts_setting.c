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


#include <sys/wait.h>
#include <Ecore.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include "tts_main.h"
#include "tts_setting.h"
#include "tts_setting_dbus.h"

static bool g_is_daemon_started = false;

static int __check_setting_tts_daemon();

static tts_setting_state_e g_state = TTS_SETTING_STATE_NONE;

static tts_setting_initialized_cb g_initialized_cb;
static void* g_user_data;

static int g_reason;

/* API Implementation */
static Eina_Bool __tts_setting_initialized(void *data)
{
	g_initialized_cb(g_state, g_reason, g_user_data);

	return EINA_FALSE;
}

static Eina_Bool __tts_setting_connect_daemon(void *data)
{
	/* Send hello */
	if (0 != tts_setting_dbus_request_hello()) {
		if (false == g_is_daemon_started) {
			g_is_daemon_started = true;
			__check_setting_tts_daemon();
		}
		return EINA_TRUE;
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "===== Connect daemon");

	/* do request initialize */
	int ret = -1;

	ret = tts_setting_dbus_request_initialize();

	if (TTS_SETTING_ERROR_ENGINE_NOT_FOUND == ret) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Engine not found");
	} else if (TTS_SETTING_ERROR_NONE != ret) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to connection : %d", ret);
	} else {
		/* success to connect tts-daemon */
		g_state = TTS_SETTING_STATE_READY;
	}

	g_reason = ret;

	ecore_timer_add(0, __tts_setting_initialized, NULL);

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return EINA_FALSE;
}

int tts_setting_initialize()
{
	SLOG(LOG_DEBUG, TAG_TTSC, "===== Initialize TTS Setting");

	if (TTS_SETTING_STATE_READY == g_state) {
		SLOG(LOG_WARN, TAG_TTSC, "[WARNING] TTS Setting has already been initialized. \n");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_SETTING_ERROR_NONE;
	}

	if( 0 != tts_setting_dbus_open_connection() ) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to open connection\n ");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_SETTING_ERROR_OPERATION_FAILED;
	}

	/* Send hello */
	if (0 != tts_setting_dbus_request_hello()) {
		__check_setting_tts_daemon();
	}

	/* do request */
	int i = 1;
	int ret = 0;
	while(1) {
		ret = tts_setting_dbus_request_initialize();

		if( TTS_SETTING_ERROR_ENGINE_NOT_FOUND == ret ) {
			SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Engine not found");
			break;
		} else if(ret) {
			sleep(1);
			if (i == 3) {
				SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Connection Time out");
				ret = TTS_SETTING_ERROR_TIMED_OUT;			    
				break;
			}    
			i++;
		} else {
			/* success to connect tts-daemon */
			break;
		}
	}

	if (TTS_SETTING_ERROR_NONE == ret) {
		g_state = TTS_SETTING_STATE_READY;
		SLOG(LOG_DEBUG, TAG_TTSC, "[SUCCESS] Initialize");
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return ret;
}

int tts_setting_initialize_async(tts_setting_initialized_cb callback, void* user_data)
{
	SLOG(LOG_DEBUG, TAG_TTSC, "===== Initialize TTS Setting");

	if (TTS_SETTING_STATE_READY == g_state) {
		SLOG(LOG_WARN, TAG_TTSC, "[WARNING] TTS Setting has already been initialized. \n");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_SETTING_ERROR_NONE;
	}

	if( 0 != tts_setting_dbus_open_connection() ) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to open connection\n ");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_SETTING_ERROR_OPERATION_FAILED;
	}
	
	g_initialized_cb = callback;
	g_user_data = user_data;

	ecore_timer_add(0, __tts_setting_connect_daemon, NULL);

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return TTS_SETTING_ERROR_NONE;
}


int tts_setting_finalize()
{
	SLOG(LOG_DEBUG, TAG_TTSC, "===== Finalize TTS Setting");

	if (TTS_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_WARN, TAG_TTSC, "[WARNING] Not initialized");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_SETTING_ERROR_INVALID_STATE;
	}

	int ret = tts_setting_dbus_request_finalilze(); 
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] result : %d", ret);
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");

		return TTS_SETTING_ERROR_OPERATION_FAILED;
	}
	g_is_daemon_started = false;
	
	if (0 != tts_setting_dbus_close_connection()) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to close connection\n ");
	} else {
		SLOG(LOG_DEBUG, TAG_TTSC, "[SUCCESS] Finalize");
	}

	g_state = TTS_SETTING_STATE_NONE;
	
	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return TTS_SETTING_ERROR_NONE;
}

int tts_setting_foreach_supported_engines(tts_setting_supported_engine_cb callback, void* user_data)
{    
	SLOG(LOG_DEBUG, TAG_TTSC, "===== Foreach supported engines");

	if (TTS_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Not initialized");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_SETTING_ERROR_INVALID_STATE;
	}

	if (NULL == callback) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Callback is NULL");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_SETTING_ERROR_INVALID_PARAMETER;
	}

	int ret = tts_setting_dbus_request_get_engine_list(callback, user_data);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Result : %d", ret);
	} else {
		SLOG(LOG_DEBUG, TAG_TTSC, "[SUCCESS] Foreach supported engines");
	}
	
	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");
		
	return ret;
}

int tts_setting_get_engine(char** engine_id)
{
	SLOG(LOG_DEBUG, TAG_TTSC, "===== Get current engine");

	if (TTS_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Not initialized");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_SETTING_ERROR_INVALID_STATE;
	}

	if (NULL == engine_id) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Engine id is NULL");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_SETTING_ERROR_INVALID_PARAMETER;
	}

	int ret = tts_setting_dbus_request_get_engine(engine_id);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Result : %d", ret);
	} else {
		SLOG(LOG_DEBUG, TAG_TTSC, "[SUCCESS] Get current engine");
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return ret;
}

int tts_setting_set_engine(const char* engine_id)
{
	SLOG(LOG_DEBUG, TAG_TTSC, "===== Set current engine");

	if (TTS_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Not initialized");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_SETTING_ERROR_INVALID_STATE;
	}

	if (NULL == engine_id) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Engine id is NULL");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_SETTING_ERROR_INVALID_PARAMETER;
	}

	int ret = tts_setting_dbus_request_set_engine(engine_id);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Result : %d", ret);
	} else {
		SLOG(LOG_DEBUG, TAG_TTSC, "[SUCCESS] Set current engine");
	}
	
	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");
    
	return ret;
}

int tts_setting_foreach_surpported_voices(tts_setting_supported_voice_cb callback, void* user_data)
{
	SLOG(LOG_DEBUG, TAG_TTSC, "===== Foreach supported voices");

	if (TTS_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Not initialized");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_SETTING_ERROR_INVALID_STATE;
	}

	if (NULL == callback) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Param is NULL");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_SETTING_ERROR_INVALID_PARAMETER;
	}

	int ret = tts_setting_dbus_request_get_voice_list(callback, user_data);

	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Result : %d", ret);
	} else {
		SLOG(LOG_DEBUG, TAG_TTSC, "[SUCCESS] Foreach supported voices");
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return ret;
}

int tts_setting_get_default_voice(char** language, tts_setting_voice_type_e* voice_type)
{
	SLOG(LOG_DEBUG, TAG_TTSC, "===== Get default voice");

	if (TTS_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Not initialized");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_SETTING_ERROR_INVALID_STATE;
	}

	if (NULL == language || NULL == voice_type) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Input parameter is NULL");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_SETTING_ERROR_INVALID_PARAMETER;
	}

	int ret = tts_setting_dbus_request_get_default_voice(language, voice_type);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Result : %d", ret);
	} else {
		SLOG(LOG_DEBUG, TAG_TTSC, "[SUCCESS] Foreach supported voices");
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return ret;
}

int tts_setting_set_default_voice(const char* language, tts_setting_voice_type_e voice_type)
{
	SLOG(LOG_DEBUG, TAG_TTSC, "===== Set default voice");

	if (TTS_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Not initialized");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_SETTING_ERROR_INVALID_STATE;
	}

	if (NULL == language) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Input parameter is NULL");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_SETTING_ERROR_INVALID_PARAMETER;
	}

	if (voice_type < TTS_SETTING_VOICE_TYPE_MALE || TTS_SETTING_VOICE_TYPE_USER3 < voice_type ) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Invalid voice type");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_SETTING_ERROR_INVALID_PARAMETER;
	}

	int ret = tts_setting_dbus_request_set_default_voice(language, voice_type);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Result : %d", ret);
	} else {
		SLOG(LOG_DEBUG, TAG_TTSC, "[SUCCESS] Set default voice");
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");
    
	return ret;
}


int tts_setting_get_default_speed(tts_setting_speed_e* speed)
{
	SLOG(LOG_DEBUG, TAG_TTSC, "===== Get default speed");

	if (TTS_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Not initialized");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_SETTING_ERROR_INVALID_STATE;
	}

	if (NULL == speed) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Param is NULL");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_SETTING_ERROR_INVALID_PARAMETER;
	}

	int temp;
	temp = 0;

	int ret = tts_setting_dbus_request_get_default_speed(&temp);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Result : %d", ret);
	} else {
		/* Copy value */
		*speed = (tts_setting_speed_e)temp;
		SLOG(LOG_DEBUG, TAG_TTSC, "[SUCCESS] Get default speed : %d ", (int)*speed);
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return ret;
}


int tts_setting_set_default_speed(tts_setting_speed_e speed)
{
	SLOG(LOG_DEBUG, TAG_TTSC, "===== Set default speed");

	if (TTS_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Not initialized");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_SETTING_ERROR_INVALID_STATE;
	}

	if (speed < TTS_SETTING_SPEED_VERY_SLOW || TTS_SETTING_SPEED_VERY_FAST < speed) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Invalid speed");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_SETTING_ERROR_INVALID_PARAMETER;
	}

	int ret = tts_setting_dbus_request_set_default_speed((int)speed);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Result : %d", ret);
	} else {
		SLOG(LOG_DEBUG, TAG_TTSC, "[SUCCESS] Set default speed");
	}
	
	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return ret;
}

int tts_setting_foreach_engine_settings(tts_setting_engine_setting_cb callback, void* user_data)
{
	SLOG(LOG_DEBUG, TAG_TTSC, "===== Foreach engine setting");

	if (TTS_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Not initialized");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_SETTING_ERROR_INVALID_STATE;
	}

	if (NULL == callback) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Input parameter is NULL");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_SETTING_ERROR_INVALID_PARAMETER;
	}

	int ret = tts_setting_dbus_request_get_engine_setting(callback, user_data);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Result : %d", ret);
	} else {
		SLOG(LOG_DEBUG, TAG_TTSC, "[SUCCESS] Foreach engine setting");
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return ret;
}

int tts_setting_set_engine_setting(const char* key, const char* value)
{
	SLOG(LOG_DEBUG, TAG_TTSC, "===== Set engine setting");

	if (TTS_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Not initialized");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_SETTING_ERROR_INVALID_STATE;
	}

	if(NULL == key || NULL == value) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Param is NULL");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_SETTING_ERROR_INVALID_PARAMETER;
	}

	int ret = tts_setting_dbus_request_set_engine_setting(key, value);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Result : %d", ret);
	} else {
		SLOG(LOG_DEBUG, TAG_TTSC, "[SUCCESS] Foreach engine setting");
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return ret;
}

int __setting_get_cmd_line(char *file, char *buf) 
{
	FILE *fp = NULL;
	int i;

	fp = fopen(file, "r");
	if (fp == NULL) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Get command line");
		return -1;
	}

	memset(buf, 0, 256);
	fgets(buf, 256, fp);
	fclose(fp);

	return 0;
}

/* Functions for tts-daemon fork */
static bool __tts_setting_is_alive()
{
	DIR *dir;
	struct dirent *entry;
	struct stat filestat;
	
	int pid;
	char cmdLine[256];
	char tempPath[256];

	dir  = opendir("/proc");
	if (NULL == dir) {
		SLOG(LOG_ERROR, TAG_TTSC, "process checking is FAILED");
		return FALSE;
	}

	while ((entry = readdir(dir)) != NULL) {
		if (0 != lstat(entry->d_name, &filestat))
			continue;

		if (!S_ISDIR(filestat.st_mode))
			continue;

		pid = atoi(entry->d_name);
		if (pid <= 0) continue;

		sprintf(tempPath, "/proc/%d/cmdline", pid);
		if (0 != __setting_get_cmd_line(tempPath, cmdLine)) {
			continue;
		}

		if (0 == strncmp(cmdLine, "[tts-daemon]", strlen("[tts-daemon]")) ||
			0 == strncmp(cmdLine, "tts-daemon", strlen("tts-daemon")) ||
			0 == strncmp(cmdLine, "/usr/bin/tts-daemon", strlen("/usr/bin/tts-daemon"))) {
				SLOG(LOG_DEBUG, TAG_TTSC, "tts-daemon is ALIVE !! \n");
				closedir(dir);
				return TRUE;
		}
	}
	SLOG(LOG_DEBUG, TAG_TTSC, "THERE IS NO tts-daemon !! \n");

	closedir(dir);
	return FALSE;

}

static int __check_setting_tts_daemon()
{
	if( TRUE == __tts_setting_is_alive() )
		return 0;

	/* fork-exec tts-daemom */
	int pid, i;

	pid = fork();

	switch(pid) {
	case -1:
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to create tts-daemon ");
		break;

	case 0:
		setsid();
		for( i = 0 ; i < _NSIG ; i++ )
			signal(i, SIG_DFL);

		execl("/usr/bin/tts-daemon", "/usr/bin/tts-daemon", NULL);
		break;

	default:
		break;
	}

	return 0;
}