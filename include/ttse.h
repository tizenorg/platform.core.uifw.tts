/*
*  Copyright (c) 2011-2016 Samsung Electronics Co., Ltd All Rights Reserved
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


#ifndef __TTSE_H__
#define __TTSE_H__

#include <tizen.h>

/**
* @addtogroup CAPI_UIX_TTSE_MODULE
* @{
*/

#ifdef __cplusplus
extern "C" {
#endif

/**
* @brief Enumerations for error codes.
* @since_tizen 3.0
*/
typedef enum {
	TTSE_ERROR_NONE				= TIZEN_ERROR_NONE,			/**< Successful */
	TTSE_ERROR_OUT_OF_MEMORY		= TIZEN_ERROR_OUT_OF_MEMORY,		/**< Out of Memory */
	TTSE_ERROR_IO_ERROR			= TIZEN_ERROR_IO_ERROR,			/**< I/O error */
	TTSE_ERROR_INVALID_PARAMETER		= TIZEN_ERROR_INVALID_PARAMETER,	/**< Invalid parameter */
	TTSE_ERROR_NETWORK_DOWN			= TIZEN_ERROR_NETWORK_DOWN,		/**< Out of network */
	TTSE_ERROR_PERMISSION_DENIED		= TIZEN_ERROR_PERMISSION_DENIED,	/**< Permission denied */
	TTSE_ERROR_INVALID_STATE		= TIZEN_ERROR_TTS | 0x01,		/**< Invalid state */
	TTSE_ERROR_INVALID_VOICE		= TIZEN_ERROR_TTS | 0x02,		/**< Invalid voice */
	TTSE_ERROR_OPERATION_FAILED		= TIZEN_ERROR_TTS | 0x04,		/**< Operation failed */
	TTSE_ERROR_NOT_SUPPORTED_FEATURE	= TIZEN_ERROR_TTS | 0x06,		/**< Not supported feature */
	TTSE_ERROR_NOT_SUPPORTED		= TIZEN_ERROR_NOT_SUPPORTED		/**< Not supported */
} ttse_error_e;

/**
* @brief Enumerations for audio type.
* @since_tizen 3.0
*/
typedef enum {
	TTSE_AUDIO_TYPE_RAW_S16 = 0,		/**< Signed 16-bit audio type */
	TTSE_AUDIO_TYPE_RAW_U8,			/**< Unsigned 8-bit audio type */
	TTSE_AUDIO_TYPE_MAX
} ttse_audio_type_e;

/**
* @brief Enumerations for result event type.
* @since_tizen 3.0
*/
typedef enum {
	TTSE_RESULT_EVENT_FAIL		= -1,	/**< Event when the voice synthesis is failed */
	TTSE_RESULT_EVENT_START		= 1,	/**< Event when the sound data is first data by callback function */
	TTSE_RESULT_EVENT_CONTINUE	= 2,	/**< Event when the next sound data exist, not first and not last */
	TTSE_RESULT_EVENT_FINISH	= 3		/**< Event when the sound data is last data or sound data is only one result */
} ttse_result_event_e;

/**
* @brief Definitions for male voice type.
* @since_tizen 3.0
*/
#define TTSE_VOICE_TYPE_MALE	1

/**
* @brief Definitions for female voice type.
* @since_tizen 3.0
*/
#define TTSE_VOICE_TYPE_FEMALE	2

/**
* @brief Definitions for child's voice type.
* @since_tizen 3.0
*/
#define TTSE_VOICE_TYPE_CHILD	3


/**
* @brief Called when TTS engine informs the engine service user about whole supported language and voice type list.
* @details This callback function is implemented by the engine service user. Therefore, the engine developer does NOT have to implement this callback function.
* @since_tizen 3.0
*
* @remarks This callback function is called by ttse_foreach_supported_voices_cb() to inform the whole supported voice list.
*	@a user_data must be transferred from ttse_foreach_supported_voices_cb().
*
* @param[in] language The language is specified as an ISO 3166 alpha-2 two-letter country code
*	followed by ISO 639-1 for the two-letter language code. \n
*	For example, "ko_KR" for Korean, "en_US" for American English.
* @param[in] type The voice type
* @param[in] user_data The user data passed from ttse_foreach_supported_voices_cb()
*
* @return @c true to continue with the next iteration of the loop \n @c false to break out of the loop
*
* @pre ttse_foreach_supported_voices_cb() will invoke this callback function.
*
* @see ttse_foreach_supported_voices_cb()
*/
typedef bool (*ttse_supported_voice_cb)(const char* language, int type, void* user_data);

/**
* @brief Called when the engine service user initializes TTS engine.
* @since_tizen 3.0
*
* @remarks This callback function is mandatory and must be registered using ttse_main().
*
* @return 0 on success, otherwise a negative error value
* @retval #TTSE_ERROR_NONE Successful
* @retval #TTSE_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #TTSE_ERROR_INVALID_STATE Already initialized
* @retval #TTSE_ERROR_OPERATION_FAILED Operation failure
* @retval #TTSE_ERROR_PERMISSION_DENIED Permission denied
*
* @see ttse_deinitialize_cb()
*/
typedef int (*ttse_initialize_cb)(void);

/**
* @brief Called when the engine service user deinitializes TTS engine.
* @since_tizen 3.0
*
* @remarks This callback function is mandatory and must be registered using ttse_main().
*	NOTE that the engine may be terminated automatically.
*	When this callback function is invoked, the release of resources is necessary.
*
* @return 0 on success, otherwise a negative error value
* @retval #TTSE_ERROR_NONE Successful
* @retval #TTSE_ERROR_INVALID_STATE Not initialized
*
* @see ttse_initialize_cb()
*/
typedef int (*ttse_deinitialize_cb)(void);

/**
* @brief Called when the engine service user gets the whole supported voice list.
* @since_tizen 3.0
*
* @remarks This callback function is mandatory and must be registered using ttse_main().
*	In this function, the engine service user's callback function 'ttse_supported_voice_cb()' is invoked repeatedly for getting all supported voices, and @a user_data must be transferred to 'ttse_supported_voice_cb()'.
*	If 'ttse_supported_voice_cb()' returns @c false, it should be stopped to call 'ttse_supported_voice_cb()'.
*
*
* @param[in] callback The callback function
* @param[in] user_data The user data which must be passed to ttse_supported_voice_cb()
*
* @return 0 on success, otherwise a negative error value
* @retval #TTSE_ERROR_NONE Successful
* @retval #TTSE_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #TTSE_ERROR_INVALID_STATE Not initialized
*
* @post	This callback function invokes ttse_supported_voice_cb() repeatedly for getting all supported voices.
*
* @see ttse_supported_voice_cb()
*/
typedef int (*ttse_foreach_supported_voices_cb)(ttse_supported_voice_cb callback, void* user_data);

/**
* @brief Called when the engine service user checks whether the voice is valid or not in TTS engine.
* @since_tizen 3.0
*
* @remarks This callback function is mandatory and must be registered using ttse_main().
*
* @param[in] language The language is specified as an ISO 3166 alpha-2 two-letter country code
*	followed by ISO 639-1 for the two-letter language code. \n
*	For example, "ko_KR" for Korean, "en_US" for American English.
* @param[in] type The voice type
* @param[out] is_valid A variable for checking whether the corresponding voice is valid or not. \n @c true to be valid, @c false to be invalid.
*
* @return 0 on success, otherwise a negative error value
* @retval #TTSE_ERROR_NONE Successful
* @retval #TTSE_ERROR_INVALID_PARAMETER Invalid parameter
*
* @see ttse_foreach_supported_voices_cb()
*/
typedef int (*ttse_is_valid_voice_cb)(const char* language, int type, bool* is_valid);

/**
* @brief Called when the engine service user sets the default pitch of TTS engine.
* @since_tizen 3.0
*
* @remarks This callback function is mandatory and must be registered using ttse_main().
*
* @param[in] pitch The default pitch
*
* @return 0 on success, otherwise a negative error value
* @retval #TTSE_ERROR_NONE Successful
* @retval #TTSE_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #TTSE_ERROR_INVALID_STATE Not initialized
* @retval #TTSE_ERROR_OPERATION_FAILED Operation failure
*/
typedef int (*ttse_set_pitch_cb)(int pitch);

/**
* @brief Called when the engine service user requests to load the corresponding voice type for the first time.
* @since_tizen 3.0
*
* @remarks This callback function is mandatory and must be registered using ttse_main().
*
* @param[in] language The language is specified as an ISO 3166 alpha-2 two-letter country code
*	followed by ISO 639-1 for the two-letter language code. \n
*	For example, "ko_KR" for Korean, "en_US" for American English.
* @param[in] type The voice type
*
* @return 0 on success, otherwise a negative error value
* @retval #TTSE_ERROR_NONE Successful
* @retval #TTSE_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #TTSE_ERROR_INVALID_STATE Not initialized
* @retval #TTSE_ERROR_OUT_OF_MEMORY Out of memory
* @retval #TTSE_ERROR_INVALID_VOICE Invalid voice
* @retval #TTSE_ERROR_OPERATION_FAILED Operation failure
*
* @see ttse_unload_voice_cb()
*/
typedef int (*ttse_load_voice_cb)(const char* language, int type);

/**
* @brief Called when the engine service user requests to unload the corresponding voice type or to stop using voice.
* @since_tizen 3.0
*
* @remarks This callback function is mandatory and must be registered using ttse_main().
*
* @param[in] language The language is specified as an ISO 3166 alpha-2 two-letter country code
*	followed by ISO 639-1 for the two-letter language code. \n
*	For example, "ko_KR" for Korean, "en_US" for American English.
* @param[in] type The voice type
*
* @return 0 on success, otherwise a negative error value
* @retval #TTSE_ERROR_NONE Successful
* @retval #TTSE_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #TTSE_ERROR_INVALID_STATE Not initialized
* @retval #TTSE_ERROR_INVALID_VOICE Invalid voice
* @retval #TTSE_ERROR_OPERATION_FAILED Operation failure
*
* @see ttse_load_voice_cb()
*/
typedef int (*ttse_unload_voice_cb)(const char* language, int type);

/**
* @brief Called when the engine service user requests for TTS engine to check whether the application agreed the usage of TTS engine.
* @details This callback function is called when the engine service user requests for TTS engine to check the application's agreement about using the engine.
*	According to the need, the engine developer can provide some user interfaces to check the agreement.
* @since_tizen 3.0
*
* @remarks This callback function is mandatory and must be registered using ttse_main().
*	If the TTS engine developer wants not to check the agreement, the developer has need to return proper values as @a is_agreed in accordance with the intention. \n @c true if the developer regards that every application agreed the usage of the engine, @c false if the developer regards that every application disagreed.
*	NOTE that, however, there may be any legal issue unless the developer checks the agreement. Therefore, we suggest that the engine developers should provide a function to check the agreement.
*
* @param[in] appid The Application ID
* @param[out] is_agreed A variable for checking whether the application agreed to use TTS engine or not. \n @c true to agree, @c false to disagree.
*
* @return 0 on success, otherwise a negative error value
* @retval #TTSE_ERROR_NONE Successful
* @retval #TTSE_ERROR_INVALID_STATE Not initialized
* @retval #TTSE_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #TTSE_ERROR_NOT_SUPPORTED_FEATURE Not supported feature
*/
typedef int (*ttse_check_app_agreed_cb)(const char* appid, bool* is_agreed);

/**
* @brief Called when the engine service user checks whether TTS engine needs the application's credential.
* @since_tizen 3.0
*
* @remarks This callback function is mandatory and must be registered using ttse_main().
*
* @return @c true if TTS engine needs the application's credential, otherwise @c false
*/
typedef bool (*ttse_need_app_credential_cb)(void);

/**
* @brief Called when the engine service user starts to synthesize a voice, asynchronously.
* @since_tizen 3.0
*
* @remarks This callback function is mandatory and must be registered using ttse_main().
*	In this callback function, TTS engine must transfer the synthesized result and @a user_data to the engine service user using ttse_send_result().
*	Also, if TTS engine needs the application's credential, it can set the credential granted to the application.
*
* @param[in] language The language is specified as an ISO 3166 alpha-2 two-letter country code
*	followed by ISO 639-1 for the two-letter language code. \n
*	For example, "ko_KR" for Korean, "en_US" for American English.
* @param[in] type The voice type
* @param[in] text Texts
* @param[in] speed The speed of speaking
* @param[in] appid The Application ID
* @param[in] credential The credential granted to the application
* @param[in] user_data The user data which must be passed to ttse_send_result()
*
* @return 0 on success, otherwise a negative error value
* @retval #TTSE_ERROR_NONE Successful
* @retval #TTSE_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #TTSE_ERROR_INVALID_STATE Not initialized or already started synthesis
* @retval #TTSE_ERROR_INVALID_VOICE Invalid voice
* @retval #TTSE_ERROR_OPERATION_FAILED Operation failure
* @retval #TTSE_ERROR_NETWORK_DOWN Out of network
* @retval #TTSE_ERROR_PERMISSION_DENIED Permission denied
*
* @post This function invokes ttse_send_result().
*
* @see ttse_send_result()
* @see ttse_cancel_synthesis_cb()
* @see ttse_need_app_credential_cb()
*/
typedef int (*ttse_start_synthesis_cb)(const char* language, int type, const char* text, int speed, const char* appid, const char* credential, void* user_data);

/**
* @brief Called when the engine service user cancels to synthesize a voice.
* @since_tizen 3.0
*
* @remarks This callback function is mandatory and must be registered using ttse_main().
*
* @return 0 on success, otherwise a negative error value
* @retval #TTSE_ERROR_NONE Successful
* @retval #TTSE_ERROR_INVALID_STATE Not initialized or not started synthesis
*
* @pre The ttse_start_synthesis_cb() should be performed
*
* @see ttse_start_synthesis_cb()
*/
typedef int (*ttse_cancel_synthesis_cb)(void);

/**
* @brief Called when the engine service user requests the basic information of TTS engine.
* @since_tizen 3.0
*
* @remarks This callback function is mandatory and must be registered using ttse_main().
*	The allocated @a engine_uuid, @a engine_name, and @a engine_setting will be released internally.
*	In order to upload the engine at Tizen Appstore, both a service app and a ui app are necessary.
*	Therefore, @a engine_setting must be transferred to the engine service user.
*
* @param[out] engine_uuid UUID of engine
* @param[out] engine_name Name of engine
* @param[out] engine_setting The engine setting application(ui app)'s app id
* @param[out] use_network The status for using network
*
* @return 0 on success, otherwise a negative error code on failure
* @retval #TTSE_ERROR_NONE Successful
* @retval #TTSE_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #TTSE_ERROR_OPERATION_FAILED Operation failure
*
*/
typedef int (*ttse_get_info_cb)(char** engine_uuid, char** engine_name, char** engine_setting, bool* use_network);

/**
* @brief Called when TTS engine receives the private data from the engine service user.
* @details This callback function is called when the engine service user sends the private data to TTS engine.
* @since_tizen 3.0
*
* @remarks This callback function is optional and is registered using ttse_set_private_data_set_cb().
*
* @param[in] key The key field of private data.
* @param[in] data The data field of private data.
*
* @return 0 on success, otherwise a negative error value
* @retval #TTSE_ERROR_NONE Successful
* @retval #TTSE_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #TTSE_ERROR_OPERATION_FAILED Operation failure
* @retval #TTSE_ERROR_NOT_SUPPORTED_FEATURE Not supported feature
*
* @see ttse_private_data_requested_cb()
* @see ttse_set_private_data_set_cb()
*/
typedef int (*ttse_private_data_set_cb)(const char* key, const char* data);

/**
* @brief Called when TTS engine provides the engine service user with the private data.
* @details This callback function is called when the engine service user gets the private data from TTS engine.
* @since_tizen 3.0
*
* @remarks This callback function is optional and is registered using ttse_set_private_data_requested_cb().
*
* @param[out] key The key field of private data.
* @param[out] data The data field of private data.
*
* @return 0 on success, otherwise a negative error value
* @retval #TTSE_ERROR_NONE Successful
* @retval #TTSE_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #TTSE_ERROR_OPERATION_FAILED Operation failure
* @retval #TTSE_ERROR_NOT_SUPPORTED_FEATURE Not supported feature
*
* @see ttse_private_data_set_cb()
* @see ttse_set_private_data_requested_cb()
*/
typedef int (*ttse_private_data_requested_cb)(const char* key, char** data);


/**
* @brief A structure for the TTS engine functions.
* @details This structure contains essential callback functions for operating TTS engine.
* @since_tizen 3.0
*
* @remarks These functions are mandatory for operating TTS engine. Therefore, all functions MUST be implemented.
*/
typedef struct {
	int version;							/**< The version of the structure 'ttse_request_callback_s' */
	ttse_get_info_cb			get_info;		/**< Called when the engine service user requests the basic information of TTS engine */

	ttse_initialize_cb			initialize;		/**< Called when the engine service user initializes TTS engine */
	ttse_deinitialize_cb			deinitialize;		/**< Called when the engine service user deinitializes TTS engine */

	ttse_foreach_supported_voices_cb	foreach_voices;		/**< Called when the engine service user gets the whole supported voice list */
	ttse_is_valid_voice_cb			is_valid_voice;		/**< Called when the engine service user checks whether the voice is valid or not in TTS engine */
	ttse_set_pitch_cb			set_pitch;		/**< Called when the engine service user sets the default pitch of TTS engine */

	ttse_load_voice_cb			load_voice;		/**< Called when the engine service user requests to load the corresponding voice type for the first time */
	ttse_unload_voice_cb			unload_voice;		/**< Called when the engine service user requests to unload the corresponding voice type or to stop using voice */

	ttse_start_synthesis_cb			start_synth;		/**< Called when the engine service user starts to synthesize a voice, asynchronously */
	ttse_cancel_synthesis_cb		cancel_synth;		/**< Called when the engine service user cancels to synthesize a voice */

	ttse_check_app_agreed_cb		check_app_agreed;	/**< Called when the engine service user requests for TTS engine to check whether the application agreed the usage of TTS engine */ 
	ttse_need_app_credential_cb		need_app_credential;	/**< Called when the engine service user checks whether TTS engine needs the application's credential */
} ttse_request_callback_s;

/**
* @brief Main function for Text-To-Speech (TTS) engine.
* @details This function is the main function for operating TTS engine.
*
* @since_tizen 3.0
*
* @remarks The service_app_main() should be used for working the engine after this function.
*
* @param[in] argc The argument count(original)
* @param[in] argv The argument(original)
* @param[in] callback The structure of engine request callback function
*
* @return This function returns zero on success, or negative with error code on failure
* @retval #TTSE_ERROR_NONE Successful
* @retval #TTSE_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #TTSE_ERROR_OPERATION_FAILED Operation failure
* @retval #TTSE_ERROR_NOT_SUPPORTED Not supported
*
* @see ttse_request_callback_s
*
* @code
#include <ttse.h>

// Required callback functions - MUST BE IMPLEMENTED
static int ttsengine_get_info_cb(char** engine_uuid, char** engine_name, char** engine_setting, bool* use_network);
static int ttsengine_initialize_cb(void);
static int ttsengine_deinitialize_cb(void);
static int ttsengine_is_valid_voice_cb(const char* language, int type, bool* is_valid);
static int ttsengine_foreach_supported_voices_cb(ttse_supported_voice_cb callback, void* user_data);
static int ttsengine_set_pitch_cb(int pitch);
static int ttsengine_load_voice_cb(const char* language, int type);
static int ttsengine_unload_voice_cb(const char* language, int type);
static int ttsengine_start_synthesis_cb(const char* language, int type, const char* text, int speed, void* user_data);
static int ttsengine_cancel_synthesis_cb(void);
static int ttsengine_check_app_agreed_cb(const char* appid, bool* is_agreed);
static bool ttsengine_need_app_credential_cb(void);

// Optional callback function
static int ttsengine_private_data_set_cb(const char* key, const char* data);

int main(int argc, char* argv[])
{
	// 1. Create a structure 'ttse_request_callback_s'
	ttse_request_callback_s engine_callback = { 0, };

	engine_callback.size = sizeof(ttse_request_callback_s);
	engine_callback.version = 1;
	engine_callback.get_info = ttsengine_get_info_cb;
	engine_callback.initialize = ttsengine_initialize_cb;
	engine_callback.deinitialize = ttsengine_deinitialize_cb;
	engine_callback.foreach_voices = ttsengine_foreach_supported_voices_cb;
	engine_callback.is_valid_voice = ttsengine_is_valid_voice_cb;
	engine_callback.set_pitch = ttsengine_set_pitch_cb;
	engine_callback.load_voice = ttsengine_load_voice_cb;
	engine_callback.unload_voice = ttsengine_unload_voice_cb;
	engine_callback.start_synth = ttsengine_start_synthesis_cb;
	engine_callback.cancel_synth = ttsengine_cancel_synthesis_cb;
	engine_callback.check_app_agreed = ttsengine_check_app_agreed_cb;
	engine_callback.need_app_credential = ttsengine_need_app_credential_cb;

	// 2. Run 'ttse_main()'
	if (0 != ttse_main(argc, argv, &engine_callback)) {
		return -1;
	}

	// Optional
	ttse_set_private_data_set_cb(ttsengine_private_data_set_cb);

	// 3. Set event callbacks for service app and Run 'service_app_main()'
	char ad[50] = { 0, };

	service_app_lifecycle_callback_s event_callback;
	app_event_handler_h handlers[5] = { NULL, };

	event_callback.create = service_app_create;
	event_callback.terminate = service_app_terminate;
	event_callback.app_control = service_app_control;

	service_app_add_event_handler(&handlers[APP_EVENT_LOW_BATTERY], APP_EVENT_LOW_BATTERY, service_app_low_battery, &ad);
	service_app_add_event_handler(&handlers[APP_EVENT_LOW_MEMORY], APP_EVENT_LOW_MEMORY, service_app_low_memory, &ad);
	service_app_add_event_handler(&handlers[APP_EVENT_LANGUAGE_CHANGED], APP_EVENT_LANGUAGE_CHANGED, service_app_lang_changed, &ad);
	service_app_add_event_handler(&handlers[APP_EVENT_REGION_FORMAT_CHANGED], APP_EVENT_REGION_FORMAT_CHANGED, service_app_region_changed, &ad);

	return service_app_main(argc, argv, &event_callback, ad);
}

* @endcode
*/
int ttse_main(int argc, char** argv, ttse_request_callback_s *callback);

/**
* @brief Gets the speed range from Tizen platform.
* @since_tizen 3.0
*
* @remarks This API is used when TTS engine wants to get the speed range from Tizen platform.
*
* @param[out] min The minimum speed value
* @param[out] normal The normal speed value
* @param[out] max The maximum speed value
*
* @return 0 on success, otherwise a negative error value
* @retval #TTSE_ERROR_NONE Successful
* @retval #TTSE_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #TTSE_ERROR_NOT_SUPPORTED Not supported
* @retval #TTSE_ERROR_OPERATION_FAILED Operation failure
*
*/
int ttse_get_speed_range(int* min, int* normal, int* max);

/**
* @brief Gets the pitch range from Tizen platform.
* @since_tizen 3.0
*
* @remarks This API is used when TTS engine wants to get the pitch range from Tizen platform.
*
* @param[out] min The minimum pitch value
* @param[out] normal The normal pitch value
* @param[out] max The maximum pitch value
*
* @return 0 on success, otherwise a negative error value
* @retval #TTSE_ERROR_NONE Successful
* @retval #TTSE_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #TTSE_ERROR_NOT_SUPPORTED Not supported
* @retval #TTSE_ERROR_OPERATION_FAILED Operation failure
*
*/
int ttse_get_pitch_range(int* min, int* normal, int* max);

/**
* @brief Sends the synthesized result to the engine service user.
* @since_tizen 3.0
*
* @remarks This API is used in ttse_start_synthesis_cb(), when TTS engine sends the synthesized result to the engine service user.
*	The synthesized result and @a user_data must be transferred to the engine service user through this function.
*
* @param[in] event The result event
* @param[in] data Result data
* @param[in] data_size Result data size
* @param[in] audio_type The audio type
* @param[in] rate The sample rate
* @param[in] user_data The user data passed from ttse_start_synthesis_cb()
*
* @return 0 on success, otherwise a negative error value
* @retval #TTSE_ERROR_NONE Successful
* @retval #TTSE_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #TTSE_ERROR_NOT_SUPPORTED Not supported
* @retval #TTSE_ERROR_OPERATION_FAILED Operation failure
*
* @pre The ttse_main() function should be invoked before this function is called.
*	ttse_start_synthesis_cb() will invoke this function.
*
* @see ttse_start_synthesis_cb()
*/
int ttse_send_result(ttse_result_event_e event, const void* data, unsigned int data_size,
			ttse_audio_type_e audio_type, int rate, void* user_data);

/**
* @brief Sends the error to the engine service user.
* @details The following error codes can be delivered.\n
*	#TTSE_ERROR_NONE,\n
*	#TTSE_ERROR_OUT_OF_MEMORY,\n
*	#TTSE_ERROR_IO_ERROR,\n
*	#TTSE_ERROR_INVALID_PARAMETER,\n
*	#TTSE_ERROR_NETWORK_DOWN,\n
*	#TTSE_ERROR_PERMISSION_DENIED,\n
*	#TTSE_ERROR_INVALID_STATE,\n
*	#TTSE_ERROR_INVALID_VOICE,\n
*	#TTSE_ERROR_OPERATION_FAILED,\n
*	#TTSE_ERROR_NOT_SUPPORTED_FEATURE,\n
*	#TTSE_ERROR_NOT_SUPPORTED.\n
*
* @since_tizen 3.0

* @param[in] error The error reason
* @param[in] msg The error message
*
* @return 0 on success, otherwise a negative error value
* @retval #TTSE_ERROR_NONE Successful
* @retval #TTSE_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #TTSE_ERROR_NOT_SUPPORTED Not supported
* @retval #TTSE_ERROR_OPERATION_FAILED Operation failure
*
* @pre The ttse_main() function should be invoked before this function is called.
*/
int ttse_send_error(ttse_error_e error, const char* msg);

/**
* @brief Sets a callback function for setting the private data.
* @since_tizen 3.0
*
* @remarks The ttse_private_data_set_cb() function is called when the engine service user sends the private data.
*
* @param[in] callback_func ttse_private_data_set event callback function
*
* @return 0 on success, otherwise a negative error value
* @retval #TTSE_ERROR_NONE Successful
* @retval #TTSE_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #TTSE_ERROR_NOT_SUPPORTED Not supported
* @retval #TTSE_ERROR_OPERATION_FAILED Operation failure
*
* @pre The ttse_main() function should be invoked before this function is called.
*
* @see ttse_private_data_set_cb()
*/
int ttse_set_private_data_set_cb(ttse_private_data_set_cb callback_func);

/**
* @brief Sets a callback function for requesting the private data.
* @since_tizen 3.0
*
* @remarks The ttse_private_data_requested_cb() function is called when the engine service user gets the private data from TTS engine.
*
* @param[in] callback_func ttse_private_data_requested event callback function
*
* @return 0 on success, otherwise a negative error value
* @retval #TTSE_ERROR_NONE Successful
* @retval #TTSE_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #TTSE_ERROR_NOT_SUPPORTED Not supported
* @retval #TTSE_ERROR_OPERATION_FAILED Operation failure
*
* @pre The ttse_main() function should be invoked before this function is called.
*
* @see ttse_private_data_requested_cb()
*/
int ttse_set_private_data_requested_cb(ttse_private_data_requested_cb callback_func);



#ifdef __cplusplus
}
#endif

#endif /* __TTSE_H__ */
