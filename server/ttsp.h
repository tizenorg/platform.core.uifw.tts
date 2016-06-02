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


#ifndef __TTSP_H__
#define __TTSP_H__

#include <errno.h>
#include <stdbool.h>
#include <tizen.h>

/**
* @addtogroup TTS_ENGINE_MODULE
* @{
*/

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * @brief Enumeration for error code.
*/
typedef enum {
	TTSP_ERROR_NONE			= TIZEN_ERROR_NONE,		/**< Successful */
	TTSP_ERROR_OUT_OF_MEMORY	= TIZEN_ERROR_OUT_OF_MEMORY,	/**< Out of Memory */
	TTSP_ERROR_IO_ERROR		= TIZEN_ERROR_IO_ERROR,		/**< I/O error */
	TTSP_ERROR_INVALID_PARAMETER	= TIZEN_ERROR_INVALID_PARAMETER,/**< Invalid parameter */
	TTSP_ERROR_OUT_OF_NETWORK	= TIZEN_ERROR_NETWORK_DOWN,	/**< Network is down */
	TTSP_ERROR_TIMED_OUT		= TIZEN_ERROR_TIMED_OUT,	/**< No answer from the daemon */
	TTSP_ERROR_PERMISSION_DENIED	= TIZEN_ERROR_PERMISSION_DENIED,/**< Permission denied */
	TTSP_ERROR_NOT_SUPPORTED	= TIZEN_ERROR_NOT_SUPPORTED,	/**< TTS NOT supported */
	TTSP_ERROR_INVALID_STATE	= TIZEN_ERROR_TTS | 0x01,	/**< Invalid state */
	TTSP_ERROR_INVALID_VOICE	= TIZEN_ERROR_TTS | 0x02,	/**< Invalid voice */
	TTSP_ERROR_ENGINE_NOT_FOUND	= TIZEN_ERROR_TTS | 0x03,	/**< No available engine */
	TTSP_ERROR_OPERATION_FAILED	= TIZEN_ERROR_TTS | 0x04,	/**< Operation failed */
	TTSP_ERROR_AUDIO_POLICY_BLOCKED	= TIZEN_ERROR_TTS | 0x05	/**< Audio policy blocked */
} ttsp_error_e;

/**
* @brief Enumerations of audio type.
*/
typedef enum {
	TTSP_AUDIO_TYPE_RAW_S16 = 0,	/**< Signed 16-bit audio sample */
	TTSP_AUDIO_TYPE_RAW_U8,		/**< Unsigned 8-bit audio sample */
	TTSP_AUDIO_TYPE_MAX
} ttsp_audio_type_e;

/**
* @brief Enumerations of result event type.
*/
typedef enum {
	TTSP_RESULT_EVENT_FAIL		= -1, /**< event when the voice synthesis is failed */
	TTSP_RESULT_EVENT_START		= 1,  /**< event when the sound data is first data by callback function */
	TTSP_RESULT_EVENT_CONTINUE	= 2,  /**< event when the next sound data exist, not first and not last */
	TTSP_RESULT_EVENT_FINISH	= 3   /**< event when the sound data is last data or sound data is only one result */
} ttsp_result_event_e;

/**
* @brief Enumerations of TTS mode.
*/
typedef enum {
	TTSP_MODE_DEFAULT	= 0,	/**< Default mode for normal application */
	TTSP_MODE_NOTIFICATION	= 1,	/**< Notification mode */
	TTSP_MODE_SCREEN_READER	= 2	/**< Accessibiliity mode */
} ttsp_mode_e;

/** 
* @brief Defines of voice type.
*/
#define TTSP_VOICE_TYPE_MALE	1
#define TTSP_VOICE_TYPE_FEMALE	2
#define TTSP_VOICE_TYPE_CHILD	3

/** 
* @brief Called when the daemon gets synthesized result.
* 
* @param[in] event A result event
* @param[in] data Result data
* @param[in] data_size Result data size
* @param[in] audio_type A audio type 
* @param[in] rate A sample rate 
* @param[in] user_data The user data passed from the start synthesis function
*
* @return @c true to continue with the next iteration of synthesis \n @c false to stop
*
* @pre ttspe_start_synthesis() will invoke this callback.
*
* @see ttspe_start_synthesis()
*/
typedef bool (*ttspe_result_cb)(ttsp_result_event_e event, const void* data, unsigned int data_size, 
			ttsp_audio_type_e audio_type, int rate, void *user_data);

/**
* @brief Called when the daemon gets a language and a voice type.
*
* @param[in] language A language is specified as an ISO 3166 alpha-2 two letter country-code
*		followed by ISO 639-1 for the two-letter language code. \n
*		For example, "ko_KR" for Korean, "en_US" for American English.
* @param[in] type A voice type
* @param[in] user_data The user data passed from the the foreach function
*
* @return @c true to continue with the next iteration of the loop \n @c false to break out of the loop
*
* @pre ttspe_foreach_supported_voices() will invoke this callback. 
*
* @see ttspe_foreach_supported_voices()
*/
typedef bool (*ttspe_supported_voice_cb)(const char* language, int type, void* user_data);

/**
* @brief Initializes the engine.
*
* @param[in] callbacks A callback function
*
* @return 0 on success, otherwise a negative error value
* @retval #TTSP_ERROR_NONE Successful
* @retval #TTSP_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #TTSP_ERROR_INVALID_STATE Already initialized
* @retval #TTSP_ERROR_OPERATION_FAILED Operation failed
* 
* @see ttspe_deinitialize()
*/
typedef int (*ttspe_initialize)(ttspe_result_cb callback);

/**
* @brief Deinitializes the engine.
*
* @return 0 on success, otherwise a negative error value
* @retval #TTSP_ERROR_NONE Successful
* @retval #TTSP_ERROR_INVALID_STATE Not initialized
* 
* @see ttspe_initialize()
*/
typedef int (*ttspe_deinitialize)(void);

/**
* @brief Retrieves all supported voices of the engine using callback function.
*
* @param[in] callback A callback function
* @param[in] user_data The user data to be passed to the callback function
*
* @return 0 on success, otherwise a negative error value
* @retval #TTSP_ERROR_NONE Successful
* @retval #TTSP_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #TTSP_ERROR_INVALID_STATE Not initialized
*
* @post	This function invokes ttspe_supported_voice_cb() repeatedly for getting all supported languages. 
*
* @see ttspe_supported_voice_cb()
*/
typedef int (*ttspe_foreach_supported_voices)(ttspe_supported_voice_cb callback, void* user_data);

/**
* @brief Checks whether the voice is valid or not.
*
* @param[in] language A language
* @param[in] type A voice type
*
* @return @c true to be valid \n @c false not to be valid
*
* @see ttspe_foreach_supported_voices()
*/
typedef bool (*ttspe_is_valid_voice)(const char* language, int type);

/**
* @brief Sets default pitch.
*
* @param[in] pitch default pitch
*
* @return 0 on success, otherwise a negative error value
* @retval #TTSP_ERROR_NONE Successful
* @retval #TTSP_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #TTSP_ERROR_INVALID_STATE Not initialized
* @retval #TTSP_ERROR_OPERATION_FAILED Fail
*/
typedef int (*ttspe_set_pitch)(int pitch);

/**
* @brief Gets credential necessity.
*
* @return @c true to be needed app credential, \n @c false not to be needed app credential.
*
*/
typedef bool (*ttspe_need_app_credential)(void);

/**
* @brief Load voice of the engine.
*
* @param[in] language language
* @param[in] type voice type
*
* @return 0 on success, otherwise a negative error value
* @retval #TTSP_ERROR_NONE Successful
* @retval #TTSP_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #TTSP_ERROR_INVALID_STATE Not initialized
* @retval #TTSP_ERROR_OUT_OF_MEMORY Out of memory
* @retval #TTSP_ERROR_INVALID_VOICE Invalid voice
* @retval #TTSP_ERROR_OPERATION_FAILED Fail
*
* @see ttspe_unload_voice()
*/
typedef int (*ttspe_load_voice)(const char* language, int type);

/**
* @brief Unload voice of the engine.
*
* @param[in] language language
* @param[in] type voice type
*
* @return 0 on success, otherwise a negative error value
* @retval #TTSP_ERROR_NONE Successful
* @retval #TTSP_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #TTSP_ERROR_INVALID_STATE Not initialized
* @retval #TTSP_ERROR_INVALID_VOICE Invalid voice
* @retval #TTSP_ERROR_OPERATION_FAILED Fail
*
* @see ttspe_load_voice()
*/
typedef int (*ttspe_unload_voice)(const char* language, int type);

/**
* @brief Starts voice synthesis, asynchronously.
*
* @param[in] language A language
* @param[in] type A voice type
* @param[in] text Texts
* @param[in] speed A speaking speed
* @parma[in] credential The app credential to allow recognition
* @param[in] user_data The user data to be passed to the callback function
*
* @return 0 on success, otherwise a negative error value
* @retval #TTSP_ERROR_NONE Successful
* @retval #TTSP_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #TTSP_ERROR_INVALID_STATE Not initialized or already started synthesis
* @retval #TTSP_ERROR_INVALID_VOICE Invalid voice
* @retval #TTSP_ERROR_OPERATION_FAILED Operation failed
* @retval #TTSP_ERROR_OUT_OF_NETWORK Out of network
* @retval #TTSP_ERROR_PERMISSION_DENIED Permission denied
*
* @post This function invokes ttspe_result_cb().
* 
* @see ttspe_result_cb()
* @see ttspe_cancel_synthesis()
*/
typedef int (*ttspe_start_synthesis)(const char* language, int type, const char* text, int speed, const char* credential, void* user_data);

/**
* @brief Cancels voice synthesis.
*
* @return 0 on success, otherwise a negative error value
* @retval #TTSP_ERROR_NONE Successful
* @retval #TTSP_ERROR_INVALID_STATE Not initialized or not started synthesis
*
* @pre The ttspe_start_synthesis() should be performed
*
* @see ttspe_start_synthesis()
*/
typedef int (*ttspe_cancel_synthesis)(void);

/**
* @brief Set private data.
* @since_tizen 3.0
*
* @param[in] key Key field of private data.
* @param[in] data Data field of private data.
*
* @return 0 on success, otherwise a negative error value
* @retval #TTSE_ERROR_NONE Successful
* @retval #TTSE_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #TTSE_ERROR_OPERATION_FAILED Operation failed
*
* @see ttse_get_private_data()
*/
typedef int (* ttspe_set_private_data)(const char* key, const char* data);

/**
* @brief Get private data.
* @since_tizen 3.0
*
* @param[out] key Key field of private data.
* @param[out] data Data field of private data.
*
* @return 0 on success, otherwise a negative error value
* @retval #TTSE_ERROR_NONE Successful
* @retval #TTSE_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #TTSE_ERROR_OPERATION_FAILED Operation failed
*
* @see ttse_set_private_data()
*/
typedef int (* ttspe_get_private_data)(const char* key, char** data);


/**
* @brief Gets the mode.
*
* @param[out] mode The tts daemon mode
*
* @return 0 on success, otherwise a negative error value
* @retval #TTSP_ERROR_NONE Successful
* @retval #TTSP_ERROR_INVALID_PARAMETER Invalid parameter
*
*/
typedef int (*ttspd_get_mode)(ttsp_mode_e* mode);

/**
* @brief Gets the speed range.
*
* @param[out] min The minimun speed value
* @param[out] normal The normal speed value
* @param[out] max The maximum speed value
*
* @return 0 on success, otherwise a negative error value
* @retval #TTSP_ERROR_NONE Successful
* @retval #TTSP_ERROR_INVALID_PARAMETER Invalid parameter
*
*/
typedef int (*ttspd_get_speed_range)(int* min, int* normal, int* max);

/**
* @brief Gets the pitch range.
*
* @param[out] min The minimun pitch value
* @param[out] normal The normal pitch value
* @param[out] max The maximum pitch value
*
* @return 0 on success, otherwise a negative error value
* @retval #TTSP_ERROR_NONE Successful
* @retval #TTSP_ERROR_INVALID_PARAMETER Invalid parameter
*
*/
typedef int (*ttspd_get_pitch_range)(int* min, int* normal, int* max);

/**
* @brief A structure of the engine functions
*/
typedef struct {
	int size;						/**< Size of structure */    
	int version;						/**< Version */

	ttspe_initialize		initialize;		/**< Initialize engine */
	ttspe_deinitialize		deinitialize;		/**< Shutdown engine */

	/* Get / Set engine information */
	ttspe_foreach_supported_voices	foreach_voices;		/**< Get voice list */
	ttspe_is_valid_voice		is_valid_voice;		/**< Check voice */
	ttspe_set_pitch			set_pitch;		/**< Set default pitch */

	/* Load / Unload voice */
	ttspe_load_voice		load_voice;		/**< Load voice */
	ttspe_unload_voice		unload_voice;		/**< Unload voice */
	ttspe_need_app_credential	need_app_credential;	/**< Get app credential necessity*/

	/* Control synthesis */
	ttspe_start_synthesis		start_synth;		/**< Start synthesis */
	ttspe_cancel_synthesis		cancel_synth;		/**< Cancel synthesis */
	ttspe_set_private_data		set_private_data;	/**< Set private data */
	ttspe_get_private_data		get_private_data;	/**< Get private data */
} ttspe_funcs_s;

/**
* @brief A structure of the daemon functions
*/
typedef struct {
	int size;						/**< size */
	int version;						/**< version */

	ttspd_get_mode			get_mode;		/**< Get mode */
	ttspd_get_speed_range		get_speed_range;	/**< Get speed range */
	ttspd_get_pitch_range		get_pitch_range;	/**< Get pitch range */
} ttspd_funcs_s;

/**
* @brief Loads the engine by the daemon.
*
* @param[in] pdfuncs The daemon functions
* @param[out] pefuncs The engine functions
*
* @return 0 on success, otherwise a negative error value
* @retval #TTSP_ERROR_NONE Successful
* @retval #TTSP_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #TTSP_ERROR_OPERATION_FAILED Operation failed
*
* @pre The ttsp_get_engine_info() should be successful.
* @post The daemon calls the engine functions of ttspe_funcs_s.
*
* @see ttsp_get_engine_info()
* @see ttsp_unload_engine()
*/
int ttsp_load_engine(ttspd_funcs_s* pdfuncs, ttspe_funcs_s* pefuncs);

/**
* @brief Unloads the engine by the daemon. 
*
* @pre The ttsp_load_engine() should be performed.
*
* @see ttsp_load_engine()
*/
void ttsp_unload_engine(void);

/**
* @brief Called to get this engine base information.
*
* @param[in] engine_uuid The engine id
* @param[in] engine_name The engine name
* @param[in] setting_ug_name The setting ug name
* @param[in] use_network @c true to need network \n @c false not to need network
* @param[in] user_data The user data passed from the engine info function
*
* @pre ttsp_get_engine_info() will invoke this callback. 
*
* @see ttsp_get_engine_info()
*/
typedef void (*ttsp_engine_info_cb)(const char* engine_uuid, const char* engine_name, const char* setting_ug_name, 
				     bool use_network, void* user_data);

/**
* @brief Gets base information of the engine by the daemon.
*
* @param[in] callback A callback function
* @param[in] user_data The user data to be passed to the callback function
*
* @return 0 on success, otherwise a negative error value
* @retval #TTSP_ERROR_NONE Successful
* @retval #TTSP_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #TTSP_ERROR_OPERATION_FAILED Operation failed
*
* @post	This function invokes ttsp_engine_info_cb() for getting engine information.
*
* @see ttsp_engine_info_cb()
* @see ttsp_load_engine()
*/
int ttsp_get_engine_info(ttsp_engine_info_cb callback, void* user_data);

#ifdef __cplusplus
}
#endif

/**
* @}@}
*/

#endif /* __TTSP_H__ */
