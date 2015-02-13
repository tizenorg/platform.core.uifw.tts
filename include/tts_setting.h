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


#ifndef __TTS_SETTING_H__
#define __TTS_SETTING_H__

#include <tizen.h>

/**
* @file tts_setting.h
*/

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
	TTS_SETTING_ERROR_NONE			= TIZEN_ERROR_NONE,		/**< Success, No error */
	TTS_SETTING_ERROR_OUT_OF_MEMORY		= TIZEN_ERROR_OUT_OF_MEMORY,	/**< Out of Memory */
	TTS_SETTING_ERROR_IO_ERROR		= TIZEN_ERROR_IO_ERROR,		/**< I/O error */
	TTS_SETTING_ERROR_INVALID_PARAMETER	= TIZEN_ERROR_INVALID_PARAMETER,/**< Invalid parameter */
	TTS_SETTING_ERROR_PERMISSION_DENIED	= TIZEN_ERROR_PERMISSION_DENIED,/**< Permission denied */
	TTS_SETTING_ERROR_NOT_SUPPORTED		= TIZEN_ERROR_NOT_SUPPORTED,	/**< TTS is NOT supported */
	TTS_SETTING_ERROR_INVALID_STATE		= TIZEN_ERROR_TTS | 0x01,	/**< Invalid state */
	TTS_SETTING_ERROR_INVALID_VOICE		= TIZEN_ERROR_TTS | 0x02,	/**< Invalid voice */
	TTS_SETTING_ERROR_ENGINE_NOT_FOUND	= TIZEN_ERROR_TTS | 0x03,	/**< No available TTS-engine  */
	TTS_SETTING_ERROR_OPERATION_FAILED	= TIZEN_ERROR_TTS | 0x04,	/**< Operation failed  */
	TTS_SETTING_ERROR_NOT_SUPPORTED_FEATURE	= TIZEN_ERROR_TTS | 0x06	/**< Not supported feature of current engine */
} tts_setting_error_e;

/** 
* @brief Definitions for male voice type.
*/
#define TTS_SETTING_VOICE_TYPE_MALE	1

/** 
* @brief Definitions for female voice type.
*/
#define TTS_SETTING_VOICE_TYPE_FEMALE	2

/** 
* @brief Definitions for child voice type.
*/
#define TTS_SETTING_VOICE_TYPE_CHILD	3

/**
* @brief Called to get a engine information.
*
* @param[in] engine_id Engine id.
* @param[in] engine_name Engine name.
* @param[in] setting_path Gadget path of engine specific setting.
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
* @param[in] engine_id Engine id
* @param[in] language A language is specified as an ISO 3166 alpha-2 two letter country-code \n
*		followed by ISO 639-1 for the two-letter language code. 
*		For example, "ko_KR" for Korean, "en_US" for American English..
* @param[in] voice_type Voice type
* @param[in] user_data User data passed from the tts_setting_foreach_surpported_voices()
*
* @return @c true to continue with the next iteration of the loop, \n @c false to break out of the loop.
* @pre tts_setting_foreach_surpported_voices() will invoke this callback. 
*
* @see tts_setting_foreach_surpported_voices()
*/
typedef bool(*tts_setting_supported_voice_cb)(const char* engine_id, const char* language, int voice_type, void* user_data);

/**
* @brief Called when the default engine is changed.
*
* @param[in] engine_id Engine id
* @param[in] user_data The user data passed from the callback registration function
*
* @pre An application registers this callback using tts_setting_set_engine_changed_cb().
*
* @see tts_setting_set_engine_changed_cb()
* @see tts_setting_unset_engine_changed_cb()
*/
typedef void (*tts_setting_engine_changed_cb)(const char* engine_id, void *user_data);

/**
* @brief Called when the default voice is changed.
*
* @param[in] language A language
* @param[in] voice_type Voice type
* @param[in] auto_voice Auto voice
* @param[in] user_data The user data passed from the callback registration function
*
* @pre An application registers this callback using registration function.
*
* @see tts_setting_set_voice_changed_cb()
* @see tts_setting_unset_voice_changed_cb()
*/
typedef void (*tts_setting_voice_changed_cb)(const char* language, int voice_type, bool auto_voice, void *user_data);

/**
* @brief Called when the default speed is changed.
*
* @param[in] speed Default speed
* @param[in] user_data The user data passed from the callback registration function
*
* @pre An application registers this callback using registration function.
*
* @see tts_setting_set_speed_changed_cb()
* @see tts_setting_unset_speed_changed_cb()
*/
typedef void (*tts_setting_speed_changed_cb)(int speed, void *user_data);

/**
* @brief Called when the default pitch is changed.
*
* @param[in] pitch Default pitch
* @param[in] user_data The user data passed from the callback registration function
*
* @pre An application registers this callback using registration function.
*
* @see tts_setting_set_pitch_changed_cb()
* @see tts_setting_unset_pitch_changed_cb()
*/
typedef void (*tts_setting_pitch_changed_cb)(int pitch, void *user_data);


/**
* @brief Initialize TTS setting.
*
* @remarks If the function succeeds, @a tts setting must be released with tts_setting_finalize().
*
* @return 0 on success, otherwise a negative error value.
* @retval #TTS_SETTING_ERROR_NONE Success.
* @retval #TTS_SETTING_ERROR_INVALID_STATE TTS setting has Already been initialized.
* @retval #TTS_SETTING_ERROR_OPERATION_FAILED Operation failure.
* @retval #TTS_SETTING_ERROR_NOT_SUPPORTED TTS NOT supported
*
* @see tts_setting_finalize()
*/
int tts_setting_initialize();

/**
* @brief finalize TTS setting. 
*
* @return 0 on success, otherwise a negative error value.
* @retval #TTS_SETTING_ERROR_NONE Success.
* @retval #TTS_SETTING_ERROR_INVALID_STATE Not initialized.
* @retval #TTS_SETTING_ERROR_OPERATION_FAILED Operation failure.
* @retval #TTS_SETTING_ERROR_NOT_SUPPORTED TTS NOT supported
*
* @see tts_setting_initialize()
*/
int tts_setting_finalize(void);

/**
* @brief Retrieve supported engine informations using callback function.
*
* @param[in] callback Callback function
* @param[in] user_data User data to be passed to the callback function
*
* @return 0 on success, otherwise a negative error value.
* @retval #TTS_SETTING_ERROR_NONE Success.
* @retval #TTS_SETTING_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #TTS_SETTING_ERROR_INVALID_STATE Not initialized.
* @retval #TTS_SETTING_ERROR_NOT_SUPPORTED TTS NOT supported
*
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
* @param[out] engine_id Engine id
*
* @return 0 on success, otherwise a negative error value.
* @retval #TTS_SETTING_ERROR_NONE Success.
* @retval #TTS_SETTING_ERROR_OUT_OF_MEMORY Out of memory.
* @retval #TTS_SETTING_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #TTS_SETTING_ERROR_INVALID_STATE Not initialized.
* @retval #TTS_SETTING_ERROR_OPERATION_FAILED Operation failure.
* @retval #TTS_SETTING_ERROR_NOT_SUPPORTED TTS NOT supported
*
* @see tts_setting_set_engine()
*/
int tts_setting_get_engine(char** engine_id);

/**
* @brief Set current engine id.
*
* @param[in] engine_id Engine id
*
* @return 0 on success, otherwise a negative error value.
* @retval #TTS_SETTING_ERROR_NONE Success.
* @retval #TTS_SETTING_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #TTS_SETTING_ERROR_INVALID_STATE Not initialized.
* @retval #TTS_SETTING_ERROR_OPERATION_FAILED Operation failure.
* @retval #TTS_SETTING_ERROR_NOT_SUPPORTED TTS NOT supported
*
* @see tts_setting_get_engine()
*/
int tts_setting_set_engine(const char* engine_id);

/**
* @brief Get supported voices of current engine.
*
* @param[in] callback Callback function
* @param[in] user_data User data to be passed to the callback function
*
* @return 0 on success, otherwise a negative error value.
* @retval #TTS_SETTING_ERROR_NONE Success.
* @retval #TTS_SETTING_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #TTS_SETTING_ERROR_INVALID_STATE Not initialized.
* @retval #TTS_SETTING_ERROR_OPERATION_FAILED Operation failure.
* @retval #TTS_SETTING_ERROR_NOT_SUPPORTED TTS NOT supported
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
* @param[out] language Default language
* @param[out] voice_type Default voice type
*
* @return 0 on success, otherwise a negative error value.
* @retval #TTS_SETTING_ERROR_NONE Success.
* @retval #TTS_SETTING_ERROR_OUT_OF_MEMORY Out of memory.
* @retval #TTS_SETTING_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #TTS_SETTING_ERROR_INVALID_STATE Not initialized.
* @retval #TTS_SETTING_ERROR_OPERATION_FAILED Operation failure.
* @retval #TTS_SETTING_ERROR_NOT_SUPPORTED TTS NOT supported
*
* @see tts_setting_set_voice()
*/
int tts_setting_get_voice(char** language, int* voice_type);

/**
* @brief Set a default voice of current engine.
*
* @param[in] language Default language
* @param[in] voice_type Default voice type
*
* @return 0 on success, otherwise a negative error value.
* @retval #TTS_SETTING_ERROR_NONE Success.
* @retval #TTS_SETTING_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #TTS_SETTING_ERROR_INVALID_STATE Not initialized.
* @retval #TTS_SETTING_ERROR_INVALID_VOICE Invalid voice.
* @retval #TTS_SETTING_ERROR_OPERATION_FAILED Operation failure.
* @retval #TTS_SETTING_ERROR_NOT_SUPPORTED TTS NOT supported
*
* @see tts_setting_get_voice()
*/
int tts_setting_set_voice(const char* language, int voice_type);

/**
* @brief Get a automatic option of voice.
*
* @param[out] value Automatic option
*
* @return 0 on success, otherwise a negative error value.
* @retval #TTS_SETTING_ERROR_NONE Success.
* @retval #TTS_SETTING_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #TTS_SETTING_ERROR_INVALID_STATE Not initialized.
* @retval #TTS_SETTING_ERROR_OPERATION_FAILED Operation failure.
* @retval #TTS_SETTING_ERROR_NOT_SUPPORTED TTS NOT supported
*
* @see tts_setting_set_auto_voice()
*/
int tts_setting_get_auto_voice(bool* value);

/**
* @brief Set a automatic option of voice.
*
* @param[in] value Automatic option
*
* @return 0 on success, otherwise a negative error value.
* @retval #TTS_SETTING_ERROR_NONE Success.
* @retval #TTS_SETTING_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #TTS_SETTING_ERROR_INVALID_STATE Not initialized.
* @retval #TTS_SETTING_ERROR_OPERATION_FAILED Operation failure.
* @retval #TTS_SETTING_ERROR_NOT_SUPPORTED TTS NOT supported
*
* @see tts_setting_get_auto_voice()
*/
int tts_setting_set_auto_voice(bool value);

/**
* @brief Gets the speed range.
*
* @param[out] min The minimun speed value
* @param[out] normal The normal speed value
* @param[out] max The maximum speed value
*
* @return 0 on success, otherwise a negative error value
* @retval #TTS_SETTING_ERROR_NONE Success.
* @retval #TTS_SETTING_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #TTS_SETTING_ERROR_INVALID_STATE Not initialized.
* @retval #TTS_SETTING_ERROR_NOT_SUPPORTED TTS NOT supported
*
* @see tts_setting_get_speed()
* @see tts_setting_set_speed()
*/
int tts_setting_get_speed_range(int* min, int* normal, int* max);

/**
* @brief Get default speed.
*
* @param[out] speed Default voice speed
*
* @return 0 on success, otherwise a negative error value.
* @retval #TTS_SETTING_ERROR_NONE Success.
* @retval #TTS_SETTING_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #TTS_SETTING_ERROR_INVALID_STATE Not initialized.
* @retval #TTS_SETTING_ERROR_OPERATION_FAILED Operation failure.
* @retval #TTS_SETTING_ERROR_NOT_SUPPORTED TTS NOT supported
*
* @see tts_setting_get_speed_range()
* @see tts_setting_set_speed()
*/
int tts_setting_get_speed(int* speed);

/**
* @brief Set default speed.
*
* @param[in] speed Default voice speed
*
* @return 0 on success, otherwise a negative error value.
* @retval #TTS_SETTING_ERROR_NONE Success.
* @retval #TTS_SETTING_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #TTS_SETTING_ERROR_INVALID_STATE Not initialized.
* @retval #TTS_SETTING_ERROR_OPERATION_FAILED Operation failure.
* @retval #TTS_SETTING_ERROR_NOT_SUPPORTED TTS NOT supported
*
* @see tts_setting_get_speed_range()
* @see tts_setting_get_speed()
*/
int tts_setting_set_speed(int speed);

/**
* @brief Gets the pitch range.
*
* @param[out] min The minimun pitch value
* @param[out] normal The normal pitch value
* @param[out] max The maximum pitch value
*
* @return 0 on success, otherwise a negative error value
* @retval #TTS_SETTING_ERROR_NONE Success.
* @retval #TTS_SETTING_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #TTS_SETTING_ERROR_INVALID_STATE Not initialized.
* @retval #TTS_SETTING_ERROR_NOT_SUPPORTED TTS NOT supported
*
* @see tts_setting_get_pitch()
* @see tts_setting_set_pitch()
*/
int tts_setting_get_pitch_range(int* min, int* normal, int* max);

/**
* @brief Set a default pitch.
*
* @param[out] pitch Default voice pitch
*
* @return 0 on success, otherwise a negative error value.
* @retval #TTS_SETTING_ERROR_NONE Success.
* @retval #TTS_SETTING_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #TTS_SETTING_ERROR_INVALID_STATE Not initialized.
* @retval #TTS_SETTING_ERROR_OPERATION_FAILED Operation failure.
* @retval #TTS_SETTING_ERROR_NOT_SUPPORTED_FEATURE Not supported feature.
* @retval #TTS_SETTING_ERROR_NOT_SUPPORTED TTS NOT supported
*
* @see tts_setting_get_pitch_range()
* @see tts_setting_set_pitch()
*/
int tts_setting_get_pitch(int* pitch);

/**
* @brief Set a default pitch.
*
* @param[in] pitch Default voice pitch
*
* @return 0 on success, otherwise a negative error value.
* @retval #TTS_SETTING_ERROR_NONE Success.
* @retval #TTS_SETTING_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #TTS_SETTING_ERROR_INVALID_STATE Not initialized.
* @retval #TTS_SETTING_ERROR_OPERATION_FAILED Operation failure.
* @retval #TTS_SETTING_ERROR_NOT_SUPPORTED_FEATURE Not supported feature.
* @retval #TTS_SETTING_ERROR_NOT_SUPPORTED TTS NOT supported
*
* @see tts_setting_get_pitch_range()
* @see tts_setting_get_pitch()
*/
int tts_setting_set_pitch(int pitch);

/**
* @brief Registers a callback function to be called when engine information is changed
*
* @param[in] callback The callback function to register
* @param[in] user_data The user data to be passed to the callback function
*
* @return 0 on success, otherwise a negative error value
* @retval #TTS_SETTING_ERROR_NONE Successful
* @retval #TTS_SETTING_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #TTS_SETTING_ERROR_OPERATION_FAILED Invalid state
* @retval #TTS_SETTING_ERROR_NOT_SUPPORTED TTS NOT supported
*
* @see tts_setting_engine_changed_cb()
* @see tts_setting_unset_engine_changed_cb()
*/
int tts_setting_set_engine_changed_cb(tts_setting_engine_changed_cb callback, void* user_data);

/**
* @brief Unregisters the callback function
*
* @return 0 on success, otherwise a negative error value
* @retval #TTS_SETTING_ERROR_NONE Successful
* @retval #TTS_SETTING_ERROR_OPERATION_FAILED Invalid state
* @retval #TTS_SETTING_ERROR_NOT_SUPPORTED TTS NOT supported
*
* @see tts_setting_set_engine_changed_cb()
*/
int tts_setting_unset_engine_changed_cb();

/**
* @brief Registers a callback function to be called when default voice is changed
*
* @param[in] callback The callback function to register
* @param[in] user_data The user data to be passed to the callback function
*
* @return 0 on success, otherwise a negative error value
* @retval #TTS_SETTING_ERROR_NONE Successful
* @retval #TTS_SETTING_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #TTS_SETTING_ERROR_OPERATION_FAILED Invalid state
* @retval #TTS_SETTING_ERROR_NOT_SUPPORTED TTS NOT supported
*
* @see tts_setting_voice_changed_cb()
* @see tts_setting_unset_voice_changed_cb()
*/
int tts_setting_set_voice_changed_cb(tts_setting_voice_changed_cb callback, void* user_data);

/**
* @brief Unregisters the callback function
*
* @return 0 on success, otherwise a negative error value
* @retval #TTS_SETTING_ERROR_NONE Successful
* @retval #TTS_SETTING_ERROR_OPERATION_FAILED Invalid state
* @retval #TTS_SETTING_ERROR_NOT_SUPPORTED TTS NOT supported
*
* @see tts_setting_set_voice_changed_cb()
*/
int tts_setting_unset_voice_changed_cb();

/**
* @brief Registers a callback function to be called when default speed is changed
*
* @param[in] callback The callback function to register
* @param[in] user_data The user data to be passed to the callback function
*
* @return 0 on success, otherwise a negative error value
* @retval #TTS_SETTING_ERROR_NONE Successful
* @retval #TTS_SETTING_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #TTS_SETTING_ERROR_OPERATION_FAILED Invalid state
* @retval #TTS_SETTING_ERROR_NOT_SUPPORTED TTS NOT supported
*
* @see tts_setting_speed_changed_cb()
* @see tts_setting_unset_speed_changed_cb()
*/
int tts_setting_set_speed_changed_cb(tts_setting_speed_changed_cb callback, void* user_data);

/**
* @brief Unregisters the callback function
*
* @return 0 on success, otherwise a negative error value
* @retval #TTS_SETTING_ERROR_NONE Successful
* @retval #TTS_SETTING_ERROR_OPERATION_FAILED Invalid state
* @retval #TTS_SETTING_ERROR_NOT_SUPPORTED TTS NOT supported
*
* @see tts_setting_set_speed_changed_cb()
*/
int tts_setting_unset_speed_changed_cb();

/**
* @brief Registers a callback function to be called when default pitch is changed
*
* @param[in] callback The callback function to register
* @param[in] user_data The user data to be passed to the callback function
*
* @return 0 on success, otherwise a negative error value
* @retval #TTS_SETTING_ERROR_NONE Successful
* @retval #TTS_SETTING_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #TTS_SETTING_ERROR_OPERATION_FAILED Invalid state
* @retval #TTS_SETTING_ERROR_NOT_SUPPORTED TTS NOT supported
*
* @see tts_setting_pitch_changed_cb()
* @see tts_setting_unset_pitch_changed_cb()
*/
int tts_setting_set_pitch_changed_cb(tts_setting_pitch_changed_cb callback, void* user_data);

/**
* @brief Unregisters the callback function
*
* @return 0 on success, otherwise a negative error value
* @retval #TTS_SETTING_ERROR_NONE Successful
* @retval #TTS_SETTING_ERROR_OPERATION_FAILED Invalid state
* @retval #TTS_SETTING_ERROR_NOT_SUPPORTED TTS NOT supported
*
* @see tts_setting_set_pitch_changed_cb()
*/
int tts_setting_unset_pitch_changed_cb();


#ifdef __cplusplus
}
#endif

/**
* @}
*/

#endif /* __TTS_SETTING_H__ */
