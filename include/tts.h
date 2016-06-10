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


#ifndef __TTS_H__
#define __TTS_H__

#include <tizen.h>

/**
* @file tts.h
*/

/**
* @addtogroup CAPI_UIX_TTS_MODULE
* @{
*/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Enumeration for error code.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
*/
typedef enum {
	TTS_ERROR_NONE			= TIZEN_ERROR_NONE,		/**< Successful */
	TTS_ERROR_OUT_OF_MEMORY		= TIZEN_ERROR_OUT_OF_MEMORY,	/**< Out of Memory */
	TTS_ERROR_IO_ERROR		= TIZEN_ERROR_IO_ERROR,		/**< I/O error */
	TTS_ERROR_INVALID_PARAMETER	= TIZEN_ERROR_INVALID_PARAMETER,/**< Invalid parameter */
	TTS_ERROR_OUT_OF_NETWORK	= TIZEN_ERROR_NETWORK_DOWN,	/**< Network is down */
	TTS_ERROR_TIMED_OUT		= TIZEN_ERROR_TIMED_OUT,	/**< No answer from the daemon */
	TTS_ERROR_PERMISSION_DENIED	= TIZEN_ERROR_PERMISSION_DENIED,/**< Permission denied */
	TTS_ERROR_NOT_SUPPORTED		= TIZEN_ERROR_NOT_SUPPORTED,	/**< TTS NOT supported */
	TTS_ERROR_INVALID_STATE		= TIZEN_ERROR_TTS | 0x01,	/**< Invalid state */
	TTS_ERROR_INVALID_VOICE		= TIZEN_ERROR_TTS | 0x02,	/**< Invalid voice */
	TTS_ERROR_ENGINE_NOT_FOUND	= TIZEN_ERROR_TTS | 0x03,	/**< No available engine */
	TTS_ERROR_OPERATION_FAILED	= TIZEN_ERROR_TTS | 0x04,	/**< Operation failed */
	TTS_ERROR_AUDIO_POLICY_BLOCKED	= TIZEN_ERROR_TTS | 0x05,	/**< Audio policy blocked */
	TTS_ERROR_NOT_SUPPORTED_FEATURE	= TIZEN_ERROR_TTS | 0x06,	/**< Not supported feature of current engine @if MOBILE (Since 3.0) @elseif WEARABLE (Since 2.3.2) @endif */
	TTS_ERROR_SERVICE_RESET		= TIZEN_ERROR_TTS | 0x07	/**< Service reset @if MOBILE (Since 3.0) @elseif WEARABLE (Since 2.3.2) @endif */
} tts_error_e;

/**
 * @brief Enumeration for TTS mode.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
*/
typedef enum {
	TTS_MODE_DEFAULT	= 0,	/**< Default mode for normal application */
	TTS_MODE_NOTIFICATION	= 1,	/**< Notification mode */
	TTS_MODE_SCREEN_READER	= 2	/**< Accessibiliity mode */
} tts_mode_e;

/**
 * @brief Enumerations for state.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
*/
typedef enum {
	TTS_STATE_CREATED	= 0,	/**< 'CREATED' state */
	TTS_STATE_READY		= 1,	/**< 'READY' state */
	TTS_STATE_PLAYING	= 2,	/**< 'PLAYING' state */
	TTS_STATE_PAUSED	= 3	/**< 'PAUSED' state*/
} tts_state_e;

/**
 * @brief Definitions for automatic speaking speed.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
*/
#define TTS_SPEED_AUTO		0

/**
 * @brief Definitions for automatic voice type.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
*/
#define TTS_VOICE_TYPE_AUTO	0

/**
 * @brief Definitions for male voice type.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
*/
#define TTS_VOICE_TYPE_MALE	1

/**
 * @brief Definitions for female voice type.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
*/
#define TTS_VOICE_TYPE_FEMALE	2

/**
 * @brief Definitions for child voice type.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
*/
#define TTS_VOICE_TYPE_CHILD	3

/**
 * @brief The TTS handle.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
*/
typedef struct tts_s *tts_h;


/**
 * @brief Called when the state of TTS is changed.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 *
 * @details If the daemon must stop player because of changing engine and
 *	the daemon must pause player because of other requests, this callback function is called.
 *
 * @param[in] tts The TTS handle
 * @param[in] previous The previous state
 * @param[in] current The current state
 * @param[in] user_data The user data passed from the callback registration function
 *
 * @pre An application registers this callback using tts_set_state_changed_cb() to detect changing state.
 *
 * @see tts_set_state_changed_cb()
 * @see tts_unset_state_changed_cb()
*/
typedef void (*tts_state_changed_cb)(tts_h tts, tts_state_e previous, tts_state_e current, void* user_data);

/**
 * @brief Called when utterance has started.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 *
 * @param[in] tts The TTS handle
 * @param[in] utt_id The utterance ID passed from the add text function
 * @param[in] user_data The user data passed from the callback registration function
 *
 * @pre An application registers this callback using tts_set_utterance_started_cb() to detect utterance started.
 *
 * @see tts_add_text()
 * @see tts_set_utterance_started_cb()
 * @see tts_unset_utterance_started_cb()
*/
typedef void (*tts_utterance_started_cb)(tts_h tts, int utt_id, void* user_data);

/**
 * @brief Called when utterance is finished.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 *
 * @param[in] tts The TTS handle
 * @param[in] utt_id The utterance ID passed from the add text function
 * @param[in] user_data The user data passed from the callback registration function
 *
 * @pre An application registers this callback using tts_set_utterance_completed_cb() to detect utterance completed.
 *
 * @see tts_add_text()
 * @see tts_set_utterance_completed_cb()
 * @see tts_unset_utterance_completed_cb()
*/
typedef void (*tts_utterance_completed_cb)(tts_h tts, int utt_id, void *user_data);

/**
 * @brief Called when an error occurs.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 *
 * @param[in] tts The TTS handle
 * @param[in] utt_id The utterance ID passed from the add text function
 * @param[in] reason The error code
 * @param[in] user_data The user data passed from the callback registration function
 *
 * @pre An application registers this callback using tts_set_error_cb() to detect error.
 *
 * @see tts_play()
 * @see tts_pause()
 * @see tts_stop()
 * @see tts_set_error_cb()
 * @see tts_unset_error_cb()
*/
typedef void (*tts_error_cb)(tts_h tts, int utt_id, tts_error_e reason, void* user_data);

/**
 * @brief Called to retrieve the supported voice.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 *
 * @param[in] tts The TTS handle
 * @param[in] language Language specified as an ISO 3166 alpha-2 two letter country-code followed by ISO 639-1 for the two-letter language code (for example, "ko_KR" for Korean, "en_US" for American English)
 * @param[in] voice_type A voice type (e.g. #TTS_VOICE_TYPE_MALE, #TTS_VOICE_TYPE_FEMALE)
 * @param[in] user_data The user data passed from the foreach function
 *
 * @return @c true to continue with the next iteration of the loop, \n @c false to break out of the loop
 * @pre tts_foreach_supported_voices() will invoke this callback function.
 *
 * @see tts_foreach_supported_voices()
*/
typedef bool(*tts_supported_voice_cb)(tts_h tts, const char* language, int voice_type, void* user_data);

/**
 * @brief Called when the default voice is changed.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 *
 * @param[in] tts The TTS handle
 * @param[in] previous_language The previous language
 * @param[in] previous_voice_type The previous voice type
 * @param[in] current_language The current language
 * @param[in] current_voice_type The current voice type
 * @param[in] user_data The user data passed from the callback registration function
 *
 * @see tts_set_default_voice_changed_cb()
*/
typedef void (*tts_default_voice_changed_cb)(tts_h tts, const char* previous_language, int previous_voice_type,
				const char* current_language, int current_voice_type, void* user_data);
/**
 * @brief Called when the engine is changed.
 * @since_tizen @if MOBILE 3.0 @elseif WEARABLE 2.3.2 @endif
 *
 * @param[in] tts The TTS handle
 * @param[in] engine_id Engine id
 * @param[in] language The default language
 * @param[in] voice_type The default voice type
 * @param[in] need_credential The necessity of credential
 * @param[in] user_data The user data passed from the callback registration function
 *
 * @see tts_set_engine_changed_cb()
*/
typedef void (*tts_engine_changed_cb)(tts_h tts, const char* engine_id, const char* language, int voice_type, bool need_credential, void* user_data);


/**
 * @brief Creates a handle for TTS.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 *
 * @remarks If the function succeeds, @a tts handle must be released with tts_destroy().
 *
 * @param[out] tts The TTS handle
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #TTS_ERROR_NONE Successful
 * @retval #TTS_ERROR_OUT_OF_MEMORY Out of memory
 * @retval #TTS_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #TTS_ERROR_ENGINE_NOT_FOUND Engine not found
 * @retval #TTS_ERROR_OPERATION_FAILED Operation failure
 * @retval #TTS_ERROR_NOT_SUPPORTED TTS NOT supported
 *
 * @post If this function is called, the TTS state will be #TTS_STATE_CREATED.
 *
 * @see tts_destroy()
*/
int tts_create(tts_h* tts);

/**
 * @brief Destroys the handle and disconnects the daemon.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 *
 * @param[in] tts The TTS handle
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #TTS_ERROR_NONE Successful
 * @retval #TTS_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #TTS_ERROR_OPERATION_FAILED Operation failure
 * @retval #TTS_ERROR_NOT_SUPPORTED TTS NOT supported
 *
 * @see tts_create()
*/
int tts_destroy(tts_h tts);

/**
 * @brief Sets the TTS mode.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 *
 * @param[in] tts The TTS handle
 * @param[in] mode The mode
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #TTS_ERROR_NONE Successful
 * @retval #TTS_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #TTS_ERROR_INVALID_STATE Invalid state
 * @retval #TTS_ERROR_OPERATION_FAILED Operation failure
 * @retval #TTS_ERROR_NOT_SUPPORTED TTS NOT supported
 *
 * @pre The state should be #TTS_STATE_CREATED.
 *
 * @see tts_get_mode()
*/
int tts_set_mode(tts_h tts, tts_mode_e mode);

/**
 * @brief Gets the TTS mode.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 *
 * @param[in] tts The TTS handle
 * @param[out] mode The mode
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #TTS_ERROR_NONE Successful
 * @retval #TTS_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #TTS_ERROR_INVALID_STATE Invalid state
 * @retval #TTS_ERROR_NOT_SUPPORTED TTS NOT supported
 *
 * @pre The state should be #TTS_STATE_CREATED.
 *
 * @see tts_set_mode()
*/
int tts_get_mode(tts_h tts, tts_mode_e* mode);

/**
 * @brief Sets the app credential.
 * @since_tizen @if MOBILE 3.0 @elseif WEARABLE 2.3.2 @endif
 *
 * @param[in] tts The TTS handle
 * @param[in] credential The app credential
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #TTS_ERROR_NONE Success
 * @retval #TTS_ERROR_INVALID_STATE Invalid state
 * @retval #TTS_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #TTS_ERROR_NOT_SUPPORTED TTS NOT supported
 *
 * @pre The state should be #TTS_STATE_CREATED or #TTS_STATE_READY.
 *
 * @see tts_play()
*/
int tts_set_credential(tts_h tts, const char* credential);

/**
 * @brief Connects the daemon asynchronously.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 *
 * @param[in] tts The TTS handle
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #TTS_ERROR_NONE Successful
 * @retval #TTS_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #TTS_ERROR_INVALID_STATE Invalid state
 * @retval #TTS_ERROR_NOT_SUPPORTED TTS NOT supported
 *
 * @pre The state should be #TTS_STATE_CREATED.
 * @post If this function is successful, the TTS state will be #TTS_STATE_READY. \n
 *	If this function is failed, the error callback is called. (e.g. #TTS_ERROR_ENGINE_NOT_FOUND)
 *
 * @see tts_unprepare()
*/
int tts_prepare(tts_h tts);

/**
 * @brief Disconnects the daemon.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 *
 * @param[in] tts The TTS handle
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #TTS_ERROR_NONE Successful
 * @retval #TTS_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #TTS_ERROR_INVALID_STATE Invalid state
 * @retval #TTS_ERROR_NOT_SUPPORTED TTS NOT supported
 *
 * @pre The state should be #TTS_STATE_READY.
 * @post If this function is called, the TTS state will be #TTS_STATE_CREATED.
 *
 * @see tts_prepare()
*/
int tts_unprepare(tts_h tts);

/**
 * @brief Retrieves all supported voices of the current engine using callback function.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 *
 * @param[in] tts The TTS handle
 * @param[in] callback The callback function to invoke
 * @param[in] user_data The user data to be passed to the callback function
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #TTS_ERROR_NONE Successful
 * @retval #TTS_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #TTS_ERROR_ENGINE_NOT_FOUND Engine not found
 * @retval #TTS_ERROR_OPERATION_FAILED Operation failure
 * @retval #TTS_ERROR_NOT_SUPPORTED TTS NOT supported
 *
 * @post This function invokes tts_supported_voice_cb() repeatedly for getting voices.
 *
 * @see tts_get_default_voice()
*/
int tts_foreach_supported_voices(tts_h tts, tts_supported_voice_cb callback, void* user_data);

/**
 * @brief Gets the default voice set by the user.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 *
 * @remarks If the function succeeds, @a language must be released with free().
 *
 * @param[in] tts The TTS handle
 * @param[out] language Language specified as an ISO 3166 alpha-2 two letter country-code followed by ISO 639-1 for the two-letter language code (for example, "ko_KR" for Korean, "en_US" for American English)
 * @param[out] voice_type The voice type
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #TTS_ERROR_NONE Successful
 * @retval #TTS_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #TTS_ERROR_ENGINE_NOT_FOUND Engine not found
 * @retval #TTS_ERROR_OPERATION_FAILED Operation failure
 * @retval #TTS_ERROR_NOT_SUPPORTED TTS NOT supported
 *
 * @see tts_foreach_supported_voices()
*/
int tts_get_default_voice(tts_h tts, char** language, int* voice_type);

/**
 * @brief Sets the private data to tts engine.
 * @since_tizen @if MOBILE 3.0 @elseif WEARABLE 2.3.2 @endif
 *
 * @param[in] tts The TTS handle
 * @param[in] key The field name of private data
 * @param[in] data The data for set
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #TTS_ERROR_NONE Successful
 * @retval #TTS_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #TTS_ERROR_INVALID_STATE Invalid state
 * @retval #TTS_ERROR_ENGINE_NOT_FOUND Engine not found
 * @retval #TTS_ERROR_OPERATION_FAILED Operation failure
 * @retval #TTS_ERROR_NOT_SUPPORTED TTS NOT supported
 *
 * @pre The state should be #TTS_STATE_READY.
 *
 * @see tts_get_private_data()
*/
int tts_set_private_data(tts_h tts, const char* key, const char* data);

/**
 * @brief Gets the private data from tts engine.
 * @since_tizen @if MOBILE 3.0 @elseif WEARABLE 2.3.2 @endif
 *
 * @remarks The @a data must be released using free() when it is no longer required.
 *
 * @param[in] tts The TTS handle
 * @param[in] key The field name of private data
 * @param[out] data The data field of private data
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #TTS_ERROR_NONE Successful
 * @retval #TTS_ERROR_INVALID_STATE Invalid state
 * @retval #TTS_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #TTS_ERROR_ENGINE_NOT_FOUND Engine not found
 * @retval #TTS_ERROR_OPERATION_FAILED Operation failure
 * @retval #TTS_ERROR_NOT_SUPPORTED TTS NOT supported
 *
 * @pre The state should be #TTS_STATE_READY.
 *
 * @see tts_set_private_data()
*/
int tts_get_private_data(tts_h tts, const char* key, char** data);

/**
 * @brief Gets the maximum byte size for text.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 *
 * @param[in] tts The TTS handle
 * @param[out] size The maximum byte size for text
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #TTS_ERROR_NONE Successful
 * @retval #TTS_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #TTS_ERROR_INVALID_STATE Invalid state
 * @retval #TTS_ERROR_NOT_SUPPORTED TTS NOT supported
 *
 * @pre The state should be #TTS_STATE_READY.
 *
 * @see tts_add_text()
*/
int tts_get_max_text_size(tts_h tts, unsigned int* size);

/**
 * @brief Gets the current state of TTS.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 *
 * @param[in] tts The TTS handle
 * @param[out] state The current state of TTS
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #TTS_ERROR_NONE Successful
 * @retval #TTS_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #TTS_ERROR_NOT_SUPPORTED TTS NOT supported
 *
 * @see tts_play()
 * @see tts_stop()
 * @see tts_pause()
*/
int tts_get_state(tts_h tts, tts_state_e* state);

/**
 * @brief Gets the speed range.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 *
 * @param[in] tts The TTS handle
 * @param[out] min The minimun speed value
 * @param[out] normal The normal speed value
 * @param[out] max The maximum speed value
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #TTS_ERROR_NONE Successful
 * @retval #TTS_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #TTS_ERROR_INVALID_STATE Invalid state
 * @retval #TTS_ERROR_OPERATION_FAILED Operation failure
 * @retval #TTS_ERROR_NOT_SUPPORTED TTS NOT supported
 *
 * @pre The state should be #TTS_STATE_CREATED.
 *
 * @see tts_add_text()
*/
int tts_get_speed_range(tts_h tts, int* min, int* normal, int* max);

/**
 * @brief Gets the current error message.
 * @since_tizen @if MOBILE 3.0 @elseif WEARABLE 2.3.2 @endif
 *
 * @remarks This function should be called during an tts error callback. If not, the error as operation failure will be returned. \n
 * If the function succeeds, @a err_msg must be released using free() when it is no longer required.
 *
 * @param[in] tts The TTS handle
 * @param[out] err_msg The current error message
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #TTS_ERROR_NONE Successful
 * @retval #TTS_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #TTS_ERROR_NOT_SUPPORTED TTS NOT supported
 * @retval #TTS_ERROR_OPERATION_FAILED Operation failure
 *
 * @see tts_set_error_cb()
 * @see tts_unset_error_cb()
*/
int tts_get_error_message(tts_h tts, char** err_msg);

/**
 * @brief Adds a text to the queue.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 *
 * @remarks Locale(e.g. setlocale()) MUST be set for utf8 text validation check.
 *
 * @param[in] tts The TTS handle
 * @param[in] text An input text based utf8
 * @param[in] language The language selected from the tts_foreach_supported_voices() (e.g. 'NULL'(Automatic), 'en_US')
 * @param[in] voice_type The voice type selected from the tts_foreach_supported_voices() (e.g. #TTS_VOICE_TYPE_AUTO, #TTS_VOICE_TYPE_FEMALE)
 * @param[in] speed A speaking speed (e.g. #TTS_SPEED_AUTO or the value from tts_get_speed_range())
 * @param[out] utt_id The utterance ID passed to the callback function
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #TTS_ERROR_NONE Successful
 * @retval #TTS_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #TTS_ERROR_INVALID_STATE Invalid state
 * @retval #TTS_ERROR_INVALID_VOICE Invalid voice about language, voice type
 * @retval #TTS_ERROR_OPERATION_FAILED Operation failure
 * @retval #TTS_ERROR_NOT_SUPPORTED TTS NOT supported
 * @retval #TTS_ERROR_PERMISSION_DENIED Permission denied
 *
 * @pre The state should be #TTS_STATE_READY, #TTS_STATE_PLAYING or #TTS_STATE_PAUSED.
 * @see tts_get_max_text_size()
 * @see tts_set_credential()
*/
int tts_add_text(tts_h tts, const char* text, const char* language, int voice_type, int speed, int* utt_id);

/**
 * @brief Starts synthesizing voice from the text and plays the synthesized audio data.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 *
 * @param[in] tts The TTS handle
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #TTS_ERROR_NONE Successful
 * @retval #TTS_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #TTS_ERROR_OUT_OF_NETWORK Out of network
 * @retval #TTS_ERROR_INVALID_STATE Invalid state
 * @retval #TTS_ERROR_OPERATION_FAILED Operation failure
 * @retval #TTS_ERROR_NOT_SUPPORTED TTS NOT supported
 * @retval #TTS_ERROR_PERMISSION_DENIED Permission denied
 *
 * @pre The current state should be #TTS_STATE_READY or #TTS_STATE_PAUSED.
 * @post If this function succeeds, the TTS state will be #TTS_STATE_PLAYING.
 *
 * @see tts_add_text()
 * @see tts_pause()
 * @see tts_stop()
 * @see tts_utterance_started_cb()
 * @see tts_utterance_completed_cb()
 * @see tts_error_cb()
 * @see tts_set_credential()
*/
int tts_play(tts_h tts);

/**
 * @brief Stops playing the utterance and clears the queue.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 *
 * @param[in] tts The TTS handle
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #TTS_ERROR_NONE Successful
 * @retval #TTS_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #TTS_ERROR_INVALID_STATE Invalid state
 * @retval #TTS_ERROR_OPERATION_FAILED Operation failure
 * @retval #TTS_ERROR_NOT_SUPPORTED TTS NOT supported
 *
 * @pre The TTS state should be #TTS_STATE_READY or #TTS_STATE_PLAYING or #TTS_STATE_PAUSED.
 * @post If this function succeeds, the TTS state will be #TTS_STATE_READY.
 *	This function will remove all text via tts_add_text() and synthesized sound data.
 *
 * @see tts_play()
 * @see tts_pause()
*/
int tts_stop(tts_h tts);

/**
 * @brief Pauses the currently playing utterance.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 *
 * @param[in] tts The TTS handle
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #TTS_ERROR_NONE Successful
 * @retval #TTS_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #TTS_ERROR_INVALID_STATE Invalid state
 * @retval #TTS_ERROR_OPERATION_FAILED Operation failure
 * @retval #TTS_ERROR_NOT_SUPPORTED TTS NOT supported
 *
 * @pre The TTS state should be #TTS_STATE_PLAYING.
 * @post If this function succeeds, the TTS state will be #TTS_STATE_PAUSED.
 *
 * @see tts_play()
 * @see tts_stop()
 * @see tts_error_cb()
 * @see tts_interrupted_cb()
*/
int tts_pause(tts_h tts);

/**
 * @brief Registers a callback function to be called when the TTS state changes.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 *
 * @param[in] tts The TTS handle
 * @param[in] callback The callback function to register
 * @param[in] user_data The user data to be passed to the callback function
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #TTS_ERROR_NONE Successful
 * @retval #TTS_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #TTS_ERROR_INVALID_STATE Invalid state
 * @retval #TTS_ERROR_NOT_SUPPORTED TTS NOT supported
 *
 * @pre The state should be #TTS_STATE_CREATED.
 *
 * @see tts_state_changed_cb()
 * @see tts_unset_state_changed_cb()
*/
int tts_set_state_changed_cb(tts_h tts, tts_state_changed_cb callback, void* user_data);

/**
 * @brief Unregisters the callback function.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 *
 * @param[in] tts The TTS handle
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #TTS_ERROR_NONE Successful
 * @retval #TTS_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #TTS_ERROR_INVALID_STATE Invalid state
 * @retval #TTS_ERROR_NOT_SUPPORTED TTS NOT supported
 *
 * @pre The state should be #TTS_STATE_CREATED.
 *
 * @see tts_set_state_changed_cb()
*/
int tts_unset_state_changed_cb(tts_h tts);

/**
 * @brief Registers a callback function to detect utterance start.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 *
 * @param[in] tts The TTS handle
 * @param[in] callback The callback function to register
 * @param[in] user_data The user data to be passed to the callback function
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #TTS_ERROR_NONE Successful
 * @retval #TTS_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #TTS_ERROR_INVALID_STATE Invalid state
 * @retval #TTS_ERROR_NOT_SUPPORTED TTS NOT supported
 *
 * @pre The state should be #TTS_STATE_CREATED.
 *
 * @see tts_utterance_started_cb()
 * @see tts_unset_utterance_started_cb()
*/
int tts_set_utterance_started_cb(tts_h tts, tts_utterance_started_cb callback, void* user_data);

/**
 * @brief Unregisters the callback function.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 *
 * @param[in] tts The TTS handle
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #TTS_ERROR_NONE Successful
 * @retval #TTS_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #TTS_ERROR_INVALID_STATE Invalid state
 * @retval #TTS_ERROR_NOT_SUPPORTED TTS NOT supported
 *
 * @pre The state should be #TTS_STATE_CREATED.
 *
 * @see tts_set_utterance_started_cb()
*/
int tts_unset_utterance_started_cb(tts_h tts);

/**
 * @brief Registers a callback function to detect utterance completion.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 *
 * @param[in] tts The TTS handle
 * @param[in] callback The callback function to register
 * @param[in] user_data The user data to be passed to the callback function
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #TTS_ERROR_NONE Successful
 * @retval #TTS_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #TTS_ERROR_INVALID_STATE Invalid state
 * @retval #TTS_ERROR_NOT_SUPPORTED TTS NOT supported
 *
 * @pre The state should be #TTS_STATE_CREATED.
 *
 * @see tts_utterance_completed_cb()
 * @see tts_unset_utterance_completed_cb()
*/
int tts_set_utterance_completed_cb(tts_h tts, tts_utterance_completed_cb callback, void* user_data);

/**
 * @brief Unregisters the callback function.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 *
 * @param[in] tts The TTS handle
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #TTS_ERROR_NONE Successful
 * @retval #TTS_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #TTS_ERROR_INVALID_STATE Invalid state
 * @retval #TTS_ERROR_NOT_SUPPORTED TTS NOT supported
 *
 * @pre The state should be #TTS_STATE_CREATED.
 *
 * @see tts_set_utterance_completed_cb()
*/
int tts_unset_utterance_completed_cb(tts_h tts);

/**
 * @brief Registers a callback function to detect errors.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 *
 * @param[in] tts The TTS handle
 * @param[in] callback The callback function to register
 * @param[in] user_data The user data to be passed to the callback function
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #TTS_ERROR_NONE Successful
 * @retval #TTS_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #TTS_ERROR_INVALID_STATE Invalid state
 * @retval #TTS_ERROR_NOT_SUPPORTED TTS NOT supported
 *
 * @pre The state should be #TTS_STATE_CREATED.
 *
 * @see tts_error_cb()
 * @see tts_unset_error_cb()
*/
int tts_set_error_cb(tts_h tts, tts_error_cb callback, void* user_data);

/**
 * @brief Unregisters the callback function.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 *
 * @param[in] tts The TTS handle
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #TTS_ERROR_NONE Successful
 * @retval #TTS_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #TTS_ERROR_INVALID_STATE Invalid state
 * @retval #TTS_ERROR_NOT_SUPPORTED TTS NOT supported
 *
 * @pre The state should be #TTS_STATE_CREATED.
 *
 * @see tts_set_error_cb()
*/
int tts_unset_error_cb(tts_h tts);

/**
 * @brief Registers a callback function to detect default voice change.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 *
 * @param[in] tts The TTS handle
 * @param[in] callback The callback function to register
 * @param[in] user_data The user data to be passed to the callback function
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #TTS_ERROR_NONE Successful
 * @retval #TTS_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #TTS_ERROR_INVALID_STATE Invalid state
 * @retval #TTS_ERROR_NOT_SUPPORTED TTS NOT supported
 *
 * @pre The state should be #TTS_STATE_CREATED.
 *
 * @see tts_default_voice_changed_cb()
 * @see tts_unset_default_voice_changed_cb()
*/
int tts_set_default_voice_changed_cb(tts_h tts, tts_default_voice_changed_cb callback, void* user_data);

/**
 * @brief Unregisters the callback function.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 *
 * @param[in] tts The TTS handle
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #TTS_ERROR_NONE Successful
 * @retval #TTS_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #TTS_ERROR_INVALID_STATE Invalid state
 * @retval #TTS_ERROR_NOT_SUPPORTED TTS NOT supported
 *
 * @pre The state should be #TTS_STATE_CREATED.
 *
 * @see tts_set_default_voice_changed_cb()
*/
int tts_unset_default_voice_changed_cb(tts_h tts);

 /**
 * @brief Registers a callback function to detect the engine change.
 * @since_tizen @if MOBILE 3.0 @elseif WEARABLE 2.3.2 @endif
 *
 * @param[in] tts The TTS handle
 * @param]in] callback The callback function to register
 * @param[in] user_data The user data to be passed to the callback function
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #TTS_ERROR_NONE Successful
 * @retval #TTS_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #TTS_ERROR_INVALID_STATE Invalid state
 * @retval #TTS_ERROR_NOT_SUPPORTED TTS NOT supported
 *
 * @pre The state should be #TTS_STATE_CREATED.
 *
 * @see tts_engine_changed_cb()
 * @see tts_unset_engine_changed_cb()
*/
int tts_set_engine_changed_cb(tts_h tts, tts_engine_changed_cb callback, void* user_data);

/**
 * @brief Unregisters the callback function.
 * @since_tizen @if MOBILE 3.0 @elseif WEARABLE 2.3.2 @endif
 *
 * @param[in] tts The TTS handle
 *
 * @return 0 on success, otherwise a negative error value
 * @retval #TTS_ERROR_NONE Successful
 * @retval #TTS_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #TTS_ERROR_INVALID_STATE Invalid state
 * @retval #TTS_ERROR_NOT_SUPPORTED TTS NOT supported
 *
 * @pre The state should be #TTS_STATE_CREATED.
 *
 * @see tts_set_engine_changed_cb()
*/
int tts_unset_engine_changed_cb(tts_h tts);


#ifdef __cplusplus
}
#endif

/**
* @}
*/

#endif	/* __TTS_H__ */
