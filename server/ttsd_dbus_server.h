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


#ifndef __TTSD_DBUS_SERVER_h__
#define __TTSD_DBUS_SERVER_h__

#include <dbus/dbus.h>

#ifdef __cplusplus
extern "C" {
#endif

int ttsd_dbus_server_hello(DBusConnection* conn, DBusMessage* msg);

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

int ttsd_dbus_server_set_private_data(DBusConnection* conn, DBusMessage* msg);

int ttsd_dbus_server_get_private_data(DBusConnection* conn, DBusMessage* msg);

#ifdef __cplusplus
}
#endif


#endif	/* __TTSD_DBUS_SERVER_h__ */
