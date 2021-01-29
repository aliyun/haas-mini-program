/*
 * Copyright (C) 2015-2020 Alibaba Group Holding Limited
 */

#ifndef __AMP_TTS_H__
#define __AMP_TTS_H__

typedef enum {
	TEXT_ENCODE_TYPE_UTF8 = 0,
	TEXT_ENCODE_TYPE_GB2312,
} amp_text_encode_type_t;


/**
 * TTS playback interface
 *
 * @param[in]  text  text to play
 * @param[in]  type  text encode format
 *
 * @return  0 : on success, negative number : if an error occurred with any step
 */
int amp_tts_play(const char *text, amp_text_encode_type_t type);

/**
 * TTS playback stop interface
 *
 * @return  0 : on success, negative number : if an error occurred with any step
 */
int amp_tts_stop(void);

/**
 * TTS state get interface
 *
 * @param[in]  volume  memory address to store state value
 *
 * @return  0 : on success, negative number : if an error occurred with any step
 */
int amp_tts_state_get(int *state);

/**
 * TTS playback volume set interface
 *
 * @param[in]  volume  volume level
 *
 * @return  0 : on success, negative number : if an error occurred with any step
 */
int amp_tts_volume_set(int volume);

/**
 * TTS playback volume get interface
 *
 * @param[in]  volume  memory address to store volume value
 *
 * @return  0 : on success, negative number : if an error occurred with any step
 */
int amp_tts_volume_get(int *volume);

/**
 * TTS playback pitch set interface
 *
 * @param[in]  pitch  pitch setting
 *
 * @return  0 : on success, negative number : if an error occurred with any step
 */
int amp_tts_pitch_set(int pitch);

/**
 * TTS playback speed set interface
 *
 * @param[in]  speed  speed level
 *
 * @return  0 : on success, negative number : if an error occurred with any step
 */
int amp_tts_speed_set(int speed);

/**
 * TTS playback speed get interface
 *
 * @param[in]  speed  memory address to store speed value
 *
 * @return  0 : on success, negative number : if an error occurred with any step
 */
int amp_tts_speed_get(int *speed);

/**
 * Get tts playing state
 *
 * @return  0 : no tts playing, 1 : tts playing
 */
int amp_tts_is_playing(void);

/**
 * TTS system init
 *
 * @return  0 : on success, negative number : if an error occurred with any step
 */
int amp_tts_init(void);


#endif /* __AMP_TTS_H__ */
