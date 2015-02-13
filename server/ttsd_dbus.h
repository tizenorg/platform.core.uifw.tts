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


#ifndef __TTSD_DBUS_h__
#define __TTSD_DBUS_h__

#ifdef __cplusplus
extern "C" {
#endif

int ttsd_dbus_open_connection();

int ttsd_dbus_close_connection();

int ttsd_file_msg_open_connection(int pid);

int ttsd_file_msg_close_connection(int pid);

int ttsd_file_clean_up();

int ttsdc_send_hello(int pid, int uid);

int ttsdc_send_utt_start_message(int pid, int uid, int uttid);

int ttsdc_send_utt_finish_message(int pid, int uid, int uttid);

int ttsdc_send_error_message(int pid, int uid, int uttid, int reason);

int ttsdc_send_set_state_message(int pid, int uid, int state);


#ifdef __cplusplus
}
#endif

#endif	/* __TTSD_DBUS_h__ */
