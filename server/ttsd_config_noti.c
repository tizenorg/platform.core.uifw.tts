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
#define NOTI_ERROR_FILE_NAME		CONFIG_DIRECTORY"/ttsd_noti.err"

#define ENGINE_ID	"ENGINE_ID"

static char*	g_engine_id;
static char*	g_language;
static int	g_vc_type;
static int	g_speed;

static ttsd_config_lang_changed_cb g_lang_cb;
static ttsd_config_speed_changed_cb g_speed_cb;

void __noti_system_language_changed_cb(runtime_info_key_e key, void *user_data)
{
	if (RUNTIME_INFO_KEY_LANGUAGE == key) {
		if (TTSD_MODE_NOTIFICATION == ttsd_get_mode()) {
			int ret = -1;
			char *value;

			ret = runtime_info_get_value_string(RUNTIME_INFO_KEY_LANGUAGE, &value);
			if (0 != ret) {
				SLOG(LOG_ERROR, get_tag(), "[Config ERROR] Fail to get system language : %d", ret);
				return;
			} else {
				SLOG(LOG_DEBUG, get_tag(), "[Config] System language changed : %s", value);
				if (NULL != g_lang_cb)
					g_lang_cb(value, g_vc_type);
		
				if (NULL != value)
					free(value);
			}
		}
	}

	return;
}

int ttsd_config_initialize(ttsd_config_lang_changed_cb lang_cb, ttsd_config_speed_changed_cb speed_cb)
{
	int ret = -1;
	char* value = NULL;
	FILE* config_fp;
	char buf_id[256] = {0};
	char buf_param[256] = {0};

	g_engine_id = NULL;
	g_language = NULL;
	g_vc_type = 2;
	g_speed = 3;

	g_lang_cb = lang_cb;
	g_speed_cb = speed_cb;
	
	ecore_file_mkpath(CONFIG_DIRECTORY);

	/* Get default engine id */
	config_fp = fopen(CONFIG_DEFAULT, "r");
	if (NULL == config_fp) {
		SLOG(LOG_ERROR, get_tag(), "[Config ERROR] Not open original config file(%s)", CONFIG_DEFAULT);
		return -1;
	}

	/* Read engine id */
	if (EOF == fscanf(config_fp, "%s %s", buf_id, buf_param)) {
		fclose(config_fp);
		SLOG(LOG_WARN, get_tag(), "[Config ERROR] Fail to read config (engine id)");
		return -1;
	}

	fclose(config_fp);

	/* Get display language */
	ret = runtime_info_get_value_string(RUNTIME_INFO_KEY_LANGUAGE, &value);
	if (0 != ret || NULL == value) {
		SLOG(LOG_ERROR, get_tag(), "[Config ERROR] Fail to get system language : %d", ret);
		return -1;
	} else {
		SLOG(LOG_DEBUG, get_tag(), "[Config] System language changed : %s", value);
		
		g_language = strdup(value);

		if (NULL != value)
			free(value);
	}

	/* Register system language changed callback */
	ret = runtime_info_set_changed_cb(RUNTIME_INFO_KEY_LANGUAGE, __noti_system_language_changed_cb, NULL);
	if (0 != ret) {
		SLOG(LOG_ERROR, get_tag(), "[Config ERROR] Fail to register callback : %d", ret);
		return TTSD_ERROR_OPERATION_FAILED;
	}

	return 0;
}

int ttsd_config_finalize()
{
	if (NULL != g_language) {
		free(g_language);
	}

	if (NULL != g_engine_id) {
		free(g_engine_id);
	}

	return 0;
}

int ttsd_config_update_language()
{
	if (TTSD_MODE_NOTIFICATION == ttsd_get_mode()) {
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
					if (NULL != g_lang_cb)
						g_lang_cb(value, g_vc_type);
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

	if (NULL != g_engine_id) {
		*engine_id = strdup(g_engine_id);
	} else {
		SLOG(LOG_ERROR, get_tag(), "[Config ERROR] Current engine id is NULL");
		return -1;
	}
	return 0;
}

int ttsd_config_set_default_engine(const char* engine_id)
{
	/* Not available in notification mode */
	return 0;
}

int ttsd_config_get_default_voice(char** language, int* type)
{
	if (NULL == language || NULL == type)
		return -1;

	if (NULL != g_language) {
		*language = strdup(g_language);
		*type = g_vc_type;
	} else {
		SLOG(LOG_ERROR, get_tag(), "[Config ERROR] Current language is NULL");
		return -1;
	}
	return 0;
}

int ttsd_config_set_default_voice(const char* language, int type)
{
	/* Not available in notification mode */
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
	/* Not available in notification mode */
	return 0;
}

int ttsd_config_save_error(int uid, int uttid, const char* lang, int vctype, const char* text, 
			   const char* func, int line, const char* message)
{
	FILE* err_fp;
	err_fp = fopen(NOTI_ERROR_FILE_NAME, "a");
	if (NULL == err_fp) {
		SLOG(LOG_WARN, get_tag(), "[WARNING] Fail to open error file (%s)", NOTI_ERROR_FILE_NAME);
		return -1;
	}
	SLOG(LOG_DEBUG, get_tag(), "Save Error File (%s)", NOTI_ERROR_FILE_NAME);

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

	fclose(err_fp);

	return 0;
}