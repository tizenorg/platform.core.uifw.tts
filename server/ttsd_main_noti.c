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

char* get_tag()
{
	return "ttsdnoti";
}

ttsd_mode_e ttsd_get_mode()
{
	return TTSD_MODE_NOTIFICATION;
}

/* Main of TTS Daemon */
int main()
{
	SLOG(LOG_DEBUG, get_tag(), "  ");
	SLOG(LOG_DEBUG, get_tag(), "  ");
	SLOG(LOG_DEBUG, get_tag(), "===== TTS DAEMON NOTI INITIALIZE");
	if (!ecore_init()) {
		SLOG(LOG_ERROR, get_tag(), "[Main ERROR] fail ecore_init()");
		return -1;
	}

	if (0 != ttsd_initialize()) {
		printf("Fail to initialize tts-daemon-noti \n");
		SLOG(LOG_ERROR, get_tag(), "[Main ERROR] fail to initialize tts-daemon-noti"); 
		return EXIT_FAILURE;
	}

	if (0 != ttsd_dbus_open_connection()) {
		printf("Fail to initialize IPC connection \n");
		SLOG(LOG_ERROR, get_tag(), "[Main ERROR] fail to open dbus connection");
		return EXIT_FAILURE;
	}

	if (0 != ttsd_network_initialize()) {
		SLOG(LOG_ERROR, get_tag(), "[Main ERROR] fail to initialize network");
		return EXIT_FAILURE;
	}

	ecore_timer_add(CLIENT_CLEAN_UP_TIME, ttsd_cleanup_client, NULL);

	SLOG(LOG_DEBUG, get_tag(), "[Main] start tts-daemon-noti start..."); 
	SLOG(LOG_DEBUG, get_tag(), "=====");
	SLOG(LOG_DEBUG, get_tag(), "  ");
	SLOG(LOG_DEBUG, get_tag(), "  ");

	printf("Start tts-daemon-noti ...\n");
	
	ecore_main_loop_begin();

	SLOG(LOG_DEBUG, get_tag(), "===== TTS DAEMON NOTI FINALIZE");

	ttsd_network_finalize();

	ttsd_dbus_close_connection();

	ttsd_finalize();

	ecore_shutdown();

	SLOG(LOG_DEBUG, get_tag(), "=====");
	SLOG(LOG_DEBUG, get_tag(), "  ");
	SLOG(LOG_DEBUG, get_tag(), "  ");

	return 0;
}

