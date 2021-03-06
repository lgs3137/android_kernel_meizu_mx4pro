/*
 * Copyright (c) 2013 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com
 *		Taikyung yu(taikyung.yu@samsung.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <mach/devfreq.h>
#include <linux/pm_qos.h>
#include <linux/regulator/consumer.h>
#include <mach/tmu.h>
#include "devfreq_exynos.h"
#include "governor.h"

/* ========== 0. DISP related function */

enum devfreq_disp_idx {
	DISP_LV0,
	DISP_LV1,
	DISP_LV2,
	DISP_LV3,
	DISP_LV_COUNT,
};

enum devfreq_disp_clk {
	DOUT_ACLK_DISP_333,
	DOUT_SCLK_DSD,
	DISP_CLK_COUNT,
};

struct devfreq_clk_list devfreq_disp_clk[DISP_CLK_COUNT] = {
	{"dout_aclk_disp_333",},
	{"dout_sclk_dsd",},
};

struct devfreq_opp_table devfreq_disp_opp_list[] = {
	{DISP_LV0,	334000,	0},
	{DISP_LV1,	222000,	0},
	{DISP_LV2,	167000,	0},
	{DISP_LV3,	134000,	0},
};

struct devfreq_clk_info aclk_disp_333[] = {
	{DISP_LV0,	334000000,	0,	NULL},
	{DISP_LV1,	222000000,	0,	NULL},
	{DISP_LV2,	167000000,	0,	NULL},
	{DISP_LV3,	134000000,	0,	NULL},
};

struct devfreq_clk_info sclk_dsd[] = {
	{DISP_LV0,	334000000,	0,	NULL},
	{DISP_LV1,	334000000,	0,	NULL},
	{DISP_LV2,	334000000,	0,	NULL},
	{DISP_LV3,	334000000,	0,	NULL},
};

struct devfreq_clk_info *devfreq_clk_disp_info_list[] = {
	aclk_disp_333,
	sclk_dsd,
};

enum devfreq_disp_clk devfreq_clk_disp_info_idx[] = {
	DOUT_ACLK_DISP_333,
	DOUT_SCLK_DSD,
};

#ifdef CONFIG_PM_RUNTIME
struct devfreq_pm_domain_link devfreq_disp_pm_domain[] = {
	{"pd-disp",},
	{"pd-disp",},
};

static int exynos5_devfreq_disp_init_pm_domain(void)
{
	struct platform_device *pdev = NULL;
	struct device_node *np = NULL;
	int i;

	for_each_compatible_node(np, NULL, "samsung,exynos-pd") {
		struct exynos_pm_domain *pd;

		if (!of_device_is_available(np))
			continue;

		pdev = of_find_device_by_node(np);
		pd = platform_get_drvdata(pdev);

		for (i = 0; i < ARRAY_SIZE(devfreq_disp_pm_domain); ++i) {
			if (devfreq_disp_pm_domain[i].pm_domain_name == NULL)
				continue;

			if (!strcmp(devfreq_disp_pm_domain[i].pm_domain_name, pd->genpd.name))
				devfreq_disp_pm_domain[i].pm_domain = pd;
		}
	}

	return 0;
}
#endif

static int exynos5_devfreq_disp_init_clock(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(devfreq_disp_clk); ++i) {
		devfreq_disp_clk[i].clk = clk_get(NULL, devfreq_disp_clk[i].clk_name);
		if (IS_ERR_OR_NULL(devfreq_disp_clk[i].clk)) {
			pr_err("DEVFREQ(DISP) : %s can't get clock\n", devfreq_disp_clk[i].clk_name);
			return -EINVAL;
		}
		pr_debug("DISP clk name: %s, rate: %lu\n", devfreq_disp_clk[i].clk_name, clk_get_rate(devfreq_disp_clk[i].clk));
	}

	return 0;
}

static int exynos5_devfreq_disp_set_freq(struct devfreq_data_disp *data,
					int target_idx,
					int old_idx)
{
	int i, j;
	struct devfreq_clk_info *clk_info;
	struct devfreq_clk_states *clk_states;
#ifdef CONFIG_PM_RUNTIME
	struct exynos_pm_domain *pm_domain;
#endif

	if (target_idx < old_idx) {
		for (i = 0; i < ARRAY_SIZE(devfreq_clk_disp_info_list); ++i) {
			clk_info = &devfreq_clk_disp_info_list[i][target_idx];
			clk_states = clk_info->states;
#ifdef CONFIG_PM_RUNTIME
			pm_domain = devfreq_disp_pm_domain[i].pm_domain;

			if (pm_domain != NULL) {
				mutex_lock(&pm_domain->access_lock);
				if ((__raw_readl(pm_domain->base + 0x4) & EXYNOS_INT_LOCAL_PWR_EN) == 0) {
					mutex_unlock(&pm_domain->access_lock);
					continue;
				}
			}
#endif
			if (clk_states) {
				for (j = 0; j < clk_states->state_count; ++j) {
					clk_set_parent(devfreq_disp_clk[clk_states->state[j].clk_idx].clk,
						devfreq_disp_clk[clk_states->state[j].parent_clk_idx].clk);
				}
			}

			if (clk_info->freq != 0) {
				clk_set_rate(devfreq_disp_clk[devfreq_clk_disp_info_idx[i]].clk, clk_info->freq);
				pr_debug("DISP clk name: %s, set_rate: %lu, get_rate: %lu\n",
						devfreq_disp_clk[devfreq_clk_disp_info_idx[i]].clk_name,
						clk_info->freq, clk_get_rate(devfreq_disp_clk[devfreq_clk_disp_info_idx[i]].clk));
			}
#ifdef CONFIG_PM_RUNTIME
			if (pm_domain != NULL)
				mutex_unlock(&pm_domain->access_lock);
#endif
		}
	} else {
		for (i = 0; i < ARRAY_SIZE(devfreq_clk_disp_info_list); ++i) {
			clk_info = &devfreq_clk_disp_info_list[i][target_idx];
			clk_states = clk_info->states;
#ifdef CONFIG_PM_RUNTIME
			pm_domain = devfreq_disp_pm_domain[i].pm_domain;

			if (pm_domain != NULL) {
				mutex_lock(&pm_domain->access_lock);
				if ((__raw_readl(pm_domain->base + 0x4) & EXYNOS_INT_LOCAL_PWR_EN) == 0) {
					mutex_unlock(&pm_domain->access_lock);
					continue;
				}
			}
#endif
			if (clk_info->freq != 0)
				clk_set_rate(devfreq_disp_clk[devfreq_clk_disp_info_idx[i]].clk, clk_info->freq);

			if (clk_states) {
				for (j = 0; j < clk_states->state_count; ++j) {
					clk_set_parent(devfreq_disp_clk[clk_states->state[j].clk_idx].clk,
						devfreq_disp_clk[clk_states->state[j].parent_clk_idx].clk);
				}
			}

			if (clk_info->freq != 0) {
				clk_set_rate(devfreq_disp_clk[devfreq_clk_disp_info_idx[i]].clk, clk_info->freq);
				pr_debug("DISP clk name: %s, set_rate: %lu, get_rate: %lu\n",
						devfreq_disp_clk[devfreq_clk_disp_info_idx[i]].clk_name,
						clk_info->freq, clk_get_rate(devfreq_disp_clk[devfreq_clk_disp_info_idx[i]].clk));
			}
#ifdef CONFIG_PM_RUNTIME
			if (pm_domain != NULL)
				mutex_unlock(&pm_domain->access_lock);
#endif
		}
	}

	return 0;
}

static int exynos5_devfreq_disp_set_clk(struct devfreq_data_disp *data,
					int target_idx,
					struct clk *clk,
					struct devfreq_clk_info *clk_info)
{
	int i;
	struct devfreq_clk_states *clk_states = clk_info->states;

	if (clk_get_rate(clk) < clk_info->freq) {
		if (clk_states) {
			for (i = 0; i < clk_states->state_count; ++i) {
				clk_set_parent(devfreq_disp_clk[clk_states->state[i].clk_idx].clk,
						devfreq_disp_clk[clk_states->state[i].parent_clk_idx].clk);
			}
		}

		if (clk_info->freq != 0)
			clk_set_rate(clk, clk_info->freq);
	} else {
		if (clk_info->freq != 0)
			clk_set_rate(clk, clk_info->freq);

		if (clk_states) {
			for (i = 0; i < clk_states->state_count; ++i) {
				clk_set_parent(devfreq_disp_clk[clk_states->state[i].clk_idx].clk,
						devfreq_disp_clk[clk_states->state[i].parent_clk_idx].clk);
			}
		}

		if (clk_info->freq != 0)
			clk_set_rate(clk, clk_info->freq);
	}

	return 0;
}

static struct devfreq_data_disp *data_disp;
void exynos5_disp_notify_power_status(const char *pd_name, unsigned int turn_on)
{
	int i;
	int cur_freq_idx;

	if (!turn_on || !data_disp->use_dvfs)
		return;

	mutex_lock(&data_disp->lock);
	cur_freq_idx = exynos5_devfreq_get_idx(devfreq_disp_opp_list,
			ARRAY_SIZE(devfreq_disp_opp_list),
			data_disp->devfreq->previous_freq);
	if (cur_freq_idx == -1) {
		mutex_unlock(&data_disp->lock);
		pr_err("DEVFREQ(INT) : can't find target_idx to apply notify of power\n");
		return;
	}

	for (i = 0; i < ARRAY_SIZE(devfreq_disp_pm_domain); ++i) {
		if (devfreq_disp_pm_domain[i].pm_domain_name == NULL)
			continue;
		if (strcmp(devfreq_disp_pm_domain[i].pm_domain_name, pd_name))
			continue;

		exynos5_devfreq_disp_set_clk(data_disp,
				cur_freq_idx,
				devfreq_disp_clk[devfreq_clk_disp_info_idx[i]].clk,
				devfreq_clk_disp_info_list[i]);
	}
	mutex_unlock(&data_disp->lock);
}

int exynos5433_devfreq_disp_init(struct devfreq_data_disp *data)
{
	int ret;
	data_disp = data;
	if (exynos5_devfreq_disp_init_clock()) {
		ret = -EINVAL;
		goto err_data;
	}

#ifdef CONFIG_PM_RUNTIME
	if (exynos5_devfreq_disp_init_pm_domain()) {
		ret = -EINVAL;
		goto err_data;
	}
#endif

	data->disp_set_freq = exynos5_devfreq_disp_set_freq;
err_data:
	return ret;
}
/* end of DISP related function */

/* ========== 1. MIF related function */
extern struct pm_qos_request exynos5_mif_bts_qos;

enum devfreq_mif_idx {
	MIF_LV0,
	MIF_LV1,
	MIF_LV2,
	MIF_LV3,
	MIF_LV4,
	MIF_LV5,
	MIF_LV6,
	MIF_LV7,
	MIF_LV8,
	MIF_LV_COUNT,
};

enum devfreq_mif_clk {
	FOUT_MEM0_PLL,
	FOUT_MEM1_PLL,
	FOUT_MFC_PLL,
	FOUT_BUS_PLL,
	MOUT_MEM0_PLL,
	MOUT_MEM0_PLL_DIV2,
	MOUT_MEM1_PLL,
	MOUT_MEM1_PLL_DIV2,
	MOUT_MFC_PLL,
	MOUT_MFC_PLL_DIV2,
	MOUT_BUS_PLL,
	MOUT_BUS_PLL_DIV2,
	MOUT_ACLK_MIF_400,
	DOUT_ACLK_MIF_400,
	DOUT_ACLK_MIF_266,
	DOUT_ACLK_MIF_200,
	DOUT_MIF_PRE,
	MOUT_ACLK_MIFNM_200,
	DOUT_ACLK_MIFNM_200,
	DOUT_ACLK_MIFND_133,
	DOUT_ACLK_MIF_133,
	DOUT_ACLK_CPIF_200,
	DOUT_CLK2X_PHY,
	DOUT_ACLK_DREX0,
	DOUT_ACLK_DREX1,
	DOUT_SCLK_HPM_MIF,
	MIF_CLK_COUNT,
};

struct devfreq_clk_list devfreq_mif_clk[MIF_CLK_COUNT] = {
	{"fout_mem0_pll",},
	{"fout_mem1_pll",},
	{"fout_mfc_pll",},
	{"fout_bus_pll",},
	{"mout_mem0_pll",},
	{"mout_mem0_pll_div2",},
	{"mout_mem1_pll",},
	{"mout_mem1_pll_div2",},
	{"mout_mfc_pll",},
	{"mout_mfc_pll_div2",},
	{"mout_bus_pll",},
	{"mout_bus_pll_div2",},
	{"mout_aclk_mif_400",},
	{"dout_aclk_mif_400",},
	{"dout_aclk_mif_266",},
	{"dout_aclk_mif_200",},
	{"dout_mif_pre",},
	{"mout_aclk_mifnm_200",},
	{"dout_aclk_mifnm_200",},
	{"dout_aclk_mifnd_133",},
	{"dout_aclk_mif_133",},
	{"dout_aclk_cpif_200",},
	{"dout_clk2x_phy",},
	{"dout_aclk_drex0",},
	{"dout_aclk_drex1",},
	{"dout_sclk_hpm_mif",},
};

struct devfreq_opp_table devfreq_mif_opp_list[] = {
	{MIF_LV0,	825000,	1050000},
	{MIF_LV1,	633000,	1000000},
	{MIF_LV2,	543000,	 975000},
	{MIF_LV3,	413000,	 950000},
	{MIF_LV4,	275000,  950000},
	{MIF_LV5,	206000,  950000},
	{MIF_LV6,	165000,  925000},
	{MIF_LV7,	138000,  900000},
	{MIF_LV8,	103000,	 875000},
};

struct devfreq_clk_value aclk_clk2x_825[] = {
	{0x1000, (0x1 << 20), ((0xF << 28) | (0x1 << 16) | (0x1 << 12))},
};

struct devfreq_clk_value aclk_clk2x_633[] = {
	{0x1000, ((0x1 << 20) | (0x1 << 16) | (0x1 << 12)), (0xF << 28)},
};

struct devfreq_clk_value aclk_clk2x_543[] = {
	{0x1000, 0, ((0xF << 28) | (0x1 << 20) | (0x1 << 16) | (0x1 << 12))},
};

struct devfreq_clk_value aclk_clk2x_413[] = {
	{0x1000, ((0x1 << 28) | (0x1 << 20)), ((0xF << 28) | (0x1 << 16) | (0x1 << 12))},
};

struct devfreq_clk_value aclk_clk2x_275[] = {
	{0x1000, ((0x2 << 28) | (0x1 << 20)), ((0xF << 28) | (0x1 << 16) | (0x1 << 12))},
};

struct devfreq_clk_value aclk_clk2x_206[] = {
	{0x1000, ((0x3 << 28) | (0x1 << 20) ),((0xF << 28) | (0x1 << 16) | (0x1 << 12))},
};

struct devfreq_clk_value aclk_clk2x_165[] = {
	{0x1000, ((0x4 << 28) | (0x1 << 20) ),((0xF << 28) | (0x1 << 16) | (0x1 << 12))},
};

struct devfreq_clk_value aclk_clk2x_138[] = {
	{0x1000, ((0x5 << 28) | (0x1 << 20) ),((0xF << 28) | (0x1 << 16) | (0x1 << 12))},
};

struct devfreq_clk_value aclk_clk2x_103[] = {
	{0x1000, ((0x7 << 28) | (0x1 << 20) ),((0xF << 28) | (0x1 << 16) | (0x1 << 12))},
};

struct devfreq_clk_value *aclk_clk2x_list[] = {
	aclk_clk2x_825,
	aclk_clk2x_633,
	aclk_clk2x_543,
	aclk_clk2x_413,
	aclk_clk2x_275,
	aclk_clk2x_206,
	aclk_clk2x_165,
	aclk_clk2x_138,
	aclk_clk2x_103,
};

struct devfreq_clk_state aclk_mif_400_mem1_pll[] = {
	{MOUT_ACLK_MIF_400,     MOUT_MEM1_PLL_DIV2},
};

struct devfreq_clk_state aclk_mifnm_200_bus_pll[] = {
	{DOUT_MIF_PRE,		MOUT_ACLK_MIFNM_200},
};

struct devfreq_clk_states aclk_mif_400_mem1_pll_list = {
	.state = aclk_mif_400_mem1_pll,
	.state_count = ARRAY_SIZE(aclk_mif_400_mem1_pll),
};

struct devfreq_clk_states aclk_mifnm_200_bus_pll_list = {
	.state = aclk_mifnm_200_bus_pll,
	.state_count = ARRAY_SIZE(aclk_mifnm_200_bus_pll),
};

struct devfreq_clk_info aclk_mif_400[] = {
	{MIF_LV0,   413000000,      0,      &aclk_mif_400_mem1_pll_list},
	{MIF_LV1,   275000000,      0,      &aclk_mif_400_mem1_pll_list},
	{MIF_LV2,   275000000,      0,      &aclk_mif_400_mem1_pll_list},
	{MIF_LV3,   275000000,      0,      &aclk_mif_400_mem1_pll_list},
	{MIF_LV4,   275000000,      0,      &aclk_mif_400_mem1_pll_list},
	{MIF_LV5,   207000000,      0,      &aclk_mif_400_mem1_pll_list},
	{MIF_LV6,   165000000,      0,      &aclk_mif_400_mem1_pll_list},
	{MIF_LV7,   165000000,      0,      &aclk_mif_400_mem1_pll_list},
	{MIF_LV8,   138000000,      0,      &aclk_mif_400_mem1_pll_list},
};

struct devfreq_clk_info aclk_mif_266[] = {
	{MIF_LV0,	267000000,      0,      NULL},
	{MIF_LV1,	267000000,      0,      NULL},
	{MIF_LV2,	200000000,      0,      NULL},
	{MIF_LV3,	200000000,      0,      NULL},
	{MIF_LV4,	200000000,      0,      NULL},
	{MIF_LV5,	134000000,      0,      NULL},
	{MIF_LV6,	100000000,      0,      NULL},
	{MIF_LV7,	100000000,      0,      NULL},
	{MIF_LV8,	 89000000,      0,      NULL},
};

struct devfreq_clk_info aclk_mif_200[] = {
	{MIF_LV0,	207000000,      0,      NULL},
	{MIF_LV1,	138000000,      0,      NULL},
	{MIF_LV2,	138000000,      0,      NULL},
	{MIF_LV3,	138000000,      0,      NULL},
	{MIF_LV4,	138000000,      0,      NULL},
	{MIF_LV5,	105000000,      0,      NULL},
	{MIF_LV6,	 83000000,      0,      NULL},
	{MIF_LV7,	 83000000,      0,      NULL},
	{MIF_LV8,	 69000000,      0,      NULL},
};

struct devfreq_clk_info mif_pre[] = {
	{MIF_LV0,   800000000,      0,      NULL},
	{MIF_LV1,   800000000,      0,      NULL},
	{MIF_LV2,   800000000,      0,      NULL},
	{MIF_LV3,   800000000,      0,      NULL},
	{MIF_LV4,   800000000,      0,      NULL},
	{MIF_LV5,   800000000,      0,      NULL},
	{MIF_LV6,   800000000,      0,      NULL},
	{MIF_LV7,   800000000,      0,      NULL},
	{MIF_LV8,   800000000,      0,      NULL},
};

struct devfreq_clk_info aclk_mifnm[] = {
	{MIF_LV0,   134000000,      0,      &aclk_mifnm_200_bus_pll_list},
	{MIF_LV1,   134000000,      0,      &aclk_mifnm_200_bus_pll_list},
	{MIF_LV2,   100000000,      0,      &aclk_mifnm_200_bus_pll_list},
	{MIF_LV3,   100000000,      0,      &aclk_mifnm_200_bus_pll_list},
	{MIF_LV4,   100000000,      0,      &aclk_mifnm_200_bus_pll_list},
	{MIF_LV5,   100000000,      0,      &aclk_mifnm_200_bus_pll_list},
	{MIF_LV6,   100000000,      0,      &aclk_mifnm_200_bus_pll_list},
	{MIF_LV7,   100000000,      0,      &aclk_mifnm_200_bus_pll_list},
	{MIF_LV8,   100000000,      0,      &aclk_mifnm_200_bus_pll_list},
};

struct devfreq_clk_info aclk_mifnd[] = {
	{MIF_LV0,	 80000000,      0,      NULL},
	{MIF_LV1,	 80000000,      0,      NULL},
	{MIF_LV2,	 67000000,      0,      NULL},
	{MIF_LV3,	 67000000,      0,      NULL},
	{MIF_LV4,	 67000000,      0,      NULL},
	{MIF_LV5,	 67000000,      0,      NULL},
	{MIF_LV6,	 67000000,      0,      NULL},
	{MIF_LV7,	 67000000,      0,      NULL},
	{MIF_LV8,	 67000000,      0,      NULL},
};

struct devfreq_clk_info aclk_mif_133[] = {
	{MIF_LV0,	 67000000,      0,      NULL},
	{MIF_LV1,	 67000000,      0,      NULL},
	{MIF_LV2,	 67000000,      0,      NULL},
	{MIF_LV3,	 67000000,      0,      NULL},
	{MIF_LV4,	 50000000,      0,      NULL},
	{MIF_LV5,	 50000000,      0,      NULL},
	{MIF_LV6,	 50000000,      0,      NULL},
	{MIF_LV7,	 50000000,      0,      NULL},
	{MIF_LV8,	 50000000,      0,      NULL},
};

struct devfreq_clk_info aclk_cpif_200[] = {
	{MIF_LV0,   100000000,      0,      NULL},
	{MIF_LV1,   100000000,      0,      NULL},
	{MIF_LV2,   100000000,      0,      NULL},
	{MIF_LV3,   100000000,      0,      NULL},
	{MIF_LV4,   100000000,      0,      NULL},
	{MIF_LV5,   100000000,      0,      NULL},
	{MIF_LV6,   100000000,      0,      NULL},
	{MIF_LV7,   100000000,      0,      NULL},
	{MIF_LV8,   100000000,      0,      NULL},
};

struct devfreq_clk_info sclk_hpm_mif[] = {
	{MIF_LV0,	207000000,      0,      NULL},
	{MIF_LV1,	167000000,      0,      NULL},
	{MIF_LV2,	136000000,      0,      NULL},
	{MIF_LV3,	105000000,      0,      NULL},
	{MIF_LV4,	 69000000,      0,      NULL},
	{MIF_LV5,	 52000000,      0,      NULL},
	{MIF_LV6,	 42000000,      0,      NULL},
	{MIF_LV7,	 35000000,      0,      NULL},
	{MIF_LV8,	 26000000,      0,      NULL},
};

struct devfreq_clk_info *devfreq_clk_mif_info_list[] = {
	aclk_mif_400,
	aclk_mif_266,
	aclk_mif_200,
	mif_pre,
	aclk_mifnm,
	aclk_mifnd,
	aclk_mif_133,
	aclk_cpif_200,
	sclk_hpm_mif,
};

enum devfreq_mif_clk devfreq_clk_mif_info_idx[] = {
	DOUT_ACLK_MIF_400,
	DOUT_ACLK_MIF_266,
	DOUT_ACLK_MIF_200,
	DOUT_MIF_PRE,
	DOUT_ACLK_MIFNM_200,
	DOUT_ACLK_MIFND_133,
	DOUT_ACLK_MIF_133,
	DOUT_ACLK_CPIF_200,
	DOUT_SCLK_HPM_MIF,
};

struct devfreq_mif_timing_parameter {
	unsigned int timing_row;
	unsigned int timing_data;
	unsigned int timing_power;
	unsigned int rd_fetch;
	unsigned int timing_rfcpb;
	unsigned int dvfs_con1;
	unsigned int mif_drex_mr_data[4];
};

struct devfreq_mif_timing_parameter dmc_timing_parameter_2gb[] = {
	{	/* 825Mhz */
		.timing_row	= 0x365A9713,
		.timing_data	= 0x4740085E,
		.timing_power	= 0x543A0446,
		.rd_fetch	= 0x00000003,
		.timing_rfcpb	= 0x00001919,
		.dvfs_con1	= 0x0C0C2121,
		.mif_drex_mr_data = {
			[0]	= 0x00000870,
			[1]	= 0x00100870,
			[2]	= 0x0000060C,
			[3]	= 0x0010060C,
		},
	}, {	/* 633Mhz */
		.timing_row	= 0x2A48758F,
		.timing_data	= 0x3530064A,
		.timing_power	= 0x402D0335,
		.rd_fetch	= 0x00000002,
		.timing_rfcpb	= 0x00001313,
		.dvfs_con1	= 0x0A0A2121,
		.mif_drex_mr_data = {
			[0]	= 0x00000860,
			[1]	= 0x00100860,
			[2]	= 0x0000040C,
			[3]	= 0x0010040C,
		},
	}, {	/* 543Mhz */
		.timing_row	= 0x244764CD,
		.timing_data	= 0x35300549,
		.timing_power	= 0x38270335,
		.rd_fetch	= 0x00000002,
		.timing_rfcpb	= 0x00001111,
		.dvfs_con1	= 0x09092121,
		.mif_drex_mr_data = {
			[0]	= 0x0000081C,
			[1]	= 0x0010081C,
			[2]	= 0x0000078C,
			[3]	= 0x0010078C,
		},
	}, {	/* 413Mhz */
		.timing_row	= 0x1B35538A,
		.timing_data	= 0x24200539,
		.timing_power	= 0x2C1D0225,
		.rd_fetch	= 0x00000002,
		.timing_rfcpb	= 0x00000D0D,
		.dvfs_con1	= 0x09092121,
		.mif_drex_mr_data = {
			[0]	= 0x0000081C,
			[1]	= 0x0010081C,
			[2]	= 0x0000060C,
			[3]	= 0x0010060C,
		},
	}, {	/* 275Mhz */
		.timing_row	= 0x12244287,
		.timing_data	= 0x23200529,
		.timing_power	= 0x1C140225,
		.rd_fetch	= 0x00000002,
		.timing_rfcpb	= 0x00000909,
		.dvfs_con1	= 0x09092121,
		.mif_drex_mr_data = {
			[0]	= 0x0000081C,
			[1]	= 0x0010081C,
			[2]	= 0x0000060C,
			[3]	= 0x0010060C,
		},
	}, {	/* 206Mhz */
		.timing_row	= 0x112331C6,
		.timing_data	= 0x23200529,
		.timing_power	= 0x140E0225,
		.rd_fetch	= 0x00000002,
		.timing_rfcpb	= 0x00000707,
		.dvfs_con1	= 0x09092121,
		.mif_drex_mr_data = {
			[0]	= 0x0000081C,
			[1]	= 0x0010081C,
			[2]	= 0x0000060C,
			[3]	= 0x0010060C,
		},
	}, {	/* 165Mhz */
		.timing_row	= 0x11223185,
		.timing_data	= 0x23200529,
		.timing_power	= 0x140C0225,
		.rd_fetch	= 0x00000002,
		.timing_rfcpb	= 0x00000505,
		.dvfs_con1	= 0x09092121,
		.mif_drex_mr_data = {
			[0]	= 0x0000081C,
			[1]	= 0x0010081C,
			[2]	= 0x0000060C,
			[3]	= 0x0010060C,
		},
	}, {	/* 138Mhz */
		.timing_row	= 0x11222144,
		.timing_data	= 0x23200529,
		.timing_power	= 0x100A0225,
		.rd_fetch	= 0x00000002,
		.timing_rfcpb	= 0x00000505,
		.dvfs_con1	= 0x09092121,
		.mif_drex_mr_data = {
			[0]	= 0x0000081C,
			[1]	= 0x0010081C,
			[2]	= 0x0000060C,
			[3]	= 0x0010060C,
		},
	}, {	/* 103Mhz */
		.timing_row	= 0x11222103,
		.timing_data	= 0x23200529,
		.timing_power	= 0x10080225,
		.rd_fetch	= 0x00000002,
		.timing_rfcpb	= 0x00000303,
		.dvfs_con1	= 0x09092121,
		.mif_drex_mr_data = {
			[0]	= 0x0000081C,
			[1]	= 0x0010081C,
			[2]	= 0x0000060C,
			[3]	= 0x0010060C,
		},
	},
};

struct devfreq_mif_timing_parameter dmc_timing_parameter_3gb[] = {
	{	/* 825Mhz */
		.timing_row	= 0x575A9713,
		.timing_data	= 0x4740085E,
		.timing_power	= 0x545B0446,
		.rd_fetch	= 0x00000003,
		.timing_rfcpb	= 0x00002626,
		.dvfs_con1	= 0x0E0E2121,
		.mif_drex_mr_data = {
			[0]	= 0x00000870,
			[1]	= 0x00100870,
			[2]	= 0x0000060C,
			[3]	= 0x0010060C,
		},
	}, {	/* 633Mhz */
		.timing_row	= 0x4348758F,
		.timing_data	= 0x3530084E,
		.timing_power	= 0x40460335,
		.rd_fetch	= 0x00000002,
		.timing_rfcpb	= 0x00001D1D,
		.dvfs_con1	= 0x0E0E2121,
		.mif_drex_mr_data = {
			[0]	= 0x00000860,
			[1]	= 0x00100860,
			[2]	= 0x0000040C,
			[3]	= 0x0010040C,
		},
	}, {	/* 543Mhz */
		.timing_row	= 0x3A4764CD,
		.timing_data	= 0x3530084E,
		.timing_power	= 0x383C0335,
		.rd_fetch	= 0x00000002,
		.timing_rfcpb	= 0x00001919,
		.dvfs_con1	= 0x0E0E2121,
		.mif_drex_mr_data = {
			[0]	= 0x0000081C,
			[1]	= 0x0010081C,
			[2]	= 0x0000078C,
			[3]	= 0x0010078C,
		},
	}, {	/* 413Mhz */
		.timing_row	= 0x2C35538A,
		.timing_data	= 0x2420083E,
		.timing_power	= 0x2C2E0225,
		.rd_fetch	= 0x00000002,
		.timing_rfcpb	= 0x00001313,
		.dvfs_con1	= 0x0E0E2121,
		.mif_drex_mr_data = {
			[0]	= 0x0000081C,
			[1]	= 0x0010081C,
			[2]	= 0x0000060C,
			[3]	= 0x0010060C,
		},
	}, {	/* 272Mhz */
		.timing_row	= 0x1D244287,
		.timing_data	= 0x2320082E,
		.timing_power	= 0x1C1F0225,
		.rd_fetch	= 0x00000002,
		.timing_rfcpb	= 0x00000D0D,
		.dvfs_con1	= 0x0E0E2121,
		.mif_drex_mr_data = {
			[0]	= 0x0000081C,
			[1]	= 0x0010081C,
			[2]	= 0x0000060C,
			[3]	= 0x0010060C,
		},
	}, {	/* 211Mhz */
		.timing_row	= 0x17233206,
		.timing_data	= 0x2320082E,
		.timing_power	= 0x181F0225,
		.rd_fetch	= 0x00000002,
		.timing_rfcpb	= 0x00000A0A,
		.dvfs_con1	= 0x0E0E2121,
		.mif_drex_mr_data = {
			[0]	= 0x0000081C,
			[1]	= 0x0010081C,
			[2]	= 0x0000060C,
			[3]	= 0x0010060C,
		},
	}, {	/* 158Mhz */
		.timing_row	= 0x12223185,
		.timing_data	= 0x2320082E,
		.timing_power	= 0x141F0225,
		.rd_fetch	= 0x00000002,
		.timing_rfcpb	= 0x00000808,
		.dvfs_con1	= 0x0E0E2121,
		.mif_drex_mr_data = {
			[0]	= 0x0000081C,
			[1]	= 0x0010081C,
			[2]	= 0x0000060C,
			[3]	= 0x0010060C,
		},
	}, {	/* 136Mhz */
		.timing_row	= 0x11222144,
		.timing_data	= 0x2320082E,
		.timing_power	= 0x101F0225,
		.rd_fetch	= 0x00000002,
		.timing_rfcpb	= 0x00000707,
		.dvfs_con1	= 0x0E0E2121,
		.mif_drex_mr_data = {
			[0]	= 0x0000081C,
			[1]	= 0x0010081C,
			[2]	= 0x0000060C,
			[3]	= 0x0010060C,
		},
	}, {	/* 109Mhz */
		.timing_row	= 0x11222103,
		.timing_data	= 0x2320082E,
		.timing_power	= 0x101F0225,
		.rd_fetch	= 0x00000002,
		.timing_rfcpb	= 0x00000505,
		.dvfs_con1	= 0x0E0E2121,
		.mif_drex_mr_data = {
			[0]	= 0x0000081C,
			[1]	= 0x0010081C,
			[2]	= 0x0000060C,
			[3]	= 0x0010060C,
		},
	},
};

static DEFINE_MUTEX(media_mutex);
static unsigned int media_enabled_fimc_lite;
static unsigned int media_enabled_gscl_local;
static unsigned int media_enabled_tv;
static unsigned int media_num_mixer_layer;
static unsigned int media_num_decon_layer;
static enum devfreq_media_resolution media_resolution;

static unsigned int (*timeout_table)[2];
static unsigned int wqhd_tv_window5;

struct devfreq_distriction_level {
	int mif_level;
	int disp_level;
};

struct devfreq_distriction_level distriction_fullhd[] = {
	{MIF_LV8,	DISP_LV3},			/* 103000 */
	{MIF_LV8,	DISP_LV3},			/* 103000 */
	{MIF_LV7,	DISP_LV3},			/* 138000 */
	{MIF_LV6,	DISP_LV2},			/* 165000 */
	{MIF_LV5,	DISP_LV2},			/* 206000 */
	{MIF_LV4,	DISP_LV2},			/* 275000 */
	{MIF_LV0,	DISP_LV0},			/* 825000 */
};

unsigned int timeout_fullhd[][2] = {
	{0x0FFF0FFF,	0x00000000},
	{0x0FFF0FFF,	0x00000000},
	{0x0FFF0FFF,	0x00000000},
	{0x0FFF0FFF,	0x00000000},
	{0x00800080,	0x000000FF},
	{0x00800080,	0x000000FF},
	{0x00800080,	0x000000FF},
	{0x00800080,	0x000000FF},
	{0x00800080,	0x000000FF},
};

struct devfreq_distriction_level distriction_fullhd_gscl[] = {
	{MIF_LV6,	DISP_LV2},
	{MIF_LV5,	DISP_LV2},
	{MIF_LV5,	DISP_LV2},
	{MIF_LV4,	DISP_LV2},
	{MIF_LV4,	DISP_LV2},
	{MIF_LV8,	DISP_LV3},
	{MIF_LV0,	DISP_LV0},
};

unsigned int timeout_fullhd_gscl[][2] = {
	{0x0FFF0FFF,	0x00000000},
	{0x0FFF0FFF,	0x00000000},
	{0x0FFF0FFF,	0x00000000},
	{0x0FFF0FFF,	0x00000000},
	{0x00800080,	0x000000FF},
	{0x00800080,	0x000000FF},
	{0x00800080,	0x000000FF},
	{0x00800080,	0x000000FF},
	{0x00800080,	0x000000FF},
};

struct devfreq_distriction_level distriction_fullhd_tv[] = {
	{MIF_LV4,	DISP_LV1},
	{MIF_LV4,	DISP_LV1},
	{MIF_LV4,	DISP_LV1},
	{MIF_LV4,	DISP_LV1},
	{MIF_LV4,	DISP_LV1},
	{MIF_LV3,	DISP_LV1},
	{MIF_LV0,	DISP_LV0},
};

unsigned int timeout_fullhd_tv[][2] = {
	{0x0FFF0FFF,	0x00000000},
	{0x0FFF0FFF,	0x00000000},
	{0x0FFF0FFF,	0x00000000},
	{0x00800080,	0x000000FF},
	{0x00800080,	0x000000FF},
	{0x00800080,	0x000000FF},
	{0x00800080,	0x000000FF},
	{0x00800080,	0x000000FF},
	{0x00800080,	0x000000FF},
};


struct devfreq_distriction_level distriction_fullhd_camera[] = {
	{MIF_LV3,	DISP_LV3},
	{MIF_LV3,	DISP_LV3},
	{MIF_LV3,	DISP_LV3},
	{MIF_LV3,	DISP_LV2},
	{MIF_LV3,	DISP_LV2},
	{MIF_LV3,	DISP_LV2},
	{MIF_LV0,	DISP_LV0},
};

unsigned int timeout_fullhd_camera[][2] = {
	{0x00800080,	0x000000FF},
	{0x00800080,	0x000000FF},
	{0x00800080,	0x000000FF},
	{0x00800080,	0x000000FF},
	{0x00800080,	0x000000FF},
	{0x00800080,	0x000000FF},
	{0x00800080,	0x000000FF},
	{0x00800080,	0x000000FF},
	{0x00800080,	0x000000FF},
};

struct devfreq_distriction_level distriction_wqhd[] = {
	{MIF_LV8,   	DISP_LV2},
	{MIF_LV7,	DISP_LV2},
	{MIF_LV5,	DISP_LV1},
	{MIF_LV3,	DISP_LV0},
	{MIF_LV3,   	DISP_LV0},
	{MIF_LV2,	DISP_LV0},
	{MIF_LV0,	DISP_LV0},
};

unsigned int timeout_wqhd[][2] = {
	{0x0FFF0FFF,	0x00000000},
	{0x0FFF0FFF,	0x00000000},
	{0x02000200,	0x000000FF},
	{0x00800080,	0x000000FF},
	{0x00800080,	0x000000FF},
	{0x00000000,	0x000000FF},
	{0x00000000,	0x000000FF},
	{0x00000000,	0x000000FF},
	{0x00000000,	0x000000FF},
};

struct devfreq_distriction_level distriction_wqhd_gscl[] = {
	{MIF_LV7,	DISP_LV2},
	{MIF_LV6,	DISP_LV2},
	{MIF_LV4,	DISP_LV1},
	{MIF_LV3,   	DISP_LV0},
	{MIF_LV2,	DISP_LV0},
	{MIF_LV2,	DISP_LV0},
	{MIF_LV0,	DISP_LV0},
};

unsigned int timeout_wqhd_gscl[][2] = {
	{0x0FFF0FFF,	0x00000000},
	{0x02000200,	0x000000FF},
	{0x00800080,	0x000000FF},
	{0x00800080,	0x000000FF},
	{0x00400040,	0x000000FF},
	{0x00400040,	0x000000FF},
	{0x00000000,	0x000000FF},
	{0x00000000,	0x000000FF},
	{0x00000000,	0x000000FF},
};

struct devfreq_distriction_level distriction_wqhd_tv[] = {
	{MIF_LV8,	DISP_LV1},
	{MIF_LV5,	DISP_LV1},
	{MIF_LV4,	DISP_LV1},
	{MIF_LV3,	DISP_LV0},
	{MIF_LV2,	DISP_LV0},
	{MIF_LV0,	DISP_LV0},
	{MIF_LV0,	DISP_LV0},
};

unsigned int timeout_wqhd_tv[][2] = {
	{0x0FFF0FFF,	0x00000000},
	{0x0FFF0FFF,	0x00000000},
	{0x02000200,	0x000000FF},
	{0x00800080,	0x000000FF},
	{0x00800080,	0x000000FF},
	{0x00400040,	0x000000FF},
	{0x00400040,	0x000000FF},
	{0x00000000,	0x000000FF},
	{0x00000000,	0x000000FF},
};

struct devfreq_distriction_level distriction_wqhd_camera[] = {
	{MIF_LV3,	DISP_LV1},
	{MIF_LV3,	DISP_LV1},
	{MIF_LV3,	DISP_LV1},
	{MIF_LV3,	DISP_LV0},
	{MIF_LV3,	DISP_LV0},
	{MIF_LV3,	DISP_LV0},
	{MIF_LV0,	DISP_LV0},
};

unsigned int timeout_wqhd_camera[][2] = {
	{0x00800080,	0x000000FF},
	{0x00800080,	0x000000FF},
	{0x00800080,	0x000000FF},
	{0x00800080,	0x000000FF},
	{0x00800080,	0x000000FF},
	{0x00800080,	0x000000FF},
	{0x00800080,	0x000000FF},
	{0x00800080,	0x000000FF},
	{0x00800080,	0x000000FF},
};

enum devfreq_mif_thermal_autorate {
	RATE_ONE = 0x000B005D,
	RATE_HALF = 0x0005002E,
	RATE_QUARTER = 0x00030017,
};

enum devfreq_mif_thermal_channel {
	THERMAL_CHANNEL0,
	THERMAL_CHANNEL1,
};

static struct workqueue_struct *devfreq_mif_thermal_wq_ch0;
static struct workqueue_struct *devfreq_mif_thermal_wq_ch1;
struct devfreq_thermal_work devfreq_mif_ch0_work = {
	.channel = THERMAL_CHANNEL0,
	.polling_period = 1000,
};
struct devfreq_thermal_work devfreq_mif_ch1_work = {
	.channel = THERMAL_CHANNEL1,
	.polling_period = 1000,
};
struct devfreq_data_mif *data_mif;

static void exynos5_devfreq_waiting_pause(struct devfreq_data_mif *data)
{
	unsigned int timeout = 1000;

	while ((__raw_readl(data->base_mif + 0x1008) & 0x00070000) != 0) {
		if (timeout == 0) {
			pr_err("DEVFREQ(MIF) : timeout to wait pause completion\n");
			return;
		}
		udelay(1);
		timeout--;
	}
}

static void exynos5_devfreq_waiting_mux(struct devfreq_data_mif *data)
{
	unsigned int timeout = 1000;

	while ((__raw_readl(data->base_mif + 0x0404) & 0x04440000) != 0) {
		if (timeout == 0) {
			pr_err("DEVFREQ(MIF) : timeout to wait mux completion\n");
			return;
		}
		udelay(1);
		timeout--;
	}
	timeout = 1000;
	while ((__raw_readl(data->base_mif + 0x0704) & 0x00000010) != 0) {
		if (timeout == 0) {
			pr_err("DEVFREQ(MIF) : timeout to wait divider completion\n");
			return;
		}
		udelay(1);
		timeout--;
	}
}

static int exynos5_devfreq_mif_set_freq(struct devfreq_data_mif *data,
					int target_idx,
					int old_idx)
{
	int i, j;
	struct devfreq_clk_info *clk_info;
	struct devfreq_clk_states *clk_states;
	unsigned int tmp;

	if (target_idx < old_idx) {
		tmp = __raw_readl(data->base_mif + aclk_clk2x_list[target_idx]->reg);
		tmp &= ~(aclk_clk2x_list[target_idx]->clr_value);
		tmp |= aclk_clk2x_list[target_idx]->set_value;
		__raw_writel(tmp, data->base_mif + aclk_clk2x_list[target_idx]->reg);

		exynos5_devfreq_waiting_pause(data);
		exynos5_devfreq_waiting_mux(data);

		for (i = 0; i < ARRAY_SIZE(devfreq_clk_mif_info_list); ++i) {
			clk_info = &devfreq_clk_mif_info_list[i][target_idx];
			clk_states = clk_info->states;
			if (clk_states) {
				for (j = 0; j < clk_states->state_count; ++j) {
					clk_set_parent(devfreq_mif_clk[clk_states->state[j].clk_idx].clk,
						devfreq_mif_clk[clk_states->state[j].parent_clk_idx].clk);
				}
			}

			if (clk_info->freq != 0) {
				clk_set_rate(devfreq_mif_clk[devfreq_clk_mif_info_idx[i]].clk, clk_info->freq);
				pr_debug("MIF clk name: %s, set_rate: %lu, get_rate: %lu\n",
						devfreq_mif_clk[devfreq_clk_mif_info_idx[i]].clk_name,
						clk_info->freq, clk_get_rate(devfreq_mif_clk[devfreq_clk_mif_info_idx[i]].clk));
			}
		}
	} else {
		for (i = 0; i < ARRAY_SIZE(devfreq_clk_mif_info_list); ++i) {
			clk_info = &devfreq_clk_mif_info_list[i][target_idx];
			clk_states = clk_info->states;

			if (clk_info->freq != 0)
				clk_set_rate(devfreq_mif_clk[devfreq_clk_mif_info_idx[i]].clk, clk_info->freq);

			if (clk_states) {
				for (j = 0; j < clk_states->state_count; ++j) {
					clk_set_parent(devfreq_mif_clk[clk_states->state[j].clk_idx].clk,
						devfreq_mif_clk[clk_states->state[j].parent_clk_idx].clk);
				}
			}

			if (clk_info->freq != 0) {
				clk_set_rate(devfreq_mif_clk[devfreq_clk_mif_info_idx[i]].clk, clk_info->freq);
				pr_debug("MIF clk name: %s, set_rate: %lu, get_rate: %lu\n",
						devfreq_mif_clk[devfreq_clk_mif_info_idx[i]].clk_name,
						clk_info->freq, clk_get_rate(devfreq_mif_clk[devfreq_clk_mif_info_idx[i]].clk));
			}
		}

		tmp = __raw_readl(data->base_mif + aclk_clk2x_list[target_idx]->reg);
		tmp &= ~(aclk_clk2x_list[target_idx]->clr_value);
		tmp |= aclk_clk2x_list[target_idx]->set_value;
		__raw_writel(tmp, data->base_mif + aclk_clk2x_list[target_idx]->reg);

		exynos5_devfreq_waiting_pause(data);
		exynos5_devfreq_waiting_mux(data);
	}
	return 0;
}

struct devfreq_mif_timing_parameter dmc_timing_parameter_default[] = {
	{	/* 825Mhz */
		.timing_row	= 0x575A9713,
		.timing_data	= 0x4740085E,
		.timing_power	= 0x545B0446,
		.rd_fetch	= 0x00000003,
		.timing_rfcpb	= 0x00002626,
		.dvfs_con1	= 0x0E0E2121,
		.mif_drex_mr_data = {
			[0]	= 0x00000870,
			[1]	= 0x00100870,
			[2]	= 0x0000060C,
			[3]	= 0x0010060C,
		},
	}, {	/* 633Mhz */
		.timing_row	= 0x4348758F,
		.timing_data	= 0x3530064A,
		.timing_power	= 0x40460335,
		.rd_fetch	= 0x00000002,
		.timing_rfcpb	= 0x00001D1D,
		.dvfs_con1	= 0x0A0A2121,
		.mif_drex_mr_data = {
			[0]	= 0x00000860,
			[1]	= 0x00100860,
			[2]	= 0x0000040C,
			[3]	= 0x0010040C,
		},
	}, {	/* 543Mhz */
		.timing_row	= 0x3A4764CD,
		.timing_data	= 0x35300549,
		.timing_power	= 0x383C0335,
		.rd_fetch	= 0x00000002,
		.timing_rfcpb	= 0x00001919,
		.dvfs_con1	= 0x09092121,
		.mif_drex_mr_data = {
			[0]	= 0x0000081C,
			[1]	= 0x0010081C,
			[2]	= 0x0000078C,
			[3]	= 0x0010078C,
		},
	}, {	/* 413Mhz */
		.timing_row	= 0x2C35538A,
		.timing_data	= 0x24200539,
		.timing_power	= 0x2C2E0225,
		.rd_fetch	= 0x00000002,
		.timing_rfcpb	= 0x00001313,
		.dvfs_con1	= 0x09092121,
		.mif_drex_mr_data = {
			[0]	= 0x0000081C,
			[1]	= 0x0010081C,
			[2]	= 0x0000060C,
			[3]	= 0x0010060C,
		},
	}, {	/* 275Mhz */
		.timing_row	= 0x1D244287,
		.timing_data	= 0x23200529,
		.timing_power	= 0x1C1F0225,
		.rd_fetch	= 0x00000002,
		.timing_rfcpb	= 0x00000D0D,
		.dvfs_con1	= 0x09092121,
		.mif_drex_mr_data = {
			[0]	= 0x0000081C,
			[1]	= 0x0010081C,
			[2]	= 0x0000060C,
			[3]	= 0x0010060C,
		},
	}, {	/* 206Mhz */
		.timing_row	= 0x162331C6,
		.timing_data	= 0x23200529,
		.timing_power	= 0x18170225,
		.rd_fetch	= 0x00000002,
		.timing_rfcpb	= 0x00000A0A,
		.dvfs_con1	= 0x09092121,
		.mif_drex_mr_data = {
			[0]	= 0x0000081C,
			[1]	= 0x0010081C,
			[2]	= 0x0000060C,
			[3]	= 0x0010060C,
		},
	}, {	/* 165Mhz */
		.timing_row	= 0x12223185,
		.timing_data	= 0x23200529,
		.timing_power	= 0x14130225,
		.rd_fetch	= 0x00000002,
		.timing_rfcpb	= 0x00000808,
		.dvfs_con1	= 0x09092121,
		.mif_drex_mr_data = {
			[0]	= 0x0000081C,
			[1]	= 0x0010081C,
			[2]	= 0x0000060C,
			[3]	= 0x0010060C,
		},
	}, {	/* 138Mhz */
		.timing_row	= 0x11222144,
		.timing_data	= 0x23200529,
		.timing_power	= 0x10100225,
		.rd_fetch	= 0x00000002,
		.timing_rfcpb	= 0x00000707,
		.dvfs_con1	= 0x09092121,
		.mif_drex_mr_data = {
			[0]	= 0x0000081C,
			[1]	= 0x0010081C,
			[2]	= 0x0000060C,
			[3]	= 0x0010060C,
		},
	}, {	/* 103Mhz */
		.timing_row	= 0x11222103,
		.timing_data	= 0x23200529,
		.timing_power	= 0x100C0225,
		.rd_fetch	= 0x00000002,
		.timing_rfcpb	= 0x00000505,
		.dvfs_con1	= 0x09092121,
		.mif_drex_mr_data = {
			[0]	= 0x0000081C,
			[1]	= 0x0010081C,
			[2]	= 0x0000060C,
			[3]	= 0x0010060C,
		},
	},
};

static bool use_mif_timing_set_0;

int exynos5_devfreq_mif_update_timingset(struct devfreq_data_mif *data)
{
	use_mif_timing_set_0 = ((__raw_readl(data->base_mif + 0x1004) & 0x1) == 0);

	return 0;
}

static int exynos5_devfreq_mif_change_timing_set(struct devfreq_data_mif *data)
{
	unsigned int tmp;

	if (use_mif_timing_set_0) {
		tmp = __raw_readl(data->base_mif + 0x1004);
		tmp |= 0x1;
		__raw_writel(tmp, data->base_mif + 0x1004);
	} else {
		tmp = __raw_readl(data->base_mif + 0x1004);
		tmp &= ~0x1;
		__raw_writel(tmp, data->base_mif + 0x1004);
	}

	exynos5_devfreq_mif_update_timingset(data);

	return 0;
}

#define TIMING_RFCPB_MASK	(0x3F)
static int exynos5_devfreq_mif_set_and_change_timing_set(struct devfreq_data_mif *data,
							int target_idx)
{
	struct devfreq_mif_timing_parameter *cur_parameter;
	unsigned int tmp;

	cur_parameter = &dmc_timing_parameter_3gb[target_idx];

	if (use_mif_timing_set_0) {
		__raw_writel(cur_parameter->timing_row, data->base_drex0 + 0xE4);
		__raw_writel(cur_parameter->timing_data, data->base_drex0 + 0xE8);
		__raw_writel(cur_parameter->timing_power, data->base_drex0 + 0xEC);
		tmp = __raw_readl(data->base_drex0 + 0x20);
		tmp &= ~(TIMING_RFCPB_MASK << 8);
		tmp |= (cur_parameter->timing_rfcpb & (TIMING_RFCPB_MASK << 8));
		__raw_writel(tmp, data->base_drex0 + 0x20);
		__raw_writel(cur_parameter->rd_fetch, data->base_drex0 + 0x50);

		__raw_writel(cur_parameter->timing_row, data->base_drex1 + 0xE4);
		__raw_writel(cur_parameter->timing_data, data->base_drex1 + 0xE8);
		__raw_writel(cur_parameter->timing_power, data->base_drex1 + 0xEC);
		tmp = __raw_readl(data->base_drex1 + 0x20);
		tmp &= ~(TIMING_RFCPB_MASK << 8);
		tmp |= (cur_parameter->timing_rfcpb & (TIMING_RFCPB_MASK << 8));
		__raw_writel(tmp, data->base_drex1 + 0x20);
		__raw_writel(cur_parameter->rd_fetch, data->base_drex1 + 0x50);
	} else {
		__raw_writel(cur_parameter->timing_row, data->base_drex0 + 0x34);
		__raw_writel(cur_parameter->timing_data, data->base_drex0 + 0x38);
		__raw_writel(cur_parameter->timing_power, data->base_drex0 + 0x3C);
		tmp = __raw_readl(data->base_drex0 + 0x20);
		tmp &= ~(TIMING_RFCPB_MASK);
		tmp |= (cur_parameter->timing_rfcpb & TIMING_RFCPB_MASK);
		__raw_writel(tmp, data->base_drex0 + 0x20);
		__raw_writel(cur_parameter->rd_fetch, data->base_drex0 + 0x4C);

		__raw_writel(cur_parameter->timing_row, data->base_drex1 + 0x34);
		__raw_writel(cur_parameter->timing_data, data->base_drex1 + 0x38);
		__raw_writel(cur_parameter->timing_power, data->base_drex1 + 0x3C);
		tmp = __raw_readl(data->base_drex1 + 0x20);
		tmp &= ~(TIMING_RFCPB_MASK);
		tmp |= (cur_parameter->timing_rfcpb & TIMING_RFCPB_MASK);
		__raw_writel(tmp, data->base_drex1 + 0x20);
		__raw_writel(cur_parameter->rd_fetch, data->base_drex1 + 0x4C);
	}
	exynos5_devfreq_mif_change_timing_set(data);

	return 0;
}

static int exynos5_devfreq_mif_set_timeout(struct devfreq_data_mif *data,
					int target_idx)
{
	if (timeout_table == NULL) {
		pr_err("DEVFREQ(MIF) : can't setting timeout value\n");
		return -EINVAL;
	}
	if (media_enabled_fimc_lite) {
		__raw_writel(timeout_table[target_idx][0], data->base_drex0 + 0xD0);
		__raw_writel(timeout_table[target_idx][0], data->base_drex0 + 0xC0);
		__raw_writel(timeout_table[target_idx][1], data->base_drex0 + 0x100);

		__raw_writel(timeout_table[target_idx][0], data->base_drex1 + 0xD0);
		__raw_writel(timeout_table[target_idx][0], data->base_drex1 + 0xC0);
		__raw_writel(timeout_table[target_idx][1], data->base_drex1 + 0x100);

		__raw_writel(0x0fff0fff, data->base_drex0 + 0xC8);
		__raw_writel(0x0fff0fff, data->base_drex1 + 0xC8);
	} else {
		if (wqhd_tv_window5 &&
				target_idx == MIF_LV1) {
			__raw_writel(timeout_table[MIF_LV2][0], data->base_drex0 + 0xD0);
			__raw_writel(timeout_table[MIF_LV2][0], data->base_drex0 + 0xC8);
			__raw_writel(timeout_table[MIF_LV2][0], data->base_drex0 + 0xC0);
			__raw_writel(timeout_table[MIF_LV2][1], data->base_drex0 + 0x100);
		} else {
			__raw_writel(timeout_table[target_idx][0], data->base_drex0 + 0xD0);
			__raw_writel(timeout_table[target_idx][0], data->base_drex0 + 0xC8);
			__raw_writel(timeout_table[target_idx][0], data->base_drex0 + 0xC0);
			__raw_writel(timeout_table[target_idx][1], data->base_drex0 + 0x100);
		}

		if (wqhd_tv_window5 &&
				target_idx == MIF_LV1) {
			__raw_writel(timeout_table[MIF_LV2][0], data->base_drex1 + 0xD0);
			__raw_writel(timeout_table[MIF_LV2][0], data->base_drex1 + 0xC8);
			__raw_writel(timeout_table[MIF_LV2][0], data->base_drex1 + 0xC0);
			__raw_writel(timeout_table[MIF_LV2][1], data->base_drex1 + 0x100);
		} else {
			__raw_writel(timeout_table[target_idx][0], data->base_drex1 + 0xD0);
			__raw_writel(timeout_table[target_idx][0], data->base_drex1 + 0xC8);
			__raw_writel(timeout_table[target_idx][0], data->base_drex1 + 0xC0);
			__raw_writel(timeout_table[target_idx][1], data->base_drex1 + 0x100);
		}
	}

	return 0;
}

extern void exynos5_update_district_disp_level(unsigned int idx);
void exynos5_update_media_layers(enum devfreq_media_type media_type, unsigned int value)
{
	unsigned int total_layer_count = 0;
	int disp_qos = MIF_LV3;
	int mif_qos = MIF_LV8;

	mutex_lock(&media_mutex);

	switch (media_type) {
	case TYPE_FIMC_LITE:
		media_enabled_fimc_lite = value;
		break;
	case TYPE_MIXER:
		media_num_mixer_layer = value;
		break;
	case TYPE_DECON:
		media_num_decon_layer = value;
		break;
	case TYPE_GSCL_LOCAL:
		media_enabled_gscl_local = value;
		break;
	case TYPE_TV:
		media_num_mixer_layer = value;
		media_enabled_tv = !!value;
		break;
	case TYPE_RESOLUTION:
		media_resolution = value;
		mutex_unlock(&media_mutex);
		return;
	}

	total_layer_count = media_num_mixer_layer + media_num_decon_layer;

	if (total_layer_count > 6)
		total_layer_count = 6;

	if (media_resolution == RESOLUTION_FULLHD) {
		if (media_enabled_fimc_lite) {
			if (mif_qos > distriction_fullhd_camera[total_layer_count].mif_level)
				mif_qos = distriction_fullhd_camera[total_layer_count].mif_level;
			timeout_table = timeout_fullhd_camera;
		}
		if (media_enabled_gscl_local) {
			if (total_layer_count == NUM_LAYER_5) {
				pr_err("DEVFREQ(MIF) : can't support mif and disp distriction. using gscl local with 5 windows.\n");
				goto out;
			}
			if (mif_qos > distriction_fullhd_gscl[total_layer_count].mif_level)
				mif_qos = distriction_fullhd_gscl[total_layer_count].mif_level;
			if (disp_qos > distriction_fullhd_gscl[total_layer_count].disp_level)
				disp_qos = distriction_fullhd_gscl[total_layer_count].disp_level;
			timeout_table = timeout_fullhd_gscl;
		}
		if (media_enabled_tv) {
			if (mif_qos > distriction_fullhd_tv[total_layer_count].mif_level)
				mif_qos = distriction_fullhd_tv[total_layer_count].mif_level;
			if (disp_qos > distriction_fullhd_tv[total_layer_count].disp_level)
				disp_qos = distriction_fullhd_tv[total_layer_count].disp_level;
			timeout_table = timeout_fullhd_tv;
		}
		if (!media_enabled_fimc_lite && !media_enabled_gscl_local && !media_enabled_tv)
			timeout_table = timeout_fullhd;
		if (mif_qos > distriction_fullhd[total_layer_count].mif_level)
			mif_qos = distriction_fullhd[total_layer_count].mif_level;
		if (disp_qos > distriction_fullhd[total_layer_count].disp_level)
			disp_qos = distriction_fullhd[total_layer_count].disp_level;
	} else if (media_resolution == RESOLUTION_WQHD) {
		if (media_enabled_fimc_lite) {
			if (mif_qos > distriction_wqhd_camera[total_layer_count + media_enabled_gscl_local].mif_level)
				mif_qos = distriction_wqhd_camera[total_layer_count + media_enabled_gscl_local].mif_level;
			if (disp_qos > distriction_wqhd_camera[total_layer_count].disp_level)
				disp_qos = distriction_wqhd_camera[total_layer_count].disp_level;
			timeout_table = timeout_wqhd_camera;
		}
		if (media_enabled_tv) {
			if (mif_qos > distriction_wqhd_tv[total_layer_count + media_enabled_gscl_local].mif_level)
				mif_qos = distriction_wqhd_tv[total_layer_count + media_enabled_gscl_local].mif_level;
			if (disp_qos > distriction_wqhd_tv[total_layer_count].disp_level)
				disp_qos = distriction_wqhd_tv[total_layer_count].disp_level;
			timeout_table = timeout_wqhd_tv;

			wqhd_tv_window5 = (total_layer_count == NUM_LAYER_5);
		} else {
			wqhd_tv_window5 = false;
		}
		if (!media_enabled_fimc_lite && !media_enabled_gscl_local && !media_enabled_tv)
			timeout_table = timeout_wqhd;
		if (mif_qos > distriction_wqhd[total_layer_count + media_enabled_gscl_local].mif_level)
			mif_qos = distriction_wqhd[total_layer_count + media_enabled_gscl_local].mif_level;
		if (disp_qos > distriction_wqhd[total_layer_count].disp_level)
			disp_qos = distriction_wqhd[total_layer_count].disp_level;
	}

	if (pm_qos_request_active(&exynos5_mif_bts_qos)) {
		if (mif_qos != MIF_LV8)
			pm_qos_update_request(&exynos5_mif_bts_qos, devfreq_mif_opp_list[mif_qos].freq);
		else
			pm_qos_update_request(&exynos5_mif_bts_qos, devfreq_mif_opp_list[MIF_LV_COUNT-1].freq);
	}

	exynos5_update_district_disp_level(disp_qos);
out:
	mutex_unlock(&media_mutex);
}

#define DLL_ON_BASE_VOLT	(900000)
static int exynos5_devfreq_calculate_dll_lock_value(struct devfreq_data_mif *data,
							unsigned long vdd_mif_l0)
{
	return  ((vdd_mif_l0 - DLL_ON_BASE_VOLT + 9999) / 10000) * 2;
}

void exynos5_devfreq_set_dll_lock_value(struct devfreq_data_mif *data,
							unsigned long vdd_mif_l0)
{
	/* 9999 make ceiling result */
	int lock_value_offset = exynos5_devfreq_calculate_dll_lock_value(data, vdd_mif_l0);
	int ctrl_force, ctrl_force_value;

	ctrl_force = __raw_readl(data->base_lpddr_phy0 + 0xB0);
	ctrl_force_value = (ctrl_force >> CTRL_FORCE_SHIFT) & CTRL_FORCE_MASK;
	ctrl_force_value += lock_value_offset;
	ctrl_force &= ~(CTRL_FORCE_MASK << CTRL_FORCE_SHIFT);
	ctrl_force |= (ctrl_force_value << CTRL_FORCE_SHIFT);
	__raw_writel(ctrl_force, data->base_lpddr_phy0 + 0xB0);

	ctrl_force = __raw_readl(data->base_lpddr_phy1 + 0xB0);
	ctrl_force_value = (ctrl_force >> CTRL_FORCE_SHIFT) & CTRL_FORCE_MASK;
	ctrl_force_value += lock_value_offset;
	ctrl_force &= ~(CTRL_FORCE_MASK << CTRL_FORCE_SHIFT);
	ctrl_force |= (ctrl_force_value << CTRL_FORCE_SHIFT);
	__raw_writel(ctrl_force, data->base_lpddr_phy1 + 0xB0);
}

static void exynos5_devfreq_mif_dynamic_setting(struct devfreq_data_mif *data,
						bool flag)
{
	unsigned int tmp;

	if (flag) {
		tmp = __raw_readl(data->base_drex0);
		tmp |= (0x2 << 6);
		__raw_writel(tmp, data->base_drex0);
		tmp = __raw_readl(data->base_drex1);
		tmp |= (0x2 << 6);
		__raw_writel(tmp, data->base_drex1);

		tmp = __raw_readl(data->base_drex0 + 0x0004);
		tmp |= ((0x1 << 29) | (0x1 << 5) | (0x1 << 1));
		__raw_writel(tmp, data->base_drex0 + 0x0004);
		tmp = __raw_readl(data->base_drex1 + 0x0004);
		tmp |= ((0x1 << 29) | (0x1 << 5) | (0x1 << 1));
		__raw_writel(tmp, data->base_drex1 + 0x0004);

		tmp = __raw_readl(data->base_drex0 + 0x0008);
		tmp |= (0x3F);
		__raw_writel(tmp, data->base_drex0 + 0x0008);
		tmp = __raw_readl(data->base_drex1 + 0x0008);
		tmp |= (0x3F);
		__raw_writel(tmp, data->base_drex1 + 0x0008);
	} else {
		tmp = __raw_readl(data->base_drex0);
		tmp &= ~(0x3 << 6);
		__raw_writel(tmp, data->base_drex0);
		tmp = __raw_readl(data->base_drex1);
		tmp &= ~(0x3 << 6);
		__raw_writel(tmp, data->base_drex1);

		tmp = __raw_readl(data->base_drex0 + 0x0004);
		tmp &= ~((0x1 << 29) | (0x1 << 5) | (0x1 << 1));
		__raw_writel(tmp, data->base_drex0 + 0x0004);
		tmp = __raw_readl(data->base_drex1 + 0x0004);
		tmp &= ~((0x1 << 29) | (0x1 << 5) | (0x1 << 1));
		__raw_writel(tmp, data->base_drex1 + 0x0004);

		tmp = __raw_readl(data->base_drex0 + 0x0008);
		tmp &= ~(0x3F);
		__raw_writel(tmp, data->base_drex0 + 0x0008);
		tmp = __raw_readl(data->base_drex1 + 0x0008);
		tmp &= ~(0x3F);
		__raw_writel(tmp, data->base_drex1 + 0x0008);
	}
}

static int exynos5_devfreq_mif_set_dll(struct devfreq_data_mif *data,
					unsigned long target_volt,
					int target_idx)
{
	unsigned int tmp;
	unsigned int lock_value;
	unsigned int timeout;

	if (target_idx == MIF_LV0) {
		/* only MIF_LV0 use DLL tacing mode(CLKM_PHY_C_ENABLE mux gating 1(enable)/0(disable)). */
		tmp = __raw_readl(data->base_lpddr_phy0 + 0xB0);
		tmp |= (0x1 << 5);
		__raw_writel(tmp, data->base_lpddr_phy0 + 0xB0);
		tmp = __raw_readl(data->base_lpddr_phy1 + 0xB0);
		tmp |= (0x1 << 5);
		__raw_writel(tmp, data->base_lpddr_phy1 + 0xB0);

		timeout = 1000;
		while ((__raw_readl(data->base_lpddr_phy0 + 0xB4) & 0x5) != 0x5) {
			if (timeout-- == 0) {
				pr_err("DEVFREQ(MIF) : Timeout to wait dll on(lpddrphy0)\n");
				return -EINVAL;
			}
			udelay(1);
		}
		timeout = 1000;
		while ((__raw_readl(data->base_lpddr_phy1 + 0xB4) & 0x5) != 0x5) {
			if (timeout-- == 0) {
				pr_err("DEVFREQ(MIF) : Timeout to wait dll on(lpddrphy1)\n");
				return -EINVAL;
			}
			udelay(1);
		}
	} else {
		/* DLL Tracing off mode */
		tmp = __raw_readl(data->base_lpddr_phy0 + 0xB0);
		tmp &= ~(0x1 << 5);
		__raw_writel(tmp, data->base_lpddr_phy0 + 0xB0);
		tmp = __raw_readl(data->base_lpddr_phy1 + 0xB0);
		tmp &= ~(0x1 << 5);
		__raw_writel(tmp, data->base_lpddr_phy1 + 0xB0);

		/* Get Current DLL lock value */
		tmp = __raw_readl(data->base_lpddr_phy0 + 0xB4);
		lock_value = (tmp >> CTRL_LOCK_VALUE_SHIFT) & CTRL_LOCK_VALUE_MASK;
		tmp = __raw_readl(data->base_lpddr_phy0 + 0xB0);
		tmp |= (0x1 << 5);
		__raw_writel(tmp, data->base_lpddr_phy0 + 0xB0);
		tmp = __raw_readl(data->base_lpddr_phy0 + 0xB0);
		tmp &= ~(CTRL_FORCE_MASK << CTRL_FORCE_SHIFT);
		tmp |= (lock_value << CTRL_FORCE_SHIFT);
		__raw_writel(tmp, data->base_lpddr_phy0 + 0xB0);
		tmp = __raw_readl(data->base_lpddr_phy0 + 0xB0);
		tmp &= ~(0x1 << 5);
		__raw_writel(tmp, data->base_lpddr_phy0 + 0xB0);

		tmp = __raw_readl(data->base_lpddr_phy1 + 0xB4);
		lock_value = (tmp >> CTRL_LOCK_VALUE_SHIFT) & CTRL_LOCK_VALUE_MASK;
		tmp = __raw_readl(data->base_lpddr_phy1 + 0xB0);
		tmp |= (0x1 << 5);
		__raw_writel(tmp, data->base_lpddr_phy1 + 0xB0);
		tmp = __raw_readl(data->base_lpddr_phy1 + 0xB0);
		tmp &= ~(CTRL_FORCE_MASK << CTRL_FORCE_SHIFT);
		tmp |= (lock_value << CTRL_FORCE_SHIFT);
		__raw_writel(tmp, data->base_lpddr_phy1 + 0xB0);
		tmp = __raw_readl(data->base_lpddr_phy1 + 0xB0);
		tmp &= ~(0x1 << 5);
		__raw_writel(tmp, data->base_lpddr_phy1 + 0xB0);
	}

	return 0;
}

static int exynos5_devfreq_mif_set_volt(struct devfreq_data_mif *data,
					unsigned long volt,
					unsigned long volt_range)
{
	if (data->old_volt == volt)
		goto out;

	regulator_set_voltage(data->vdd_mif, volt, volt_range);
	data->old_volt = volt;
out:
	return 0;
}

#ifdef CONFIG_EXYNOS_THERMAL
unsigned int get_limit_voltage(unsigned int voltage, unsigned int volt_offset)
{
	if (voltage > LIMIT_COLD_VOLTAGE)
		return voltage;

	if (voltage + volt_offset > LIMIT_COLD_VOLTAGE)
		return LIMIT_COLD_VOLTAGE;

	return voltage + volt_offset;
}
#endif

#ifdef CONFIG_EXYNOS_THERMAL
extern struct pm_qos_request min_mif_thermal_qos;
int exynos5_devfreq_mif_tmu_notifier(struct notifier_block *nb, unsigned long event,
						void *v)
{
	struct devfreq_data_mif *data = container_of(nb, struct devfreq_data_mif,
							tmu_notifier);
	unsigned int prev_volt, set_volt;
	unsigned int *on = v;
	unsigned int tmp;
	unsigned int ctrl_force_value;

	if (event == TMU_COLD) {
		if (pm_qos_request_active(&min_mif_thermal_qos))
			pm_qos_update_request(&min_mif_thermal_qos,
					data->initial_freq);

		if (*on) {
			mutex_lock(&data->lock);

			prev_volt = regulator_get_voltage(data->vdd_mif);

			if (data->volt_offset != COLD_VOLT_OFFSET) {
				data->volt_offset = COLD_VOLT_OFFSET;
			} else {
				mutex_unlock(&data->lock);
				return NOTIFY_OK;
			}

			set_volt = get_limit_voltage(prev_volt, data->volt_offset);
			regulator_set_voltage(data->vdd_mif, set_volt, set_volt + VOLT_STEP);

			/* Update CTRL FORCE */
			tmp = __raw_readl(data->base_lpddr_phy0 + 0xB0);
			ctrl_force_value = (tmp >> CTRL_FORCE_SHIFT) & CTRL_FORCE_MASK;
			ctrl_force_value += CTRL_FORCE_OFFSET;
			tmp &= ~(CTRL_FORCE_MASK << CTRL_FORCE_SHIFT);
			tmp |= (ctrl_force_value << CTRL_FORCE_SHIFT);
			__raw_writel(tmp, data->base_lpddr_phy0 + 0xB0);

			tmp = __raw_readl(data->base_lpddr_phy1 + 0xB0);
			ctrl_force_value = (tmp >> CTRL_FORCE_SHIFT) & CTRL_FORCE_MASK;
			ctrl_force_value += CTRL_FORCE_OFFSET;
			tmp &= ~(CTRL_FORCE_MASK << CTRL_FORCE_SHIFT);
			tmp |= (ctrl_force_value << CTRL_FORCE_SHIFT);
			__raw_writel(tmp, data->base_lpddr_phy1 + 0xB0);

			mutex_unlock(&data->lock);
		} else {
			mutex_lock(&data->lock);

			prev_volt = regulator_get_voltage(data->vdd_mif);

			if (data->volt_offset != 0) {
				data->volt_offset = 0;
			} else {
				mutex_unlock(&data->lock);
				return NOTIFY_OK;
			}

			set_volt = get_limit_voltage(prev_volt - COLD_VOLT_OFFSET, data->volt_offset);
			regulator_set_voltage(data->vdd_mif, set_volt, set_volt + VOLT_STEP);

			/* Update CTRL FORCE */
			tmp = __raw_readl(data->base_lpddr_phy0 + 0xB0);
			ctrl_force_value = (tmp >> CTRL_FORCE_SHIFT) & CTRL_FORCE_MASK;
			ctrl_force_value -= CTRL_FORCE_OFFSET;
			tmp &= ~(CTRL_FORCE_MASK << CTRL_FORCE_SHIFT);
			tmp |= (ctrl_force_value << CTRL_FORCE_SHIFT);
			__raw_writel(tmp, data->base_lpddr_phy0 + 0xB0);

			tmp = __raw_readl(data->base_lpddr_phy1 + 0xB0);
			ctrl_force_value = (tmp >> CTRL_FORCE_SHIFT) & CTRL_FORCE_MASK;
			ctrl_force_value -= CTRL_FORCE_OFFSET;
			tmp &= ~(CTRL_FORCE_MASK << CTRL_FORCE_SHIFT);
			tmp |= (ctrl_force_value << CTRL_FORCE_SHIFT);
			__raw_writel(tmp, data->base_lpddr_phy1 + 0xB0);

			mutex_unlock(&data->lock);
		}

		if (pm_qos_request_active(&min_mif_thermal_qos))
			pm_qos_update_request(&min_mif_thermal_qos,
					data->default_qos);
	}

	return NOTIFY_OK;
}
#endif

static void exynos5_devfreq_thermal_event(struct devfreq_thermal_work *work)
{
	if (work->polling_period == 0)
		return;

#ifdef CONFIG_SCHED_HMP
	mod_delayed_work_on(0,
			work->work_queue,
			&work->devfreq_mif_thermal_work,
			msecs_to_jiffies(work->polling_period));
#else
	queue_delayed_work(work->work_queue,
			&work->devfreq_mif_thermal_work,
			msecs_to_jiffies(work->polling_period));
#endif
}

static ssize_t mif_show_templvl_ch0_0(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%u\n", devfreq_mif_ch0_work.thermal_level_cs0);
}
static ssize_t mif_show_templvl_ch0_1(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%u\n", devfreq_mif_ch0_work.thermal_level_cs1);
}
static ssize_t mif_show_templvl_ch1_0(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%u\n", devfreq_mif_ch1_work.thermal_level_cs0);
}
static ssize_t mif_show_templvl_ch1_1(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%u\n", devfreq_mif_ch1_work.thermal_level_cs1);
}

static DEVICE_ATTR(mif_templvl_ch0_0, 0644, mif_show_templvl_ch0_0, NULL);
static DEVICE_ATTR(mif_templvl_ch0_1, 0644, mif_show_templvl_ch0_1, NULL);
static DEVICE_ATTR(mif_templvl_ch1_0, 0644, mif_show_templvl_ch1_0, NULL);
static DEVICE_ATTR(mif_templvl_ch1_1, 0644, mif_show_templvl_ch1_1, NULL);

static struct attribute *devfreq_mif_sysfs_entries[] = {
	&dev_attr_mif_templvl_ch0_0.attr,
	&dev_attr_mif_templvl_ch0_1.attr,
	&dev_attr_mif_templvl_ch1_0.attr,
	&dev_attr_mif_templvl_ch1_1.attr,
	NULL,
};

struct attribute_group devfreq_mif_attr_group = {
	.name   = "temp_level",
	.attrs  = devfreq_mif_sysfs_entries,
};

#define MRSTATUS_THERMAL_BIT_SHIFT	(7)
#define MRSTATUS_THERMAL_BIT_MASK	(1)
#define MRSTATUS_THERMAL_LV_MASK	(0x7)
static void exynos5_devfreq_thermal_monitor(struct work_struct *work)
{
	struct delayed_work *d_work = container_of(work, struct delayed_work, work);
	struct devfreq_thermal_work *thermal_work =
			container_of(d_work, struct devfreq_thermal_work, devfreq_mif_thermal_work);
	unsigned int mrstatus, tmp_thermal_level, max_thermal_level = 0;
	unsigned int timingaref_value = RATE_HALF;
	unsigned long max_freq = data_mif->cal_qos_max;
	bool throttling = false;
	void __iomem *base_drex = NULL;

	if (thermal_work->channel == THERMAL_CHANNEL0) {
		base_drex = data_mif->base_drex0;
	} else if (thermal_work->channel == THERMAL_CHANNEL1) {
		base_drex = data_mif->base_drex1;
	}


	mutex_lock(&data_mif->lock);

	__raw_writel(0x09001000, base_drex + 0x10);
	mrstatus = __raw_readl(base_drex + 0x54);
	tmp_thermal_level = (mrstatus & MRSTATUS_THERMAL_LV_MASK);
	if (tmp_thermal_level > max_thermal_level)
		max_thermal_level = tmp_thermal_level;

	thermal_work->thermal_level_cs0 = tmp_thermal_level;

	__raw_writel(0x09101000, base_drex + 0x10);
	mrstatus = __raw_readl(base_drex + 0x54);
	tmp_thermal_level = (mrstatus & MRSTATUS_THERMAL_LV_MASK);
	if (tmp_thermal_level > max_thermal_level)
		max_thermal_level = tmp_thermal_level;

	thermal_work->thermal_level_cs1 = tmp_thermal_level;

	mutex_unlock(&data_mif->lock);

	switch (max_thermal_level) {
	case 0:
	case 1:
	case 2:
	case 3:
		timingaref_value = RATE_HALF;
		thermal_work->polling_period = 1000;
		break;
	case 4:
		timingaref_value = RATE_HALF;
		thermal_work->polling_period = 300;
		pr_info("MIF: max_thermal_level is %d\n", max_thermal_level);
		break;
	case 6:
		throttling = true;
	case 5:
		timingaref_value = RATE_QUARTER;
		thermal_work->polling_period = 100;
		pr_info("MIF: max_thermal_level is %d, so it need throttling\n", max_thermal_level);
		break;
	default:
		pr_err("DEVFREQ(MIF) : can't support memory thermal level\n");
		return;
	}

	if (throttling)
		max_freq = devfreq_mif_opp_list[MIF_LV5].freq;
	else
		max_freq = data_mif->cal_qos_max;

	if (thermal_work->max_freq != max_freq) {
		thermal_work->max_freq = max_freq;
		mutex_lock(&data_mif->devfreq->lock);
		update_devfreq(data_mif->devfreq);
		mutex_unlock(&data_mif->devfreq->lock);
	}

	__raw_writel(timingaref_value, base_drex + 0x30);
	exynos5_devfreq_thermal_event(thermal_work);
}

void exynos5_devfreq_init_thermal(void)
{
	devfreq_mif_thermal_wq_ch0 = create_freezable_workqueue("devfreq_thermal_wq_ch0");
	devfreq_mif_thermal_wq_ch1 = create_freezable_workqueue("devfreq_thermal_wq_ch1");

	INIT_DELAYED_WORK(&devfreq_mif_ch0_work.devfreq_mif_thermal_work,
			exynos5_devfreq_thermal_monitor);
	INIT_DELAYED_WORK(&devfreq_mif_ch1_work.devfreq_mif_thermal_work,
			exynos5_devfreq_thermal_monitor);

	devfreq_mif_ch0_work.work_queue = devfreq_mif_thermal_wq_ch0;
	devfreq_mif_ch1_work.work_queue = devfreq_mif_thermal_wq_ch1;

	exynos5_devfreq_thermal_event(&devfreq_mif_ch0_work);
	exynos5_devfreq_thermal_event(&devfreq_mif_ch1_work);
}

int exynos5_devfreq_mif_init_clock(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(devfreq_mif_clk); ++i) {
		devfreq_mif_clk[i].clk = clk_get(NULL, devfreq_mif_clk[i].clk_name);
		if (IS_ERR_OR_NULL(devfreq_mif_clk[i].clk)) {
			pr_err("DEVFREQ(MIF) : %s can't get clock\n", devfreq_mif_clk[i].clk_name);
			return -EINVAL;
		}
		pr_debug("MIF clk name: %s, rate: %lu\n", devfreq_mif_clk[i].clk_name, clk_get_rate(devfreq_mif_clk[i].clk));
	}

	return 0;
}

int exynos5_devfreq_mif_init_parameter(struct devfreq_data_mif *data)
{
	data->base_mif = ioremap(0x105B0000, SZ_64K);
	data->base_sysreg_mif = ioremap(0x105E0000, SZ_64K);
	data->base_drex0 = ioremap(0x10400000, SZ_64K);
	data->base_drex1 = ioremap(0x10440000, SZ_64K);
	data->base_lpddr_phy0 = ioremap(0x10420000, SZ_64K);
	data->base_lpddr_phy1 = ioremap(0x10460000, SZ_64K);

	exynos5_devfreq_mif_update_timingset(data);

	return 0;
}

int exynos5433_devfreq_mif_init(struct devfreq_data_mif *data)
{
	int ret = 0;
	timeout_table = timeout_fullhd;
	media_enabled_fimc_lite = false;
	media_enabled_gscl_local = false;
	media_enabled_tv = false;
	media_num_mixer_layer = false;
	media_num_decon_layer = false;
	wqhd_tv_window5 = false;
	data_mif = data;

	data->max_state = MIF_LV_COUNT;

	data->mif_set_freq = exynos5_devfreq_mif_set_freq;
	data->mif_set_and_change_timing_set= exynos5_devfreq_mif_set_and_change_timing_set;
	data->mif_set_timeout = exynos5_devfreq_mif_set_timeout;
	data->mif_set_dll = exynos5_devfreq_mif_set_dll;
	data->mif_dynamic_setting = exynos5_devfreq_mif_dynamic_setting;
	data->mif_set_volt = exynos5_devfreq_mif_set_volt;

	data->mif_asv_abb_table = kzalloc(sizeof(int) * data->max_state, GFP_KERNEL);
	if (data->mif_asv_abb_table == NULL) {
		pr_err("DEVFREQ(MIF) : Failed to allocate abb table\n");
		ret = -ENOMEM;
		return ret;
	}

	exynos5_devfreq_mif_init_clock();
	exynos5_devfreq_mif_init_parameter(data);

	return ret;
}

int exynos5433_devfreq_mif_deinit(struct devfreq_data_mif *data)
{
	flush_workqueue(devfreq_mif_thermal_wq_ch0);
	destroy_workqueue(devfreq_mif_thermal_wq_ch0);
	flush_workqueue(devfreq_mif_thermal_wq_ch1);
	destroy_workqueue(devfreq_mif_thermal_wq_ch1);

	return 0;
}

/* end of MIF related function */

/* ========== 2. INT related function */
static struct devfreq_data_int *data_int;

extern struct pm_qos_request min_int_thermal_qos;

enum devfreq_int_idx {
	INT_LV0,
	INT_LV1,
	INT_LV2,
	INT_LV3,
	INT_LV4,
	INT_LV5,
	INT_LV6,
	INT_LV_COUNT,
};

enum devfreq_int_clk {
	DOUT_ACLK_BUS0_400,
	DOUT_ACLK_BUS1_400,
	DOUT_MIF_PRE_4_INT,
	DOUT_ACLK_BUS2_400,
	MOUT_BUS_PLL_USER_4_INT,
	MOUT_MFC_PLL_USER_4_INT,
	MOUT_ISP_PLL_4_INT,
	MOUT_MPHY_PLL_USER,
	MOUT_ACLK_G2D_400_A,
	DOUT_ACLK_G2D_400,
	DOUT_ACLK_G2D_266,
	DOUT_ACLK_GSCL_333,
	MOUT_ACLK_MSCL_400_A,
	DOUT_ACLK_MSCL_400,
	MOUT_SCLK_JPEG_A,
	MOUT_SCLK_JPEG_B,
	MOUT_SCLK_JPEG_C,
	DOUT_SCLK_JPEG,
	MOUT_ACLK_MFC_400_A,
	MOUT_ACLK_MFC_400_B,
	MOUT_ACLK_MFC_400_C,
	DOUT_ACLK_MFC_400,
	DOUT_ACLK_HEVC_400,
	INT_CLK_COUNT,
};

struct devfreq_clk_list devfreq_int_clk[INT_CLK_COUNT] = {
	{"dout_aclk_bus0_400",},
	{"dout_aclk_bus1_400",},
	{"dout_mif_pre",},
	{"dout_aclk_bus2_400",},
	{"mout_bus_pll_user",},
	{"mout_mfc_pll_user",},
	{"mout_isp_pll",},
	{"mout_mphy_pll_user",},
	{"mout_aclk_g2d_400_a",},
	{"dout_aclk_g2d_400",},
	{"dout_aclk_g2d_266",},
	{"dout_aclk_gscl_333",},
	{"mout_aclk_mscl_400_a",},
	{"dout_aclk_mscl_400", },
	{"mout_sclk_jpeg_a",},
	{"mout_sclk_jpeg_b",},
	{"mout_sclk_jpeg_c",},
	{"dout_sclk_jpeg",},
	{"mout_aclk_mfc_400_a",},
	{"mout_aclk_mfc_400_b",},
	{"mout_aclk_mfc_400_c",},
	{"dout_aclk_mfc_400",},
	{"dout_aclk_hevc_400",},
};

struct devfreq_opp_table devfreq_int_opp_list[] = {
	{INT_LV0,	400000,	1075000},
	{INT_LV1,	317000,	1025000},
	{INT_LV2,	267000,	1000000},
	{INT_LV3,	200000,	 975000},
	{INT_LV4,	160000,	 962500},
	{INT_LV5,	133000,	 950000},
	{INT_LV6,	100000,	 937500},
};

struct devfreq_clk_state aclk_g2d_mfc_pll[] = {
	{MOUT_ACLK_G2D_400_A,	MOUT_MFC_PLL_USER_4_INT},
};

struct devfreq_clk_state aclk_g2d_bus_pll[] = {
	{MOUT_ACLK_G2D_400_A,	MOUT_BUS_PLL_USER_4_INT},
};

struct devfreq_clk_state aclk_mscl_mfc_pll[] = {
	{MOUT_ACLK_MSCL_400_A,	MOUT_MFC_PLL_USER_4_INT},
};

struct devfreq_clk_state aclk_mscl_bus_pll[] = {
	{MOUT_ACLK_MSCL_400_A,	MOUT_BUS_PLL_USER_4_INT},
};

struct devfreq_clk_state sclk_jpeg_mfc_pll[] = {
	{MOUT_SCLK_JPEG_B,	MOUT_MFC_PLL_USER_4_INT},
	{MOUT_SCLK_JPEG_C,	MOUT_SCLK_JPEG_B},
};

struct devfreq_clk_state sclk_jpeg_bus_pll[] = {
	{MOUT_SCLK_JPEG_A,	MOUT_BUS_PLL_USER_4_INT},
	{MOUT_SCLK_JPEG_B,	MOUT_SCLK_JPEG_A},
	{MOUT_SCLK_JPEG_C,	MOUT_SCLK_JPEG_B},
};

struct devfreq_clk_state aclk_mfc_400_bus_pll[] = {
	{MOUT_ACLK_MFC_400_B,	MOUT_BUS_PLL_USER_4_INT},
	{MOUT_ACLK_MFC_400_C,	MOUT_ACLK_MFC_400_B},
};

struct devfreq_clk_state aclk_mfc_400_mfc_pll[] = {
	{MOUT_ACLK_MFC_400_A,	MOUT_MFC_PLL_USER_4_INT},
	{MOUT_ACLK_MFC_400_B,	MOUT_ACLK_MFC_400_A},
	{MOUT_ACLK_MFC_400_C,	MOUT_ACLK_MFC_400_B},
};

struct devfreq_clk_states aclk_g2d_mfc_pll_list = {
	.state = aclk_g2d_mfc_pll,
	.state_count = ARRAY_SIZE(aclk_g2d_mfc_pll),
};

struct devfreq_clk_states aclk_g2d_bus_pll_list = {
	.state = aclk_g2d_bus_pll,
	.state_count = ARRAY_SIZE(aclk_g2d_bus_pll),
};

struct devfreq_clk_states aclk_mscl_mfc_pll_list = {
	.state = aclk_mscl_mfc_pll,
	.state_count = ARRAY_SIZE(aclk_mscl_mfc_pll),
};

struct devfreq_clk_states aclk_mscl_bus_pll_list = {
	.state = aclk_mscl_bus_pll,
	.state_count = ARRAY_SIZE(aclk_mscl_bus_pll),
};

struct devfreq_clk_states sclk_jpeg_mfc_pll_list = {
	.state = sclk_jpeg_mfc_pll,
	.state_count = ARRAY_SIZE(sclk_jpeg_mfc_pll),
};

struct devfreq_clk_states sclk_jpeg_bus_pll_list = {
	.state = sclk_jpeg_bus_pll,
	.state_count = ARRAY_SIZE(sclk_jpeg_bus_pll),
};

struct devfreq_clk_states aclk_mfc_400_bus_pll_list = {
	.state = aclk_mfc_400_bus_pll,
	.state_count = ARRAY_SIZE(aclk_mfc_400_bus_pll),
};

struct devfreq_clk_states aclk_mfc_400_mfc_pll_list = {
	.state = aclk_mfc_400_mfc_pll,
	.state_count = ARRAY_SIZE(aclk_mfc_400_bus_pll),
};

struct devfreq_clk_info aclk_bus0_400[] = {
	{INT_LV0,	400000000,	0,	NULL},
	{INT_LV1,	267000000,	0,	NULL},
	{INT_LV2,	267000000,	0,	NULL},
	{INT_LV3,	200000000,	0,	NULL},
	{INT_LV4,	160000000,	0,	NULL},
	{INT_LV5,	134000000,	0,	NULL},
	{INT_LV6,	100000000,	0,	NULL},
};

struct devfreq_clk_info aclk_bus1_400[] = {
	{INT_LV0,	400000000,	0,	NULL},
	{INT_LV1,	267000000,	0,	NULL},
	{INT_LV2,	267000000,	0,	NULL},
	{INT_LV3,	200000000,	0,	NULL},
	{INT_LV4,	160000000,	0,	NULL},
	{INT_LV5,	134000000,	0,	NULL},
	{INT_LV6,	100000000,	0,	NULL},
};

struct devfreq_clk_info dout_mif_pre[] = {
	{INT_LV0,	800000000,	0,	NULL},
	{INT_LV1,	800000000,	0,	NULL},
	{INT_LV2,	800000000,	0,	NULL},
	{INT_LV3,	800000000,	0,	NULL},
	{INT_LV4,	800000000,	0,	NULL},
	{INT_LV5,	800000000,	0,	NULL},
	{INT_LV6,	800000000,	0,	NULL},
};

struct devfreq_clk_info aclk_bus2_400[] = {
	{INT_LV0,	400000000,	0,	NULL},
	{INT_LV1,	267000000,	0,	NULL},
	{INT_LV2,	267000000,	0,	NULL},
	{INT_LV3,	200000000,	0,	NULL},
	{INT_LV4,	160000000,	0,	NULL},
	{INT_LV5,	134000000,	0,	NULL},
	{INT_LV6,	100000000,	0,	NULL},
};

struct devfreq_clk_info aclk_g2d_400[] = {
	{INT_LV0,	400000000,	0,	&aclk_g2d_bus_pll_list},
	{INT_LV1,	334000000,	0,	&aclk_g2d_mfc_pll_list},
	{INT_LV2,	267000000,	0,	&aclk_g2d_bus_pll_list},
	{INT_LV3,	200000000,	0,	&aclk_g2d_bus_pll_list},
	{INT_LV4,	160000000,	0,	&aclk_g2d_bus_pll_list},
	{INT_LV5,	134000000,	0,	&aclk_g2d_bus_pll_list},
	{INT_LV6,	100000000,	0,	&aclk_g2d_bus_pll_list},
};

struct devfreq_clk_info aclk_g2d_266[] = {
	{INT_LV0,	267000000,	0,	NULL},
	{INT_LV1,	267000000,	0,	NULL},
	{INT_LV2,	200000000,	0,	NULL},
	{INT_LV3,	160000000,	0,	NULL},
	{INT_LV4,	134000000,	0,	NULL},
	{INT_LV5,	100000000,	0,	NULL},
	{INT_LV6,	100000000,	0,	NULL},
};

struct devfreq_clk_info aclk_gscl_333[] = {
	{INT_LV0,	334000000,	0,	NULL},
	{INT_LV1,	334000000,	0,	NULL},
	{INT_LV2,	334000000,	0,	NULL},
	{INT_LV3,	334000000,	0,	NULL},
	{INT_LV4,	334000000,	0,	NULL},
	{INT_LV5,	334000000,	0,	NULL},
	{INT_LV6,	167000000,	0,	NULL},
};

struct devfreq_clk_info aclk_mscl[] = {
	{INT_LV0,	400000000,	0,	&aclk_mscl_bus_pll_list},
	{INT_LV1,	334000000,	0,	&aclk_mscl_mfc_pll_list},
	{INT_LV2,	267000000,	0,	&aclk_mscl_bus_pll_list},
	{INT_LV3,	200000000,	0,	&aclk_mscl_bus_pll_list},
	{INT_LV4,	160000000,	0,	&aclk_mscl_bus_pll_list},
	{INT_LV5,	134000000,	0,	&aclk_mscl_bus_pll_list},
	{INT_LV6,	100000000,	0,	&aclk_mscl_bus_pll_list},
};

struct devfreq_clk_info sclk_jpeg[] = {
	{INT_LV0,	400000000,	0,	&sclk_jpeg_bus_pll_list},
	{INT_LV1,	334000000,	0,	&sclk_jpeg_mfc_pll_list},
	{INT_LV2,	267000000,	0,	&sclk_jpeg_bus_pll_list},
	{INT_LV3,	200000000,	0,	&sclk_jpeg_bus_pll_list},
	{INT_LV4,	160000000,	0,	&sclk_jpeg_bus_pll_list},
	{INT_LV5,	134000000,	0,	&sclk_jpeg_bus_pll_list},
	{INT_LV6,	100000000,	0,	&sclk_jpeg_bus_pll_list},
};

struct devfreq_clk_info aclk_mfc_400[] = {
	{INT_LV0,	400000000,	0,	&aclk_mfc_400_bus_pll_list},
	{INT_LV1,	334000000,	0,	&aclk_mfc_400_mfc_pll_list},
	{INT_LV2,	267000000,	0,	&aclk_mfc_400_bus_pll_list},
	{INT_LV3,	200000000,	0,	&aclk_mfc_400_bus_pll_list},
	{INT_LV4,	200000000,	0,	&aclk_mfc_400_bus_pll_list},
	{INT_LV5,	160000000,	0,	&aclk_mfc_400_bus_pll_list},
	{INT_LV6,	100000000,	0,	&aclk_mfc_400_bus_pll_list},
};

struct devfreq_clk_info aclk_hevc_400[] = {
	{INT_LV0,	400000000,	0,	NULL},
	{INT_LV1,	267000000,	0,	NULL},
	{INT_LV2,	267000000,	0,	NULL},
	{INT_LV3,	200000000,	0,	NULL},
	{INT_LV4,	160000000,	0,	NULL},
	{INT_LV5,	134000000,	0,	NULL},
	{INT_LV6,	100000000,	0,	NULL},
};

struct devfreq_clk_info *devfreq_clk_int_info_list[] = {
	aclk_bus0_400,
	aclk_bus1_400,
	dout_mif_pre,
	aclk_bus2_400,
	aclk_g2d_400,
	aclk_g2d_266,
	aclk_gscl_333,
	aclk_mscl,
	sclk_jpeg,
	aclk_mfc_400,
	aclk_hevc_400,
};

enum devfreq_int_clk devfreq_clk_int_info_idx[] = {
	DOUT_ACLK_BUS0_400,
	DOUT_ACLK_BUS1_400,
	DOUT_MIF_PRE_4_INT,
	DOUT_ACLK_BUS2_400,
	DOUT_ACLK_G2D_400,
	DOUT_ACLK_G2D_266,
	DOUT_ACLK_GSCL_333,
	DOUT_ACLK_MSCL_400,
	DOUT_SCLK_JPEG,
	DOUT_ACLK_MFC_400,
	DOUT_ACLK_HEVC_400,
};

#ifdef CONFIG_PM_RUNTIME
struct devfreq_pm_domain_link devfreq_int_pm_domain[] = {
	{"pd-bus0",},
	{"pd-bus1",},
	{"pd-bus2",},
	{"pd-bus2",},
	{"pd-g2d",},
	{"pd-g2d",},
	{"pd-gscl",},
	{"pd-mscl",},
	{"pd-mscl",},
	{"pd-mfc",},
	{"pd-hevc",},
};

static int exynos5_devfreq_int_init_pm_domain(void)
{
	struct platform_device *pdev = NULL;
	struct device_node *np = NULL;
	int i;

	for_each_compatible_node(np, NULL, "samsung,exynos-pd") {
		struct exynos_pm_domain *pd;

		if (!of_device_is_available(np))
			continue;

		pdev = of_find_device_by_node(np);
		pd = platform_get_drvdata(pdev);

		for (i = 0; i < ARRAY_SIZE(devfreq_int_pm_domain); ++i) {
			if (devfreq_int_pm_domain[i].pm_domain_name == NULL)
				continue;

			if (!strcmp(devfreq_int_pm_domain[i].pm_domain_name, pd->genpd.name))
				devfreq_int_pm_domain[i].pm_domain = pd;
		}
	}

	return 0;
}
#endif

int district_level_by_disp_333[] = {
	INT_LV2,
	INT_LV4,
	INT_LV6,
	INT_LV6,
};

extern struct pm_qos_request exynos5_int_bts_qos;

void exynos5_update_district_int_level(int aclk_disp_333_idx)
{
	int int_qos = INT_LV6;

	if (aclk_disp_333_idx < 0 ||
		ARRAY_SIZE(district_level_by_disp_333) <= aclk_disp_333_idx) {
		pr_err("DEVFREQ(INT) : can't update distriction of int level by aclk_disp_333\n");
		return;
	}

	int_qos = district_level_by_disp_333[aclk_disp_333_idx];

	if (pm_qos_request_active(&exynos5_int_bts_qos))
		pm_qos_update_request(&exynos5_int_bts_qos, devfreq_int_opp_list[int_qos].freq);
}

static int exynos5_devfreq_int_init_clock(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(devfreq_int_clk); ++i) {
		devfreq_int_clk[i].clk = clk_get(NULL, devfreq_int_clk[i].clk_name);
		if (IS_ERR_OR_NULL(devfreq_int_clk[i].clk)) {
			pr_err("DEVFREQ(INT) : %s can't get clock\n", devfreq_int_clk[i].clk_name);
			return -EINVAL;
		}
		pr_debug("INT clk name: %s, rate: %lu\n", devfreq_int_clk[i].clk_name, clk_get_rate(devfreq_int_clk[i].clk));
	}

	return 0;
}

static int exynos5_devfreq_int_set_volt(struct devfreq_data_int *data,
					unsigned long volt,
					unsigned long volt_range)
{
	if (data->old_volt == volt)
		goto out;

	regulator_set_voltage(data->vdd_int, volt, volt_range);
	data->old_volt = volt;
out:
	return 0;
}

static int exynos5_devfreq_int_set_freq(struct devfreq_data_int *data,
					int target_idx,
					int old_idx)
{
	int i, j;
	struct devfreq_clk_info *clk_info;
	struct devfreq_clk_states *clk_states;
#ifdef CONFIG_PM_RUNTIME
	struct exynos_pm_domain *pm_domain;
#endif

	if (target_idx < old_idx) {
		for (i = 0; i < ARRAY_SIZE(devfreq_clk_int_info_list); ++i) {
			clk_info = &devfreq_clk_int_info_list[i][target_idx];
			clk_states = clk_info->states;

#ifdef CONFIG_PM_RUNTIME
			pm_domain = devfreq_int_pm_domain[i].pm_domain;

			if (pm_domain != NULL) {
				mutex_lock(&pm_domain->access_lock);
				if ((__raw_readl(pm_domain->base + 0x4) & EXYNOS_INT_LOCAL_PWR_EN) == 0) {
					mutex_unlock(&pm_domain->access_lock);
					continue;
				}
			}
#endif

			if (clk_states) {
				for (j = 0; j < clk_states->state_count; ++j) {
					clk_set_parent(devfreq_int_clk[clk_states->state[j].clk_idx].clk,
						devfreq_int_clk[clk_states->state[j].parent_clk_idx].clk);
				}
			}

			if (clk_info->freq != 0) {
				clk_set_rate(devfreq_int_clk[devfreq_clk_int_info_idx[i]].clk, clk_info->freq);
				pr_debug("INT clk name: %s, set_rate: %lu, get_rate: %lu\n",
						devfreq_int_clk[devfreq_clk_int_info_idx[i]].clk_name,
						clk_info->freq, clk_get_rate(devfreq_int_clk[devfreq_clk_int_info_idx[i]].clk));
			}
#ifdef CONFIG_PM_RUNTIME
			if (pm_domain != NULL)
				mutex_unlock(&pm_domain->access_lock);
#endif
		}
	} else {
		for (i = 0; i < ARRAY_SIZE(devfreq_clk_int_info_list); ++i) {
			clk_info = &devfreq_clk_int_info_list[i][target_idx];
			clk_states = clk_info->states;

#ifdef CONFIG_PM_RUNTIME
			pm_domain = devfreq_int_pm_domain[i].pm_domain;

			if (pm_domain != NULL) {
				mutex_lock(&pm_domain->access_lock);
				if ((__raw_readl(pm_domain->base + 0x4) & EXYNOS_INT_LOCAL_PWR_EN) == 0) {
					mutex_unlock(&pm_domain->access_lock);
					continue;
				}
			}
#endif

			if (clk_info->freq != 0)
				clk_set_rate(devfreq_int_clk[devfreq_clk_int_info_idx[i]].clk, clk_info->freq);

			if (clk_states) {
				for (j = 0; j < clk_states->state_count; ++j) {
					clk_set_parent(devfreq_int_clk[clk_states->state[j].clk_idx].clk,
						devfreq_int_clk[clk_states->state[j].parent_clk_idx].clk);
				}
			}

			if (clk_info->freq != 0) {
				clk_set_rate(devfreq_int_clk[devfreq_clk_int_info_idx[i]].clk, clk_info->freq);
				pr_debug("INT clk name: %s, set_rate: %lu, get_rate: %lu\n",
						devfreq_int_clk[devfreq_clk_int_info_idx[i]].clk_name,
						clk_info->freq, clk_get_rate(devfreq_int_clk[devfreq_clk_int_info_idx[i]].clk));
			}
#ifdef CONFIG_PM_RUNTIME
			if (pm_domain != NULL)
				mutex_unlock(&pm_domain->access_lock);
#endif
		}
	}
	return 0;
}

int exynos5_devfreq_get_idx(struct devfreq_opp_table *table,
				unsigned int size,
				unsigned long freq)
{
	int i;

	for (i = 0; i < size; ++i) {
		if (table[i].freq == freq)
			return i;
	}

	return -1;
}

int exynos5_int_check_voltage_constraint(unsigned long isp_voltage)
{
	int i;

	mutex_lock(&data_int->lock);

	for (i = ARRAY_SIZE(devfreq_int_opp_list) - 1; i >= 0; --i) {
		if (devfreq_int_opp_list[i].volt + data_int->volt_offset > isp_voltage)
			break;
	}

	if (i < 0) {
		mutex_unlock(&data_int->lock);
		pr_err("DEVFREQ(INT) : can't find lower voltage than constraint isp voltage\n");
		return -ENOENT;
	}

	data_int->volt_constraint_isp = devfreq_int_opp_list[i].volt + data_int->volt_offset;
	if (data_int->target_volt < data_int->volt_constraint_isp) {
		exynos5_devfreq_int_set_volt(data_int,
			data_int->volt_constraint_isp, data_int->volt_constraint_isp + VOLT_STEP);
	} else {
		exynos5_devfreq_int_set_volt(data_int,
			data_int->target_volt, data_int->target_volt + VOLT_STEP);
	}

	mutex_unlock(&data_int->lock);

	return 0;
}

#ifdef CONFIG_EXYNOS_THERMAL
int exynos5_devfreq_int_tmu_notifier(struct notifier_block *nb, unsigned long event,
						void *v)
{
	struct devfreq_data_int *data = container_of(nb, struct devfreq_data_int, tmu_notifier);
	unsigned int prev_volt, set_volt;
	unsigned int *on = v;

	if (event == TMU_COLD) {
		if (pm_qos_request_active(&min_int_thermal_qos))
			pm_qos_update_request(&min_int_thermal_qos, data->initial_freq);

		if (*on) {
			mutex_lock(&data->lock);

			prev_volt = regulator_get_voltage(data->vdd_int);

			if (data->volt_offset != COLD_VOLT_OFFSET) {
				data->volt_offset = COLD_VOLT_OFFSET;
			} else {
				mutex_unlock(&data->lock);
				return NOTIFY_OK;
			}

			set_volt = get_limit_voltage(prev_volt, data->volt_offset);
			regulator_set_voltage(data->vdd_int, set_volt, set_volt + VOLT_STEP);

			mutex_unlock(&data->lock);
		} else {
			mutex_lock(&data->lock);

			prev_volt = regulator_get_voltage(data->vdd_int);

			if (data->volt_offset != 0) {
				data->volt_offset = 0;
			} else {
				mutex_unlock(&data->lock);
				return NOTIFY_OK;
			}

			set_volt = get_limit_voltage(prev_volt - COLD_VOLT_OFFSET, data->volt_offset);
			regulator_set_voltage(data->vdd_int, set_volt, set_volt + VOLT_STEP);

			mutex_unlock(&data->lock);
		}

		if (pm_qos_request_active(&min_int_thermal_qos))
			pm_qos_update_request(&min_int_thermal_qos, data->default_qos);
	}

	return NOTIFY_OK;
}
#endif

static int exynos5_devfreq_int_set_clk(struct devfreq_data_int *data,
					int target_idx,
					struct clk *clk,
					struct devfreq_clk_info *clk_info)
{
	int i;
	struct devfreq_clk_states *clk_states = clk_info->states;

	if (clk_get_rate(clk) < clk_info->freq) {
		if (clk_states) {
			for (i = 0; i < clk_states->state_count; ++i) {
				clk_set_parent(devfreq_int_clk[clk_states->state[i].clk_idx].clk,
					devfreq_int_clk[clk_states->state[i].parent_clk_idx].clk);
			}
		}

		if (clk_info->freq != 0)
			clk_set_rate(clk, clk_info->freq);
	} else {
		if (clk_info->freq != 0)
			clk_set_rate(clk, clk_info->freq);

		if (clk_states) {
			for (i = 0; i < clk_states->state_count; ++i) {
				clk_set_parent(devfreq_int_clk[clk_states->state[i].clk_idx].clk,
					devfreq_int_clk[clk_states->state[i].parent_clk_idx].clk);
			}
		}

		if (clk_info->freq != 0)
			clk_set_rate(clk, clk_info->freq);
	}

	return 0;
}
static struct devfreq_data_int *data_int;
void exynos5_int_notify_power_status(const char *pd_name, unsigned int turn_on)
{
	int i;
	int cur_freq_idx;

	if (!turn_on || !data_int->use_dvfs)
		return;

	mutex_lock(&data_int->lock);
	cur_freq_idx = exynos5_devfreq_get_idx(devfreq_int_opp_list,
                                                ARRAY_SIZE(devfreq_int_opp_list),
                                                data_int->devfreq->previous_freq);
	if (cur_freq_idx == -1) {
		mutex_unlock(&data_int->lock);
		pr_err("DEVFREQ(INT) : can't find target_idx to apply notify of power\n");
		return;
	}

	for (i = 0; i < ARRAY_SIZE(devfreq_int_pm_domain); ++i) {
		if (devfreq_int_pm_domain[i].pm_domain_name == NULL)
			continue;
		if (strcmp(devfreq_int_pm_domain[i].pm_domain_name, pd_name))
			continue;

		exynos5_devfreq_int_set_clk(data_int,
						cur_freq_idx,
						devfreq_int_clk[devfreq_clk_int_info_idx[i]].clk,
						devfreq_clk_int_info_list[i]);
	}
	mutex_unlock(&data_int->lock);
}

int exynos5433_devfreq_int_init(struct devfreq_data_int *data)
{
	int ret = 0;

	data_int = data;
	data->max_state = INT_LV_COUNT;

	if (exynos5_devfreq_int_init_clock()) {
		ret = -EINVAL;
		return ret;
	}

#ifdef CONFIG_PM_RUNTIME
	if (exynos5_devfreq_int_init_pm_domain()) {
		ret = -EINVAL;
		return ret;
	}
#endif
	data->int_set_volt = exynos5_devfreq_int_set_volt;
	data->int_set_freq = exynos5_devfreq_int_set_freq;

	return ret;
}
/* end of INT related function */

/* ========== 3. ISP related function */
extern struct pm_qos_request min_isp_thermal_qos;

enum devfreq_isp_idx {
	ISP_LV0,
	ISP_LV1,
	ISP_LV2,
	ISP_LV3,
	ISP_LV4,
	ISP_LV5,
	ISP_LV6,
	ISP_LV7,
	ISP_LV8,
	ISP_LV_COUNT,
};

enum devfreq_isp_clk {
	ISP_PLL,
	DOUT_ACLK_CAM0_552,
	DOUT_ACLK_CAM0_400,
	DOUT_ACLK_CAM0_333,
	DOUT_ACLK_CAM0_BUS_400,
	DOUT_ACLK_CSIS0,
	DOUT_ACLK_LITE_A,
	DOUT_ACLK_3AA0,
	DOUT_ACLK_CSIS1,
	DOUT_ACLK_LITE_B,
	DOUT_ACLK_3AA1,
	DOUT_ACLK_LITE_D,
	DOUT_SCLK_PIXEL_INIT_552,
	DOUT_SCLK_PIXEL_333,
	DOUT_ACLK_CAM1_552,
	DOUT_ACLK_CAM1_400,
	DOUT_ACLK_CAM1_333,
	DOUT_ACLK_FD_400,
	DOUT_ACLK_CSIS2_333,
	DOUT_ACLK_LITE_C,
	DOUT_ACLK_ISP_400,
	DOUT_ACLK_ISP_DIS_400,
	MOUT_ACLK_CAM1_552_A,
	MOUT_ACLK_CAM1_552_B,
	MOUT_ISP_PLL_4_ISP,
	MOUT_BUS_PLL_USER_4_ISP,
	MOUT_MFC_PLL_USER_ISP,
	MOUT_ACLK_FD_A,
	MOUT_ACLK_FD_B,
	MOUT_ACLK_ISP_400,
	MOUT_ACLK_CAM1_333_USER,
	MOUT_ACLK_CAM1_400_USER,
	MOUT_ACLK_CAM1_552_USER,
	MOUT_SCLK_PIXELASYNC_LITE_C_B,
	MOUT_ACLK_CAM0_333_USER,
	ACLK_CAM0_333,
	MOUT_ACLK_LITE_C_B,
	MOUT_ACLK_CSIS2_B,
	MOUT_ACLK_ISP_DIS_400,
	MOUT_SCLK_LITE_FREECNT_C,
	DOUT_PCLK_LITE_D,
	CLK_COUNT,
};

#ifdef CONFIG_PM_RUNTIME
struct devfreq_pm_domain_link devfreq_isp_pm_domain[] = {
	{"pd-cam0",},
	{"pd-cam0",},
	{"pd-cam0",},
	{"pd-cam0",},
	{"pd-cam0",},
	{"pd-cam0",},
	{"pd-cam0",},
	{"pd-cam0",},
	{"pd-cam0",},
	{"pd-cam0",},
	{"pd-cam0",},
	{"pd-cam0",},
	{"pd-cam0",},
	{"pd-cam1",},
	{"pd-cam1",},
	{"pd-cam1",},
	{"pd-cam1",},
	{"pd-cam1",},
	{"pd-cam1",},
	{"pd-isp",},
	{"pd-isp",},
	{"pd-cam0",},
};
#endif

struct devfreq_clk_list devfreq_isp_clk[CLK_COUNT] = {
	{"fout_isp_pll",},
	{"dout_aclk_cam0_552",},
	{"dout_aclk_cam0_400",},
	{"dout_aclk_cam0_333",},
	{"dout_aclk_cam0_bus_400",},
	{"dout_aclk_csis0",},
	{"dout_aclk_lite_a",},
	{"dout_aclk_3aa0",},
	{"dout_aclk_csis1",},
	{"dout_aclk_lite_b",},
	{"dout_aclk_3aa1",},
	{"dout_aclk_lite_d",},
	{"dout_sclk_pixelasync_lite_c_init",},
	{"dout_sclk_pixelasync_lite_c",},
	{"dout_aclk_cam1_552",},
	{"dout_aclk_cam1_400",},
	{"dout_aclk_cam1_333",},
	{"dout_aclk_fd",},
	{"dout_aclk_csis2",},
	{"dout_aclk_lite_c",},
	{"dout_aclk_isp_400",},
	{"dout_aclk_isp_dis_400",},
	{"mout_aclk_cam1_552_a",},
	{"mout_aclk_cam1_552_b",},
	{"mout_isp_pll",},
	{"mout_bus_pll_user",},
	{"mout_mfc_pll_user",},
	{"mout_aclk_fd_a",},
	{"mout_aclk_fd_b",},
	{"mout_aclk_isp_400",},
	{"mout_aclk_cam1_333_user",},
	{"mout_aclk_cam1_400_user",},
	{"mout_aclk_cam1_552_user",},
	{"mout_sclk_pixelasync_lite_c_b",},
	{"mout_aclk_cam0_333_user",},
	{"aclk_cam0_333",},
	{"mout_aclk_lite_c_b",},
	{"mout_aclk_csis2_b",},
	{"mout_aclk_isp_dis_400",},
	{"mout_sclk_lite_freecnt_c",},
	{"dout_pclk_lite_d",},
};

struct devfreq_opp_table devfreq_isp_opp_list[] = {
	{ISP_LV0,	777000, 950000},
	{ISP_LV1,	666000,	950000},
	{ISP_LV2,	555000,	950000},
	{ISP_LV3,	466000,	950000},
	{ISP_LV4,	455000,	950000},
	{ISP_LV5,	444000,	950000},
	{ISP_LV6,	333000, 950000},
	{ISP_LV7,	222000,	925000},
	{ISP_LV8,	111000,	925000},
};

struct devfreq_clk_state mux_sclk_pixelasync_lite_c[] = {
	{MOUT_SCLK_PIXELASYNC_LITE_C_B,	MOUT_ACLK_CAM0_333_USER},
};

struct devfreq_clk_state mux_sclk_lite_freecnt_c[] = {
	{MOUT_SCLK_LITE_FREECNT_C,	DOUT_PCLK_LITE_D},
};

struct devfreq_clk_state aclk_cam1_552_isp_pll[] = {
	{MOUT_ACLK_CAM1_552_A,	MOUT_ISP_PLL_4_ISP},
	{MOUT_ACLK_CAM1_552_B,	MOUT_ACLK_CAM1_552_A},
};

struct devfreq_clk_state aclk_cam1_552_bus_pll[] = {
	{MOUT_ACLK_CAM1_552_A,	MOUT_BUS_PLL_USER_4_ISP},
	{MOUT_ACLK_CAM1_552_B,	MOUT_ACLK_CAM1_552_A},
};

struct devfreq_clk_state mux_aclk_csis2[] = {
	{MOUT_ACLK_CSIS2_B,	MOUT_ACLK_CAM1_333_USER},
};

struct devfreq_clk_state mux_aclk_lite_c[] = {
	{MOUT_ACLK_LITE_C_B,	MOUT_ACLK_CAM1_333_USER},
};

struct devfreq_clk_state aclk_fd_400_bus_pll[] = {
	{MOUT_ACLK_FD_A,	MOUT_ACLK_CAM1_400_USER},
	{MOUT_ACLK_FD_B,	MOUT_ACLK_FD_A},
};

struct devfreq_clk_state aclk_fd_400_mfc_pll[] = {
	{MOUT_ACLK_FD_B,	MOUT_ACLK_CAM1_333_USER},
};

struct devfreq_clk_state aclk_isp_400_bus_pll[] = {
	{MOUT_ACLK_ISP_400,	MOUT_BUS_PLL_USER_4_ISP},
};

struct devfreq_clk_state aclk_isp_400_mfc_pll[] = {
	{MOUT_ACLK_ISP_400,	MOUT_MFC_PLL_USER_ISP},
};

struct devfreq_clk_state mux_aclk_isp_dis_400_bus_pll[] = {
	{MOUT_ACLK_ISP_DIS_400,	MOUT_BUS_PLL_USER_4_ISP},
};

struct devfreq_clk_state mux_aclk_isp_dis_400_mfc_pll[] = {
	{MOUT_ACLK_ISP_DIS_400,	MOUT_MFC_PLL_USER_ISP},
};

struct devfreq_clk_states sclk_lite_freecnt_c_list = {
	.state = mux_sclk_lite_freecnt_c,
	.state_count = ARRAY_SIZE(mux_sclk_lite_freecnt_c),
};

struct devfreq_clk_states sclk_pixelasync_lite_c_list = {
	.state = mux_sclk_pixelasync_lite_c,
	.state_count = ARRAY_SIZE(mux_sclk_pixelasync_lite_c),
};

struct devfreq_clk_states aclk_cam1_552_isp_pll_list = {
	.state = aclk_cam1_552_isp_pll,
	.state_count = ARRAY_SIZE(aclk_cam1_552_isp_pll),
};

struct devfreq_clk_states aclk_cam1_552_bus_pll_list = {
	.state = aclk_cam1_552_bus_pll,
	.state_count = ARRAY_SIZE(aclk_cam1_552_bus_pll),
};

struct devfreq_clk_states mux_aclk_csis2_list = {
	.state = mux_aclk_csis2,
	.state_count = ARRAY_SIZE(mux_aclk_csis2),
};

struct devfreq_clk_states mux_aclk_lite_c_list = {
	.state = mux_aclk_lite_c,
	.state_count = ARRAY_SIZE(mux_aclk_lite_c),
};

struct devfreq_clk_states aclk_fd_400_bus_pll_list = {
	.state = aclk_fd_400_bus_pll,
	.state_count = ARRAY_SIZE(aclk_fd_400_bus_pll),
};

struct devfreq_clk_states aclk_fd_400_mfc_pll_list = {
	.state = aclk_fd_400_mfc_pll,
	.state_count = ARRAY_SIZE(aclk_fd_400_mfc_pll),
};

struct devfreq_clk_states aclk_isp_400_bus_pll_list = {
	.state = aclk_isp_400_bus_pll,
	.state_count = ARRAY_SIZE(aclk_isp_400_bus_pll),
};

struct devfreq_clk_states aclk_isp_400_mfc_pll_list = {
	.state = aclk_isp_400_mfc_pll,
	.state_count = ARRAY_SIZE(aclk_isp_400_mfc_pll),
};

struct devfreq_clk_states aclk_isp_dis_400_bus_pll_list = {
	.state = mux_aclk_isp_dis_400_bus_pll,
	.state_count = ARRAY_SIZE(mux_aclk_isp_dis_400_bus_pll),
};

struct devfreq_clk_states aclk_isp_dis_400_mfc_pll_list = {
	.state = mux_aclk_isp_dis_400_mfc_pll,
	.state_count = ARRAY_SIZE(mux_aclk_isp_dis_400_mfc_pll),
};

struct devfreq_clk_info aclk_cam0_552[] = {
	{ISP_LV0,	552000000,	0,	NULL},
	{ISP_LV1,	552000000,	0,	NULL},
	{ISP_LV2,	552000000,	0,	NULL},
	{ISP_LV3,	552000000,	0,	NULL},
	{ISP_LV4,	552000000,	0,	NULL},
	{ISP_LV5,	552000000,	0,	NULL},
	{ISP_LV6,	552000000,	0,	NULL},
	{ISP_LV7,	 79000000,	0,	NULL},
	{ISP_LV8,	 79000000,	0,	NULL},
};

struct devfreq_clk_info aclk_cam0_400[] = {
	{ISP_LV0,	400000000,	0,	NULL},
	{ISP_LV1,	400000000,	0,	NULL},
	{ISP_LV2,	400000000,	0,	NULL},
	{ISP_LV3,	400000000,	0,	NULL},
	{ISP_LV4,	400000000,	0,	NULL},
	{ISP_LV5,	400000000,	0,	NULL},
	{ISP_LV6,	400000000,	0,	NULL},
	{ISP_LV7,	134000000,	0,	NULL},
	{ISP_LV8,	134000000,	0,	NULL},
};

struct devfreq_clk_info aclk_cam0_333[] = {
	{ISP_LV0,	317000000,	0,	NULL},
	{ISP_LV1,	317000000,	0,	NULL},
	{ISP_LV2,	317000000,	0,	NULL},
	{ISP_LV3,	317000000,	0,	NULL},
	{ISP_LV4,	317000000,	0,	NULL},
	{ISP_LV5,	317000000,	0,	NULL},
	{ISP_LV6,	317000000,	0,	NULL},
	{ISP_LV7,	 79000000,	0,	NULL},
	{ISP_LV8,	 79000000,	0,	NULL},
};

struct devfreq_clk_info aclk_cam0_bus_400[] = {
	{ISP_LV0,	400000000,	0,	NULL},
	{ISP_LV1,	400000000,	0,	NULL},
	{ISP_LV2,	400000000,	0,	NULL},
	{ISP_LV3,	400000000,	0,	NULL},
	{ISP_LV4,	400000000,	0,	NULL},
	{ISP_LV5,	400000000,	0,	NULL},
	{ISP_LV6,	400000000,	0,	NULL},
	{ISP_LV7,	134000000,	0,	NULL},
	{ISP_LV8,	 17000000,	0,	NULL},
};

struct devfreq_clk_info aclk_csis0[] = {
	{ISP_LV0,	552000000,	0,	NULL},
	{ISP_LV1,	552000000,	0,	NULL},
	{ISP_LV2,	552000000,	0,	NULL},
	{ISP_LV3,	552000000,	0,	NULL},
	{ISP_LV4,	552000000,	0,	NULL},
	{ISP_LV5,	552000000,	0,	NULL},
	{ISP_LV6,	552000000,	0,	NULL},
	{ISP_LV7,	 10000000,	0,	NULL},
	{ISP_LV8,	 10000000,	0,	NULL},
};

struct devfreq_clk_info aclk_lite_a[] = {
	{ISP_LV0,	552000000,	0,	NULL},
	{ISP_LV1,	552000000,	0,	NULL},
	{ISP_LV2,	552000000,	0,	NULL},
	{ISP_LV3,	552000000,	0,	NULL},
	{ISP_LV4,	552000000,	0,	NULL},
	{ISP_LV5,	552000000,	0,	NULL},
	{ISP_LV6,	552000000,	0,	NULL},
	{ISP_LV7,	 10000000,	0,	NULL},
	{ISP_LV8,	 10000000,	0,	NULL},
};

struct devfreq_clk_info aclk_3aa0[] = {
	{ISP_LV0,	552000000,	0,	NULL},
	{ISP_LV1,	552000000,	0,	NULL},
	{ISP_LV2,	552000000,	0,	NULL},
	{ISP_LV3,	552000000,	0,	NULL},
	{ISP_LV4,	552000000,	0,	NULL},
	{ISP_LV5,	276000000,	0,	NULL},
	{ISP_LV6,	276000000,	0,	NULL},
	{ISP_LV7,	 10000000,	0,	NULL},
	{ISP_LV8,	 10000000,	0,	NULL},
};

struct devfreq_clk_info aclk_csis1[] = {
	{ISP_LV0,	 69000000,	0,	NULL},
	{ISP_LV1,	 69000000,	0,	NULL},
	{ISP_LV2,	 69000000,	0,	NULL},
	{ISP_LV3,	 69000000,	0,	NULL},
	{ISP_LV4,	 69000000,	0,	NULL},
	{ISP_LV5,	 69000000,	0,	NULL},
	{ISP_LV6,	 69000000,	0,	NULL},
	{ISP_LV7,	 10000000,	0,	NULL},
	{ISP_LV8,	 10000000,	0,	NULL},
};

struct devfreq_clk_info aclk_lite_b[] = {
	{ISP_LV0,	 69000000,	0,	NULL},
	{ISP_LV1,	 69000000,	0,	NULL},
	{ISP_LV2,	 69000000,	0,	NULL},
	{ISP_LV3,	 69000000,	0,	NULL},
	{ISP_LV4,	 69000000,	0,	NULL},
	{ISP_LV5,	 69000000,	0,	NULL},
	{ISP_LV6,	 69000000,	0,	NULL},
	{ISP_LV7,	 10000000,	0,	NULL},
	{ISP_LV8,	 10000000,	0,	NULL},
};

struct devfreq_clk_info aclk_3aa1[] = {
	{ISP_LV0,	552000000,	0,	NULL},
	{ISP_LV1,	552000000,	0,	NULL},
	{ISP_LV2,	552000000,	0,	NULL},
	{ISP_LV3,	552000000,	0,	NULL},
	{ISP_LV4,	552000000,	0,	NULL},
	{ISP_LV5,	 79000000,	0,	NULL},
	{ISP_LV6,	 79000000,	0,	NULL},
	{ISP_LV7,	 79000000,	0,	NULL},
	{ISP_LV8,	 11000000,	0,	NULL},
};

struct devfreq_clk_info aclk_lite_d[] = {
	{ISP_LV0,	 138000000,	0,	NULL},
	{ISP_LV1,	 138000000,	0,	NULL},
	{ISP_LV2,	 138000000,	0,	NULL},
	{ISP_LV3,	 138000000,	0,	NULL},
	{ISP_LV4,	 138000000,	0,	NULL},
	{ISP_LV5,	 138000000,	0,	NULL},
	{ISP_LV6,	 138000000,	0,	NULL},
	{ISP_LV7,	  10000000,	0,	NULL},
	{ISP_LV8,	  10000000,	0,	NULL},
};

struct devfreq_clk_info sclk_pixel_init_552[] = {
	{ISP_LV0,	  69000000,	0,	NULL},
	{ISP_LV1,	  69000000,	0,	NULL},
	{ISP_LV2,	  69000000,	0,	NULL},
	{ISP_LV3,	  69000000,	0,	NULL},
	{ISP_LV4,	  69000000,	0,	NULL},
	{ISP_LV5,	  69000000,	0,	NULL},
	{ISP_LV6,	  69000000,	0,	NULL},
	{ISP_LV7,	  10000000,	0,	NULL},
	{ISP_LV8,	  10000000,	0,	NULL},
};

struct devfreq_clk_info sclk_pixel_333[] = {
	{ISP_LV0,	  40000000,	0,	&sclk_pixelasync_lite_c_list},
	{ISP_LV1,	  40000000,	0,	&sclk_pixelasync_lite_c_list},
	{ISP_LV2,	  40000000,	0,	&sclk_pixelasync_lite_c_list},
	{ISP_LV3,	  40000000,	0,	&sclk_pixelasync_lite_c_list},
	{ISP_LV4,	  40000000,	0,	&sclk_pixelasync_lite_c_list},
	{ISP_LV5,	  40000000,	0,	&sclk_pixelasync_lite_c_list},
	{ISP_LV6,	  40000000,	0,	&sclk_pixelasync_lite_c_list},
	{ISP_LV7,	  10000000,	0,	&sclk_pixelasync_lite_c_list},
	{ISP_LV8,	  10000000,	0,	&sclk_pixelasync_lite_c_list},
};

struct devfreq_clk_info aclk_cam1_552[] = {
	{ISP_LV0,	552000000,	0,	&aclk_cam1_552_isp_pll_list},
	{ISP_LV1,	552000000,	0,	&aclk_cam1_552_isp_pll_list},
	{ISP_LV2,	552000000,	0,	&aclk_cam1_552_isp_pll_list},
	{ISP_LV3,	552000000,	0,	&aclk_cam1_552_isp_pll_list},
	{ISP_LV4,	552000000,	0,	&aclk_cam1_552_isp_pll_list},
	{ISP_LV5,	552000000,	0,	&aclk_cam1_552_isp_pll_list},
	{ISP_LV6,	552000000,	0,	&aclk_cam1_552_isp_pll_list},
	{ISP_LV7,	400000000,	0,	&aclk_cam1_552_bus_pll_list},
	{ISP_LV8,	400000000,	0,	&aclk_cam1_552_bus_pll_list},
};

struct devfreq_clk_info aclk_cam1_400[] = {
	{ISP_LV0,	400000000,	0,	NULL},
	{ISP_LV1,	400000000,	0,	NULL},
	{ISP_LV2,	400000000,	0,	NULL},
	{ISP_LV3,	400000000,	0,	NULL},
	{ISP_LV4,	400000000,	0,	NULL},
	{ISP_LV5,	400000000,	0,	NULL},
	{ISP_LV6,	400000000,	0,	NULL},
	{ISP_LV7,	267000000,	0,	NULL},
	{ISP_LV8,	267000000,	0,	NULL},
};

struct devfreq_clk_info aclk_cam1_333[] = {
	{ISP_LV0,	317000000,	0,	NULL},
	{ISP_LV1,	317000000,	0,	NULL},
	{ISP_LV2,	317000000,	0,	NULL},
	{ISP_LV3,	317000000,	0,	NULL},
	{ISP_LV4,	317000000,	0,	NULL},
	{ISP_LV5,	317000000,	0,	NULL},
	{ISP_LV6,	317000000,	0,	NULL},
	{ISP_LV7,	159000000,	0,	NULL},
	{ISP_LV8,	159000000,	0,	NULL},
};

struct devfreq_clk_info aclk_fd_400[] = {
	{ISP_LV0,	400000000,	0,	&aclk_fd_400_bus_pll_list},
	{ISP_LV1,	400000000,	0,	&aclk_fd_400_bus_pll_list},
	{ISP_LV2,	317000000,	0,	&aclk_fd_400_mfc_pll_list},
	{ISP_LV3,	317000000,	0,	&aclk_fd_400_mfc_pll_list},
	{ISP_LV4,	159000000,	0,	&aclk_fd_400_mfc_pll_list},
	{ISP_LV5,	317000000,	0,	&aclk_fd_400_mfc_pll_list},
	{ISP_LV6,	159000000,	0,	&aclk_fd_400_mfc_pll_list},
	{ISP_LV7,	 80000000,	0,	&aclk_fd_400_mfc_pll_list},
	{ISP_LV8,	 80000000,	0,	&aclk_fd_400_mfc_pll_list},
};

struct devfreq_clk_info aclk_csis2_333[] = {
	{ISP_LV0,	 80000000,	0,	&mux_aclk_csis2_list},
	{ISP_LV1,	 40000000,	0,	&mux_aclk_csis2_list},
	{ISP_LV2,	 80000000,	0,	&mux_aclk_csis2_list},
	{ISP_LV3,	 80000000,	0,	&mux_aclk_csis2_list},
	{ISP_LV4,	 40000000,	0,	&mux_aclk_csis2_list},
	{ISP_LV5,	 80000000,	0,	&mux_aclk_csis2_list},
	{ISP_LV6,	 40000000,	0,	&mux_aclk_csis2_list},
	{ISP_LV7,	 80000000,	0,	&mux_aclk_csis2_list},
	{ISP_LV8,	 40000000,	0,	&mux_aclk_csis2_list},
};

struct devfreq_clk_info aclk_lite_c[] = {
	{ISP_LV0,	 80000000,	0,	&mux_aclk_lite_c_list},
	{ISP_LV1,	 40000000,	0,	&mux_aclk_lite_c_list},
	{ISP_LV2,	 80000000,	0,	&mux_aclk_lite_c_list},
	{ISP_LV3,	 80000000,	0,	&mux_aclk_lite_c_list},
	{ISP_LV4,	 40000000,	0,	&mux_aclk_lite_c_list},
	{ISP_LV5,	 80000000,	0,	&mux_aclk_lite_c_list},
	{ISP_LV6,	 40000000,	0,	&mux_aclk_lite_c_list},
	{ISP_LV7,	 80000000,	0,	&mux_aclk_lite_c_list},
	{ISP_LV8,	 40000000,	0,	&mux_aclk_lite_c_list},
};

struct devfreq_clk_info aclk_isp_400[] = {
	{ISP_LV0,	400000000,	0,	&aclk_isp_400_bus_pll_list},
	{ISP_LV1,	400000000,	0,	&aclk_isp_400_bus_pll_list},
	{ISP_LV2,	317000000,	0,	&aclk_isp_400_mfc_pll_list},
	{ISP_LV3,	267000000,	0,	&aclk_isp_400_bus_pll_list},
	{ISP_LV4,	159000000,	0,	&aclk_isp_400_mfc_pll_list},
	{ISP_LV5,	267000000,	0,	&aclk_isp_400_bus_pll_list},
	{ISP_LV6,	159000000,	0,	&aclk_isp_400_mfc_pll_list},
	{ISP_LV7,	159000000,	0,	&aclk_isp_400_mfc_pll_list},
	{ISP_LV8,	159000000,	0,	&aclk_isp_400_mfc_pll_list},
};

struct devfreq_clk_info aclk_isp_dis_400[] = {
	{ISP_LV0,	400000000,	0,	&aclk_isp_dis_400_bus_pll_list},
	{ISP_LV1,	400000000,	0,	&aclk_isp_dis_400_bus_pll_list},
	{ISP_LV2,	317000000,	0,	&aclk_isp_dis_400_mfc_pll_list},
	{ISP_LV3,	267000000,	0,	&aclk_isp_dis_400_bus_pll_list},
	{ISP_LV4,	159000000,	0,	&aclk_isp_dis_400_mfc_pll_list},
	{ISP_LV5,	267000000,	0,	&aclk_isp_dis_400_bus_pll_list},
	{ISP_LV6,	159000000,	0,	&aclk_isp_dis_400_mfc_pll_list},
	{ISP_LV7,	159000000,	0,	&aclk_isp_dis_400_mfc_pll_list},
	{ISP_LV8,	159000000,	0,	&aclk_isp_dis_400_mfc_pll_list},
};

struct devfreq_clk_info sclk_lite_freecnt_c[] = {
	{ISP_LV0,	 0,	0,	&sclk_lite_freecnt_c_list},
	{ISP_LV1,	 0,	0,	&sclk_lite_freecnt_c_list},
	{ISP_LV2,	 0,	0,	&sclk_lite_freecnt_c_list},
	{ISP_LV3,	 0,	0,	&sclk_lite_freecnt_c_list},
	{ISP_LV4,	 0,	0,	&sclk_lite_freecnt_c_list},
	{ISP_LV5,	 0,	0,	&sclk_lite_freecnt_c_list},
	{ISP_LV6,	 0,	0,	&sclk_lite_freecnt_c_list},
	{ISP_LV7,	 0,	0,	&sclk_lite_freecnt_c_list},
	{ISP_LV8,	 0,	0,	&sclk_lite_freecnt_c_list},
};

struct devfreq_clk_info *devfreq_clk_isp_info_list[] = {
	aclk_cam0_552,
	aclk_cam0_400,
	aclk_cam0_333,
	aclk_cam0_bus_400,
	aclk_csis0,
	aclk_lite_a,
	aclk_3aa0,
	aclk_csis1,
	aclk_lite_b,
	aclk_3aa1,
	aclk_lite_d,
	sclk_pixel_init_552,
	sclk_pixel_333,
	aclk_cam1_552,
	aclk_cam1_400,
	aclk_cam1_333,
	aclk_fd_400,
	aclk_csis2_333,
	aclk_lite_c,
	aclk_isp_400,
	aclk_isp_dis_400,
	sclk_lite_freecnt_c,
};

enum devfreq_isp_clk devfreq_clk_isp_info_idx[] = {
	DOUT_ACLK_CAM0_552,
	DOUT_ACLK_CAM0_400,
	DOUT_ACLK_CAM0_333,
	DOUT_ACLK_CAM0_BUS_400,
	DOUT_ACLK_CSIS0,
	DOUT_ACLK_LITE_A,
	DOUT_ACLK_3AA0,
	DOUT_ACLK_CSIS1,
	DOUT_ACLK_LITE_B,
	DOUT_ACLK_3AA1,
	DOUT_ACLK_LITE_D,
	DOUT_SCLK_PIXEL_INIT_552,
	DOUT_SCLK_PIXEL_333,
	DOUT_ACLK_CAM1_552,
	DOUT_ACLK_CAM1_400,
	DOUT_ACLK_CAM1_333,
	DOUT_ACLK_FD_400,
	DOUT_ACLK_CSIS2_333,
	DOUT_ACLK_LITE_C,
	DOUT_ACLK_ISP_400,
	DOUT_ACLK_ISP_DIS_400,
	MOUT_SCLK_LITE_FREECNT_C,
};

static int exynos5_devfreq_isp_set_clk(struct devfreq_data_isp *data,
					int target_idx,
					struct clk *clk,
					struct devfreq_clk_info *clk_info)
{
	int i;
	struct devfreq_clk_states *clk_states = clk_info->states;

	if (clk_get_rate(clk) < clk_info->freq) {
		if (clk_states) {
			for (i = 0; i < clk_states->state_count; ++i) {
				clk_set_parent(devfreq_isp_clk[clk_states->state[i].clk_idx].clk,
						devfreq_isp_clk[clk_states->state[i].parent_clk_idx].clk);
			}
		}

		if (clk_info->freq != 0)
			clk_set_rate(clk, clk_info->freq);
	} else {
		if (clk_info->freq != 0)
			clk_set_rate(clk, clk_info->freq);

		if (clk_states) {
			for (i = 0; i < clk_states->state_count; ++i) {
				clk_set_parent(devfreq_isp_clk[clk_states->state[i].clk_idx].clk,
						devfreq_isp_clk[clk_states->state[i].parent_clk_idx].clk);
			}
		}

		if (clk_info->freq != 0)
			clk_set_rate(clk, clk_info->freq);
	}

	return 0;
}

static struct devfreq_data_isp *data_isp;
void exynos5_isp_notify_power_status(const char *pd_name, unsigned int turn_on)
{
	int i;
	int cur_freq_idx;

	if (!turn_on ||
		!data_isp->use_dvfs)
		return;

	mutex_lock(&data_isp->lock);
	cur_freq_idx = exynos5_devfreq_get_idx(devfreq_isp_opp_list,
			ARRAY_SIZE(devfreq_isp_opp_list),
			data_isp->devfreq->previous_freq);
	if (cur_freq_idx == -1) {
		mutex_unlock(&data_isp->lock);
		pr_err("DEVFREQ(INT) : can't find target_idx to apply notify of power\n");
		return;
	}

	for (i = 0; i < ARRAY_SIZE(devfreq_isp_pm_domain); ++i) {
		if (devfreq_isp_pm_domain[i].pm_domain_name == NULL)
			continue;
		if (strcmp(devfreq_isp_pm_domain[i].pm_domain_name, pd_name))
			continue;

		exynos5_devfreq_isp_set_clk(data_isp,
				cur_freq_idx,
				devfreq_isp_clk[devfreq_clk_isp_info_idx[i]].clk,
				devfreq_clk_isp_info_list[i]);
	}
	mutex_unlock(&data_isp->lock);
}

static int exynos5_devfreq_isp_set_freq(struct devfreq_data_isp *data,
					int target_idx,
					int old_idx)
{
	int i, j;
	struct devfreq_clk_info *clk_info;
	struct devfreq_clk_states *clk_states;
#ifdef CONFIG_PM_RUNTIME
	struct exynos_pm_domain *pm_domain;
#endif

	if (target_idx < old_idx) {
		for (i = 0; i < ARRAY_SIZE(devfreq_clk_isp_info_list); ++i) {
			clk_info = &devfreq_clk_isp_info_list[i][target_idx];
			clk_states = clk_info->states;

#ifdef CONFIG_PM_RUNTIME
		pm_domain = devfreq_isp_pm_domain[i].pm_domain;

		if (pm_domain != NULL) {
			mutex_lock(&pm_domain->access_lock);
			if ((__raw_readl(pm_domain->base + 0x4) & EXYNOS_INT_LOCAL_PWR_EN) == 0) {
				mutex_unlock(&pm_domain->access_lock);
				continue;
			}
		}
#endif

			if (clk_states) {
				for (j = 0; j < clk_states->state_count; ++j) {
					clk_set_parent(devfreq_isp_clk[clk_states->state[j].clk_idx].clk,
						devfreq_isp_clk[clk_states->state[j].parent_clk_idx].clk);
				}
			}

			if (clk_info->freq != 0)
				clk_set_rate(devfreq_isp_clk[devfreq_clk_isp_info_idx[i]].clk, clk_info->freq);
#ifdef CONFIG_PM_RUNTIME
		if (pm_domain != NULL)
			mutex_unlock(&pm_domain->access_lock);
#endif
		}
	} else {
		for (i = 0; i < ARRAY_SIZE(devfreq_clk_isp_info_list); ++i) {
			clk_info = &devfreq_clk_isp_info_list[i][target_idx];
			clk_states = clk_info->states;

#ifdef CONFIG_PM_RUNTIME
		pm_domain = devfreq_isp_pm_domain[i].pm_domain;

		if (pm_domain != NULL) {
			mutex_lock(&pm_domain->access_lock);
			if ((__raw_readl(pm_domain->base + 0x4) & EXYNOS_INT_LOCAL_PWR_EN) == 0) {
				mutex_unlock(&pm_domain->access_lock);
				continue;
			}
		}
#endif

			if (clk_info->freq != 0)
				clk_set_rate(devfreq_isp_clk[devfreq_clk_isp_info_idx[i]].clk, clk_info->freq);

			if (clk_states) {
				for (j = 0; j < clk_states->state_count; ++j) {
					clk_set_parent(devfreq_isp_clk[clk_states->state[j].clk_idx].clk,
						devfreq_isp_clk[clk_states->state[j].parent_clk_idx].clk);
				}
			}

			if (clk_info->freq != 0)
				clk_set_rate(devfreq_isp_clk[devfreq_clk_isp_info_idx[i]].clk, clk_info->freq);
#ifdef CONFIG_PM_RUNTIME
		if (pm_domain != NULL)
			mutex_unlock(&pm_domain->access_lock);
#endif
		}
	}

	return 0;
}

#define CONSTRAINT_VOLT		900000
extern int exynos5_int_check_voltage_constraint(unsigned long isp_voltage);

static int exynos5_devfreq_isp_set_volt(struct devfreq_data_isp *data,
		unsigned long volt,
		unsigned long volt_range,
		bool tolower)
{
	if (data->old_volt == volt)
		goto out;

	if (!tolower && (volt >= CONSTRAINT_VOLT))
		exynos5_int_check_voltage_constraint(volt);

	regulator_set_voltage(data->vdd_isp, volt, volt_range);
	data->old_volt = volt;

	if (tolower && (volt >= CONSTRAINT_VOLT))
		exynos5_int_check_voltage_constraint(volt);
out:
	return 0;
}

#ifdef CONFIG_PM_RUNTIME
static int exynos5_devfreq_isp_init_pm_domain(void)
{
	struct platform_device *pdev = NULL;
	struct device_node *np = NULL;
	int i;

	for_each_compatible_node(np, NULL, "samsung,exynos-pd") {
		struct exynos_pm_domain *pd;

		if (!of_device_is_available(np))
			continue;

		pdev = of_find_device_by_node(np);
		pd = platform_get_drvdata(pdev);

		for (i = 0; i < ARRAY_SIZE(devfreq_isp_pm_domain); ++i) {
			if (devfreq_isp_pm_domain[i].pm_domain_name == NULL)
				continue;

			if (!strcmp(devfreq_isp_pm_domain[i].pm_domain_name, pd->genpd.name))
				devfreq_isp_pm_domain[i].pm_domain = pd;
		}
	}

	return 0;
}
#endif

static int exynos5_devfreq_isp_init_clock(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(devfreq_isp_clk); ++i) {
		devfreq_isp_clk[i].clk = __clk_lookup(devfreq_isp_clk[i].clk_name);
		if (IS_ERR_OR_NULL(devfreq_isp_clk[i].clk)) {
			pr_err("DEVFREQ(ISP) : %s can't get clock\n", devfreq_isp_clk[i].clk_name);
			return -EINVAL;
		}
	}

	return 0;
}

#ifdef CONFIG_EXYNOS_THERMAL
int exynos5_devfreq_isp_tmu_notifier(struct notifier_block *nb, unsigned long event,
						void *v)
{
	struct devfreq_data_isp *data = container_of(nb, struct devfreq_data_isp, tmu_notifier);
	unsigned int prev_volt, set_volt;
	unsigned int *on = v;

	if (event == TMU_COLD) {
		if (pm_qos_request_active(&min_isp_thermal_qos))
			pm_qos_update_request(&min_isp_thermal_qos, data->initial_freq);

		if (*on) {
			mutex_lock(&data->lock);

			prev_volt = regulator_get_voltage(data->vdd_isp);

			if (data->volt_offset != COLD_VOLT_OFFSET) {
				data->volt_offset = COLD_VOLT_OFFSET;
			} else {
				mutex_unlock(&data->lock);
				return NOTIFY_OK;
			}

			set_volt = get_limit_voltage(prev_volt, data->volt_offset);
			regulator_set_voltage(data->vdd_isp, set_volt, set_volt + VOLT_STEP);

			mutex_unlock(&data->lock);
		} else {
			mutex_lock(&data->lock);

			prev_volt = regulator_get_voltage(data->vdd_isp);

			if (data->volt_offset != 0) {
				data->volt_offset = 0;
			} else {
				mutex_unlock(&data->lock);
				return NOTIFY_OK;
			}

			set_volt = get_limit_voltage(prev_volt - COLD_VOLT_OFFSET, data->volt_offset);
			regulator_set_voltage(data->vdd_isp, set_volt, set_volt + VOLT_STEP);

			mutex_unlock(&data->lock);
		}

		if (pm_qos_request_active(&min_isp_thermal_qos))
			pm_qos_update_request(&min_isp_thermal_qos, data->default_qos);
	}

	return NOTIFY_OK;
}
#endif

int exynos5433_devfreq_isp_init(struct devfreq_data_isp *data)
{
	int ret;
	data_isp = data;
	if (exynos5_devfreq_isp_init_clock()) {
		ret = -EINVAL;
		goto err_data;
	}

#ifdef CONFIG_PM_RUNTIME
	if (exynos5_devfreq_isp_init_pm_domain()) {
		ret = -EINVAL;
		goto err_data;
	}
#endif
	data->isp_set_freq = exynos5_devfreq_isp_set_freq;
	data->isp_set_volt = exynos5_devfreq_isp_set_volt;
err_data:
	return ret;
}
int exynos5433_devfreq_isp_deinit(struct devfreq_data_isp *data)
{
	return 0;
}
/* end of ISP related function */
