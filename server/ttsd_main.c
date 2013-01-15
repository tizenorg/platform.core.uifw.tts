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

/* Main of TTS Daemon */
int main()
{
	SLOG(LOG_DEBUG, TAG_TTSD, "  ");
	SLOG(LOG_DEBUG, TAG_TTSD, "  ");
	SLOG(LOG_DEBUG, TAG_TTSD, "===== TTS DAEMON INITIALIZE");
	if (!ecore_init()) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Main ERROR] fail ecore_init() \n");
		return -1;
	}

	if (0 != ttsd_initialize()) {
		printf("Fail to initialize tts-daemon \n");
		SLOG(LOG_ERROR, TAG_TTSD, "[Main ERROR] fail to initialize tts-daemon \n"); 
		return EXIT_FAILURE;
	}
	
	if (0 != ttsd_dbus_open_connection()) {
		printf("Fail to initialize IPC connection \n");
		SLOG(LOG_ERROR, TAG_TTSD, "[Main ERROR] fail to open dbus connection \n");
		return EXIT_FAILURE;
	}

	if (0 != ttsd_network_initialize()) {
		SLOG(LOG_ERROR, TAG_TTSD, "[Main ERROR] fail to initialize network \n");
		return EXIT_FAILURE;
	}

	ecore_timer_add(CLIENT_CLEAN_UP_TIME, ttsd_cleanup_client, NULL);

	SLOG(LOG_DEBUG, TAG_TTSD, "[Main] tts-daemon start...\n"); 
	SLOG(LOG_DEBUG, TAG_TTSD, "=====");
	SLOG(LOG_DEBUG, TAG_TTSD, "  ");
	SLOG(LOG_DEBUG, TAG_TTSD, "  ");

	printf("TTS-Daemon Start...\n");
	
	ecore_main_loop_begin();
	
	ecore_shutdown();

	ttsd_dbus_close_connection();

	ttsd_network_finalize();

	return 0;
}

