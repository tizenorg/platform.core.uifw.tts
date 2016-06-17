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

#include <dlog.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vconf.h>

#include "tts_config_parser.h"
#include "tts_defs.h"


#define TTS_TAG_ENGINE_BASE_TAG		"tts-engine"
#define TTS_TAG_ENGINE_NAME		"name"
#define TTS_TAG_ENGINE_ID		"id"
#define TTS_TAG_ENGINE_SETTING		"setting"
#define TTS_TAG_ENGINE_VOICE_SET	"voices"
#define TTS_TAG_ENGINE_VOICE		"voice"
#define TTS_TAG_ENGINE_VOICE_TYPE	"type"
#define TTS_TAG_ENGINE_PITCH_SUPPORT	"pitch-support"

#define TTS_TAG_CONFIG_BASE_TAG		"tts-config"
#define TTS_TAG_CONFIG_ENGINE_ID	"engine"
#define TTS_TAG_CONFIG_ENGINE_SETTING	"engine-setting"
#define TTS_TAG_CONFIG_AUTO_VOICE	"auto"
#define TTS_TAG_CONFIG_VOICE_TYPE	"voice-type"
#define TTS_TAG_CONFIG_LANGUAGE		"language"
#define TTS_TAG_CONFIG_SPEECH_RATE	"speech-rate"
#define TTS_TAG_CONFIG_PITCH		"pitch"
#define TTS_TAG_VOICE_TYPE_FEMALE	"female"
#define TTS_TAG_VOICE_TYPE_MALE		"male"
#define TTS_TAG_VOICE_TYPE_CHILD	"child"


extern char* tts_tag();

static xmlDocPtr g_config_doc = NULL;

int tts_parser_get_engine_info(const char* path, tts_engine_info_s** engine_info)
{
	if (NULL == path || NULL == engine_info) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Input parameter is NULL");
		return -1;
	}

	xmlDocPtr doc = NULL;
	xmlNodePtr cur = NULL;
	xmlChar *key = NULL;
	xmlChar *attr = NULL;

	doc = xmlParseFile(path);
	if (doc == NULL) {
		return -1;
	}

	cur = xmlDocGetRootElement(doc);
	if (cur == NULL) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Empty document");
		xmlFreeDoc(doc);
		return -1;
	}

	if (xmlStrcmp(cur->name, (const xmlChar *)TTS_TAG_ENGINE_BASE_TAG)) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] The wrong type, root node is NOT 'tts-engine'");
		xmlFreeDoc(doc);
		return -1;
	}

	cur = cur->xmlChildrenNode;
	if (cur == NULL) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Empty document");
		xmlFreeDoc(doc);
		return -1;
	}

	/* alloc engine info */
	tts_engine_info_s* temp;
	temp = (tts_engine_info_s*)calloc(1, sizeof(tts_engine_info_s));
	if (NULL == temp) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Out of memory");
		xmlFreeDoc(doc);
		return -1;
	}

	temp->name = NULL;
	temp->uuid = NULL;
	temp->voices = NULL;
	temp->setting = NULL;
	temp->pitch_support = false;

	while (cur != NULL) {
		if (0 == xmlStrcmp(cur->name, (const xmlChar *)TTS_TAG_ENGINE_NAME)) {
			key = xmlNodeGetContent(cur);
			if (NULL != key) {
				if (NULL != temp->name)	free(temp->name);
				temp->name = strdup((char*)key);
				xmlFree(key);
			} else {
				SLOG(LOG_ERROR, tts_tag(), "[ERROR] <%s> has no content", TTS_TAG_ENGINE_NAME);
			}
		} else if (0 == xmlStrcmp(cur->name, (const xmlChar *)TTS_TAG_ENGINE_ID)) {
			key = xmlNodeGetContent(cur);
			if (NULL != key) {
				if (NULL != temp->uuid)	free(temp->uuid);
				temp->uuid = strdup((char*)key);
				xmlFree(key);
			} else {
				SLOG(LOG_ERROR, tts_tag(), "[ERROR] <%s> has no content", TTS_TAG_ENGINE_ID);
			}
		} else if (0 == xmlStrcmp(cur->name, (const xmlChar *)TTS_TAG_ENGINE_SETTING)) {
			key = xmlNodeGetContent(cur);
			if (NULL != key) {
				if (NULL != temp->setting)	free(temp->setting);
				temp->setting = strdup((char*)key);
				xmlFree(key);
			} else {
				SLOG(LOG_ERROR, tts_tag(), "[ERROR] <%s> has no content", TTS_TAG_ENGINE_SETTING);
			}
		} else if (0 == xmlStrcmp(cur->name, (const xmlChar *)TTS_TAG_ENGINE_VOICE_SET)) {
			xmlNodePtr voice_node = NULL;
			voice_node = cur->xmlChildrenNode;

			while (NULL != voice_node) {
				if (0 == xmlStrcmp(voice_node->name, (const xmlChar *)TTS_TAG_ENGINE_VOICE)) {

					tts_config_voice_s* temp_voice = (tts_config_voice_s*)calloc(1, sizeof(tts_config_voice_s));
					if (NULL == temp_voice) {
						SLOG(LOG_ERROR, tts_tag(), "[ERROR] Out of memory");
						break;
					}

					attr = xmlGetProp(voice_node, (const xmlChar*)TTS_TAG_ENGINE_VOICE_TYPE);
					if (NULL != attr) {
						if (0 == xmlStrcmp(attr, (const xmlChar *)TTS_TAG_VOICE_TYPE_FEMALE)) {
							temp_voice->type = (int)TTS_CONFIG_VOICE_TYPE_FEMALE;
						} else if (0 == xmlStrcmp(attr, (const xmlChar *)TTS_TAG_VOICE_TYPE_MALE)) {
							temp_voice->type = (int)TTS_CONFIG_VOICE_TYPE_MALE;
						} else if (0 == xmlStrcmp(attr, (const xmlChar *)TTS_TAG_VOICE_TYPE_CHILD)) {
							temp_voice->type = (int)TTS_CONFIG_VOICE_TYPE_CHILD;
						} else {
							temp_voice->type = (int)TTS_CONFIG_VOICE_TYPE_USER_DEFINED;
						}
						xmlFree(attr);
					} else {
						SLOG(LOG_ERROR, tts_tag(), "[ERROR] <%s> has no content", TTS_TAG_ENGINE_VOICE_TYPE);
					}

					key = xmlNodeGetContent(voice_node);
					if (NULL != key) {
						if (NULL != temp_voice->language)	free(temp_voice->language);
						temp_voice->language = strdup((char*)key);
						xmlFree(key);
						temp->voices = g_slist_append(temp->voices, temp_voice);
					} else {
						SLOG(LOG_ERROR, tts_tag(), "[ERROR] <%s> has no content", TTS_TAG_ENGINE_VOICE);
						if (NULL != temp_voice) {
							free(temp_voice);
						}
					}
				}
				voice_node = voice_node->next;
			}
		} else if (0 == xmlStrcmp(cur->name, (const xmlChar *)TTS_TAG_ENGINE_PITCH_SUPPORT)) {
			key = xmlNodeGetContent(cur);
			if (NULL != key) {
				if (0 == xmlStrcmp(key, (const xmlChar *)"true")) {
					temp->pitch_support = true;
				}
				xmlFree(key);
			} else {
				SLOG(LOG_ERROR, tts_tag(), "[ERROR] <%s> has no content", TTS_TAG_ENGINE_PITCH_SUPPORT);
			}
		}
		cur = cur->next;
	}

	xmlFreeDoc(doc);

	if (NULL == temp->name || NULL == temp->uuid) {
		/* Invalid engine */
		SECURE_SLOG(LOG_ERROR, tts_tag(), "[ERROR] Invalid engine : %s", path);
		tts_parser_free_engine_info(temp);
		return -1;
	}

	*engine_info = temp;

	return 0;
}

int tts_parser_free_engine_info(tts_engine_info_s* engine_info)
{
	if (NULL == engine_info) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Input parameter is NULL");
		return -1;
	}

	if (NULL != engine_info->name)		free(engine_info->name);
	if (NULL != engine_info->uuid)		free(engine_info->uuid);
	if (NULL != engine_info->setting)	free(engine_info->setting);

	tts_config_voice_s *temp_voice;
	temp_voice = g_slist_nth_data(engine_info->voices, 0);

	while (NULL != temp_voice) {
		if (NULL != temp_voice) {
			if (NULL != temp_voice->language)	free(temp_voice->language);
			engine_info->voices = g_slist_remove(engine_info->voices, temp_voice);
			free(temp_voice);
		}

		temp_voice = g_slist_nth_data(engine_info->voices, 0);
	}

	if (NULL != engine_info)	free(engine_info);

	return 0;	
}

int tts_parser_print_engine_info(tts_engine_info_s* engine_info)
{
	if (NULL == engine_info) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Input parameter is NULL");
		return -1;
	}

	SLOG(LOG_DEBUG, tts_tag(), "== get engine info ==");
	SLOG(LOG_DEBUG, tts_tag(), " name : %s", engine_info->name);
	SLOG(LOG_DEBUG, tts_tag(), " id   : %s", engine_info->uuid);
	if (NULL != engine_info->setting)
		SLOG(LOG_DEBUG, tts_tag(), " setting : %s", engine_info->setting);

	SLOG(LOG_DEBUG, tts_tag(), " voices");
	GSList *iter = NULL;
	tts_config_voice_s *temp_voice;

	if (g_slist_length(engine_info->voices) > 0) {
		/* Get a first item */
		iter = g_slist_nth(engine_info->voices, 0);

		int i = 1;	
		while (NULL != iter) {
			/*Get handle data from list*/
			temp_voice = iter->data;

			SLOG(LOG_DEBUG, tts_tag(), "  [%dth] type(%d) lang(%s)", 
				i, temp_voice->type, temp_voice->language);

			/*Get next item*/
			iter = g_slist_next(iter);
			i++;
		}
	} else {
		SLOG(LOG_ERROR, tts_tag(), "  Voice is NONE");
	}

	SLOG(LOG_DEBUG, tts_tag(), "=====================");

	return 0;
}

int tts_parser_load_config(tts_config_s** config_info)
{
	if (NULL == config_info) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Input parameter is NULL");
		return -1;
	}

	xmlDocPtr doc = NULL;
	xmlNodePtr cur = NULL;
	xmlChar *key;
	bool is_default_open = false;

	if (0 != access(TTS_CONFIG, F_OK)) {
		doc = xmlParseFile(TTS_DEFAULT_CONFIG);
		if (doc == NULL) {
			SLOG(LOG_ERROR, tts_tag(), "[ERROR] Fail to parse file error : %s", TTS_DEFAULT_CONFIG);
			return -1;
		}
		is_default_open = true;
	} else {
		int retry_count = 0;

		while (NULL == doc) {
			doc = xmlParseFile(TTS_CONFIG);
			if (NULL != doc) {
				break;
			}
			retry_count++;
			usleep(10000);

			if (TTS_RETRY_COUNT == retry_count) {
				SLOG(LOG_ERROR, tts_tag(), "[ERROR] Fail to parse file error : %s", TTS_CONFIG);
				return -1;
			}
		}
	}

	cur = xmlDocGetRootElement(doc);
	if (cur == NULL) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Empty document");
		xmlFreeDoc(doc);
		return -1;
	}

	if (xmlStrcmp(cur->name, (const xmlChar *) TTS_TAG_CONFIG_BASE_TAG)) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] The wrong type, root node is NOT %s", TTS_TAG_CONFIG_BASE_TAG);
		xmlFreeDoc(doc);
		return -1;
	}

	cur = cur->xmlChildrenNode;
	if (cur == NULL) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Empty document");
		xmlFreeDoc(doc);
		return -1;
	}

	/* alloc engine info */
	tts_config_s* temp;
	temp = (tts_config_s*)calloc(1, sizeof(tts_config_s));
	if (NULL == temp) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Out of memory");
		xmlFreeDoc(doc);
		return -1;
	}

	temp->engine_id = NULL;
	temp->setting = NULL;
	temp->language = NULL;

	while (cur != NULL) {
		if (0 == xmlStrcmp(cur->name, (const xmlChar *)TTS_TAG_CONFIG_ENGINE_ID)) {
			key = xmlNodeGetContent(cur);
			if (NULL != key) {
				if (NULL != temp->engine_id)	free(temp->engine_id);
				temp->engine_id = strdup((char*)key);
				xmlFree(key);
			} else {
				SLOG(LOG_ERROR, tts_tag(), "[ERROR] engine id is NULL");
			}
		} else if (0 == xmlStrcmp(cur->name, (const xmlChar *)TTS_TAG_CONFIG_ENGINE_SETTING)) {
			key = xmlNodeGetContent(cur);
			if (NULL != key) {
				if (NULL != temp->setting)	free(temp->setting);
				temp->setting = strdup((char*)key);
				xmlFree(key);
			} else {
				SLOG(LOG_ERROR, tts_tag(), "[ERROR] setting path is NULL");
			}
			
		} else if (0 == xmlStrcmp(cur->name, (const xmlChar *)TTS_TAG_CONFIG_AUTO_VOICE)) {
			key = xmlNodeGetContent(cur);
			if (NULL != key) {
				if (0 == xmlStrcmp(key, (const xmlChar *)"on")) {
					temp->auto_voice = true;
				} else if (0 == xmlStrcmp(key, (const xmlChar *)"off")) {
					temp->auto_voice = false;
				} else {
					SLOG(LOG_ERROR, tts_tag(), "Auto voice is wrong");
					temp->auto_voice = true;
				}

				xmlFree(key);
			} else {
				SLOG(LOG_ERROR, tts_tag(), "[ERROR] voice type is NULL");
			}
		} else if (0 == xmlStrcmp(cur->name, (const xmlChar *)TTS_TAG_CONFIG_VOICE_TYPE)) {
			key = xmlNodeGetContent(cur);
			if (NULL != key) {
				if (0 == xmlStrcmp(key, (const xmlChar *)TTS_TAG_VOICE_TYPE_MALE)) {
					temp->type = (int)TTS_CONFIG_VOICE_TYPE_MALE;
				} else if (0 == xmlStrcmp(key, (const xmlChar *)TTS_TAG_VOICE_TYPE_FEMALE)) {
					temp->type = (int)TTS_CONFIG_VOICE_TYPE_FEMALE;
				} else if (0 == xmlStrcmp(key, (const xmlChar *)TTS_TAG_VOICE_TYPE_CHILD)) {
					temp->type = (int)TTS_CONFIG_VOICE_TYPE_CHILD;
				} else {
					SLOG(LOG_WARN, tts_tag(), "Voice type is user defined");
					temp->type = (int)TTS_CONFIG_VOICE_TYPE_USER_DEFINED;
				}

				xmlFree(key);
			} else {
				SLOG(LOG_ERROR, tts_tag(), "[ERROR] voice type is NULL");
			}
		} else if (0 == xmlStrcmp(cur->name, (const xmlChar *)TTS_TAG_CONFIG_LANGUAGE)) {
			key = xmlNodeGetContent(cur);
			if (NULL != key) {
				if (NULL != temp->language)	free(temp->language);
				temp->language = strdup((char*)key);
				xmlFree(key);
			} else {
				SLOG(LOG_ERROR, tts_tag(), "[ERROR] engine uuid is NULL");
			}

		} else if (0 == xmlStrcmp(cur->name, (const xmlChar *)TTS_TAG_CONFIG_SPEECH_RATE)) {
			key = xmlNodeGetContent(cur);
			if (NULL != key) {
				temp->speech_rate = atoi((char*)key);
				xmlFree(key);
			} else {
				SLOG(LOG_ERROR, tts_tag(), "[ERROR] speech rate is NULL");
			}
		} else if (0 == xmlStrcmp(cur->name, (const xmlChar *)TTS_TAG_CONFIG_PITCH)) {
			key = xmlNodeGetContent(cur);
			if (NULL != key) {
				temp->pitch = atoi((char*)key);
				xmlFree(key);
			} else {
				SLOG(LOG_ERROR, tts_tag(), "[ERROR] Pitch is NULL");
			}
		} else {

		}

		cur = cur->next;
	}

	*config_info = temp;
	g_config_doc = doc;

	if (true == is_default_open) {
		int ret = xmlSaveFile(TTS_CONFIG, g_config_doc);
		if (0 > ret) {
			SLOG(LOG_ERROR, tts_tag(), "[ERROR] Save result : %d", ret);
		}

		/* Set mode */
		if (0 > chmod(TTS_CONFIG, 0666)) {
			SLOG(LOG_ERROR, tts_tag(), "[ERROR] Fail to change file mode : %d", ret);
		}

		/* Set owner */
		if (0 > chown(TTS_CONFIG, 5000, 5000)) {
			SLOG(LOG_ERROR, tts_tag(), "[ERROR] Fail to change file owner : %d", ret);
		}
		SLOG(LOG_DEBUG, tts_tag(), "Default config is changed : pid(%d)", getpid());
	}

	return 0;
}

int tts_parser_unload_config(tts_config_s* config_info)
{
	if (NULL != g_config_doc)	xmlFreeDoc(g_config_doc);
	if (NULL != config_info) {
		if (NULL != config_info->engine_id)	free(config_info->engine_id);
		if (NULL != config_info->setting)	free(config_info->setting);
		if (NULL != config_info->language)	free(config_info->language);
		free(config_info);
	}

	return 0;
}


int tts_parser_copy_xml(const char* original, const char* destination)
{
	if (NULL == original || NULL == destination) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Input parameter is NULL");
		return -1;
	}

	xmlDocPtr doc = NULL;
	doc = xmlParseFile(original);
	if (doc == NULL) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Fail to parse file error : %s", original);
		return -1;
	}

	int ret = xmlSaveFile(destination, doc);
	if (0 > ret) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Save result : %d", ret);
	}

	/* Set mode */
	if (0 > chmod(destination, 0666)) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Fail to change file mode : %d", ret);
	}

	SLOG(LOG_DEBUG, tts_tag(), "[SUCCESS] Copying xml");

	return 0;
}

int tts_parser_set_engine(const char* engine_id, const char* setting, const char* language, int type)
{
	if (NULL == engine_id) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Input parameter is NULL");
		return -1;
	}

	xmlNodePtr cur = NULL;
	cur = xmlDocGetRootElement(g_config_doc);
	if (cur == NULL) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Empty document");
		return -1;
	}

	if (xmlStrcmp(cur->name, (const xmlChar *) TTS_TAG_CONFIG_BASE_TAG)) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] The wrong type, root node is NOT %s", TTS_TAG_CONFIG_BASE_TAG);
		return -1;
	}

	cur = cur->xmlChildrenNode;
	if (cur == NULL) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Empty document");
		return -1;
	}

	while (cur != NULL) {
		if (0 == xmlStrcmp(cur->name, (const xmlChar *)TTS_TAG_CONFIG_ENGINE_ID)) {
			xmlNodeSetContent(cur, (const xmlChar *)engine_id);
		}

		if (0 == xmlStrcmp(cur->name, (const xmlChar *)TTS_TAG_CONFIG_LANGUAGE)) {
			xmlNodeSetContent(cur, (const xmlChar *)language);
		}

		if (0 == xmlStrcmp(cur->name, (const xmlChar *)TTS_TAG_CONFIG_ENGINE_SETTING)) {
			xmlNodeSetContent(cur, (const xmlChar *)setting);
		}

		if (0 == xmlStrcmp(cur->name, (const xmlChar *)TTS_TAG_CONFIG_VOICE_TYPE)) {
			switch (type) {
			case TTS_CONFIG_VOICE_TYPE_MALE:	xmlNodeSetContent(cur, (const xmlChar*)TTS_TAG_VOICE_TYPE_MALE);	break;
			case TTS_CONFIG_VOICE_TYPE_FEMALE:	xmlNodeSetContent(cur, (const xmlChar*)TTS_TAG_VOICE_TYPE_FEMALE);	break;
			case TTS_CONFIG_VOICE_TYPE_CHILD:	xmlNodeSetContent(cur, (const xmlChar*)TTS_TAG_VOICE_TYPE_CHILD);	break;
			default:				xmlNodeSetContent(cur, (const xmlChar*)TTS_TAG_VOICE_TYPE_FEMALE);	break;
			}
		}
		
		cur = cur->next;
	}

	int ret = xmlSaveFile(TTS_CONFIG, g_config_doc);
	if (0 > ret) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Save result : %d", ret);
	}

	return 0;
}

int tts_parser_set_voice(const char* language, int type)
{
	if (NULL == language) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Input parameter is NULL");
		return -1;
	}

	xmlNodePtr cur = NULL;
	cur = xmlDocGetRootElement(g_config_doc);
	if (cur == NULL) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Empty document");
		return -1;
	}

	if (xmlStrcmp(cur->name, (const xmlChar *) TTS_TAG_CONFIG_BASE_TAG)) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] The wrong type, root node is NOT %s", TTS_TAG_CONFIG_BASE_TAG);
		return -1;
	}

	cur = cur->xmlChildrenNode;
	if (cur == NULL) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Empty document");
		return -1;
	}

	while (cur != NULL) {
		if (0 == xmlStrcmp(cur->name, (const xmlChar *)TTS_TAG_CONFIG_LANGUAGE)) {
			xmlNodeSetContent(cur, (const xmlChar *)language);
		} 

		if (0 == xmlStrcmp(cur->name, (const xmlChar *)TTS_TAG_CONFIG_VOICE_TYPE)) {
			switch (type) {
			case TTS_CONFIG_VOICE_TYPE_MALE:	xmlNodeSetContent(cur, (const xmlChar*)TTS_TAG_VOICE_TYPE_MALE);	break;
			case TTS_CONFIG_VOICE_TYPE_FEMALE:	xmlNodeSetContent(cur, (const xmlChar*)TTS_TAG_VOICE_TYPE_FEMALE);	break;
			case TTS_CONFIG_VOICE_TYPE_CHILD:	xmlNodeSetContent(cur, (const xmlChar*)TTS_TAG_VOICE_TYPE_CHILD);	break;
			default:
				SLOG(LOG_ERROR, tts_tag(), "[ERROR] Invalid type : %d", type);
				xmlNodeSetContent(cur, (const xmlChar*)TTS_TAG_VOICE_TYPE_FEMALE);
				break;
			}
		}

		cur = cur->next;
	}

	int ret = xmlSaveFile(TTS_CONFIG, g_config_doc);
	if (0 > ret) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Save result : %d", ret);
	}

	return 0;
}

int tts_parser_set_auto_voice(bool value)
{
	xmlNodePtr cur = NULL;
	cur = xmlDocGetRootElement(g_config_doc);
	if (cur == NULL) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Empty document");
		return -1;
	}

	if (xmlStrcmp(cur->name, (const xmlChar *) TTS_TAG_CONFIG_BASE_TAG)) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] The wrong type, root node is NOT %s", TTS_TAG_CONFIG_BASE_TAG);
		return -1;
	}

	cur = cur->xmlChildrenNode;
	if (cur == NULL) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Empty document");
		return -1;
	}

	while (cur != NULL) {
		if (0 == xmlStrcmp(cur->name, (const xmlChar *)TTS_TAG_CONFIG_AUTO_VOICE)) {
			if (true == value) {
				xmlNodeSetContent(cur, (const xmlChar *)"on");
			} else if (false == value) {
				xmlNodeSetContent(cur, (const xmlChar *)"off");
			} else {
				SLOG(LOG_ERROR, tts_tag(), "[ERROR] The wrong value of auto voice");
				return -1;
			}
			break;
		}
		cur = cur->next;
	}

	int ret = xmlSaveFile(TTS_CONFIG, g_config_doc);
	if (0 > ret) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Save result : %d", ret);
	}

	return 0;
}

int tts_parser_set_speech_rate(int value)
{
	xmlNodePtr cur = NULL;
	cur = xmlDocGetRootElement(g_config_doc);
	if (cur == NULL) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Empty document");
		return -1;
	}

	if (xmlStrcmp(cur->name, (const xmlChar *) TTS_TAG_CONFIG_BASE_TAG)) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] The wrong type, root node is NOT %s", TTS_TAG_CONFIG_BASE_TAG);
		return -1;
	}

	cur = cur->xmlChildrenNode;
	if (cur == NULL) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Empty document");
		return -1;
	}

	while (cur != NULL) {
		if (0 == xmlStrcmp(cur->name, (const xmlChar *)TTS_TAG_CONFIG_SPEECH_RATE)) {
			char temp[10];
			memset(temp, '\0', 10);
			snprintf(temp, 10, "%d", value);

			xmlNodeSetContent(cur, (const xmlChar *)temp);

			SLOG(LOG_DEBUG, tts_tag(), "Set speech rate : %s", temp);
			break;
		}

		cur = cur->next;
	}

	int ret = xmlSaveFile(TTS_CONFIG, g_config_doc);
	if (0 > ret) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Save result : %d", ret);
	}

	return 0;
}

int tts_parser_set_pitch(int value)
{
	xmlNodePtr cur = NULL;
	cur = xmlDocGetRootElement(g_config_doc);
	if (cur == NULL) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Empty document");
		return -1;
	}

	if (xmlStrcmp(cur->name, (const xmlChar *) TTS_TAG_CONFIG_BASE_TAG)) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] The wrong type, root node is NOT %s", TTS_TAG_CONFIG_BASE_TAG);
		return -1;
	}

	cur = cur->xmlChildrenNode;
	if (cur == NULL) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Empty document");
		return -1;
	}

	while (cur != NULL) {
		if (0 == xmlStrcmp(cur->name, (const xmlChar *)TTS_TAG_CONFIG_PITCH)) {
			char temp[10];
			memset(temp, '\0', 10);
			snprintf(temp, 10, "%d", value);
			xmlNodeSetContent(cur, (const xmlChar *)temp);
			break;
		} 

		cur = cur->next;
	}

	int ret = xmlSaveFile(TTS_CONFIG, g_config_doc);
	if (0 > ret) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Save result : %d", ret);
	}

	return 0;
}

int tts_parser_find_config_changed(char** engine, char**setting, bool* auto_voice, char** language, int* voice_type, 
				   int* speech_rate, int* pitch)
{
	if (NULL == engine || NULL == setting || NULL == language || NULL == voice_type || NULL == speech_rate) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Input parameter is NULL");
		return -1;
	}

	xmlDocPtr doc = NULL;
	xmlNodePtr cur_new = NULL;
	xmlNodePtr cur_old = NULL;

	xmlChar *key_new;
	xmlChar *key_old;

	int retry_count = 0;
	while (NULL == doc) {
		doc = xmlParseFile(TTS_CONFIG);
		if (NULL != doc) {
			break;
		}
		retry_count++;
		usleep(10000);

		if (TTS_RETRY_COUNT == retry_count) {
			SLOG(LOG_ERROR, tts_tag(), "[ERROR] Fail to parse file error : %s", TTS_CONFIG);
			return -1;
		}
	}

	cur_new = xmlDocGetRootElement(doc);
	cur_old = xmlDocGetRootElement(g_config_doc);
	if (cur_new == NULL || cur_old == NULL) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Empty document");
		xmlFreeDoc(doc);
		return -1;
	}

	if (xmlStrcmp(cur_new->name, (const xmlChar*)TTS_TAG_CONFIG_BASE_TAG) || 
	xmlStrcmp(cur_old->name, (const xmlChar*)TTS_TAG_CONFIG_BASE_TAG)) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] The wrong type, root node is NOT %s", TTS_TAG_CONFIG_BASE_TAG);
		xmlFreeDoc(doc);
		return -1;
	}

	cur_new = cur_new->xmlChildrenNode;
	cur_old = cur_old->xmlChildrenNode;
	if (cur_new == NULL || cur_old == NULL) {
		SLOG(LOG_ERROR, tts_tag(), "[ERROR] Empty document");
		xmlFreeDoc(doc);
		return -1;
	}

	while (cur_new != NULL && cur_old != NULL) {
		if (0 == xmlStrcmp(cur_new->name, (const xmlChar*)TTS_TAG_CONFIG_ENGINE_ID)) {
			if (0 == xmlStrcmp(cur_old->name, (const xmlChar*)TTS_TAG_CONFIG_ENGINE_ID)) {
				key_old = xmlNodeGetContent(cur_old);
				if (NULL != key_old) {
					key_new = xmlNodeGetContent(cur_new);
					if (NULL != key_new) {
						if (0 != xmlStrcmp(key_old, key_new)) {
							SLOG(LOG_DEBUG, tts_tag(), "Old engine id(%s), New engine(%s)", 
								(char*)key_old, (char*)key_new);
							if (NULL != *engine)	free(*engine);
							*engine = strdup((char*)key_new);
						}
						xmlFree(key_new);
					}
					xmlFree(key_old);
				}
			} else {
				SLOG(LOG_ERROR, tts_tag(), "[ERROR] old config and new config are different");
			}
		} else if (0 == xmlStrcmp(cur_new->name, (const xmlChar*)TTS_TAG_CONFIG_ENGINE_SETTING)) {
			if (0 == xmlStrcmp(cur_old->name, (const xmlChar*)TTS_TAG_CONFIG_ENGINE_SETTING)) {
				key_old = xmlNodeGetContent(cur_old);
				if (NULL != key_old) {
					key_new = xmlNodeGetContent(cur_new);
					if (NULL != key_new) {
						if (0 != xmlStrcmp(key_old, key_new)) {
							SLOG(LOG_DEBUG, tts_tag(), "Old engine setting(%s), New engine setting(%s)", 
								(char*)key_old, (char*)key_new);
							if (NULL != *setting)	free(*setting);
							*setting = strdup((char*)key_new);
						}
						xmlFree(key_new);
					}
					xmlFree(key_old);
				}
			} else {
				SLOG(LOG_ERROR, tts_tag(), "[ERROR] old config and new config are different");
			}
		} else if (0 == xmlStrcmp(cur_new->name, (const xmlChar*)TTS_TAG_CONFIG_AUTO_VOICE)) {
			if (0 == xmlStrcmp(cur_old->name, (const xmlChar*)TTS_TAG_CONFIG_AUTO_VOICE)) {
				key_old = xmlNodeGetContent(cur_old);
				if (NULL != key_old) {
					key_new = xmlNodeGetContent(cur_new);
					if (NULL != key_new) {
						if (0 != xmlStrcmp(key_old, key_new)) {
							SLOG(LOG_DEBUG, tts_tag(), "Old auto voice (%s), New auto voice(%s)", 
								(char*)key_old, (char*)key_new);
							if (0 == xmlStrcmp((const xmlChar*)"on", key_new)) {
								*auto_voice = true;
							} else {
								*auto_voice = false;
							}
						}
						xmlFree(key_new);
					}
					xmlFree(key_old);
				}
			} else {
				SLOG(LOG_ERROR, tts_tag(), "[ERROR] old config and new config are different");
			}
		} else if (0 == xmlStrcmp(cur_new->name, (const xmlChar*)TTS_TAG_CONFIG_LANGUAGE)) {
			if (0 == xmlStrcmp(cur_old->name, (const xmlChar*)TTS_TAG_CONFIG_LANGUAGE)) {
				key_old = xmlNodeGetContent(cur_old);
				if (NULL != key_old) {
					key_new = xmlNodeGetContent(cur_new);
					if (NULL != key_new) {
						if (0 != xmlStrcmp(key_old, key_new)) {
							SLOG(LOG_DEBUG, tts_tag(), "Old language(%s), New language(%s)", 
								(char*)key_old, (char*)key_new);
							if (NULL != *language)	free(*language);
							*language = strdup((char*)key_new);
						}
						xmlFree(key_new);
					}
					xmlFree(key_old);
				}
			} else {
				SLOG(LOG_ERROR, tts_tag(), "[ERROR] old config and new config are different");
			}
		} else if (0 == xmlStrcmp(cur_new->name, (const xmlChar*)TTS_TAG_CONFIG_VOICE_TYPE)) {
			if (0 == xmlStrcmp(cur_old->name, (const xmlChar*)TTS_TAG_CONFIG_VOICE_TYPE)) {
				key_old = xmlNodeGetContent(cur_old);
				if (NULL != key_old) {
					key_new = xmlNodeGetContent(cur_new);
					if (NULL != key_new) {
						if (0 != xmlStrcmp(key_old, key_new)) {
							SLOG(LOG_DEBUG, tts_tag(), "Old voice type(%s), New voice type(%s)", 
								(char*)key_old, (char*)key_new);
							if (0 == xmlStrcmp(key_new, (const xmlChar *)TTS_TAG_VOICE_TYPE_FEMALE)) {
								*voice_type = (int)TTS_CONFIG_VOICE_TYPE_FEMALE;
							} else if (0 == xmlStrcmp(key_new, (const xmlChar *)TTS_TAG_VOICE_TYPE_MALE)) {
								*voice_type = (int)TTS_CONFIG_VOICE_TYPE_MALE;
							} else if (0 == xmlStrcmp(key_new, (const xmlChar *)TTS_TAG_VOICE_TYPE_CHILD)) {
								*voice_type = (int)TTS_CONFIG_VOICE_TYPE_CHILD;
							} else {
								SLOG(LOG_ERROR, tts_tag(), "[ERROR] New voice type is not valid");
							}
						}
						xmlFree(key_new);
					}
					xmlFree(key_old);
				}
			} else {
				SLOG(LOG_ERROR, tts_tag(), "[ERROR] old config and new config are different");
			}
		} else if (0 == xmlStrcmp(cur_new->name, (const xmlChar*)TTS_TAG_CONFIG_SPEECH_RATE)) {
			if (0 == xmlStrcmp(cur_old->name, (const xmlChar*)TTS_TAG_CONFIG_SPEECH_RATE)) {
				key_old = xmlNodeGetContent(cur_old);
				if (NULL != key_old) {
					key_new = xmlNodeGetContent(cur_new);
					if (NULL != key_new) {
						if (0 != xmlStrcmp(key_old, key_new)) {
							SLOG(LOG_DEBUG, tts_tag(), "Old speech rate(%s), New speech rate(%s)", 
								(char*)key_old, (char*)key_new);
							*speech_rate = atoi((char*)key_new);
						}
						xmlFree(key_new);
					}
					xmlFree(key_old);
				}
			} else {
				SLOG(LOG_ERROR, tts_tag(), "[ERROR] old config and new config are different");
			}
		} else if (0 == xmlStrcmp(cur_new->name, (const xmlChar*)TTS_TAG_CONFIG_PITCH)) {
			if (0 == xmlStrcmp(cur_old->name, (const xmlChar*)TTS_TAG_CONFIG_PITCH)) {
				key_old = xmlNodeGetContent(cur_old);
				if (NULL != key_old) {
					key_new = xmlNodeGetContent(cur_new);
					if (NULL != key_new) {
						if (0 != xmlStrcmp(key_old, key_new)) {
							SLOG(LOG_DEBUG, tts_tag(), "Old pitch(%s), New pitch(%s)", 
								(char*)key_old, (char*)key_new);
							*pitch = atoi((char*)key_new);
						}
						xmlFree(key_new);
					}
					xmlFree(key_old);
				}
			} else {
				SLOG(LOG_ERROR, tts_tag(), "[ERROR] old config and new config are different");
			}
		} else {

		}

		cur_new = cur_new->next;
		cur_old = cur_old->next;
	}
	
	xmlFreeDoc(g_config_doc);
	g_config_doc = doc;

	return 0;
}