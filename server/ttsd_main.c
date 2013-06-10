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


#include "ttsd_main.h"
#include "ttsd_server.h"
#include "ttsd_dbus.h"
#include "ttsd_network.h"

#include <Ecore.h>

#define CLIENT_CLEAN_UP_TIME 500

static Ecore_Timer* g_check_client_timer = NULL;

char* get_tag()
{
	return "ttsd";
}

ttsd_mode_e ttsd_get_mode()
{
	return TTSD_MODE_DEFAULT;
}

/* Main of TTS Daemon */
int main()
{
	SLOG(LOG_DEBUG, get_tag(), "  ");
	SLOG(LOG_DEBUG, get_tag(), "  ");
	SLOG(LOG_DEBUG, get_tag(), "===== TTS DAEMON DEFAULT INITIALIZE");
	if (!ecore_init()) {
		SLOG(LOG_ERROR, get_tag(), "[Main ERROR] Fail ecore_init()");
		return -1;
	}

	if (0 != ttsd_initialize()) {
		SLOG(LOG_ERROR, get_tag(), "[Main ERROR] Fail to initialize tts-daemon"); 
		return EXIT_FAILURE;
	}
	
	if (0 != ttsd_dbus_open_connection()) {
		SLOG(LOG_ERROR, get_tag(), "[Main ERROR] Fail to open dbus connection");
		return EXIT_FAILURE;
	}

	if (0 != ttsd_network_initialize()) {
		SLOG(LOG_ERROR, get_tag(), "[Main ERROR] Fail to initialize network");
		return EXIT_FAILURE;
	}

	g_check_client_timer = ecore_timer_add(CLIENT_CLEAN_UP_TIME, ttsd_cleanup_client, NULL);
	if (NULL == g_check_client_timer) {
		SLOG(LOG_WARN, get_tag(), "[Main Warning] Fail to create timer of client check");
	}

	SLOG(LOG_DEBUG, get_tag(), "[Main] tts-daemon start...\n"); 
	SLOG(LOG_DEBUG, get_tag(), "=====");
	SLOG(LOG_DEBUG, get_tag(), "  ");
	SLOG(LOG_DEBUG, get_tag(), "  ");
	
	ecore_main_loop_begin();

	SLOG(LOG_DEBUG, get_tag(), "===== TTS DAEMON DEFAULT FINALIZE");
	
	if (NULL != g_check_client_timer) {
		ecore_timer_del(g_check_client_timer);
	}

	ttsd_network_finalize();

	ttsd_dbus_close_connection();

	ttsd_finalize();

	ecore_shutdown();

	SLOG(LOG_DEBUG, get_tag(), "=====");
	SLOG(LOG_DEBUG, get_tag(), "  ");
	SLOG(LOG_DEBUG, get_tag(), "  ");

	return 0;
}

