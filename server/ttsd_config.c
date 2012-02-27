/*
* Copyright (c) 2011 Samsung Electronics Co., Ltd All Rights Reserved 
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


#include <vconf.h>
#include "ttsd_main.h"
#include "ttsd_config.h"

/*
* tts-daemon config
*/

int ttsd_config_get_char_type(const char* key, char** value)
{
	if (NULL == key || NULL == value) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Config ERROR] Input parameter is NULL\n");
		return TTSD_ERROR_INVALID_PARAMETER;
	} 

	*value = vconf_get_str(key);
	if (NULL == *value) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Config ERROR] fail to get char type from config : key(%s)\n", key);
		return -1;
	}

	return 0;
}

int ttsd_config_set_char_type(const char* key, const char* value)
{
	if (NULL == key || NULL == value) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Config ERROR] Input parameter is NULL\n");
		return TTSD_ERROR_INVALID_PARAMETER;
	} 

	if (0 != vconf_set_str(key, value)) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Config ERROR] fail to set char type \n"); 
		return -1;
	}

	return 0;
}

int ttsd_config_get_bool_type(const char* key, bool* value)
{
	if (NULL == key || NULL == value) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Config ERROR] Input parameter is NULL\n");
		return TTSD_ERROR_INVALID_PARAMETER;
	} 

	int result ;
	if (0 != vconf_get_int(key, &result)) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Config ERROR] fail to get bool type config : key(%s)\n", key);
		return -1;
	}

	*value = (bool) result;

	return 0;
}

int ttsd_config_set_bool_type(const char* key, const bool value)
{
	if (NULL == key) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Config ERROR] Input parameter is NULL\n");
		return TTSD_ERROR_INVALID_PARAMETER;
	} 

	int result = (int)value;
	if (0 != vconf_set_int(key, result)) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Config ERROR] fail to set bool type config : key(%s)\n", key);
		return -1;
	}

	return 0;
}

int ttsd_config_get_int_type(const char* key, int* value)
{
	if (NULL == key || NULL == value) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Config ERROR] Input parameter is NULL\n");
		return TTSD_ERROR_INVALID_PARAMETER;
	} 

	if (0 != vconf_get_int(key, value)) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Config ERROR] fail to get bool type config : key(%s)\n", key);
		return -1;
	}

	return 0;
}

int ttsd_config_set_int_type(const char* key, const int value)
{
	if (NULL == key) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Config ERROR] Input parameter is NULL\n");
		return TTSD_ERROR_INVALID_PARAMETER;
	} 

	if (0 != vconf_set_int(key, value)) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Config ERROR] fail to set int type config : key(%s)\n", key);
		return -1;
	}

	return 0;
}

/*
* interface for engine plug-in
*/

int config_make_key_for_engine(const char* engine_id, const char* key, char** out_key)
{
	int key_size = strlen(TTSD_CONFIG_PREFIX) + strlen(engine_id) + strlen(key) + 2; /* 2 is '/' and '\0' */

	*out_key = (char*) g_malloc0( sizeof(char) * key_size);

	if (*out_key == NULL) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Config ERROR] Not enough memory!! \n");
		return -1;
	} else {
		snprintf(*out_key, key_size, "%s%s/%s", TTSD_CONFIG_PREFIX, engine_id, key );
		SLOG(LOG_DEBUG, TAG_TTSD, "[Config DEBUG] make key (%s) \n", *out_key);
	}

	return 0;
}

int ttsd_config_set_persistent_data(const char* engine_id, const char* key, const char* value)
{
	char* vconf_key = NULL;

	if (0 != config_make_key_for_engine(engine_id, key, &vconf_key)) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Config ERROR] fail config_make_key_for_engine()\n"); 
		return -1;
	}

	if (0 != vconf_set_str(vconf_key, value)) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Config ERROR] fail to set key, value\n");
		
		if(vconf_key != NULL)	
			g_free(vconf_key);

		return -1;
	}

	SLOG(LOG_DEBUG, TAG_TTSD, "[Config DEBUG] Set data : key(%s), value(%s) \n", vconf_key, value);

	if (vconf_key != NULL)	
		g_free(vconf_key);

	return 0;
}

int ttsd_config_get_persistent_data(const char* engine_id, const char* key, char** value)
{
	char* vconf_key = NULL;

	if (0 != config_make_key_for_engine(engine_id, key, &vconf_key)) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Config ERROR] fail config_make_key_for_engine()\n");
		return -1;
	}

	char* temp;
	temp = vconf_get_str(vconf_key);
	if (temp == NULL) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Config ERROR] fail to get value\n");

		if(vconf_key != NULL)	
			g_free(vconf_key);

		return -1;
	}

	*value = g_strdup(temp);

	SLOG(LOG_DEBUG, TAG_TTSD, "[Config DEBUG] Get data : key(%s), value(%s) \n", vconf_key, *value);

	if (NULL != vconf_key)	
		g_free(vconf_key);

	if (NULL != temp)		
		g_free(temp);

	return 0;
}

int ttsd_config_remove_persistent_data(const char* engine_id, const char* key)
{
	char* vconf_key = NULL;
	int result = 0;

	if (0 != config_make_key_for_engine(engine_id, key, &vconf_key)) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Config ERROR] fail config_make_key_for_engine()\n");
		return -1;
	}

	if( NULL == vconf_key )		
		return -1;

	if (0 != vconf_unset(vconf_key)) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Config ERROR] fail to remove key\n");
		result = -1;
	} else {
		SLOG(LOG_DEBUG, TAG_TTSD, "[Config DEBUG] Remove data : key(%s)", vconf_key);
	}

	if( vconf_key != NULL )	
		g_free(vconf_key);

	return result;
}
