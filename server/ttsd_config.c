/*
*  Copyright (c) 2011 Samsung Electronics Co., Ltd All Rights Reserved 
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
#include "ttsd_main.h"
#include "ttsd_config.h"

#define CONFIG_FILE_PATH	BASE_DIRECTORY_DOWNLOAD"/ttsd.conf"
#define CONFIG_DEFAULT		BASE_DIRECTORY_DEFAULT"/ttsd.conf"

#define ENGINE_ID	"ENGINE_ID"
#define VOICE		"VOICE"
#define SPEED		"SPEED"


static char*	g_engine_id;
static char*	g_language;
static int	g_vc_type;
static int	g_speed;

int __ttsd_config_save()
{
	if (0 != access(CONFIG_FILE_PATH, R_OK|W_OK)) {
		if (0 == ecore_file_mkpath(BASE_DIRECTORY_DOWNLOAD)) {
			SLOG(LOG_ERROR, TAG_TTSD, "[Config ERROR ] Fail to create directory (%s)", BASE_DIRECTORY_DOWNLOAD);
			return -1;
		}

		SLOG(LOG_WARN, TAG_TTSD, "[Config] Create directory (%s)", BASE_DIRECTORY_DOWNLOAD);
	}

	FILE* config_fp;
	config_fp = fopen(CONFIG_FILE_PATH, "w+");

	if (NULL == config_fp) {
		/* make file and file default */
		SLOG(LOG_WARN, TAG_TTSD, "[Config WARNING] Fail to open config (%s)", CONFIG_FILE_PATH);
		return -1;
	}

	SLOG(LOG_DEBUG, TAG_TTSD, "[Config] Rewrite config file");

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

	config_fp = fopen(CONFIG_FILE_PATH, "r");

	if (NULL == config_fp) {
		SLOG(LOG_WARN, TAG_TTSD, "[Config WARNING] Not open file(%s)", CONFIG_FILE_PATH);
		
		config_fp = fopen(CONFIG_DEFAULT, "r");
		if (NULL == config_fp) {
			SLOG(LOG_ERROR, TAG_TTSD, "[Config WARNING] Not open original config file(%s)", CONFIG_FILE_PATH);
			return -1;
		}
		is_default_open = true;
	}

	/* Read engine id */
	if (EOF == fscanf(config_fp, "%s %s", buf_id, buf_param)) {
		fclose(config_fp);
		SLOG(LOG_WARN, TAG_TTSD, "[Config WARNING] Fail to read config (engine id)");
		__ttsd_config_save();
		return -1;
	} else {
		if (0 == strncmp(ENGINE_ID, buf_id, strlen(ENGINE_ID))) {
			g_engine_id = strdup(buf_param);
		} else {
			fclose(config_fp);
			SLOG(LOG_WARN, TAG_TTSD, "[Config WARNING] Fail to load config (engine id)");
			__ttsd_config_save();
			return -1;
		}
	}

	

	/* Read voice */
	if (EOF == fscanf(config_fp, "%s %s %d", buf_id, buf_param, &int_param)) {
		fclose(config_fp);
		SLOG(LOG_WARN, TAG_TTSD, "[Config WARNING] Fail to read config (voice)");
		__ttsd_config_save();
		return -1;
	} else {
		if (0 == strncmp(VOICE, buf_id, strlen(VOICE))) {
			g_language = strdup(buf_param);
			g_vc_type = int_param;
		} else {
			fclose(config_fp);
			SLOG(LOG_WARN, TAG_TTSD, "[Config WARNING] Fail to load config (voice)");
			__ttsd_config_save();
			return -1;
		}
	}
	
	/* Read speed */
	if (EOF == fscanf(config_fp, "%s %d", buf_id, &int_param)) {
		fclose(config_fp);
		SLOG(LOG_WARN, TAG_TTSD, "[Config WARNING] Fail to read config (speed)");
		__ttsd_config_save();
		return -1;
	} else {
		if (0 == strncmp(SPEED, buf_id, strlen(SPEED))) {
			g_speed = int_param;
		} else {
			fclose(config_fp);
			SLOG(LOG_WARN, TAG_TTSD, "[Config WARNING] Fail to load config (speed)");
			__ttsd_config_save();
			return -1;
		}
	}
	
	fclose(config_fp);

	SLOG(LOG_DEBUG, TAG_TTSD, "[Config] Load config : engine(%s), voice(%s,%d), speed(%d)",
		g_engine_id, g_language, g_vc_type, g_speed);

	if (true == is_default_open) {
		if(0 == __ttsd_config_save()) {
			SLOG(LOG_DEBUG, TAG_TTSD, "[Config] Create config(%s)", CONFIG_FILE_PATH);
		}
	}

	return 0;
}

int ttsd_config_initialize()
{
	g_engine_id = NULL;
	g_language = NULL;
	g_vc_type = 1;
	g_speed = 3;

	__ttsd_config_load();

	return 0;
}

int ttsd_config_finalize()
{
	__ttsd_config_save();
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
