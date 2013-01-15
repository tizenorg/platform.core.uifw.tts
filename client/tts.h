/*
 * Copyright (c) 2012, 2013 Samsung Electronics Co., Ltd All Rights Reserved
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

#include <errno.h>
#include <stdbool.h>

/**
* @addtogroup CAPI_UIX_TTS_MODULE
* @{
*/

#ifdef __cplusplus
extern "C" {
#endif

/** 
* @brief Enumerations of error codes.
*/
typedef enum {
	TTS_ERROR_NONE			= 0,			/**< Successful */
	TTS_ERROR_OUT_OF_MEMORY		= -ENOMEM,		/**< Out of Memory */
	TTS_ERROR_IO_ERROR		= -EIO,			/**< I/O error */
	TTS_ERROR_INVALID_PARAMETER	= -EINVAL,		/**< Invalid parameter */
	TTS_ERROR_OUT_OF_NETWORK	= -ENETDOWN,		/**< Out of network */
	TTS_ERROR_INVALID_STATE		= -0x0100000 | 0x21,	/**< Invalid state */
	TTS_ERROR_INVALID_VOICE		= -0x0100000 | 0x22,	/**< Invalid voice */
	TTS_ERROR_ENGINE_NOT_FOUND	= -0x0100000 | 0x23,	/**< No available engine  */
	TTS_ERROR_TIMED_OUT		= -0x0100000 | 0x24,	/**< No answer from the daemon */
	TTS_ERROR_OPERATION_FAILED	= -0x0100000 | 0x25	/**< Operation failed  */
} tts_error_e;

/** 
* @brief Enumerations of speaking speed.
*/
typedef enum {
	TTS_SPEED_AUTO,			/**< Speed from settings */
	TTS_SPEED_VERY_SLOW,		/**< Very slow */
	TTS_SPEED_SLOW,			/**< Slow */
	TTS_SPEED_NORMAL,		/**< Normal */
	TTS_SPEED_FAST,			/**< Fast */
	TTS_SPEED_VERY_FAST		/**< Very fast */
} tts_speed_e;

/** 
* @brief Enumerations of voice type.
*/
typedef enum {
	TTS_VOICE_TYPE_AUTO,		/**< Voice type from settings or auto selection based language */
	TTS_VOICE_TYPE_MALE,		/**< Male */
	TTS_VOICE_TYPE_FEMALE,		/**< Female */
	TTS_VOICE_TYPE_CHILD,		/**< Child */
	TTS_VOICE_TYPE_USER1,		/**< Engine defined */
	TTS_VOICE_TYPE_USER2,		/**< Engine defined */
	TTS_VOICE_TYPE_USER3		/**< Engine defined */
} tts_voice_type_e;

/** 
* @brief Enumerations of state.
*/
typedef enum {
	TTS_STATE_CREATED = 0,		/**< 'CREATED' state */
	TTS_STATE_READY,		/**< 'READY' state */
	TTS_STATE_PLAYING,		/**< 'PLAYING' state */
	TTS_STATE_PAUSED		/**< 'PAUSED' state*/
}tts_state_e;

/** 
* @brief A structure of handle for identification
*/
typedef struct tts_s *tts_h;


/**
* @brief Called when the state of TTS is changed. 
*
* @details If the daemon must stop player because of changing engine and 
*	the daemon must pause player because of other requests, this callback function is called.
*
* @param[in] tts The handle for TTS
* @param[in] previous A previous state
* @param[in] current A current state
* @param[in] user_data The user data passed from the callback registration function.
*
* @pre An application registers this callback using tts_set_state_changed_cb() to detect changing state.
*
* @see tts_set_state_changed_cb()
* @see tts_unset_state_changed_cb()
*/
typedef void (*tts_state_changed_cb)(tts_h tts, tts_state_e previous, tts_state_e current, void* user_data);

/**
* @brief Called when utterance has started.
*
* @param[in] tts The handle for TTS
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
*
* @param[in] tts The handle for TTS
* @param[in] utt_id The utterance ID passed from the add text function
* @param[in] user_data The user data passed from from the callback registration function
*
* @pre An application registers this callback using tts_set_utterance_completed_cb() to detect utterance completed.
*
* @see tts_add_text()
* @see tts_set_utterance_completed_cb()
* @see tts_unset_utterance_completed_cb()
*/
typedef void (*tts_utterance_completed_cb)(tts_h tts, int utt_id, void *user_data);

/**
* @brief Called when error occurred. 
*
* @param[in] tts The handle for TTS
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
*
* @param[in] tts The handle for TTS
* @param[in] language A language is specified as an ISO 3166 alpha-2 two letter country-code \n
*			followed by ISO 639-1 for the two-letter language code. \n
*			For example, "ko_KR" for Korean, "en_US" for American English
* @param[in] voice_type A voice type
* @param[in] user_data The user data passed from the foreach function
*
* @return @c true to continue with the next iteration of the loop \n @c false to break out of the loop
* @pre tts_foreach_supported_voices() will invoke this callback. 
*
* @see tts_foreach_supported_voices()
*/
typedef bool(*tts_supported_voice_cb)(tts_h tts, const char* language, tts_voice_type_e voice_type, void* user_data);


/**
* @brief Creates a handle for TTS. 
*
* @param[out] tts The handle for TTS
*
* @return 0 on success, otherwise a negative error value
* @retval #TTS_ERROR_NONE Successful
* @retval #TTS_ERROR_INVALID_PARAMETER Invalid parameter
*
* @see tts_destroy()
*/
int tts_create(tts_h* tts);

/**
* @brief Destroys the handle and disconnects the daemon. 
*
* @param[in] tts The handle for TTS
*
* @return 0 on success, otherwise a negative error value
* @retval #TTS_ERROR_NONE Successful
* @retval #TTS_ERROR_INVALID_PARAMETER Invalid parameter
*
* @see tts_create()
*/
int tts_destroy(tts_h tts);

/**
* @brief Connects the daemon asynchronously. 
*
* @param[in] tts The handle for TTS
*
* @return 0 on success, otherwise a negative error value
* @retval #TTS_ERROR_NONE Successful
* @retval #TTS_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #TTS_ERROR_INVALID_STATE Invalid state
*
* @pre The state should be #TTS_STATE_CREATED.
* @post If this function is called, the TTS state will be #TTS_STATE_READY.
*
* @see tts_unprepare()
*/
int tts_prepare(tts_h tts);

/**
* @brief Disconnects the daemon.
*
* @param[in] tts The handle for TTS
*
* @return 0 on success, otherwise a negative error value
* @retval #TTS_ERROR_NONE Successful
* @retval #TTS_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #STT_ERROR_INVALID_STATE Invalid state
*
* @pre The state should be #TTS_STATE_READY.
* @post If this function is called, the TTS state will be #TTS_STATE_CREATED.
*
* @see tts_prepare()
*/
int tts_unprepare(tts_h tts);

/**
* @brief Retrieves all supported voices of the current engine using callback function.
*
* @param[in] tts The handle for TTS
* @param[in] callback The callback function to invoke
* @param[in] user_data The user data to be passed to the callback function
*
* @return 0 on success, otherwise a negative error value
* @retval #TTS_ERROR_NONE Successful
* @retval #TTS_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #TTS_ERROR_OPERATION_FAILED Operation failure
*
* @pre The state should be #TTS_STATE_READY.
* @post	This function invokes tts_supported_voice_cb() repeatedly for getting voices. 
*
* @see tts_get_default_voice()
*/
int tts_foreach_supported_voices(tts_h tts, tts_supported_voice_cb callback, void* user_data);

/**
* @brief Gets the default voice set by user.
*
* @remark If the function succeeds, @a language must be released with free() by you.
*
* @param[in] tts The handle for TTS
* @param[out] language A language is specified as an ISO 3166 alpha-2 two letter country-code \n
*			followed by ISO 639-1 for the two-letter language code. \n
*			For example, "ko_KR" for Korean, "en_US" for American English.
* @param[out] voice_type The voice type
*
* @return 0 on success, otherwise a negative error value.
* @retval #TTS_ERROR_NONE Successful
* @retval #TTS_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #TTS_ERROR_OUT_OF_MEMORY Out of memory
* @retval #TTS_ERROR_OPERATION_FAILED Operation failure
*
* @pre The state should be #TTS_STATE_READY.
*
* @see tts_foreach_supported_voices()
*/
int tts_get_default_voice(tts_h tts, char** language, tts_voice_type_e* voice_type);

/**
* @brief Gets the maximum text count of a current engine.
*
* @param[in] tts The handle for TTS
* @param[out] count The maximum text count of the current engine
*
* @return 0 on success, otherwise a negative error value
* @retval #TTS_ERROR_NONE Successful
* @retval #TTS_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #TTS_ERROR_OPERATION_FAILED Operation failure
*
* @pre The state should be #TTS_STATE_READY.
*
* @see tts_add_text()
*/
int tts_get_max_text_count(tts_h tts, int* count);

/**
* @brief Gets the current state of tts.
*
* @param[in] tts The handle for TTS
* @param[out] state The current state of TTS
*
* @return 0 on success, otherwise a negative error value
* @retval #TTS_ERROR_NONE Successful
* @retval #TTS_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #TTS_ERROR_OPERATION_FAILED Operation failure
*
* @see tts_play()
* @see tts_stop()
* @see tts_pause()
*/
int tts_get_state(tts_h tts, tts_state_e* state);

/**
* @brief Adds a text to the queue.
*
* @param[in] tts The handle for TTS
* @param[in] text A input text
* @param[in] language The language selected from the foreach function
* @param[in] voice_type The voice type selected from the foreach function
* @param[in] speed A speaking speed
* @param[out] utt_id The utterance ID passed to the callback function
*
* @return 0 on success, otherwise a negative error value
* @retval #TTS_ERROR_NONE Successful
* @retval #TTS_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #TTS_ERROR_INVALID_VOICE Invalid voice about language, voice type
* @retval #TTS_ERROR_OUT_OF_MEMORY Out of memory
* @retval #TTS_ERROR_OPERATION_FAILED Operation failure
*
* @pre The state should be #TTS_STATE_READY, #TTS_STATE_PLAYING or #TTS_STATE_PAUSED.
* @see tts_get_max_text_count()
*/
int tts_add_text(tts_h tts, const char* text, const char* language, tts_voice_type_e voice_type, tts_speed_e speed, int* utt_id);

/**
* @brief Starts synthesizing voice from text and plays synthesized audio data.
*
* @param[in] tts The handle for TTS
*
* @return 0 on success, otherwise a negative error value
* @retval #TTS_ERROR_NONE Successful
* @retval #TTS_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #TTS_ERROR_INVALID_STATE Invalid state
* @retval #TTS_ERROR_OPERATION_FAILED Operation failure
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
* @see tts_interrupted_cb()
*/
int tts_play(tts_h tts);

/**
* @brief Stops playing utterance and clears queue.
*
* @param[in] tts The handle for TTS
*
* @return 0 on success, otherwise a negative error value
* @retval #TTS_ERROR_NONE Successful
* @retval #TTS_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #TTS_ERROR_INVALID_STATE Invalid state
* @retval #TTS_ERROR_OPERATION_FAILED Operation failure
*
* @pre The TTS state should be #TTS_STATE_PLAYING or #TTS_STATE_PAUSED.
* @post If this function succeeds, the TTS state will be #TTS_STATE_READY. 
*	This function will remove all text via tts_add_text() and synthesized sound data.
*
* @see tts_play()
* @see tts_pause()
*/
int tts_stop(tts_h tts);

/**
* @brief Pauses currently playing utterance.
*
* @param[in] tts The handle for TTS
*
* @return 0 on success, otherwise a negative error value
* @retval #TTS_ERROR_NONE Successful
* @retval #TTS_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #TTS_ERROR_INVALID_STATE Invalid state
* @retval #TTS_ERROR_OPERATION_FAILED Operation failure
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
* @brief Registers a callback function to be called when TTS state changes.
*
* @param[in] tts The handle for TTS
* @param[in] callback The callback function to register
* @param[in] user_data The user data to be passed to the callback function
*
* @return 0 on success, otherwise a negative error value
* @retval #TTS_ERROR_NONE Successful
* @retval #TTS_ERROR_INVALID_PARAMETER Invalid parameter
*
* @pre The state should be #TTS_STATE_CREATED.
*
* @see tts_state_changed_cb()
* @see tts_unset_state_changed_cb()
*/
int tts_set_state_changed_cb(tts_h tts, tts_state_changed_cb callback, void* user_data);

/**
* @brief Unregisters the callback function.
*
* @param[in] tts The handle for TTS
*
* @return 0 on success, otherwise a negative error value
* @retval #TTS_ERROR_NONE Successful
* @retval #TTS_ERROR_INVALID_PARAMETER Invalid parameter
*
* @pre The state should be #TTS_STATE_CREATED.
*
* @see tts_set_state_changed_cb()
*/
int tts_unset_state_changed_cb(tts_h tts);

/**
* @brief Registers a callback function for detecting utterance started.
*
* @param[in] tts The handle for TTS
* @param[in] callback The callback function to register
* @param[in] user_data The user data to be passed to the callback function
*
* @return 0 on success, otherwise a negative error value
* @retval #TTS_ERROR_NONE Successful
* @retval #TTS_ERROR_INVALID_PARAMETER Invalid parameter
*
* @pre The state should be #TTS_STATE_CREATED.
*
* @see tts_utterance_started_cb()
* @see tts_unset_utterance_started_cb()
*/
int tts_set_utterance_started_cb(tts_h tts, tts_utterance_started_cb callback, void* user_data);

/**
* @brief Unregisters the callback function.
*
* @param[in] tts The handle for TTS
*
* @return 0 on success, otherwise a negative error value
* @retval #TTS_ERROR_NONE Successful
* @retval #TTS_ERROR_INVALID_PARAMETER Invalid parameter
*
* @pre The state should be #TTS_STATE_CREATED.
*
* @see tts_set_utterance_started_cb()
*/
int tts_unset_utterance_started_cb(tts_h tts);

/**
* @brief Registers a callback function for detecting utterance completed.
*
* @param[in] tts The handle for TTS
* @param[in] callback The callback function to register
* @param[in] user_data The user data to be passed to the callback function
*
* @return 0 on success, otherwise a negative error value
* @retval #TTS_ERROR_NONE Successful
* @retval #TTS_ERROR_INVALID_PARAMETER Invalid parameter
*
* @pre The state should be #TTS_STATE_CREATED.
*
* @see tts_utterance_completed_cb()
* @see tts_unset_utterance_completed_cb()
*/
int tts_set_utterance_completed_cb(tts_h tts, tts_utterance_completed_cb callback, void* user_data);

/**
* @brief Unregisters the callback function.
*
* @param[in] tts The handle for TTS
*
* @return 0 on success, otherwise a negative error value
* @retval #TTS_ERROR_NONE Successful
* @retval #TTS_ERROR_OUT_OF_MEMORY Out of memory
*
* @pre The state should be #TTS_STATE_CREATED.
*
* @see tts_set_utterance_completed_cb()
*/
int tts_unset_utterance_completed_cb(tts_h tts);

/**
* @brief Registers a callback function for detecting error.
*
* @param[in] tts The handle for TTS
* @param[in] callback The callback function to register
* @param[in] user_data The user data to be passed to the callback function
*
* @return 0 on success, otherwise a negative error value
* @retval #TTS_ERROR_NONE Successful
* @retval #TTS_ERROR_INVALID_PARAMETER Invalid parameter
*
* @pre The state should be #TTS_STATE_CREATED.
*
* @see tts_error_cb()
* @see tts_unset_error_cb()
*/
int tts_set_error_cb(tts_h tts, tts_error_cb callback, void* user_data);

/**
* @brief Unregisters the callback function.
*
* @param[in] tts The handle for TTS
*
* @return 0 on success, otherwise a negative error value
* @retval #TTS_ERROR_NONE Successful
* @retval #TTS_ERROR_INVALID_PARAMETER Invalid parameter
*
* @pre The state should be #TTS_STATE_CREATED.
*
* @see tts_set_error_cb()
*/
int tts_unset_error_cb(tts_h tts);


#ifdef __cplusplus
}
#endif

/**
* @}
*/

#endif	/* __TTS_H__ */
