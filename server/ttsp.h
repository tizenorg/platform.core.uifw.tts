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


#ifndef __TTSP_H__
#define __TTSP_H__

#include <errno.h>
#include <stdbool.h>

/**
* @addtogroup TTS_ENGINE_MODULE
* @{
*/

#ifdef __cplusplus
extern "C" {
#endif

/** 
* @brief Enumerations of error codes.
*/
typedef enum {
	TTSP_ERROR_NONE			= 0,		/**< Successful */
	TTSP_ERROR_OUT_OF_MEMORY	= -ENOMEM,	/**< Out of Memory */
	TTSP_ERROR_IO_ERROR		= -EIO,		/**< I/O error */
	TTSP_ERROR_INVALID_PARAMETER	= -EINVAL,	/**< Invalid parameter */
	TTSP_ERROR_OUT_OF_NETWORK	= -ENETDOWN,	/**< Out of network */
	TTSP_ERROR_INVALID_STATE	= -0x0100021,	/**< Invalid state */
	TTSP_ERROR_INVALID_VOICE	= -0x0100022,	/**< Invalid voice */
	TTSP_ERROR_OPERATION_FAILED	= -0x0100025	/**< Operation failed */
}ttsp_error_e;

/**
* @brief Enumerations of speaking speed.
*/
typedef enum {
	TTSP_SPEED_VERY_SLOW = 1,	/**< Very slow */
	TTSP_SPEED_SLOW,		/**< Slow */
	TTSP_SPEED_NORMAL,		/**< Normal */
	TTSP_SPEED_FAST,		/**< Fast */
	TTSP_SPEED_VERY_FAST		/**< Very fast */
}ttsp_speed_e;

/** 
* @brief Enumerations of voice type.
*/
typedef enum {
	TTSP_VOICE_TYPE_MALE = 1,	/**< Male */
	TTSP_VOICE_TYPE_FEMALE,		/**< Female */
	TTSP_VOICE_TYPE_CHILD,		/**< Child */
	TTSP_VOICE_TYPE_USER1,		/**< Engine defined */
	TTSP_VOICE_TYPE_USER2,		/**< Engine defined */
	TTSP_VOICE_TYPE_USER3		/**< Engine defined */
}ttsp_voice_type_e;

/**
* @brief Enumerations of audio type.
*/
typedef enum {
	TTSP_AUDIO_TYPE_RAW = 0,	/**< PCM audio type */
	TTSP_AUDIO_TYPE_WAV,		/**< Wave audio type */
	TTSP_AUDIO_TYPE_MP3,		/**< MP3 audio type */
	TTSP_AUDIO_TYPE_AMR		/**< AMR audio type */
}ttsp_audio_type_e;

/**
* @brief Enumerations of result event type.
*/
typedef enum {
	TTSP_RESULT_EVENT_FAIL		= -1, /**< event when the voice synthesis is failed */
	TTSP_RESULT_EVENT_START		= 1,  /**< event when the sound data is first data by callback function */
	TTSP_RESULT_EVENT_CONTINUE	= 2,  /**< event when the next sound data exist, not first and not last */
	TTSP_RESULT_EVENT_FINISH	= 3,  /**< event when the sound data is last data or sound data is only one result */
	TTSP_RESULT_EVENT_CANCEL	= 4,  /**< event when the voice synthesis has been canceled */
}ttsp_result_event_e;

/** 
* @brief Called when the daemon gets synthesized result.
* 
* @param[in] event A result event
* @param[in] data Result data
* @param[in] data_size Result data size
* @param[in] user_data The user data passed from the start synthesis function
*
* @return @c true to continue with the next iteration of synthesis \n @c false to stop
*
* @pre ttspe_start_synthesis() will invoke this callback.
*
* @see ttspe_start_synthesis()
*/
typedef bool (*ttspe_result_cb)(ttsp_result_event_e event, const void* data, unsigned int data_size, void *user_data);

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
typedef bool (*ttspe_supported_voice_cb)(const char* language, ttsp_voice_type_e type, void* user_data);

/**
* @brief Called when the daemon gets a engine setting.
*
* @param[in] key A key
* @param[in] value A value
* @param[in] user_data The user data passed from the foreach function
*
* @return @c true to continue with the next iteration of the loop \n @c false to break out of the loop
*
* @pre ttspe_foreach_engine_settings() will invoke this callback. 
*
* @see ttspe_foreach_engine_settings()
*/
typedef bool (*ttspe_engine_setting_cb)(const char* key, const char* value, void* user_data);

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
typedef int (* ttspe_initialize)(ttspe_result_cb callback);

/**
* @brief Deinitializes the engine.
*
* @return 0 on success, otherwise a negative error value
* @retval #TTSP_ERROR_NONE Successful
* @retval #TTSP_ERROR_INVALID_STATE Not initialized
* 
* @see ttspe_initialize()
*/
typedef int (* ttspe_deinitialize)(void);

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
typedef int (* ttspe_foreach_supported_voices)(ttspe_supported_voice_cb callback, void* user_data);

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
typedef bool (* ttspe_is_valid_voice)(const char* language, ttsp_voice_type_e type);

/**
* @brief Gets audio format of the engine.
*
* @param[out] types A audio type 
* @param[out] rate A sample rate 
* @param[out] channels Channels
*
* @return 0 on success, otherwise a negative error value
* @retval #TTSP_ERROR_NONE Successful
* @retval #TTSP_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #TTSP_ERROR_INVALID_STATE Not initialized
*/
typedef int (* ttspe_get_audio_format)(ttsp_audio_type_e* audio_type, int* rate, int* channel);

/**
* @brief Starts voice synthesis, asynchronously.
*
* @param[in] language A language
* @param[in] type A voice type
* @param[in] text Texts
* @param[in] speed A speaking speed
* @param[in] user_data The user data to be passed to the callback function
*
* @return 0 on success, otherwise a negative error value
* @retval #TTSP_ERROR_NONE Successful
* @retval #TTSP_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #TTSP_ERROR_INVALID_STATE Not initialized or already started synthesis
* @retval #TTSP_ERROR_INVALID_VOICE Invalid voice
* @retval #TTSP_ERROR_OPERATION_FAILED Operation failed
* @retval #TTSP_ERROR_OUT_OF_NETWORK Out of network
*
* @post This function invokes ttspe_result_cb().
* 
* @see ttspe_result_cb()
* @see ttspe_cancel_synthesis()
*/
typedef int (* ttspe_start_synthesis)(const char* language, ttsp_voice_type_e type, const char* text, ttsp_speed_e speed, void* user_data);

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
typedef int (* ttspe_cancel_synthesis)(void);

/**
* @brief Gets setting information of the engine.
*
* @param[in] callback A callback function.
* @param[in] user_data The user data to be passed to the callback function.
*
* @return 0 on success, otherwise a negative error value
* @retval #TTSP_ERROR_NONE Successful
* @retval #TTSP_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #TTSP_ERROR_INVALID_STATE Not initialized
* @retval #TTSP_ERROR_OPERATION_FAILED Operation failed
*
* @post	This function invokes ttspe_engine_setting_cb() repeatedly for getting all engine settings. 
*
* @see ttspe_engine_setting_cb()
*/
typedef int (* ttspe_foreach_engine_settings)(ttspe_engine_setting_cb callback, void* user_data);

/**
* @brief Sets setting information of the engine.
*
* @param[in] key A key.
* @param[in] value A value.
*
* @return 0 on success, otherwise a negative error value
* @retval #TTSP_ERROR_NONE Successful
* @retval #TTSP_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #TTSP_ERROR_INVALID_STATE Not initialized
* @retval #TTSP_ERROR_OPERATION_FAILED Operation failed
*
* @see ttspe_foreach_engine_settings()
*/
typedef int (* ttspe_set_engine_setting)(const char* key, const char* value);

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
	ttspe_get_audio_format		get_audio_format;	/**< Get audio format function */

	/* Control synthesis */
	ttspe_start_synthesis		start_synth;		/**< Start synthesis */
	ttspe_cancel_synthesis		cancel_synth;		/**< Cancel synthesis */

	/* Engine setting */
	ttspe_foreach_engine_settings	foreach_engine_setting;	/**< Foreach engine setting */
	ttspe_set_engine_setting	set_engine_setting;	/**< Set engine setting */
} ttspe_funcs_s;

/**
* @brief A structure of the daemon functions
*/
typedef struct {
	int size;					/**< size */
	int version;					/**< version */

}ttspd_funcs_s;

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
