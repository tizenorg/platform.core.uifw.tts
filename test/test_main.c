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

#include <stdio.h>
#include <Ecore.h>

#include <tts.h>
#include <dlog.h>

#define TTS_STRDUP(src) 		((src != NULL) ? strdup(src) : NULL)

static tts_h g_tts;
static char* g_text = NULL;

const char* tts_tag()
{
	return "ttstest";
}

Eina_Bool __tts_test_destroy(void *data);

static bool __tts_test_get_text_from_file(const char* path, char** text)
{
	if (!path) return 0;
	if (!text) return 0;

	FILE *fp = NULL;

	if ((fp = fopen(path, "rb")) == NULL) {
		SLOG(LOG_ERROR, tts_tag(), "Fail to open file (%s)", path);
		return 0;
	}

	fseek(fp , 0 , SEEK_END);

	int text_len = ftell(fp);
	if (0 >= text_len) {
		SLOG(LOG_ERROR, tts_tag(), "File has no contents");
		fclose(fp);
		return 0;
	}
	SLOG(LOG_ERROR, tts_tag(), "text_len(%d)", text_len);
	rewind(fp);

	char* temp = NULL;
	temp = (char*)calloc(1, text_len+1);

	if (temp == NULL) {
		SLOG(LOG_ERROR, tts_tag(), "Fail to memory allocation");		
		fclose(fp);
		return 0;
	}

	int result_len = 1;
	while (!feof(fp)) {
		result_len = fread(temp, sizeof(char), text_len, fp);
		if (result_len != text_len) {
			SLOG(LOG_ERROR, tts_tag(), "Fail to read : result(%d) text_len(%d)", result_len, text_len);
			fclose(fp);
			if (NULL != temp) {
				free(temp);
				temp = NULL;
			}
			return 0;
		}
	}

	temp[result_len] = '\0';

	text = &temp;

	fclose(fp);
	return 1;
}

Eina_Bool __tts_test_resume(void *data)
{
	int ret = tts_play(g_tts);
	if (TTS_ERROR_NONE != ret) {
		SLOG(LOG_ERROR, tts_tag(), "Fail to resume");
		ecore_timer_add(0, __tts_test_destroy, NULL);
		return EINA_FALSE;
	}

	return EINA_FALSE;
}

Eina_Bool __tts_test_pause(void *data)
{
	int ret = tts_pause(g_tts);
	if (TTS_ERROR_NONE != ret) {
		SLOG(LOG_ERROR, tts_tag(), "Fail to pause");
		ecore_timer_add(0, __tts_test_destroy, NULL);
		return EINA_FALSE;
	}

	ecore_timer_add(3, __tts_test_resume, data);

	return EINA_FALSE;
}

Eina_Bool __tts_test_play(void *data)
{
	int utt_id;
	int ret;
	char* lang = NULL;

	lang = (char*)data;

	ret = tts_add_text(g_tts, g_text, lang, TTS_VOICE_TYPE_AUTO, TTS_SPEED_AUTO, &utt_id);
	if (TTS_ERROR_NONE != ret) {
		SLOG(LOG_ERROR, tts_tag(), "Fail to add text");
		ecore_timer_add(0, __tts_test_destroy, NULL);
		return EINA_FALSE;
	}

	SLOG(LOG_ERROR, tts_tag(), "Play : utt id(%d)", utt_id);
	ret = tts_play(g_tts);
	if (TTS_ERROR_NONE != ret) {
		SLOG(LOG_ERROR, tts_tag(), "Fail to play");
		ecore_timer_add(0, __tts_test_destroy, NULL);
		return EINA_FALSE;
	}

//	ecore_timer_add(1, __tts_test_pause, data);

	return EINA_FALSE;
}

Eina_Bool __tts_test_destroy(void *data)
{
	int ret;
	SLOG(LOG_ERROR, tts_tag(), "Stop");
	ret = tts_stop(g_tts);
	if (TTS_ERROR_NONE != ret) {
		SLOG(LOG_ERROR, tts_tag(), "Fail to stop");
	}

	SLOG(LOG_ERROR, tts_tag(), "Unprepare (Disconnection)");
	ret = tts_unprepare(g_tts);
	if (TTS_ERROR_NONE != ret) {
		SLOG(LOG_ERROR, tts_tag(), "Fail to unprepare");
	}

	SLOG(LOG_ERROR, tts_tag(), "Destory tts client");
	ret = tts_destroy(g_tts);
	if (TTS_ERROR_NONE != ret) {
		SLOG(LOG_ERROR, tts_tag(), "Fail to destroy");
	}

	ecore_main_loop_quit();

	return EINA_FALSE;
}

static void __tts_test_state_changed_cb(tts_h tts, tts_state_e previous, tts_state_e current, void* user_data)
{
	if (TTS_STATE_CREATED == previous && TTS_STATE_READY == current) {
		SLOG(LOG_ERROR, tts_tag(), "State is ready after prepare");
		ecore_timer_add(0, __tts_test_play, user_data);
	}

	return;
}

static void __tts_test_utt_started_cb(tts_h tts, int utt_id, void* user_data)
{
	SLOG(LOG_DEBUG, tts_tag(), "Utterance started : utt id(%d)", utt_id);

	return;
}

static void __tts_test_utt_completed_cb(tts_h tts, int utt_id, void* user_data)
{
	SLOG(LOG_DEBUG, tts_tag(), "Utterance completed : utt id(%d)", utt_id);
	ecore_timer_add(0, __tts_test_destroy, NULL);

	return;
}

int main(int argc, char *argv[])
{
	if (1 == argc || 5 < argc) {
		SLOG(LOG_DEBUG, tts_tag(), "Please check parameter");
		SLOG(LOG_DEBUG, tts_tag(), "Ex> tts-test -t 'text'");
		return 0;
	}

	char* lang = NULL;
	char* src_path = NULL;

	int n = 0;

	while (NULL != argv[n]) {

		if (!strcmp("-h", argv[n])) {
			SLOG(LOG_DEBUG, tts_tag(), " ==========================================");
			SLOG(LOG_DEBUG, tts_tag(), "  TTS test usage");
			SLOG(LOG_DEBUG, tts_tag(), " ==========================================");
			SLOG(LOG_DEBUG, tts_tag(), "  -t : Synthesize text");
			SLOG(LOG_DEBUG, tts_tag(), "  -l : Determine langage to synthesize text, ex) en_US, ko_KR ...");
			SLOG(LOG_DEBUG, tts_tag(), "  -f : Determine file path which include text");
			SLOG(LOG_DEBUG, tts_tag(), " ***************************************************");
			SLOG(LOG_DEBUG, tts_tag(), "    Example : #tts-test -l en_US -t \"1 2 3 4\" ");
			SLOG(LOG_DEBUG, tts_tag(), " ***************************************************");
			return 0;
		}

		/* check langage option */
		if (!strcmp("-l", argv[n])) {
			lang = TTS_STRDUP(argv[n+1]);
			SLOG(LOG_ERROR, tts_tag(), "Language : %s", lang);
		}
		/* check text to synthesize */
		else if (!strcmp("-t", argv[n])) {
			g_text = TTS_STRDUP(argv[n+1]);
			SLOG(LOG_ERROR, tts_tag(), "Text : %s", g_text);
		}
		/* check file path to synthesize */
		else if (!strcmp("-f", argv[n])) {
			src_path = TTS_STRDUP(argv[n+1]);
			SLOG(LOG_ERROR, tts_tag(), "File path : %s", src_path);
			if (!__tts_test_get_text_from_file(src_path, &g_text)) {
				return 0;
			}
		}
		n++;
	}

	if (!g_text && !src_path) {
		SLOG(LOG_ERROR, tts_tag(), "Invalid parameter, check help with command tts-test -h");
		return 0;
	}

	/*=================================== */

	tts_mode_e mode = TTS_MODE_DEFAULT;

	SLOG(LOG_DEBUG, tts_tag(), "  ");
	SLOG(LOG_DEBUG, tts_tag(), "  ");
	SLOG(LOG_DEBUG, tts_tag(), "===== TTS Sample start =====");

	SLOG(LOG_DEBUG, tts_tag(), "Input text : %s", g_text ? g_text : "NULL");
	SLOG(LOG_DEBUG, tts_tag(), "Input lang : %s", lang ? lang : "NULL");
	SLOG(LOG_DEBUG, tts_tag(), "Input file path : %s", src_path ? src_path : "NULL");

	if (!ecore_init()) {
		SLOG(LOG_ERROR, tts_tag(), "[Main ERROR] Fail ecore_init()");
		return 0;
	}

	int ret;

	SLOG(LOG_DEBUG, tts_tag(), "Create tts client");
	ret = tts_create(&g_tts);
	if (TTS_ERROR_NONE != ret) {
		SLOG(LOG_ERROR, tts_tag(), "Fail to create");
		return 0;
	}

	SLOG(LOG_DEBUG, tts_tag(), "Set tts mode - %d", mode);
	ret = tts_set_mode(g_tts, mode);
	if (TTS_ERROR_NONE != ret) {
		SLOG(LOG_ERROR, tts_tag(), "Fail to set mode");
		tts_destroy(g_tts);
		return 0;
	}

	SLOG(LOG_DEBUG, tts_tag(), "Set Callback func");
	ret = tts_set_state_changed_cb(g_tts, __tts_test_state_changed_cb, (void*)lang);
	if (TTS_ERROR_NONE != ret) {
		SLOG(LOG_ERROR, tts_tag(), "Fail to set state changed cb");
		tts_destroy(g_tts);
		return 0;
	}

	ret = tts_set_utterance_started_cb(g_tts, __tts_test_utt_started_cb, NULL);
	if (TTS_ERROR_NONE != ret) {
		SLOG(LOG_ERROR, tts_tag(), "Fail to set utt started cb");
		tts_destroy(g_tts);
		return 0;
	}

	ret = tts_set_utterance_completed_cb(g_tts, __tts_test_utt_completed_cb, NULL);
	if (TTS_ERROR_NONE != ret) {
		SLOG(LOG_ERROR, tts_tag(), "Fail to set utt completed cb");
		tts_destroy(g_tts);
		return 0;
	}

	SLOG(LOG_DEBUG, tts_tag(), "Prepare (Daemon Connection) asynchronously : Wait for ready state");
	ret = tts_prepare(g_tts);
	if (TTS_ERROR_NONE != ret) {
		SLOG(LOG_ERROR, tts_tag(), "Fail to prepare");
		tts_destroy(g_tts);
		return 0;
	}

	ecore_main_loop_begin();

	ecore_shutdown();

	if (src_path) free(src_path);
	if (lang) free(lang);
	if (g_text) free(g_text);

	SLOG(LOG_DEBUG, tts_tag(), "===== TTS END =====");

	return 0;
}
