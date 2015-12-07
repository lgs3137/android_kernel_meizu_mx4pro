/*
 *  espresso_es515.c
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/module.h>
#include <linux/of.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/iio/consumer.h>
#include <linux/iio/driver.h>

#include <sound/soc.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/jack.h>
#include <sound/control.h>

#include <mach/regs-pmu.h>

#include "i2s.h"
#include "i2s-regs.h"
#include "spdif.h"
#include "../codecs/es515.h"

/* ESPRESSO use CLKOUT from AP */
#define ESPRESSO_MCLK_FREQ		24000000
#define ESPRESSO_AUD_PLL_FREQ	196608009

static bool clkout_enabled;
static struct snd_soc_card espresso;

static void espresso_enable_mclk(bool on)
{
	pr_debug("%s: %s\n", __func__, on ? "on" : "off");

	clkout_enabled = on;
	writel(on ? 0x1000 : 0x1001, EXYNOS_PMU_DEBUG);
}

#if !defined(CONFIG_SND_SOC_ES515_I2S_MASTER) \
	|| defined(CONFIG_SND_SAMSUNG_AUX_HDMI) \
	|| defined(CONFIG_SND_SAMSUNG_AUX_SPDIF)
static int set_aud_pll_rate(unsigned long rate)
{
	struct clk *fout_aud_pll;

	fout_aud_pll = clk_get(espresso.dev, "fout_aud_pll");
	if (IS_ERR(fout_aud_pll)) {
		printk(KERN_ERR "%s: failed to get fout_aud_pll\n", __func__);
		return PTR_ERR(fout_aud_pll);
	}

	if (rate == clk_get_rate(fout_aud_pll))
		goto out;

	rate += 20;		/* margin */
	clk_set_rate(fout_aud_pll, rate);
	pr_debug("%s: aud_pll rate = %ld\n",
		__func__, clk_get_rate(fout_aud_pll));
out:
	clk_put(fout_aud_pll);

	return 0;
}
#endif
#ifdef CONFIG_SND_SOC_ES515_I2S_MASTER
/*
 * ESPRESSO eS515 I2S DAI operations. (Codec master)
 */
static int espresso_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int ret;

	/* CLKOUT(XXTI) for eS515 MCLK */
	espresso_enable_mclk(true);

	/* Set Codec DAI configuration */
	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S
					 | SND_SOC_DAIFMT_NB_NF
					 | SND_SOC_DAIFMT_CBM_CFM);
	if (ret < 0)
		return ret;

	/* Set CPU DAI configuration */
	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S
					 | SND_SOC_DAIFMT_NB_NF
					 | SND_SOC_DAIFMT_CBM_CFM);
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_sysclk(cpu_dai, SAMSUNG_I2S_CDCLK,
					0, SND_SOC_CLOCK_IN);
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_sysclk(cpu_dai, SAMSUNG_I2S_OPCLK,
					0, MOD_OPCLK_PCLK);
	if (ret < 0)
		return ret;

	return 0;
}
#else
/*
 * ESPRESSO eS515 I2S DAI operations. (AP master)
 */
static int espresso_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int pll, div, sclk, bfs, psr, rfs, ret;
	unsigned long rclk;

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_U24:
	case SNDRV_PCM_FORMAT_S24:
		bfs = 48;
		break;
	case SNDRV_PCM_FORMAT_U16_LE:
	case SNDRV_PCM_FORMAT_S16_LE:
		bfs = 32;
		break;
	default:
		return -EINVAL;
	}

	switch (params_rate(params)) {
	case 16000:
	case 22050:
	case 24000:
	case 32000:
	case 44100:
	case 48000:
	case 88200:
	case 96000:
		if (bfs == 48)
			rfs = 384;
		else
			rfs = 256;
		break;
	case 64000:
		rfs = 384;
		break;
	case 8000:
	case 11025:
	case 12000:
		if (bfs == 48)
			rfs = 768;
		else
			rfs = 512;
		break;
	default:
		return -EINVAL;
	}

	rclk = params_rate(params) * rfs;

	switch (rclk) {
	case 4096000:
	case 5644800:
	case 6144000:
	case 8467200:
	case 9216000:
		psr = 8;
		break;
	case 8192000:
	case 11289600:
	case 12288000:
	case 16934400:
	case 18432000:
		psr = 4;
		break;
	case 22579200:
	case 24576000:
	case 33868800:
	case 36864000:
		psr = 2;
		break;
	case 67737600:
	case 73728000:
		psr = 1;
		break;
	default:
		printk("Not yet supported!\n");
		return -EINVAL;
	}

	/* Set AUD_PLL frequency */
	sclk = rclk * psr;
	for (div = 2; div <= 16; div++) {
		if (sclk * div > ESPRESSO_AUD_PLL_FREQ)
			break;
	}
	pll = sclk * (div - 1);
	set_aud_pll_rate(pll);

	/* CLKOUT(XXTI) for eS515 MCLK */
	espresso_enable_mclk(true);

	/* Set Codec DAI configuration */
	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S
					 | SND_SOC_DAIFMT_NB_NF
					 | SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0)
		return ret;

	/* Set CPU DAI configuration */
	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S
					| SND_SOC_DAIFMT_NB_NF
					| SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_sysclk(cpu_dai, SAMSUNG_I2S_CDCLK,
					0, SND_SOC_CLOCK_OUT);
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_sysclk(cpu_dai, SAMSUNG_I2S_OPCLK,
					0, MOD_OPCLK_PCLK);
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_sysclk(cpu_dai, SAMSUNG_I2S_RCLKSRC_1, 0, 0);
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_clkdiv(cpu_dai, SAMSUNG_I2S_DIV_BCLK, bfs);
	if (ret < 0)
		return ret;

	return 0;
}
#endif
static struct snd_soc_ops espresso_ops = {
	.hw_params = espresso_hw_params,
};

#ifdef CONFIG_SND_SAMSUNG_AUX_HDMI
/*
 * ESPRESSO HDMI I2S DAI operations.
 */
static int espresso_hdmi_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int pll, div, sclk, bfs, psr, rfs, ret;
	unsigned long rclk;

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_U24:
	case SNDRV_PCM_FORMAT_S24:
		bfs = 48;
		break;
	case SNDRV_PCM_FORMAT_U16_LE:
	case SNDRV_PCM_FORMAT_S16_LE:
		bfs = 32;
		break;
	default:
		return -EINVAL;
	}

	switch (params_rate(params)) {
	case 48000:
	case 96000:
	case 192000:
		if (bfs == 48)
			rfs = 384;
		else
			rfs = 256;
		break;
	default:
		return -EINVAL;
	}

	rclk = params_rate(params) * rfs;

	switch (rclk) {
	case 12288000:
	case 18432000:
		psr = 4;
		break;
	case 24576000:
	case 36864000:
		psr = 2;
		break;
	case 49152000:
	case 73728000:
		psr = 1;
		break;
	default:
		printk("Not yet supported!\n");
		return -EINVAL;
	}

	/* Set AUD_PLL frequency */
	sclk = rclk * psr;
	for (div = 2; div <= 16; div++) {
		if (sclk * div > ESPRESSO_AUD_PLL_FREQ)
			break;
	}
	pll = sclk * (div - 1);
	set_aud_pll_rate(pll);

	/* Set CPU DAI configuration */
	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S
					| SND_SOC_DAIFMT_NB_NF
					| SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_sysclk(cpu_dai, SAMSUNG_I2S_CDCLK,
					0, SND_SOC_CLOCK_OUT);
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_sysclk(cpu_dai, SAMSUNG_I2S_OPCLK,
					0, MOD_OPCLK_PCLK);
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_sysclk(cpu_dai, SAMSUNG_I2S_RCLKSRC_1, 0, 0);
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_clkdiv(cpu_dai, SAMSUNG_I2S_DIV_BCLK, bfs);
	if (ret < 0)
		return ret;

	return 0;
}

static struct snd_soc_ops espresso_hdmi_ops = {
	.hw_params = espresso_hdmi_hw_params,
};
#endif

#ifdef CONFIG_SND_SAMSUNG_AUX_SPDIF
/*
 * ESPRESSO S/PDIF DAI operations. (AP master)
 */
static int espresso_spdif_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	unsigned long rclk;
	int ret, ratio, pll, div, sclk;

	switch (params_rate(params)) {
	case 48000:
	case 96000:
		break;
	default:
		return -EINVAL;
	}

	/* Setting ratio to 512fs helps to use S/PDIF with HDMI without
	 * modify S/PDIF ASoC machine driver.
	 */
	ratio = 512;
	rclk = params_rate(params) * ratio;

	/* Set AUD_PLL frequency */
	sclk = rclk;
	for (div = 2; div <= 16; div++) {
		if (sclk * div > ESPRESSO_AUD_PLL_FREQ)
			break;
	}
	pll = sclk * (div - 1);
	set_aud_pll_rate(pll);

	/* Set S/PDIF uses internal source clock */
	ret = snd_soc_dai_set_sysclk(cpu_dai, SND_SOC_SPDIF_INT_MCLK,
					rclk, SND_SOC_CLOCK_IN);
	if (ret < 0)
		return ret;

	return ret;
}

static struct snd_soc_ops espresso_spdif_ops = {
	.hw_params = espresso_spdif_hw_params,
};
#endif

static struct snd_soc_dai_link espresso_dai[] = {
	{ /* Primary DAI i/f */
		.name = "ES515 PRI",
		.stream_name = "i2s0-pri",
		.codec_dai_name = "es515-porta",
		.ops = &espresso_ops,
	}, { /* Secondary DAI i/f */
		.name = "ES515 SEC",
		.stream_name = "i2s0-sec",
		.cpu_dai_name = "samsung-i2s-sec",
		.platform_name = "samsung-i2s-sec",
		.codec_dai_name = "es515-porta",
		.ops = &espresso_ops,
#ifdef CONFIG_SND_SAMSUNG_AUX_HDMI
	}, { /* Aux DAI i/f */
		.name = "HDMI",
		.stream_name = "i2s1",
		.codec_dai_name = "dummy-aif1",
		.ops = &espresso_hdmi_ops,
#endif
#ifdef CONFIG_SND_SAMSUNG_AUX_SPDIF
	}, { /* Aux DAI i/f */
		.name = "S/PDIF",
		.stream_name = "spdif",
		.codec_dai_name = "dummy-aif2",
		.ops = &espresso_spdif_ops,
#endif
	}
};

static int espresso_suspend_post(struct snd_soc_card *card)
{
#ifdef CONFIG_SND_SOC_ES515_POWERSAVE
	espresso_enable_mclk(false);
#endif
	return 0;
}

static int espresso_resume_pre(struct snd_soc_card *card)
{
#ifdef CONFIG_SND_SOC_ES515_POWERSAVE
	espresso_enable_mclk(true);
#endif
	return 0;
}

#define SAMSUNG_JACK_ADC_CHANNEL (5)
#define MIC_MIN_THRESHOLD (0xE4) /*about 100mV with 1.8V */
#define MIC_MAX_THRESHOLD (0xFF0)

static struct iio_channel *samsung_jack_adc;

static int samsung_jack_status_check(void);
static struct snd_soc_jack samsung_jack;
unsigned int earjack_gpio;


static struct snd_soc_jack_gpio samsung_jack_gpios[] = {
	{
		.name = "EAR_DET",
		.report = SND_JACK_HEADSET,
		.invert = 0,
		.debounce_time = 200,
		.wake = 1,
		.jack_status_check = samsung_jack_status_check,
	}
};

static int samsung_jack_status_check(void)
{
	int adc_val, ret;

	if(samsung_jack_adc == NULL) {
		printk("%s() :  samsunc_jack_adc is not initialized. \n",__func__);
		return -EINVAL;
	} else {
		ret = iio_read_channel_raw(samsung_jack_adc, &adc_val);
		if(ret < 0) {
			pr_err("iio_read_channel_raw() error : %d\n", ret);
			return ret;
		}

		if (adc_val < MIC_MIN_THRESHOLD) {
			return SND_JACK_HEADPHONE;
		} else if (adc_val < MIC_MAX_THRESHOLD) {
			return SND_JACK_HEADSET;
		} else {
			return 0;
		}

	}

	pr_debug("EAR_ADC_3.5 = %03X (%dmV)\n", adc_val, 1800 * adc_val / 0xFFF);

}

static int espresso_late_probe(struct snd_soc_card *card)
{
	struct snd_soc_codec *codec = card->rtd[0].codec;
	struct device_node *np = codec->dev->of_node;
	int ret = 0;

#ifdef ES515_JACK_DETECTION
	ret = es515_jack_assign(codec);
	if (ret != 0) {
		dev_err(codec->dev, "es515_jack_assign is failed.\n");
	}
#else
	/* earjack detect gpio set*/
	samsung_jack_gpios[0].gpio = of_get_gpio(np, 3);
	if (!gpio_is_valid(samsung_jack_gpios[0].gpio))
		pr_err("%s : gpio for earjack detection is invalid. \n", __func__);

	/* earjack adc set 14d10000.adc */
	samsung_jack_adc = iio_channel_get(NULL, "adc5");
	if(IS_ERR(samsung_jack_adc))
			pr_err("%s : fail to get iio channel for adc. \n",__func__);

	ret = snd_soc_jack_new(codec, "es515 jack", SND_JACK_HEADSET, &samsung_jack);
	if (ret) {
		dev_err(codec->dev, "snd_soc_jack_new is failed.\n");
	} else {
		ret = snd_soc_jack_add_gpios(&samsung_jack,
					ARRAY_SIZE(samsung_jack_gpios), samsung_jack_gpios);
		if (ret)
			dev_err(codec->dev, "snd_soc_jack_add_gpio is failed.\n");
	}

#endif
	return ret;
}

int espresso_remove(struct snd_soc_card *card)
{
#ifdef ES515_JACK_DETECTION
	struct snd_soc_codec *codec = card->rtd[0].codec;
	es515_jack_remove(codec);
#else
	iio_channel_release(samsung_jack_adc);
#endif
	return 0;
}

static struct snd_soc_card espresso = {
	.name = "ESPRESSO-I2S",
	.owner = THIS_MODULE,
	.suspend_post = espresso_suspend_post,
	.resume_pre = espresso_resume_pre,
	.dai_link = espresso_dai,
	.num_links = ARRAY_SIZE(espresso_dai),
	.late_probe = espresso_late_probe,
	.remove = espresso_remove,
};

static int espresso_audio_probe(struct platform_device *pdev)
{
	int n, ret;
	struct device_node *np = pdev->dev.of_node;
	struct snd_soc_card *card = &espresso;
#ifdef CONFIG_SND_SAMSUNG_AUX_HDMI
	bool hdmi_avail = true;
#endif
#ifdef CONFIG_SND_SAMSUNG_AUX_SPDIF
	bool spdif_avail = true;
#endif
	card->dev = &pdev->dev;

	/* CLKOUT(XXTI) for eS515 MCLK */
	espresso_enable_mclk(true);

	for (n = 0; np && n < ARRAY_SIZE(espresso_dai); n++) {
		if (!espresso_dai[n].cpu_dai_name) {
			espresso_dai[n].cpu_of_node = of_parse_phandle(np,
					"samsung,audio-cpu", n);

#ifdef CONFIG_SND_SAMSUNG_AUX_HDMI
			if (!espresso_dai[n].cpu_of_node && hdmi_avail) {
				espresso_dai[n].cpu_of_node = of_parse_phandle(np,
					"samsung,audio-cpu-hdmi", 0);
				hdmi_avail = false;
			}
#endif
#ifdef CONFIG_SND_SAMSUNG_AUX_SPDIF
			if (!espresso_dai[n].cpu_of_node && spdif_avail) {
				espresso_dai[n].cpu_of_node = of_parse_phandle(np,
					"samsung,audio-cpu-spdif", 0);
				spdif_avail = false;
			}
#endif
			if (!espresso_dai[n].cpu_of_node) {
				dev_err(&pdev->dev, "Property "
				"'samsung,audio-cpu' missing or invalid\n");
				ret = -EINVAL;
			}
		}

		if (!espresso_dai[n].platform_name)
			espresso_dai[n].platform_of_node = espresso_dai[n].cpu_of_node;

		espresso_dai[n].codec_name = NULL;
		espresso_dai[n].codec_of_node = of_parse_phandle(np,
				"samsung,audio-codec", n);
		if (!espresso_dai[0].codec_of_node) {
			dev_err(&pdev->dev,
			"Property 'samsung,audio-codec' missing or invalid\n");
			ret = -EINVAL;
		}
	}

	ret = snd_soc_register_card(card);

	if (ret)
		dev_err(&pdev->dev, "snd_soc_register_card() failed:%d\n", ret);

	return ret;
}

static int espresso_audio_remove(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);

	snd_soc_unregister_card(card);

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id samsung_es515_of_match[] = {
	{ .compatible = "samsung,espresso-es515", },
	{},
};
MODULE_DEVICE_TABLE(of, samsung_es515_of_match);
#endif /* CONFIG_OF */

static struct platform_driver espresso_audio_driver = {
	.driver		= {
		.name	= "espresso-audio",
		.owner	= THIS_MODULE,
		.pm = &snd_soc_pm_ops,
		.of_match_table = of_match_ptr(samsung_es515_of_match),
	},
	.probe		= espresso_audio_probe,
	.remove		= espresso_audio_remove,
};

module_platform_driver(espresso_audio_driver);

MODULE_DESCRIPTION("ALSA SoC ESPRESSO eS515");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:espresso-audio");
