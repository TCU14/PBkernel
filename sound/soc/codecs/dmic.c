/*
 * dmic.c  --  SoC audio for Generic Digital MICs
 *
 * Author: Liam Girdwood <lrg@slimlogic.co.uk>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>

#define MAX_MODESWITCH_DELAY 70
static int modeswitch_delay;
module_param(modeswitch_delay, uint, 0644);

struct dmic {
	struct gpio_desc *gpio_en;
	int wakeup_delay;
	/* Delay after DMIC mode switch */
	int modeswitch_delay;
};

int dmic_daiops_trigger(struct snd_pcm_substream *substream,
		int cmd, struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
	struct dmic *dmic = snd_soc_codec_get_drvdata(codec);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_STOP:
		if (dmic->modeswitch_delay)
			mdelay(dmic->modeswitch_delay);

		break;
	}

	return 0;
}

static const struct snd_soc_dai_ops dmic_dai_ops = {
	.trigger	= dmic_daiops_trigger,
};

static int dmic_aif_event(struct snd_soc_dapm_widget *w,
			  struct snd_kcontrol *kcontrol, int event) {
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	struct dmic *dmic = snd_soc_codec_get_drvdata(codec);

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		if (dmic->gpio_en)
			gpiod_set_value(dmic->gpio_en, 1);

		if (dmic->wakeup_delay)
			msleep(dmic->wakeup_delay);
		break;
	case SND_SOC_DAPM_POST_PMD:
		if (dmic->gpio_en)
			gpiod_set_value(dmic->gpio_en, 0);
		break;
	}

	return 0;
}

static struct snd_soc_dai_driver dmic_dai = {
	.name = "dmic-hifi",
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = 8,
		.rates = SNDRV_PCM_RATE_CONTINUOUS,
		.formats = SNDRV_PCM_FMTBIT_S32_LE
			| SNDRV_PCM_FMTBIT_S24_LE
			| SNDRV_PCM_FMTBIT_S16_LE,
	},
	.ops    = &dmic_dai_ops,
};

static int dmic_codec_probe(struct snd_soc_codec *codec)
{
	struct dmic *dmic;

	dmic = devm_kzalloc(codec->dev, sizeof(*dmic), GFP_KERNEL);
	if (!dmic)
		return -ENOMEM;

	dmic->gpio_en = devm_gpiod_get_optional(codec->dev,
						"dmicen", GPIOD_OUT_LOW);
	if (IS_ERR(dmic->gpio_en))
		return PTR_ERR(dmic->gpio_en);

	device_property_read_u32(codec->dev, "wakeup-delay-ms",
				 &dmic->wakeup_delay);
	device_property_read_u32(codec->dev, "modeswitch-delay-ms",
				 &dmic->modeswitch_delay);
	if (modeswitch_delay)
		dmic->modeswitch_delay  = modeswitch_delay;

	if (dmic->modeswitch_delay > MAX_MODESWITCH_DELAY)
		dmic->modeswitch_delay = MAX_MODESWITCH_DELAY;

	snd_soc_codec_set_drvdata(codec, dmic);

	return 0;
}

static const struct snd_soc_dapm_widget dmic_dapm_widgets[] = {
	SND_SOC_DAPM_AIF_OUT_E("DMIC AIF", "Capture", 0,
			       SND_SOC_NOPM, 0, 0, dmic_aif_event,
			       SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_INPUT("DMic"),
};

static const struct snd_soc_dapm_route intercon[] = {
	{"DMIC AIF", NULL, "DMic"},
};

static struct snd_soc_codec_driver soc_dmic = {
	.probe = dmic_codec_probe,
	.dapm_widgets = dmic_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(dmic_dapm_widgets),
	.dapm_routes = intercon,
	.num_dapm_routes = ARRAY_SIZE(intercon),
};

static int dmic_dev_probe(struct platform_device *pdev)
{
	int err;
	u32 chans;
	struct snd_soc_dai_driver *dai_drv = &dmic_dai;

	if (pdev->dev.of_node) {
		err = of_property_read_u32(pdev->dev.of_node, "num-channels", &chans);
		if (err && (err != -EINVAL))
			return err;

		if (!err) {
			if (chans < 1 || chans > 8)
				return -EINVAL;

			dai_drv = devm_kzalloc(&pdev->dev, sizeof(*dai_drv), GFP_KERNEL);
			if (!dai_drv)
				return -ENOMEM;

			memcpy(dai_drv, &dmic_dai, sizeof(*dai_drv));
			dai_drv->capture.channels_max = chans;
		}
	}

	return snd_soc_register_codec(&pdev->dev,
			&soc_dmic, dai_drv, 1);
}

static int dmic_dev_remove(struct platform_device *pdev)
{
	snd_soc_unregister_codec(&pdev->dev);
	return 0;
}

MODULE_ALIAS("platform:dmic-codec");

static const struct of_device_id dmic_dev_match[] = {
	{.compatible = "dmic-codec"},
	{}
};

static struct platform_driver dmic_driver = {
	.driver = {
		.name = "dmic-codec",
		.of_match_table = dmic_dev_match,
	},
	.probe = dmic_dev_probe,
	.remove = dmic_dev_remove,
};

module_platform_driver(dmic_driver);

MODULE_DESCRIPTION("Generic DMIC driver");
MODULE_AUTHOR("Liam Girdwood <lrg@slimlogic.co.uk>");
MODULE_LICENSE("GPL");
