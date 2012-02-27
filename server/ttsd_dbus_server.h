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


#ifndef __TTSD_DBUS_SERVER_h__
#define __TTSD_DBUS_SERVER_h__

#include <dbus/dbus.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
* Dbus Server functions for APIs
*/ 

int ttsd_dbus_server_initialize(DBusConnection* conn, DBusMessage* msg);

int ttsd_dbus_server_finalize(DBusConnection* conn, DBusMessage* msg);

int ttsd_dbus_server_get_support_voices(DBusConnection* conn, DBusMessage* msg);

int ttsd_dbus_server_get_current_voice(DBusConnection* conn, DBusMessage* msg);

int ttsd_dbus_server_add_text(DBusConnection* conn, DBusMessage* msg);

int ttsd_dbus_server_play(DBusConnection* conn, DBusMessage* msg);

int ttsd_dbus_server_stop(DBusConnection* conn, DBusMessage* msg);

int ttsd_dbus_server_pause(DBusConnection* conn, DBusMessage* msg);


/*
* Dbus Server functions for Setting
*/ 

int ttsd_dbus_server_setting_initialize(DBusConnection* conn, DBusMessage* msg);

int ttsd_dbus_server_setting_finalize(DBusConnection* conn, DBusMessage* msg);

int ttsd_dbus_server_setting_get_engine_list(DBusConnection* conn, DBusMessage* msg);

int ttsd_dbus_server_setting_get_engine(DBusConnection* conn, DBusMessage* msg);

int ttsd_dbus_server_setting_set_engine(DBusConnection* conn, DBusMessage* msg);

int ttsd_dbus_server_setting_get_voice_list(DBusConnection* conn, DBusMessage* msg);

int ttsd_dbus_server_setting_get_default_voice(DBusConnection* conn, DBusMessage* msg);

int ttsd_dbus_server_setting_set_default_voice(DBusConnection* conn, DBusMessage* msg);

int ttsd_dbus_server_setting_get_speed(DBusConnection* conn, DBusMessage* msg);

int ttsd_dbus_server_setting_set_speed(DBusConnection* conn, DBusMessage* msg);

int ttsd_dbus_server_setting_get_engine_setting(DBusConnection* conn, DBusMessage* msg);

int ttsd_dbus_server_setting_set_engine_setting(DBusConnection* conn, DBusMessage* msg);


/*
* Dbus Server functions for tts daemon internal
*/ 

int ttsd_dbus_server_start_next_play(DBusMessage* msg);

int ttsd_dbus_server_start_next_synthesis(DBusMessage* msg);

#ifdef __cplusplus
}
#endif


#endif	/* __TTSD_DBUS_SERVER_h__ */
