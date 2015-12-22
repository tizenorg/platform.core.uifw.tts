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


#include "ttsd_main.h"
#include "ttsd_network.h"

#include <vconf.h>

int ttsd_network_initialize()
{
	return 0;
}

int ttsd_network_finalize()
{
	return 0;
}


bool ttsd_network_is_connected()
{
	/* Check network */
	int network_status = 0;
	vconf_get_int(VCONFKEY_NETWORK_STATUS, &network_status);

	if (network_status == VCONFKEY_NETWORK_OFF) {
		SLOG(LOG_WARN, get_tag(), "[Network] Current network connection is OFF.");
		return false;
	}

	SLOG(LOG_DEBUG, get_tag(), "[Network] Network status is %d", network_status);

	return true;
}