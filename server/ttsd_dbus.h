/*
*  Copyright (c) 2011 Samsung Electronics Co., Ltd All Rights Reserved 
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


int ttsdc_send_utt_start_message(int pid, int uid, int uttid);

int ttsdc_send_utt_finish_message(int pid, int uid, int uttid);

int ttsdc_send_error_message(int pid, int uid, int uttid, int reason);

int ttsdc_send_interrupt_message(int pid, int uid, ttsd_interrupted_code_e code);

int ttsd_send_start_next_play_message(int uid);

int ttsd_send_start_next_synthesis_message(int uid);

#ifdef __cplusplus
}
#endif

#endif	/* __TTSD_DBUS_h__ */
