/*
 * Copyright (c) 2011-2016 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef __TIZEN_UIX_TTS_ENGINE_DOC_H__
#define __TIZEN_UIX_TTS_ENGINE_DOC_H__

/**
 * @ingroup CAPI_UIX_FRAMEWORK
 * @defgroup CAPI_UIX_TTSE_MODULE TTS Engine
 * @brief The @ref CAPI_UIX_TTSE_MODULE APIs provide functions to operate Text-To-Speech Engine.
 *
 *
 * @section CAPI_UIX_TTSE_MODULE_HEADER Required Header
 *   \#include <ttse.h>
 *
 *
 * @section CAPI_UIX_TTSE_MODULE_OVERVIEW Overview
 * Text-To-Speech Engine (below TTSE) is an engine for synthesizing voice from text and playing synthesized sound data.
 * Using the @ref CAPI_UIX_TTSE_MODULE APIs, TTSE developers can provide TTSE service users, who want to apply TTSE, with functions necessary to operate the engine.
 * According to the indispensability of TTSE services, there are two ways to provide them to the users. <br>
 *
 * <b>A. Required TTSE services</b> <br>
 * These services are indispensable to operate TTSE. Therefore, the TTSE developers MUST implement callback functions corresponding to the required TTSE services.
 * The following is a list of the callback functions. <br>
 *
 * <table>
 * <tr>
 * <th>FUNCTION</th>
 * <th>DESCRIPTION</th>
 * </tr>
 * <tr>
 * <td>ttse_get_info_cb()</td>
 * <td>Called when the engine service user requests the basic information of TTS engine.</td>
 * </tr>
 * <tr>
 * <td>ttse_initialize_cb()</td>
 * <td>Called when the engine service user initializes TTS engine.</td>
 * </tr>
 * <tr>
 * <td>ttse_deinitialize_cb()</td>
 * <td>Called when the engine service user deinitializes TTS engine.</td>
 * </tr>
 * <tr>
 * <td>ttse_is_valid_voice_cb()</td>
 * <td>Called when the engine service user checks whether the voice is valid or not in TTS engine.</td>
 * </tr>
 * <tr>
 * <td>ttse_foreach_supported_voices_cb()</td>
 * <td>Called when the engine service user gets the whole supported voice list.</td>
 * </tr>
 * <tr>
 * <td>ttse_set_pitch_cb()</td>
 * <td>Called when the engine service user sets the default pitch of TTS engine.</td>
 * </tr>
 * <tr>
 * <td>ttse_load_voice_cb()</td>
 * <td>Called when the engine service user requests to load the corresponding voice type for the first time.</td>
 * </tr>
 * <tr>
 * <td>ttse_unload_voice_cb()</td>
 * <td>Called when the engine service user requests to unload the corresponding voice type or to stop using voice.</td>
 * </tr>
 * <tr>
 * <td>ttse_start_synthesis_cb()</td>
 * <td>Called when the engine service user starts to synthesize a voice, asynchronously.</td>
 * </tr>
 * <tr>
 * <td>ttse_cancel_synthesis_cb()</td>
 * <td>Called when the engine service user cancels to synthesize a voice.</td>
 * </tr>
 * <tr>
 * <td>ttse_check_app_agreed_cb()</td>
 * <td>Called when the engine service user requests for TTS engine to check whether the application agreed the usage of TTS engine.</td>
 * </tr>
 * <tr>
 * <td>ttse_need_app_credential_cb()</td>
 * <td>Called when the engine service user checks whether TTS engine needs the application's credential.</td>
 * </tr>
 * </table>
 *
 * The TTSE developers can register the above callback functions at a time with using a structure 'ttse_request_callback_s' and an API 'ttse_main()'.
 * To operate TTSE, the following steps should be used: <br>
 * 1. Create a structure 'ttse_request_callback_s'
 * 2. Implement callback functions. (NOTE that the callback functions should return appropriate values in accordance with the instruction.
 *	If the callback function returns an unstated value, TTS framework will handle it as #TTSE_ERROR_OPERATION_FAILED.) <br>
 * 3. Register callback functions using 'ttse_main()'. (The registered callback functions will be invoked when the TTSE service users request the TTSE services.) <br>
 * 4. Use 'service_app_main()' for working TTSE. <br>
 *
 * <b>B. Optional TTSE services</b> <br>
 * Unlike the required TTSE services, these services are optional to operate TTSE. The followings are optional TTSE services. <br>
 * - receive/provide the private data <br>
 *
 * If the TTSE developers want to provide the above services, use the following APIs and implement the corresponding callback functions: <br>
 * <table>
 * <tr>
 * <th>FUNCTION</th>
 * <th>DESCRIPTION</th>
 * <th>CORRESPONDING CALLBACK</th>
 * </tr>
 * <tr>
 * <td>ttse_set_private_data_set_cb()</td>
 * <td>Sets a callback function for receiving the private data from the engine service user.</td>
 * <td>ttse_private_data_set_cb()</td>
 * </tr>
 * <tr>
 * <td>ttse_set_private_data_requested_cb()</td>
 * <td>Sets a callback function for providing the private data to the engine service user.</td>
 * <td>ttse_private_data_requested_cb()</td>
 * </tr>
 * </table>
 *
 * Using the above APIs, the TTSE developers can register the optional callback functions respectively.
 * (For normal operation, put those APIs before 'service_app_main()' starts.)
 *
 * Unlike callback functions, the following APIs are functions for getting and sending data. The TTSE developers can use these APIs when they implement TTSE services: <br>
 * <table>
 * <tr>
 * <th>FUNCTION</th>
 * <th>DESCRIPTION</th>
 * </tr>
 * <tr>
 * <td>ttse_get_speed_range()</td>
 * <td>Gets the speed range from Tizen platform.</td>
 * </tr>
 * <tr>
 * <td>ttse_get_pitch_range()</td>
 * <td>Gets the pitch range from Tizen platform.</td>
 * </tr>
 * <tr>
 * <td>ttse_send_result()</td>
 * <td>Sends the synthesized result to the engine service user.</td>
 * </tr>
 * <tr>
 * <td>ttse_send_error()</td>
 * <td>Sends the error to the engine service user.</td>
 * </tr>
 * </table>
 *
 *
 * @section CAPI_UIX_TTSE_MODULE_FEATURE Related Features
 * This API is related with the following features:<br>
 *  - http://tizen.org/feature/speech.synthesis<br>
 *
 * It is recommended to design feature related codes in your application for reliability.<br>
 * You can check if a device supports the related features for this API by using @ref CAPI_SYSTEM_SYSTEM_INFO_MODULE, thereby controlling the procedure of your application.<br>
 * To ensure your application is only running on the device with specific features, please define the features in your manifest file using the manifest editor in the SDK.<br>
 * More details on featuring your application can be found from <a href="https://developer.tizen.org/development/tools/native-tools/manifest-text-editor#feature"><b>Feature Element</b>.</a>
 *
 */

#endif /* __TIZEN_UIX_TTS_ENGINE_DOC_H__ */

