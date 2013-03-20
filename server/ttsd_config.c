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

#include <Ecore_File.h>
#include <runtime_info.h>
#include "ttsd_main.h"
#include "ttsd_config.h"
#include "ttsd_engine_agent.h"
#include "ttsd_data.h"

#define CONFIG_DEFAULT			BASE_DIRECTORY_DEFAULT"/ttsd.conf"

#define DEFAULT_CONFIG_FILE_NAME	"/ttsd_default.conf"
#define NOTI_CONFIG_FILE_NAME		"/ttsd_noti.conf"
#define SR_CONFIG_FILE_NAME		"/ttsd_sr.conf"

#define DEFAULT_ERROR_FILE_NAME		"/ttsd_default.err"
#define NOTI_ERROR_FILE_NAME		"/ttsd_noti.err"
#define SR_ERROR_FILE_NAME		"/ttsd_sr.err"

#define ENGINE_ID	"ENGINE_ID"
#define VOICE		"VOICE"
#define SPEED		"SPEED"

static char*	g_engine_id;
static char*	g_language;
static int	g_vc_type;
static int	g_speed;

static char*	g_config_file;

static ttsd_config_lang_changed_cb g_callback;

void __system_language_changed_cb(runtime_info_key_e key, void *user_data)
{
	if (RUNTIME_INFO_KEY_LANGUAGE == key) {
		if (TTSD_MODE_NOTIFICATION == ttsd_get_mode() || TTSD_MODE_SCREEN_READER == ttsd_get_mode()) {
			int ret = -1;
			char *value;

			ret = runtime_info_get_value_string(RUNTIME_INFO_KEY_LANGUAGE, &value);
			if (0 != ret) {
				SLOG(LOG_ERROR, get_tag(), "[Config ERROR] Fail to get system language : %d", ret);
				return;
			} else {
				SLOG(LOG_DEBUG, get_tag(), "[Config] System language changed : %s", value);
				if (NULL != g_callback)
					g_callback(value, g_vc_type);
		
				if (NULL != value)
					free(value);
			}
		}
	}

	return;
}

int __ttsd_config_save()
{
	if (0 != access(g_config_file, R_OK|W_OK)) {
		if (0 == ecore_file_mkpath(CONFIG_DIRECTORY)) {
			SLOG(LOG_ERROR, get_tag(), "[Config ERROR ] Fail to create directory (%s)", CONFIG_DIRECTORY);
			return -1;
		}

		SLOG(LOG_WARN, get_tag(), "[Config] Create directory (%s)", CONFIG_DIRECTORY);
	}

	FILE* config_fp;
	config_fp = fopen(g_config_file, "w+");

	if (NULL == config_fp) {
		/* make file and file default */
		SLOG(LOG_WARN, get_tag(), "[Config WARNING] Fail to open config (%s)", g_config_file);
		return -1;
	}

	SLOG(LOG_DEBUG, get_tag(), "[Config] Rewrite config file");

	/* Write engine id */
	fprintf(config_fp, "%s %s\n", ENGINE_ID, g_engine_id);

	/* Write voice */
	fprintf(config_fp, "%s %s %d\n", VOICE, g_language, g_vc_type);

	/* Read speed */
	fprintf(config_fp, "%s %d\n", SPEED, g_speed);

	fclose(config_fp);

	return 0;
}

int __ttsd_config_load()
{
	FILE* config_fp;
	char buf_id[256] = {0};
	char buf_param[256] = {0};
	int int_param = 0;
	bool is_default_open = false;

	config_fp = fopen(g_config_file, "r");

	if (NULL == config_fp) {
		SLOG(LOG_WARN, get_tag(), "[Config WARNING] Not open file(%s)", g_config_file);
		
		config_fp = fopen(CONFIG_DEFAULT, "r");
		if (NULL == config_fp) {
			SLOG(LOG_ERROR, get_tag(), "[Config WARNING] Not open original config file(%s)", CONFIG_DEFAULT);
			return -1;
		}
		is_default_open = true;
	}

	/* Read engine id */
	if (EOF == fscanf(config_fp, "%s %s", buf_id, buf_param)) {
		fclose(config_fp);
		SLOG(LOG_WARN, get_tag(), "[Config WARNING] Fail to read config (engine id)");
		__ttsd_config_save();
		return -1;
	} else {
		if (0 == strncmp(ENGINE_ID, buf_id, strlen(ENGINE_ID))) {
			g_engine_id = strdup(buf_param);
		} else {
			fclose(config_fp);
			SLOG(LOG_WARN, get_tag(), "[Config WARNING] Fail to load config (engine id)");
			__ttsd_config_save();
			return -1;
		}
	}

	/* Read voice */
	if (EOF == fscanf(config_fp, "%s %s %d", buf_id, buf_param, &int_param)) {
		fclose(config_fp);
		SLOG(LOG_WARN, get_tag(), "[Config WARNING] Fail to read config (voice)");
		__ttsd_config_save();
		return -1;
	} else {
		if (0 == strncmp(VOICE, buf_id, strlen(VOICE))) {
			g_language = strdup(buf_param);
			g_vc_type = int_param;
		} else {
			fclose(config_fp);
			SLOG(LOG_WARN, get_tag(), "[Config WARNING] Fail to load config (voice)");
			__ttsd_config_save();
			return -1;
		}
	}
	
	/* Read speed */
	if (EOF == fscanf(config_fp, "%s %d", buf_id, &int_param)) {
		fclose(config_fp);
		SLOG(LOG_WARN, get_tag(), "[Config WARNING] Fail to read config (speed)");
		__ttsd_config_save();
		return -1;
	} else {
		if (0 == strncmp(SPEED, buf_id, strlen(SPEED))) {
			g_speed = int_param;
		} else {
			fclose(config_fp);
			SLOG(LOG_WARN, get_tag(), "[Config WARNING] Fail to load config (speed)");
			__ttsd_config_save();
			return -1;
		}
	}
	
	fclose(config_fp);

	SLOG(LOG_DEBUG, get_tag(), "[Config] Load config : engine(%s), voice(%s,%d), speed(%d)",
		g_engine_id, g_language, g_vc_type, g_speed);

	if (true == is_default_open) {
		if(0 == __ttsd_config_save()) {
			SLOG(LOG_DEBUG, get_tag(), "[Config] Create config(%s)", g_config_file);
		}
	}

	return 0;
}

int ttsd_config_initialize(ttsd_config_lang_changed_cb callback)
{
	g_engine_id = NULL;
	g_language = NULL;
	g_vc_type = 1;
	g_speed = 3;

	g_callback = callback;

	if (TTSD_MODE_NOTIFICATION == ttsd_get_mode()) {
		g_config_file = (char*)malloc(strlen(CONFIG_DIRECTORY) + strlen(NOTI_CONFIG_FILE_NAME) + 1);
		strcpy(g_config_file, CONFIG_DIRECTORY);
		strcat(g_config_file, NOTI_CONFIG_FILE_NAME);
	} else if (TTSD_MODE_SCREEN_READER == ttsd_get_mode()) {
		g_config_file = (char*)malloc(strlen(CONFIG_DIRECTORY) + strlen(SR_CONFIG_FILE_NAME) + 1);
		strcpy(g_config_file, CONFIG_DIRECTORY);
		strcat(g_config_file, SR_CONFIG_FILE_NAME);
	} else {
		g_config_file = (char*)malloc(strlen(CONFIG_DIRECTORY) + strlen(DEFAULT_CONFIG_FILE_NAME) + 1);
		strcpy(g_config_file, CONFIG_DIRECTORY);
		strcat(g_config_file, DEFAULT_CONFIG_FILE_NAME);
	}

	__ttsd_config_load();

	/* Register system language changed callback */
	int ret = runtime_info_set_changed_cb(RUNTIME_INFO_KEY_LANGUAGE, __system_language_changed_cb, NULL);
	if (0 != ret) {
		SLOG(LOG_ERROR, get_tag(), "[Config ERROR] Fail to register callback : %d", ret);
		return TTSD_ERROR_OPERATION_FAILED;
	}

	return 0;
}

int ttsd_config_finalize()
{
	__ttsd_config_save();

	if (NULL != g_config_file) {
		free(g_config_file);
	}
	
	return 0;
}

int ttsd_config_update_language()
{
	if (TTSD_MODE_NOTIFICATION == ttsd_get_mode() || TTSD_MODE_SCREEN_READER == ttsd_get_mode()) {
		int ret = -1;
		char *value;

		ret = runtime_info_get_value_string(RUNTIME_INFO_KEY_LANGUAGE, &value);
		if (0 != ret) {
			SLOG(LOG_ERROR, get_tag(), "[Config ERROR] Fail to get system language : %d", ret);
			return -1;
		} else {
			if (NULL == value) {
				SLOG(LOG_ERROR, get_tag(), "[Config] Fail to get system language");
				return -1;
			} else {
				SLOG(LOG_DEBUG, get_tag(), "[Config] System language : %s", value);

				if (0 != strcmp(value, g_language)) {
					if (NULL != g_callback)
						g_callback(value, g_vc_type);
				}

				free(value);
			}
		}
	}

	return 0;
}

int ttsd_config_get_default_engine(char** engine_id)
{
	if (NULL == engine_id)
		return -1;

	*engine_id = strdup(g_engine_id);
	return 0;
}

int ttsd_config_set_default_engine(const char* engine_id)
{
	if (NULL == engine_id)
		return -1;

	if (NULL != g_engine_id)
		free(g_engine_id);

	g_engine_id = strdup(engine_id);
	__ttsd_config_save();
	return 0;
}

int ttsd_config_get_default_voice(char** language, int* type)
{
	if (NULL == language || NULL == type)
		return -1;

	*language = strdup(g_language);
	*type = g_vc_type;

	return 0;
}

int ttsd_config_set_default_voice(const char* language, int type)
{
	if (NULL == language)
		return -1;

	if (NULL != g_language)
		free(g_language);

	g_language = strdup(language);
	g_vc_type = type;

	__ttsd_config_save();

	return 0;
}

int ttsd_config_get_default_speed(int* speed)
{
	if (NULL == speed)
		return -1;

	*speed = g_speed;

	return 0;
}

int ttsd_config_set_default_speed(int speed)
{
	g_speed = speed;
	__ttsd_config_save();
	return 0;
}

int ttsd_config_save_error(int uid, int uttid, const char* lang, int vctype, const char* text, 
			   const char* func, int line, const char* message)
{
	char* err_file;

	if (TTSD_MODE_NOTIFICATION == ttsd_get_mode()) {
		err_file = (char*)malloc(strlen(CONFIG_DIRECTORY) + strlen(NOTI_ERROR_FILE_NAME) + 1);
		if (NULL == err_file) {
			SLOG(LOG_WARN, get_tag(), "[WARNING] Fail to get error file name");
			return -1;
		}
		strcpy(err_file, CONFIG_DIRECTORY);
		strcat(err_file, NOTI_ERROR_FILE_NAME);
	} else if (TTSD_MODE_SCREEN_READER == ttsd_get_mode()) {
		err_file = (char*)malloc(strlen(CONFIG_DIRECTORY) + strlen(SR_ERROR_FILE_NAME) + 1);
		if (NULL == err_file) {
			SLOG(LOG_WARN, get_tag(), "[WARNING] Fail to get error file name");
			return -1;
		}
		strcpy(err_file, CONFIG_DIRECTORY);
		strcat(err_file, SR_ERROR_FILE_NAME);
	} else {
		err_file = (char*)malloc(strlen(CONFIG_DIRECTORY) + strlen(DEFAULT_ERROR_FILE_NAME) + 1);
		if (NULL == err_file) {
			SLOG(LOG_WARN, get_tag(), "[WARNING] Fail to get error file name");
			return -1;
		}
		strcpy(err_file, CONFIG_DIRECTORY);
		strcat(err_file, DEFAULT_ERROR_FILE_NAME);
	}

	FILE* err_fp;
	err_fp = fopen(err_file, "w+");
	if (NULL == err_fp) {
		SLOG(LOG_WARN, get_tag(), "[WARNING] Fail to open error file (%s)", err_file);
		free(err_file);
		return -1;
	}
	SLOG(LOG_DEBUG, get_tag(), "Save Error File (%s)", err_file);

	/* func */
	if (NULL != func) {
		fprintf(err_fp, "function - %s\n", func);
	}
	
	/* line */
	fprintf(err_fp, "line - %d\n", line);

	/* message */
	if (NULL != message) {
		fprintf(err_fp, "message - %s\n", message);
	}

	int ret;
	/* uid */
	fprintf(err_fp, "uid - %d\n", uid);
	
	/* uttid */
	fprintf(err_fp, "uttid - %d\n", uttid);

	/* lang */
	if (NULL != lang) {
		fprintf(err_fp, "language - %s\n", lang);
	}

	/* vctype */
	fprintf(err_fp, "vctype - %d\n", vctype);

	/* text */
	if (NULL != text) {
		fprintf(err_fp, "text - %s\n", text);
	}

	/* get current engine */
	char *engine_id = NULL;

	ret = ttsd_engine_setting_get_engine(&engine_id);
	if (0 != ret) {
		SLOG(LOG_ERROR, get_tag(), "[ERROR] Fail to get current engine");
	} else {
		fprintf(err_fp, "current engine - %s", engine_id);
	}

	/* get data */
	ttsd_data_save_error_log(uid, err_fp);

	free(err_file);

	fclose(err_fp);

	return 0;
}