/*
*  Copyright (c) 2012, 2013 Samsung Electronics Co., Ltd All Rights Reserved 
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


#ifndef __TTS_SETTING_H__
#define __TTS_SETTING_H__

#include <errno.h>
#include <stdbool.h>

/**
* @addtogroup TTS_SETTING_MODULE
* @{
*/

#ifdef __cplusplus
extern "C" {
#endif

/** 
* @brief Enumerations of error codes.
*/
typedef enum {
	TTS_SETTING_ERROR_NONE			= 0,		/**< Success, No error */
	TTS_SETTING_ERROR_OUT_OF_MEMORY		= -ENOMEM,	/**< Out of Memory */
	TTS_SETTING_ERROR_IO_ERROR		= -EIO,		/**< I/O error */
	TTS_SETTING_ERROR_INVALID_PARAMETER	= -EINVAL,	/**< Invalid parameter */
	TTS_SETTING_ERROR_INVALID_STATE		= -0x0100021,	/**< Invalid state */
	TTS_SETTING_ERROR_INVALID_VOICE		= -0x0100022,	/**< Invalid voice */
	TTS_SETTING_ERROR_ENGINE_NOT_FOUND	= -0x0100023,	/**< No available TTS-engine  */
	TTS_SETTING_ERROR_TIMED_OUT		= -0x0100024,	/**< No answer from TTS daemon */
	TTS_SETTING_ERROR_OPERATION_FAILED	= -0x0100025,	/**< TTS daemon failed  */
} tts_setting_error_e;

/** 
* @brief Enumerations of speaking speed.
*/
typedef enum {
	TTS_SETTING_SPEED_AUTO	= 0,	/**< Speed from settings */
	TTS_SETTING_SPEED_VERY_SLOW,	/**< Very slow */
	TTS_SETTING_SPEED_SLOW,		/**< Slow */
	TTS_SETTING_SPEED_NORMAL,	/**< Normal */
	TTS_SETTING_SPEED_FAST,		/**< Fast */
	TTS_SETTING_SPEED_VERY_FAST	/**< Very fast */
} tts_setting_speed_e;

/** 
* @brief Enumerations of voice type.
*/
typedef enum {
	TTS_SETTING_VOICE_TYPE_AUTO = 0,	/**< Voice type from settings or auto selection based language*/
	TTS_SETTING_VOICE_TYPE_MALE,		/**< Male */
	TTS_SETTING_VOICE_TYPE_FEMALE,		/**< Female */
	TTS_SETTING_VOICE_TYPE_CHILD,		/**< Child */
	TTS_SETTING_VOICE_TYPE_USER1,		/**< Engine defined */
	TTS_SETTING_VOICE_TYPE_USER2,		/**< Engine defined */
	TTS_SETTING_VOICE_TYPE_USER3		/**< Engine defined */
} tts_setting_voice_type_e;

/** 
* @brief Enumerations of setting state.
*/
typedef enum {
	TTS_SETTING_STATE_NONE = 0,
	TTS_SETTING_STATE_READY
} tts_setting_state_e;

/**
* @brief Called to get a engine information.
*
* @param[in] engine_id Engine id.
* @param[in] engine_name engine name.
* @param[in] setting_path gadget path of engine specific setting.
* @param[in] user_data User data passed from the tts_setting_foreach_supported_engines().
*
* @return @c true to continue with the next iteration of the loop, \n @c false to break out of the loop.
* @pre tts_setting_foreach_supported_engines() will invoke this callback. 
*
* @see tts_setting_foreach_supported_engines()
*/
typedef bool(*tts_setting_supported_engine_cb)(const char* engine_id, const char* engine_name, const char* setting_path, void* user_data);

/**
* @brief Called to get a voice.
*
* @param[in] engine_id Engine id.
* @param[in] language A language is specified as an ISO 3166 alpha-2 two letter country-code
*		followed by ISO 639-1 for the two-letter language code. 
*		For example, "ko_KR" for Korean, "en_US" for American English..
* @param[in] voice_type Voice type.
* @param[in] user_data User data passed from the tts_setting_foreach_surpported_voices().
*
* @return @c true to continue with the next iteration of the loop, \n @c false to break out of the loop.
* @pre tts_setting_foreach_surpported_voices() will invoke this callback. 
*
* @see tts_setting_foreach_surpported_voices()
*/
typedef bool(*tts_setting_supported_voice_cb)(const char* engine_id, const char* language, tts_setting_voice_type_e voice_type, void* user_data);

/**
* @brief Called to get a engine setting.
*
* @param[in] engine_id Engine id.
* @param[in] key Key.
* @param[in] value Value.
* @param[in] user_data User data passed from the tts_setting_foreach_engine_settings().
*
* @return @c true to continue with the next iteration of the loop, \n @c false to break out of the loop.
* @pre tts_setting_foreach_engine_settings() will invoke this callback. 
*
* @see tts_setting_foreach_engine_settings()
*/
typedef bool(*tts_setting_engine_setting_cb)(const char* engine_id, const char* key, const char* value, void* user_data);

/**
* @brief Called to initialize setting.
*
* @param[in] state Current state.
* @param[in] reason Error reason.
* @param[in] user_data User data passed from the tts_setting_initialize_async().
*
* @pre tts_setting_initialize_async() will invoke this callback. 
*
* @see tts_setting_initialize_async()
*/
typedef void(*tts_setting_initialized_cb)(tts_setting_state_e state, tts_setting_error_e reason, void* user_data);

/**
* @brief Initialize TTS setting and connect to tts-daemon asynchronously.
*
* @return 0 on success, otherwise a negative error value.
* @retval #TTS_SETTING_ERROR_NONE Success.
* @retval #TTS_SETTING_ERROR_TIMED_OUT tts daemon is blocked or tts daemon do not exist.
* @retval #TTS_SETTING_ERROR_INVALID_STATE TTS setting has Already been initialized. 
* @retval #TTS_SETTING_ERROR_ENGINE_NOT_FOUND No available tts-engine. Engine should be installed.
*
* @see tts_setting_finalize()
*/
int tts_setting_initialize(tts_setting_initialized_cb callback, void* user_data);

int tts_setting_initialize_async(tts_setting_initialized_cb callback, void* user_data);

/**
* @brief finalize tts setting and disconnect to tts-daemon. 
*
* @return 0 on success, otherwise a negative error value.
* @retval #TTS_SETTING_ERROR_NONE Success.
* @retval #TTS_SETTING_ERROR_INVALID_STATE TTS Not initialized. 
* @retval #TTS_SETTING_ERROR_OPERATION_FAILED Operation failure.
*
* @see tts_setting_initialize()
*/
int tts_setting_finalize(void);

/**
* @brief Retrieve supported engine informations using callback function.
*
* @param[in] callback callback function
* @param[in] user_data User data to be passed to the callback function.
*
* @return 0 on success, otherwise a negative error value.
* @retval #TTS_SETTING_ERROR_NONE Success.
* @retval #TTS_SETTING_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #TTS_SETTING_ERROR_INVALID_STATE TTS Not initialized. 
* @post	This function invokes tts_setting_supported_engine_cb() repeatedly for getting engine information. 
*
* @see tts_setting_supported_engine_cb()
*/
int tts_setting_foreach_supported_engines(tts_setting_supported_engine_cb callback, void* user_data);

/**
* @brief Get current engine id.
*
* @remark If the function is success, @a engine_id must be released with free() by you.
*
* @param[out] engine_id engine id.
*
* @return 0 on success, otherwise a negative error value.
* @retval #TTS_SETTING_ERROR_NONE Success.
* @retval #TTS_SETTING_ERROR_OUT_OF_MEMORY Out of memory.
* @retval #TTS_SETTING_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #TTS_SETTING_ERROR_INVALID_STATE TTS Not initialized. 
* @retval #TTS_SETTING_ERROR_OPERATION_FAILED Operation failure.
*
* @see tts_setting_set_engine()
*/
int tts_setting_get_engine(char** engine_id);

/**
* @brief Set current engine id.
*
* @param[in] engine_id engine id.
*
* @return 0 on success, otherwise a negative error value.
* @retval #TTS_SETTING_ERROR_NONE Success.
* @retval #TTS_SETTING_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #TTS_SETTING_ERROR_INVALID_STATE TTS Not initialized. 
* @retval #TTS_SETTING_ERROR_OPERATION_FAILED Operation failure.
*
* @see tts_setting_get_engine()
*/
int tts_setting_set_engine(const char* engine_id);

/**
* @brief Get supported voices of current engine.
*
* @param[in] callback callback function.
* @param[in] user_data User data to be passed to the callback function.
*
* @return 0 on success, otherwise a negative error value.
* @retval #TTS_SETTING_ERROR_NONE Success.
* @retval #TTS_SETTING_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #TTS_SETTING_ERROR_INVALID_STATE TTS Not initialized. 
* @retval #TTS_SETTING_ERROR_OPERATION_FAILED Operation failure.
*
* @post	This function invokes tts_setting_supported_voice_cb() repeatedly for getting supported voices. 
*
* @see tts_setting_supported_voice_cb()
*/
int tts_setting_foreach_surpported_voices(tts_setting_supported_voice_cb callback, void* user_data);

/**
* @brief Get a default voice of current engine.
*
* @remark If the function is success, @a language must be released with free() by you.
*
* @param[out] language current language
* @param[out] voice_type current voice type
*
* @return 0 on success, otherwise a negative error value.
* @retval #TTS_SETTING_ERROR_NONE Success.
* @retval #TTS_SETTING_ERROR_OUT_OF_MEMORY Out of memory.
* @retval #TTS_SETTING_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #TTS_SETTING_ERROR_INVALID_STATE TTS Not initialized. 
* @retval #TTS_SETTING_ERROR_OPERATION_FAILED Operation failure.
*
* @see tts_setting_set_default_voice()
*/
int tts_setting_get_default_voice(char** language, tts_setting_voice_type_e* voice_type);

/**
* @brief Set a default voice of current engine.
*
* @param[in] language language
* @param[in] voice_type voice type.
*
* @return 0 on success, otherwise a negative error value.
* @retval #TTS_SETTING_ERROR_NONE Success.
* @retval #TTS_SETTING_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #TTS_SETTING_ERROR_INVALID_STATE TTS Not initialized. 
* @retval #TTS_SETTING_ERROR_OPERATION_FAILED Operation failure.
*
* @see tts_setting_get_default_voice()
*/
int tts_setting_set_default_voice(const char* language, tts_setting_voice_type_e voice_type);

/**
* @brief Get default speed.
*
* @param[out] speed voice speed.
*
* @return 0 on success, otherwise a negative error value.
* @retval #TTS_SETTING_ERROR_NONE Success.
* @retval #TTS_SETTING_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #TTS_SETTING_ERROR_INVALID_STATE TTS Not initialized. 
* @retval #TTS_SETTING_ERROR_OPERATION_FAILED Operation failure.
*
* @see tts_setting_set_default_speed()
*/
int tts_setting_get_default_speed(tts_setting_speed_e* speed);

/**
* @brief Set a default speed.
*
* @param[in] speed voice speed
*
* @return 0 on success, otherwise a negative error value.
* @retval #TTS_SETTING_ERROR_NONE Success.
* @retval #TTS_SETTING_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #TTS_SETTING_ERROR_INVALID_STATE TTS Not initialized. 
* @retval #TTS_SETTING_ERROR_OPERATION_FAILED Operation failure.
*
* @see tts_setting_get_default_speed()
*/
int tts_setting_set_default_speed(tts_setting_speed_e speed);

/**
* @brief Get setting information of current engine.
*
* @param[in] callback callback function
* @param[in] user_data User data to be passed to the callback function.
*
* @return 0 on success, otherwise a negative error value.
* @retval #TTS_SETTING_ERROR_NONE Success.
* @retval #TTS_SETTING_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #TTS_SETTING_ERROR_INVALID_STATE TTS Not initialized. 
* @retval #TTS_SETTING_ERROR_OPERATION_FAILED Operation failure.
*
* @post	This function invokes tts_setting_engine_setting_cb() repeatedly for getting engine settings. 
*
* @see tts_setting_engine_setting_cb()
*/
int tts_setting_foreach_engine_settings(tts_setting_engine_setting_cb callback, void* user_data);

/**
* @brief Set setting information.
*
* @param[in] key Key.
* @param[in] value Value.
*
* @return 0 on success, otherwise a negative error value.
* @retval #TTS_SETTING_ERROR_NONE Success.
* @retval #TTS_SETTING_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #TTS_SETTING_ERROR_INVALID_STATE TTS Not initialized. 
* @retval #TTS_SETTING_ERROR_OPERATION_FAILED Operation failure.
*
* @see tts_setting_foreach_engine_settings()
*/
int tts_setting_set_engine_setting(const char* key, const char* value);

#ifdef __cplusplus
}
#endif

/**
* @}
*/

#endif /* __TTS_SETTING_H__ */
