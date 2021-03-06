/*
 * es515.h  --	Audience eS515 ALSA SoC Audio driver
 *
 * Copyright 2013 Audience, Inc.
 *
 * Author:
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _ES515_H
#define _ES515_H

/*******************************************************************************
 * Include header files
 ******************************************************************************/

#include <sound/soc.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <sound/jack.h>
#include <linux/i2c.h>
#include <linux/firmware.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/i2c/esxxx.h> /* TODO: common location for i2c and slimbus */

/*******************************************************************************
 * Macro definitions
 ******************************************************************************/
#define MAX_MACRO_LEN			128 /* In Bytes */

#define ES515_READ_VE_OFFSET		0x0804
#define ES515_READ_VE_WIDTH		4
#define ES515_WRITE_VE_OFFSET		0x0800
#define ES515_WRITE_VE_WIDTH		4

#define ES515_MCLK_DIV			0x0000
#define ES515_CLASSD_CLK_DIV		0x0001
#define ES515_CP_CLK_DIV		0x0002

#define ES515_BOOT_CMD			0x0001
#define ES515_BOOT_ACK			0x0101

#define ES515_SYNC_CMD			0x8000
#define ES515_SYNC_POLLING		0x0000
#define ES515_SYNC_INTR_ACITVE_LOW	0x0001
#define ES515_SYNC_INTR_ACITVE_HIGH	0x0002
#define ES515_SYNC_INTR_FALLING_EDGE	0x0003
#define ES515_SYNC_INTR_RISING_EDGE	0x0004
#define ES515_SYNC_ACK			0x00000000

#define ES515_RESET_CMD			0x8002
#define ES515_RESET_IMMED		0x0000
#define ES515_RESET_DELAYED		0x0001

#define ES515_SET_POWER_STATE		0x8010
#define ES515_SET_POWER_STATE_SLEEP	0x0001
#define ES515_SET_POWER_STATE_MP_SLEEP	0x0002
#define ES515_SET_POWER_STATE_MP_CMD	0x0003
#define ES515_SET_POWER_STATE_NORMAL	0x0004

#define ES515_SET_SMOOTH 0x904E
#define ES515_SET_SMOOTH_RATE 0x0000
/*
 * bit15 - reserved
 * bit[14:12] - access type
 * bit11 - commit = 0, staged = 1
 * bit[10:0] - psuedo address
 */
#define ES515_ACCESS_MASK	(7 << 12)
#define ES515_ALGO_ACCESS	(0 << 12)
#define ES515_DEV_ACCESS	(1 << 12)
#define ES515_CMD_ACCESS	(2 << 12)
#define ES515_OTHER_ACCESS	(3 << 12)

#define ES515_CMD_MASK		(1 << 11)
#define ES515_STAGED_CMD	(1 << 11)
#define ES515_COMMIT_CMD	(0 << 11)

#define ES515_ADDR_MASK		0x7ff

#define ES515_STAGED_MSG_BIT	(1 << 13)
#define ES515_SR_BIT_MASK	(1 << 4)
/*
 * Device parameter command codes
 */
#define ES515_DEV_PARAM_OFFSET		0x2000
#define ES515_GET_DEV_PARAM		0x800b
#define ES515_SET_DEV_PARAMID		0x800c
#define ES515_SET_DEV_PARAM		0x800d

/*
 * Algoithm parameter command codes
 */
#define ES515_ALGO_PARAM_OFFSET		0x0000
#define ES515_GET_ALGO_PARAM		0x8016
#define ES515_SET_ALGO_PARAMID		0x8017
#define ES515_SET_ALGO_PARAM		0x8018

#if 1	/* yman, 24000 removed */
#define ES515_RATES (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_11025 |\
		     SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_22050 |\
		     SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 |\
		     SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_96000 |\
		     SNDRV_PCM_RATE_192000)
#else
#define ES515_RATES (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_11025 |\
		     SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_22050 |\
		     SNDRV_PCM_RATE_24000 | SNDRV_PCM_RATE_32000 |\
		     SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000 |\
		     SNDRV_PCM_RATE_96000 | SNDRV_PCM_RATE_192000)
#endif
#define ES515_SLIMBUS_RATES (SNDRV_PCM_RATE_8000|\
			     SNDRV_PCM_RATE_16000|\
			     SNDRV_PCM_RATE_48000)

#define ES515_FORMATS (SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_U8 |\
			SNDRV_PCM_FMTBIT_S16_LE	| SNDRV_PCM_FMTBIT_S16_BE |\
			SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S20_3BE |\
			SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S24_BE |\
			SNDRV_PCM_FMTBIT_S32_LE | SNDRV_PCM_FMTBIT_S32_BE)
#define ES515_SLIMBUS_FORMATS (SNDRV_PCM_FMTBIT_S16_LE |\
			       SNDRV_PCM_FMTBIT_S16_BE)

#if defined(CONFIG_SND_SOC_ES515_I2C)
#define ES515_BUS_READ(x_es515, x_buf, x_len, x_bus_order) \
	es515_i2c_read(x_es515, x_buf, x_len)
#define ES515_BUS_WRITE(x_es515, x_buf, x_len, x_bus_order) \
	es515_i2c_write(x_es515, x_buf, x_len)
#endif


#define ES515_CMD_ACCESS_WR_MAX 4
#define ES515_CMD_ACCESS_RD_MAX 4

#define ES515_INTERNAL_ROUTE_MAX ARRAY_SIZE(es515_def_route_configs)
#define ES515_PRESET_MODE_MAX 255

#define ES515_FW_LOAD_BUF_SZ 16

#define ES515_MAX_INTR_STATUS_BITS	14

#undef ES515_JACK_DETECTION
/*******************************************************************************
 * Structure declarations
 ******************************************************************************/
struct es515_micdet {
	struct snd_soc_jack *jack;
	int det;
	int shrt;
	int asd;
};

struct es515_cmd_access {
	u8 read_msg[ES515_CMD_ACCESS_RD_MAX];
	unsigned int read_msg_len;
	u8 write_msg[ES515_CMD_ACCESS_WR_MAX];
	unsigned int write_msg_len;
	unsigned int val_shift;
	unsigned int val_max;
};

struct es515_priv {
	struct snd_soc_codec *codec;
	const struct firmware *fw;
	struct es515_micdet micdet[2];
	struct esxxx_platform_data *pdata;
	struct i2c_client *this_client;
	int intr_type;
	struct snd_soc_jack *jack;
#if defined (CONFIG_SND_SOC_ES515_SLIMBUS)
	struct slim_device *intf_client;
	struct slim_device *gen0_client;
	struct es515_slim_dai_data dai[ES515_NUM_CODEC_SLIM_DAIS];
	struct es515_slim_ch slim_rx[ES515_SLIM_RX_PORTS];
	struct es515_slim_ch slim_tx[ES515_SLIM_TX_PORTS];
#endif
};

/*******************************************************************************
 * Enum declarations
 ******************************************************************************/
enum es515_interrupt_type {
	ES515_POLLING = 0x0,
	ES515_LEVEL_LOW,
	ES515_LEVEL_HIGH,
	ES515_FALLING_EDGE,
	ES515_RISING_EDGE,
};

enum es515_power_state {
	ES515_POWER_STATE_SLEEP,
	ES515_POWER_STATE_MP_SLEEP,
	ES515_POWER_STATE_MP_COMMAND,
	ES515_POWER_STATE_NORMAL,
};

enum es515_codec_ports {
	ES515_CODEC_PORT_EARPIECE,
	ES515_CODEC_PORT_HEADPHONE,
	ES515_CODEC_PORT_SPEAKER,
	ES515_CODEC_PORT_LINEOUT,
	ES515_CODEC_PORT_INVALID,
};
/*
 * Address in enum
 * For stagged or with ES515_STAGED_CMD
 */
enum es515_algo_paramid_index {
	ES515_MIC_CONFIG = 0x0000,
	ES515_AEC_MODE,
	ES515_TX_AGC,
	ES515_TX_AGC_TARGET_LEVEL,
	ES515_TX_AGC_NOISE_FLOOR,
	ES515_TX_AGC_SNR_IMPROVEMENT,
	ES515_VEQ_ENABLE,
	ES515_RX_OUT_LIMITER_MAX_LEVEL,
	ES515_RX_NOISE_SUPPRESS,
	ES515_RX_STS,
	ES515_RX_STS_RATE,
	ES515_AEC_SPEAKER_VOLUME,
	ES515_SIDETONE,
	ES515_SIDETONE_GAIN,
	ES515_TX_COMFORT_NOISE,
	ES515_TX_COMFORT_NOISE_LEVEL,
	ES515_ALGORITHM_RESET,
	ES515_RX_POST_EQ,
	ES515_TX_POST_EQ,
	ES515_AEC_CNG,
	ES515_VEQ_NOISE_ESTIMATION_ADJUSTMENT,
	ES515_TX_AGC_SLEW_RATE_UP,
	ES515_TX_AGC_SLEW_RATE_DOWN,
	ES515_RX_AGC,
	ES515_RX_AGC_TARGET_LEVEL,
	ES515_RX_AGC_NOISE_FLOOR,
	ES515_RX_AGC_SNR_IMPROVEMENT,
	ES515_RX_AGC_SLEW_RATE_UP,
	ES515_RX_AGC_SLEW_RATE_DOWN,
	ES515_AEC_CNG_GAIN,
	ES515_TX_MBC,
	ES515_RX_MBC,
	ES515_AEC_ESE,
	ES515_TX_NS_ADAPTATION_SPEED,
	ES515_TX_SNR_ESTIMATE,
	ES515_VEQ_MAX_GAIN,
	ES515_TX_AGC_GUARDBAND,
	ES515_RX_AGC_GUARDBAND,
	ES515_TX_OUT_LIMITER_MAX_LEVEL,
	ES515_TX_IN_LIMITER_MAX_LEVEL,
	ES515_RX_NS_ADAPTATION_SPEED,
	ES515_AEC_VARIABLE_ECHO_REF_DELAY,
	ES515_TX_NOISE_SUPPRESS_LEVEL,
	ES515_RX_NOISE_SUPPRESS_LEVEL,
	ES515_RX_CNG,
	ES515_RX_CNG_GAIN,

	/** Newly Added **/
	ES515_BWE_ENABLE,
	ES515_BWE_HIGH_BAND_GAIN,
	ES515_BWE_MAX_SNR,
	ES515_BWE_POST_EQ_ENABLE,
	ES515_DEREVERB_ENABLE,
	ES515_DEREVERB_GAIN,

	ES515_TX_AGC_MAX_GAIN,
	ES515_RX_AGC_MAX_GAIN,
	ES515_AVALON_API_VERSION_LO,
	ES515_AVALON_API_VERSION_HI,
	ES515_AVALON_AV_PROCESSOR,
	ES515_AVALON_AV_CONFIG,
	ES515_AVALON_EQ_PRESET,
	ES515_AVALON_STEREO_WIDTH,
	ES515_AVALON_AV_DIGITAL_OUT_GAIN,
	ES515_AVALON_TDMBC,
	ES515_AVALON_AV_OUT_LIMIT,
	ES515_AVALON_STEREO_WIDENING,
	ES515_AVALON_STAT_NS,
	ES515_AVALON_STAT_NS_SUPPRESS,
	ES515_AVALON_STAT_NS_ADAP_SPEED,
	ES515_AVALON_STAT_NS_MODE,
	ES515_AVLALON_STAT_NS_MAX_NOISE_ENERGY,
	ES515_AVALON_VBB,
	ES515_AVALON_VBB_STRENGTH,
	ES515_AVALON_EQ_MODE,
	ES515_AVALON_EQ_GRAPHIC_BAND1_GAIN,
	ES515_AVALON_EQ_GRAPHIC_BAND2_GAIN,
	ES515_AVALON_EQ_GRAPHIC_BAND3_GAIN,
	ES515_AVALON_EQ_GRAPHIC_BAND4_GAIN,
	ES515_AVALON_EQ_GRAPHIC_BAND5_GAIN,
	ES515_AVALON_EQ_GRAPHIC_BAND6_GAIN,
	ES515_AVALON_EQ_GRAPHIC_BAND7_GAIN,
	ES515_AVALON_EQ_GRAPHIC_BAND8_GAIN,
	ES515_AVALON_EQ_GRAPHIC_BAND9_GAIN,
	ES515_AVALON_EQ_GRAPHIC_BAND10_GAIN,
	ES515_AVALON_TDDRC,
	ES515_AVALON_TDDRC_STRENGTH,
	ES515_AVALON_LIMITER,
	ES515_AVALON_EQ,
	ES515_DIRAC,
	ES515_DIRAC_OUT_HEADROOM_LIMITER,
	ES515_DIRAC_MODE,
	ES515_DIRAC_IN_HEADROOM_LIMITER,
	ES515_DIRAC_COMFORT_NOISE,
	ES515_DIRAC_COMFORT_NOISE_LEVEL,
	ES515_DIRAC_NARRATOR_VQOS,
	ES515_DIRAC_NARRATOR_POSITION_SUPPRESS,
	ES515_DIRAC_NARRATOR_AGC_OUT,
	ES515_DIRAC_NARRATOR_AGC_SPEECH_TARGET,
	ES515_DIRAC_NARRATOR_AGC_SNR_IMPROVE,
	ES515_DIRAC_NARRATOR_AGC_NOISE_FLOOR,
	ES515_DIRAC_NARRATOR_AGC_MAX_GAIN,
	ES515_DIRAC_NARRATOR_AGC_UP_RATE,
	ES515_DIRAC_NARRATOR_AGC_DOWN_RATE,
	ES515_DIRAC_NARRATOR_AGC_GUARDBAND,
	ES515_DIRAC_NARRATOR_POST_EQ_MODE,
	ES515_DIRAC_NARRATOR_MBC_MODE,
	ES515_DIRAC_SCENE_BEAM_WIDTH,
	ES515_DIRAC_SCENE_AGC_OUT,
	ES515_DIRAC_SCENE_AGC_SPEECH_TARGET,
	ES515_DIRAC_SCENE_AGC_SNR_IMPROVE,
	ES515_DIRAC_SCENE_AGC_NOISE_FLOOR,
	ES515_DIRAC_SCENE_AGC_MAX_GAIN,
	ES515_DIRAC_SCENE_AGC_UP_RATE,
	ES515_DIRAC_SCENE_AGC_DOWN_RATE,
	ES515_DIRAC_SCENE_AGC_GUARDBAND,
	ES515_DIRAC_SCENE_VQOS,
	ES515_DIRAC_SCENE_POST_EQ_MODE,
	ES515_DIRAC_SCENE_MBC_MODE,
	ES515_TONE_PARAM_API_VERSION_LO,
	ES515_TONE_PARAM_API_VERSION_HI,
	ES515_TONE_PARAM_ENABLE_BEEP_SYS,
	ES515_TONE_PARAM_ENABLE_GEN_BEEP,
	ES515_TONE_PARAM_GEN_BEEP_ON,
	ES515_TONE_PARAM_GEN_BEEP_FREQ1,
	ES515_TONE_PARAM_GEN_BEEP_FREQ2,
	ES515_TONE_PARAM_GEN_BEEP_PAN_LR,
	ES515_TONE_PARAM_GEN_BEEP_GAIN,
};

enum es515_dev_paramid_index {
	ES515_PORTA_WORD_LEN = 0x1000,
	ES515_PORTA_TDM_SLOTS_PER_FRAME,
	ES515_PORTA_TX_DELAY_FROM_FS,
	ES515_PORTA_RX_DELAY_FROM_FS,
	ES515_PORTA_LATCH_EDGE,
	ES515_PORTA_ENDIAN,
	ES515_PORTA_TRISTATE,
	ES515_PORTA_AUDIO_PORT_MODE,
	ES515_PORTA_TDM_ENABLED,
	ES515_PORTA_CLOCK_CONTROL,
	ES515_PORTA_DATA_JUSTIFICATION,
	ES515_PORTA_FS_DURATION,
	ES515_PORTB_WORD_LEN,
	ES515_PORTB_TDM_SLOTS_PER_FRAME,
	ES515_PORTB_TX_DELAY_FROM_FS,
	ES515_PORTB_RX_DELAY_FROM_FS,
	ES515_PORTB_LATCH_EDGE,
	ES515_PORTB_ENDIAN,
	ES515_PORTB_TRISTATE,
	ES515_PORTB_AUDIO_PORT_MODE,
	ES515_PORTB_TDM_ENABLED,
	ES515_PORTB_CLOCK_CONTROL,
	ES515_PORTB_DATA_JUSTIFICATION,
	ES515_PORTB_FS_DURATION,
	ES515_PORTC_WORD_LEN,
	ES515_PORTC_TDM_SLOTS_PER_FRAME,
	ES515_PORTC_TX_DELAY_FROM_FS,
	ES515_PORTC_RX_DELAY_FROM_FS,
	ES515_PORTC_LATCH_EDGE,
	ES515_PORTC_ENDIAN,
	ES515_PORTC_TRISTATE,
	ES515_PORTC_AUDIO_PORT_MODE,
	ES515_PORTC_TDM_ENABLED,
	ES515_PORTC_CLOCK_CONTROL,
	ES515_PORTC_DATA_JUSTIFICATION,
	ES515_PORTC_FS_DURATION,
	ES515_PORTD_WORD_LEN,
	ES515_PORTD_TDM_SLOTS_PER_FRAME,
	ES515_PORTD_TX_DELAY_FROM_FS,
	ES515_PORTD_RX_DELAY_FROM_FS,
	ES515_PORTD_LATCH_EDGE,
	ES515_PORTD_ENDIAN,
	ES515_PORTD_TRISTATE,
	ES515_PORTD_AUDIO_PORT_MODE,
	ES515_PORTD_TDM_ENABLED,
	ES515_PORTD_CLOCK_CONTROL,
	ES515_PORTD_DATA_JUSTIFICATION,
	ES515_PORTD_FS_DURATION,
	ES515_SLIMBUS_LINK_MULTI_CHANNEL,
};



/*
 * addresses
 */
enum es515_cmd_access_index {
	ES515_POWER_STATE = 0x2000,
	ES515_STREAMING,
	ES515_FE_STREAMING,
	ES515_PRESET,
	ES515_DIGITAL_GAIN,
	ES515_ALGO_STATS,
	ES515_ALGO_PROCESSING,
	ES515_ALGO_SAMPLE_RATE,
	ES515_SMOOTH_RATE,
	ES515_CHANGE_STATUS,
	ES515_DIGITAL_PASS_THROUGH,
	ES515_DATA_PATH,
	ES515_ALGORITHM,
	ES515_MIX_SAMPLE_RATE,
	ES515_SIGNAL_RMS_PORTA_DIN_LEFT,
	ES515_SIGNAL_RMS_PORTA_DIN_RIGHT,
	ES515_SIGNAL_RMS_PORTA_DOUT_LEFT,
	ES515_SIGNAL_RMS_PORTA_DOUT_RIGHT,
	ES515_SIGNAL_RMS_PORTB_DIN_LEFT,
	ES515_SIGNAL_RMS_PORTB_DIN_RIGHT,
	ES515_SIGNAL_RMS_PORTB_DOUT_LEFT,
	ES515_SIGNAL_RMS_PORTB_DOUT_RIGHT,
	ES515_SIGNAL_RMS_PORTC_DIN_LEFT,
	ES515_SIGNAL_RMS_PORTC_DIN_RIGHT,
	ES515_SIGNAL_RMS_PORTC_DOUT_LEFT,
	ES515_SIGNAL_RMS_PORTC_DOUT_RIGHT,
	ES515_SIGNAL_RMS_PORTD_DIN_LEFT,
	ES515_SIGNAL_RMS_PORTD_DIN_RIGHT,
	ES515_SIGNAL_RMS_PORTD_DOUT_LEFT,
	ES515_SIGNAL_RMS_PORTD_DOUT_RIGHT,
	ES515_SIGNAL_PEAK_PORTA_DIN_LEFT,
	ES515_SIGNAL_PEAK_PORTA_DIN_RIGHT,
	ES515_SIGNAL_PEAK_PORTA_DOUT_LEFT,
	ES515_SIGNAL_PEAK_PORTA_DOUT_RIGHT,
	ES515_SIGNAL_PEAK_PORTB_DIN_LEFT,
	ES515_SIGNAL_PEAK_PORTB_DIN_RIGHT,
	ES515_SIGNAL_PEAK_PORTB_DOUT_LEFT,
	ES515_SIGNAL_PEAK_PORTB_DOUT_RIGHT,
	ES515_SIGNAL_PEAK_PORTC_DIN_LEFT,
	ES515_SIGNAL_PEAK_PORTC_DIN_RIGHT,
	ES515_SIGNAL_PEAK_PORTC_DOUT_LEFT,
	ES515_SIGNAL_PEAK_PORTC_DOUT_RIGHT,
	ES515_SIGNAL_PEAK_PORTD_DIN_LEFT,
	ES515_SIGNAL_PEAK_PORTD_DIN_RIGHT,
	ES515_SIGNAL_PEAK_PORTD_DOUT_LEFT,
	ES515_SIGNAL_PEAK_PORTD_DOUT_RIGHT,
	ES515_DIGITAL_GAIN_PRIMARY,
	ES515_DIGITAL_GAIN_SECONDARY,
	ES515_DIGITAL_GAIN_TERTIARY,
	ES515_DIGITAL_GAIN_QUAD,
	ES515_DIGITAL_GAIN_FEIN,
	ES515_DIGITAL_GAIN_AUDIN1,
	ES515_DIGITAL_GAIN_AUDIN2,
	ES515_DIGITAL_GAIN_AUDIN3,
	ES515_DIGITAL_GAIN_AUDIN4,
	ES515_DIGITAL_GAIN_UITONE1,
	ES515_DIGITAL_GAIN_UITONE2,
	ES515_DIGITAL_GAIN_CSOUT,
	ES515_DIGITAL_GAIN_FEOUT1,
	ES515_DIGITAL_GAIN_FEOUT2,
	ES515_DIGITAL_GAIN_AUDOUT1,
	ES515_DIGITAL_GAIN_AUDOUT2,
	ES515_DIGITAL_GAIN_AUDOUT3,
	ES515_DIGITAL_GAIN_AUDOUT4,
	ES515_PRIMARY_PATH_MUX,
	ES515_SECONDARY_PATH_MUX,
	ES515_TERTIARY_PATH_MUX,
	ES515_QUAD_PATH_MUX,
	ES515_FEIN_PATH_MUX,
	ES515_AUDIN1_PATH_MUX,
	ES515_AUDIN2_PATH_MUX,
	ES515_AUDIN3_PATH_MUX,
	ES515_AUDIN4_PATH_MUX,
	ES515_UITONE1_PATH_MUX,
	ES515_UITONE2_PATH_MUX,
	ES515_PCM0_0_PATH_MUX,
	ES515_PCM0_1_PATH_MUX,
	ES515_PCM0_2_PATH_MUX,
	ES515_PCM0_3_PATH_MUX,
	ES515_PCM0_4_PATH_MUX,
	ES515_PCM0_5_PATH_MUX,
	ES515_PCM0_6_PATH_MUX,
	ES515_PCM0_7_PATH_MUX,
	ES515_PCM0_8_PATH_MUX,
	ES515_PCM0_9_PATH_MUX,
	ES515_PCM0_10_PATH_MUX,
	ES515_PCM0_11_PATH_MUX,
	ES515_PCM0_12_PATH_MUX,
	ES515_PCM0_13_PATH_MUX,
	ES515_PCM0_14_PATH_MUX,
	ES515_PCM0_15_PATH_MUX,
	ES515_PCM0_16_PATH_MUX,
	ES515_PCM0_17_PATH_MUX,
	ES515_PCM0_18_PATH_MUX,
	ES515_PCM0_19_PATH_MUX,
	ES515_PCM0_20_PATH_MUX,
	ES515_PCM0_21_PATH_MUX,
	ES515_PCM0_22_PATH_MUX,
	ES515_PCM0_23_PATH_MUX,
	ES515_PCM0_24_PATH_MUX,
	ES515_PCM0_25_PATH_MUX,
	ES515_PCM0_26_PATH_MUX,
	ES515_PCM0_27_PATH_MUX,
	ES515_PCM0_28_PATH_MUX,
	ES515_PCM0_29_PATH_MUX,
	ES515_PCM0_30_PATH_MUX,
	ES515_PCM0_31_PATH_MUX,
	ES515_PCM1_0_PATH_MUX,
	ES515_PCM1_1_PATH_MUX,
	ES515_PCM1_2_PATH_MUX,
	ES515_PCM1_3_PATH_MUX,
	ES515_PCM1_4_PATH_MUX,
	ES515_PCM1_5_PATH_MUX,
	ES515_PCM1_6_PATH_MUX,
	ES515_PCM1_7_PATH_MUX,
	ES515_PCM1_8_PATH_MUX,
	ES515_PCM1_9_PATH_MUX,
	ES515_PCM1_10_PATH_MUX,
	ES515_PCM1_11_PATH_MUX,
	ES515_PCM1_12_PATH_MUX,
	ES515_PCM1_13_PATH_MUX,
	ES515_PCM1_14_PATH_MUX,
	ES515_PCM1_15_PATH_MUX,
	ES515_PCM1_16_PATH_MUX,
	ES515_PCM1_17_PATH_MUX,
	ES515_PCM1_18_PATH_MUX,
	ES515_PCM1_19_PATH_MUX,
	ES515_PCM1_20_PATH_MUX,
	ES515_PCM1_21_PATH_MUX,
	ES515_PCM1_22_PATH_MUX,
	ES515_PCM1_23_PATH_MUX,
	ES515_PCM1_24_PATH_MUX,
	ES515_PCM1_25_PATH_MUX,
	ES515_PCM1_26_PATH_MUX,
	ES515_PCM1_27_PATH_MUX,
	ES515_PCM1_28_PATH_MUX,
	ES515_PCM1_29_PATH_MUX,
	ES515_PCM1_30_PATH_MUX,
	ES515_PCM1_31_PATH_MUX,
	ES515_PCM2_0_PATH_MUX,
	ES515_PCM2_1_PATH_MUX,
	ES515_PCM2_2_PATH_MUX,
	ES515_PCM2_3_PATH_MUX,
	ES515_PCM2_4_PATH_MUX,
	ES515_PCM2_5_PATH_MUX,
	ES515_PCM2_6_PATH_MUX,
	ES515_PCM2_7_PATH_MUX,
	ES515_PCM2_8_PATH_MUX,
	ES515_PCM2_9_PATH_MUX,
	ES515_PCM2_10_PATH_MUX,
	ES515_PCM2_11_PATH_MUX,
	ES515_PCM2_12_PATH_MUX,
	ES515_PCM2_13_PATH_MUX,
	ES515_PCM2_14_PATH_MUX,
	ES515_PCM2_15_PATH_MUX,
	ES515_PCM2_16_PATH_MUX,
	ES515_PCM2_17_PATH_MUX,
	ES515_PCM2_18_PATH_MUX,
	ES515_PCM2_19_PATH_MUX,
	ES515_PCM2_20_PATH_MUX,
	ES515_PCM2_21_PATH_MUX,
	ES515_PCM2_22_PATH_MUX,
	ES515_PCM2_23_PATH_MUX,
	ES515_PCM2_24_PATH_MUX,
	ES515_PCM2_25_PATH_MUX,
	ES515_PCM2_26_PATH_MUX,
	ES515_PCM2_27_PATH_MUX,
	ES515_PCM2_28_PATH_MUX,
	ES515_PCM2_29_PATH_MUX,
	ES515_PCM2_30_PATH_MUX,
	ES515_PCM2_31_PATH_MUX,
	ES515_PCM3_0_PATH_MUX,
	ES515_PCM3_1_PATH_MUX,
	ES515_PCM3_2_PATH_MUX,
	ES515_PCM3_3_PATH_MUX,
	ES515_PCM3_4_PATH_MUX,
	ES515_PCM3_5_PATH_MUX,
	ES515_PCM3_6_PATH_MUX,
	ES515_PCM3_7_PATH_MUX,
	ES515_PCM3_8_PATH_MUX,
	ES515_PCM3_9_PATH_MUX,
	ES515_PCM3_10_PATH_MUX,
	ES515_PCM3_11_PATH_MUX,
	ES515_PCM3_12_PATH_MUX,
	ES515_PCM3_13_PATH_MUX,
	ES515_PCM3_14_PATH_MUX,
	ES515_PCM3_15_PATH_MUX,
	ES515_PCM3_16_PATH_MUX,
	ES515_PCM3_17_PATH_MUX,
	ES515_PCM3_18_PATH_MUX,
	ES515_PCM3_19_PATH_MUX,
	ES515_PCM3_20_PATH_MUX,
	ES515_PCM3_21_PATH_MUX,
	ES515_PCM3_22_PATH_MUX,
	ES515_PCM3_23_PATH_MUX,
	ES515_PCM3_24_PATH_MUX,
	ES515_PCM3_25_PATH_MUX,
	ES515_PCM3_26_PATH_MUX,
	ES515_PCM3_27_PATH_MUX,
	ES515_PCM3_28_PATH_MUX,
	ES515_PCM3_29_PATH_MUX,
	ES515_PCM3_30_PATH_MUX,
	ES515_PCM3_31_PATH_MUX,
	ES515_SBUS_TX0_PATH_MUX,
	ES515_SBUS_TX1_PATH_MUX,
	ES515_SBUS_TX2_PATH_MUX,
	ES515_SBUS_TX3_PATH_MUX,
	ES515_SBUS_TX4_PATH_MUX,
	ES515_SBUS_TX5_PATH_MUX,
	ES515_FLUSH,
	ES515_BEEP_GENERATION,
	ES515_GET_SYS_INTERRUPT_STATUS,
	ES515_SYS_INTERRUPT_MASK,
	ES515_CLEAR_SYS_INTERRUPT_STATUS,
	ES515_ACCESSORY_DET_CONFIG,
	ES515_ACCESSORY_DET_STATUS,
	ES515_OUTPUT_KNOWN_SIG,
	ES515_DEVICE_PARAM_ID,
	ES515_DEVICE_PARAM,
	ES515_ALGORITHM_PARAM_ID,
	ES515_ALGORITHM_PARAM,
	ES515_SYNC_CONTROL_MODE,
	ES515_SOFT_RESET,
	ES515_BOOTLOAD_INIT,
	ES515_ANALOG_PASS_THROUGH,
	ES515_MP3_MODE,
	ES515_SET_CODEC_ADDR,
	ES515_CODEC_VAL,
	ES515_COPY_CODEC,
	ES515_DELETE_CODEC,
	ES515_SWITCH_CODEC,
};

#ifdef ES515_JACK_DETECTION
int es515_jack_assign(struct snd_soc_codec *codec);
void es515_jack_remove(struct snd_soc_codec *codec);
#endif

#endif /* _ES515_H */
