/*
 * Copyright (C) 2015-2020 Alibaba Group Holding Limited
 */

#ifndef __AMP_PCM_H__
#define __AMP_PCM_H__


#define AMP_PCM_DIR_OUT		0
#define AMP_PCM_DIR_IN		1

typedef enum {
    AMP_PCM_FORMAT_INVALID = -1,
    AMP_PCM_FORMAT_S16_LE = 0,  /* 16-bit signed */
    AMP_PCM_FORMAT_S32_LE,      /* 32-bit signed */
    AMP_PCM_FORMAT_S8,          /* 8-bit signed */
    AMP_PCM_FORMAT_S24_LE,      /* 24-bits in 4-bytes */
    AMP_PCM_FORMAT_S24_3LE,     /* 24-bits in 3-bytes */
    AMP_PCM_FORMAT_MAX,
} amp_pcm_format_t;

typedef struct {
    int rate;
	int channels;
    int period_size;
    int period_count;
	amp_pcm_format_t format;
} amp_pcm_config_t;

typedef struct {
	amp_pcm_config_t config;
	unsigned int dir:1;
	unsigned int card:2;
	unsigned int state:3;
	void *private_data;
} amp_pcm_device_t;

typedef enum {
	AMP_SND_DEVICE_NONE = 0,
	AMP_SND_DEVICE_MIN = AMP_SND_DEVICE_NONE,
	AMP_SND_DEVICE_OUT_BEGIN,
	AMP_SND_DEVICE_OUT_SPEAKER = AMP_SND_DEVICE_OUT_BEGIN,
	AMP_SND_DEVICE_OUT_HEADPHONE,
	AMP_SND_DEVICE_OUT_SPEAKER_AND_HEADPHONE,
	AMP_SND_DEVICE_OUT_HEADSET,
	AMP_SND_DEVICE_OUT_SPEAKER_AND_HEADSET,
	AMP_SND_DEVICE_OUT_END = AMP_SND_DEVICE_OUT_SPEAKER_AND_HEADSET,
	AMP_SND_DEVICE_IN_BEGIN,
	AMP_SND_DEVICE_IN_PRIMARY_MIC = AMP_SND_DEVICE_IN_BEGIN,
	AMP_SND_DEVICE_IN_SECONDARY_MIC,
	AMP_SND_DEVICE_IN_TERTIARY_MIC,
	AMP_SND_DEVICE_IN_QUATERNARY_MIC,
	AMP_SND_DEVICE_IN_QUINARY_MIC,
	AMP_SND_DEVICE_IN_HEADSET_MIC,
	AMP_SND_DEVICE_IN_END = AMP_SND_DEVICE_IN_HEADSET_MIC,
	AMP_SND_DEVICE_MAX = AMP_SND_DEVICE_IN_END,
} amp_snd_device_t;


/*
 * Set output volume.
 *
 * Return:
 *   0 -- success or unsupport
 *  -1 -- failed
 */
int amp_set_volume(amp_snd_device_t device, int volume);


/*
 * Select audio output/input device
 *
 * Return:
 *   0 -- success or unsupport
 *  -1 -- failed
 */
int amp_set_path(amp_pcm_device_t *pcm, amp_snd_device_t device);


/*
 * Mute audio output/input.
 *
 * Return:
 *   0 -- success or unsupport
 *  -1 -- failed
 */
int amp_dev_mute(amp_pcm_device_t *pcm, amp_snd_device_t device, int mute);


/**
 * Configure pcm parameters
 *
 * Return:
 *   0 -- success
 *  -1 -- failed
 */
int amp_pcm_setup(amp_pcm_device_t *pcm);

/**
 * Open pcm device
 *
 * Return:
 *   0 -- success
 *  -1 -- failed
 */
int amp_pcm_open(amp_pcm_device_t *pcm);

/**
 * Read data in. Block reading if input data not ready.
 *
 * Return read length, or negative if failed.
 */
int amp_pcm_read(amp_pcm_device_t *pcm, unsigned int *buffer, int nbytes);

/*
 * Block write if free dma buffer not ready, otherwise,
 * please return after copied.
 *
 * Return writen length, or negative if failed.
 *
 */
int amp_pcm_write(amp_pcm_device_t *pcm, unsigned int *buffer, int nbytes);

/*
 * Flush remaining data in dma buffer
 *
 * Return:
 *   0 -- success or unsupport
 *  -1 -- failed
 */
int amp_pcm_flush(amp_pcm_device_t *pcm);

/*
 * close pcm device
 *
 * Return:
 *   0 -- success
 *  -1 -- failed
 *
 */
int amp_pcm_close(amp_pcm_device_t *pcm);


#endif
