/* sound/soc/samsung/lpass.c
 *
 * Low Power Audio SubSystem driver for Samsung Exynos
 *
 * Copyright (c) 2013 Samsung Electronics Co. Ltd.
 *	Yeongman Seo <yman.seo@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/pm_runtime.h>
#include <linux/pm_qos.h>
#include <linux/fb.h>
#include <linux/iommu.h>
#include <linux/dma-mapping.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/sched.h>
#include <linux/sched/rt.h>
#include <linux/cpu.h>
#include <linux/kthread.h>

#include <sound/exynos.h>

#include <mach/map.h>
#include <mach/regs-audss.h>
#include <mach/regs-pmu.h>
#include <mach/regs-clock-exynos5430.h>
#include <mach/regs-clock-exynos5422.h>
#include <mach/cpufreq.h>

#include "lpass.h"

#ifdef USE_EXYNOS_AUD_SCHED
#define AUD_TASK_CPU_UHQ	(5)
#endif

#ifdef CONFIG_PM_DEVFREQ
#define USE_AUD_DEVFREQ
#ifdef CONFIG_SOC_EXYNOS5422
#define AUD_CPU_FREQ_UHQA	(1000000)
#define AUD_KFC_FREQ_UHQA	(1300000)
#define AUD_MIF_FREQ_UHQA	(413000)
#define AUD_INT_FREQ_UHQA	(0)
#define AUD_CPU_FREQ_HIGH	(0)
#define AUD_KFC_FREQ_HIGH	(1300000)
#define AUD_MIF_FREQ_HIGH	(0)
#define AUD_INT_FREQ_HIGH	(0)
#define AUD_CPU_FREQ_NORM	(0)
#define AUD_KFC_FREQ_NORM	(0)
#define AUD_MIF_FREQ_NORM	(0)
#define AUD_INT_FREQ_NORM	(0)
#else
#define AUD_CPU_FREQ_UHQA	(1000000)
#define AUD_KFC_FREQ_UHQA	(1300000)
#define AUD_MIF_FREQ_UHQA	(413000)
#define AUD_INT_FREQ_UHQA	(0)
#define AUD_CPU_FREQ_HIGH	(0)
#define AUD_KFC_FREQ_HIGH	(600000)
#define AUD_MIF_FREQ_HIGH	(0)
#define AUD_INT_FREQ_HIGH	(0)
#define AUD_CPU_FREQ_NORM	(0)
#define AUD_KFC_FREQ_NORM	(0)
#define AUD_MIF_FREQ_NORM	(0)
#define AUD_INT_FREQ_NORM	(0)
#endif
#endif

/* Target clock rate */
#define TARGET_ACLKENM_RATE	(133000000)
#define TARGET_PCLKEN_DBG_RATE	(66000000)

/* Default interrupt mask */
#define INTR_CA5_MASK_VAL	(LPASS_INTR_SFR)
#define INTR_CPU_MASK_VAL	(LPASS_INTR_DMA | LPASS_INTR_I2S | \
				 LPASS_INTR_PCM | LPASS_INTR_SB | \
				 LPASS_INTR_UART | LPASS_INTR_SFR)

/* Default clk gate for slimbus, pcm, i2s, pclk_dbg, ca5 */
#define INIT_CLK_GATE_MASK	(1 << 11 | 1 << 10 | 1 << 8 | 1 << 7 | \
				 1 <<  6 | 1 <<  5 | 1 << 1 | 1 << 0)

#define msecs_to_loops(t) (loops_per_jiffy / 1000 * HZ * t)

/* Audio subsystem version */
enum {
	LPASS_VER_000100 = 0,		/* pega/carmen */
	LPASS_VER_010100,		/* hudson */
	LPASS_VER_100100 = 16,		/* gaia/adonis */
	LPASS_VER_110100,		/* ares */
	LPASS_VER_370100 = 32,		/* rhea/helsinki */
	LPASS_VER_MAX
};

struct lpass_info {
	spinlock_t		lock;
	bool			valid;
	bool			enabled;
	int			ver;
	struct platform_device	*pdev;
	void __iomem		*regs;
	void __iomem		*mem;
	struct iommu_domain	*domain;
	struct proc_dir_entry	*proc_file;
	struct clk		*clk_dmac;
	struct clk		*clk_sramc;
	struct clk		*clk_intr;
	struct clk		*clk_timer;
	struct clk		*clk_fout_dpll;
	struct clk		*clk_mout_dpll_ctrl;
	struct clk		*clk_mout_mau_epll_clk;
	struct clk		*clk_mout_mau_epll_clk_user;
	struct clk		*clk_mout_ass_clk;
	struct clk		*clk_mout_ass_i2s;
	struct clk		*clk_fin_pll;
	struct clk		*clk_fout_aud_pll;
	struct clk		*clk_mout_aud_pll;
	struct clk		*clk_mout_aud_pll_user_top;
	struct clk		*clk_mout_aud_pll_user;
	struct clk		*clk_mout_aud_pll_sub;
	bool			rpm_enabled;
	atomic_t		use_cnt;
	atomic_t		stream_cnt;
	bool			display_on;
	bool			uhqa_on;
#ifdef USE_AUD_DEVFREQ
	struct pm_qos_request	aud_kfc_min_num_qos;
	struct pm_qos_request	aud_cpu_qos;
	struct pm_qos_request	aud_kfc_qos;
	struct pm_qos_request	aud_mif_qos;
	struct pm_qos_request	aud_int_qos;
	int			kfc_min_num_qos;
	int			cpu_qos;
	int			kfc_qos;
	int			mif_qos;
	int			int_qos;
#endif
} lpass;

struct aud_reg {
	void __iomem		*reg;
	u32			val;
	struct list_head	node;
};

struct subip_info {
	struct device		*dev;
	const char		*name;
	void			(*cb)(struct device *dev);
	atomic_t		use_cnt;
	struct list_head	node;
};

static LIST_HEAD(reg_list);
static LIST_HEAD(subip_list);

extern int check_adma_status(void);
extern int check_fdma_status(void);
extern int check_esa_status(void);

static void lpass_update_qos(void);

static inline bool is_old_ass(void)
{
	return lpass.ver < LPASS_VER_370100 ? true : false;
}

static inline bool is_new_ass(void)
{
	return lpass.ver >= LPASS_VER_370100 ? true : false;
}

static inline bool is_running_only(const char *name)
{
	struct subip_info *si;

	if (atomic_read(&lpass.use_cnt) != 1)
		return false;

	list_for_each_entry(si, &subip_list, node) {
		if (atomic_read(&si->use_cnt) > 0 &&
			!strncmp(name, si->name, strlen(si->name)))
			return true;
	}

	return false;
}

int exynos_check_aud_pwr(void)
{
	int dram_used = check_adma_status();

#ifdef CONFIG_SND_SAMSUNG_FAKEDMA
	dram_used |= check_fdma_status();
#endif
#ifdef CONFIG_SND_SAMSUNG_SEIREN
	dram_used |= check_esa_status();
#endif
	if (!lpass.enabled)
		return AUD_PWR_SLEEP;
	else if (is_running_only("aud-uart"))
		return AUD_PWR_LPC;
	else if (!dram_used)
		return AUD_PWR_LPA;

	if (is_new_ass())
		return AUD_PWR_ALPA;
	else
		return AUD_PWR_AFTR;
}

void __iomem *lpass_get_regs(void)
{
	return lpass.regs;
}

void __iomem *lpass_get_mem(void)
{
	return lpass.mem;
}

struct iommu_domain *lpass_get_iommu_domain(void)
{
	return lpass.domain;
}

void ass_reset(int ip, int op)
{
	spin_lock(&lpass.lock);

	spin_unlock(&lpass.lock);
}

void lpass_reset(int ip, int op)
{
	u32 reg, val, bit;

	if (is_old_ass()) {
		ass_reset(ip, op);
		return;
	}

	spin_lock(&lpass.lock);
	reg = LPASS_CORE_SW_RESET;
	switch (ip) {
	case LPASS_IP_DMA:
		bit = LPASS_SW_RESET_DMA;
		break;
	case LPASS_IP_MEM:
		bit = LPASS_SW_RESET_MEM;
		break;
	case LPASS_IP_TIMER:
		bit = LPASS_SW_RESET_TIMER;
		break;
	case LPASS_IP_I2S:
		bit = LPASS_SW_RESET_I2S;
		break;
	case LPASS_IP_PCM:
		bit = LPASS_SW_RESET_PCM;
		break;
	case LPASS_IP_UART:
		bit = LPASS_SW_RESET_UART;
		break;
	case LPASS_IP_SLIMBUS:
		bit = LPASS_SW_RESET_SB;
		break;
	case LPASS_IP_CA5:
		reg = LPASS_CA5_SW_RESET;
		bit = LPASS_SW_RESET_CA5;
		break;
	default:
		spin_unlock(&lpass.lock);
		pr_err("%s: wrong ip type %d!\n", __func__, ip);
		return;
	}

	val = readl(lpass.regs + reg);
	switch (op) {
	case LPASS_OP_RESET:
		val &= ~bit;
		break;
	case LPASS_OP_NORMAL:
		val |= bit;
		break;
	default:
		spin_unlock(&lpass.lock);
		pr_err("%s: wrong op type %d!\n", __func__, op);
		return;
	}

	writel(val, lpass.regs + reg);
	spin_unlock(&lpass.lock);
}

void lpass_reset_toggle(int ip)
{
	pr_debug("%s: %d\n", __func__, ip);

	lpass_reset(ip, LPASS_OP_RESET);
	udelay(100);
	lpass_reset(ip, LPASS_OP_NORMAL);
}

int lpass_register_subip(struct device *ip_dev, const char *ip_name)
{
	struct device *dev = &lpass.pdev->dev;
	struct subip_info *si;

	si = devm_kzalloc(dev, sizeof(struct subip_info), GFP_KERNEL);
	if (!si)
		return -1;

	si->dev = ip_dev;
	si->name = ip_name;
	si->cb = NULL;
	atomic_set(&si->use_cnt, 0);
	list_add(&si->node, &subip_list);

	pr_info("%s: %s(%p) registered\n", __func__, ip_name, ip_dev);

	if (!lpass.rpm_enabled &&
		(!strncmp(ip_name, "i2s", 3) || !strncmp(ip_name, "ca5", 3))) {
		lpass.rpm_enabled = true;
		pm_runtime_enable(&lpass.pdev->dev);
	}

	return 0;
}

int lpass_set_gpio_cb(struct device *ip_dev, void (*ip_cb)(struct device *dev))
{
	struct subip_info *si;

	list_for_each_entry(si, &subip_list, node) {
		if (si->dev == ip_dev) {
			si->cb = ip_cb;
			pr_info("%s: %s(cb: %p)\n", __func__,
				si->name, si->cb);
			return 0;
		}
	}

	return -EINVAL;
}

void lpass_get_sync(struct device *ip_dev)
{
	struct subip_info *si;

	list_for_each_entry(si, &subip_list, node) {
		if (si->dev == ip_dev) {
			atomic_inc(&si->use_cnt);
			atomic_inc(&lpass.use_cnt);
			pr_debug("%s: %s (use:%d)\n", __func__,
				si->name, atomic_read(&si->use_cnt));
			pm_runtime_get_sync(&lpass.pdev->dev);
		}
	}

	lpass_update_qos();
}

void lpass_put_sync(struct device *ip_dev)
{
	struct subip_info *si;

	list_for_each_entry(si, &subip_list, node) {
		if (si->dev == ip_dev) {
			atomic_dec(&si->use_cnt);
			atomic_dec(&lpass.use_cnt);
			pr_debug("%s: %s (use:%d)\n", __func__,
				si->name, atomic_read(&si->use_cnt));
			pm_runtime_put_sync(&lpass.pdev->dev);
		}
	}

	lpass_update_qos();
}

void lpass_set_sched(pid_t pid, int mode)
{
	struct sched_param param_fifo = {.sched_priority = MAX_RT_PRIO >> 1};
	struct task_struct *task;

	rcu_read_lock();
	task = find_task_by_vpid(pid);
	rcu_read_unlock();

	switch (mode) {
	case AUD_MODE_UHQA:
		lpass.uhqa_on = true;
		break;
	case AUD_MODE_NORM:
		lpass.uhqa_on = false;
		break;
	default:
		break;
	}

	lpass_update_qos();

	if (task) {
		sched_setscheduler_nocheck(task,
				SCHED_FIFO, &param_fifo);
		pr_info("%s: [%s] pid = %d, prio = %d\n",
				__func__, task->comm, pid, task->prio);
	} else {
		pr_err("%s: task not found (pid = %d)\n",
				__func__, pid);
	}
}

#ifdef USE_EXYNOS_AUD_CPU_HOTPLUG

void lpass_get_cpu_hotplug(void)
{
	pr_debug("%s ++\n", __func__);
	pm_qos_update_request(&lpass.aud_kfc_min_num_qos, lpass.kfc_min_num_qos);
}

void lpass_put_cpu_hotplug(void)
{
	pr_debug("%s --\n", __func__);
	pm_qos_update_request(&lpass.aud_kfc_min_num_qos, 1);
}
#endif

void lpass_add_stream(void)
{
	atomic_inc(&lpass.stream_cnt);
	lpass_update_qos();
}

void lpass_remove_stream(void)
{
	atomic_dec(&lpass.stream_cnt);
	lpass_update_qos();
}

static void lpass_reg_save(void)
{
	struct aud_reg *ar;

	pr_debug("Registers of LPASS are saved\n");

	list_for_each_entry(ar, &reg_list, node)
		ar->val = readl(ar->reg);

	return;
}

static void lpass_reg_restore(void)
{
	struct aud_reg *ar;

	pr_debug("Registers of LPASS are restore\n");

	list_for_each_entry(ar, &reg_list, node)
		writel(ar->val, ar->reg);

	return;
}

static void lpass_pll_enable(bool on)
{
	u32 pll_con0;

#define PLL_CON0_ENABLE		(1 << 31)
#define PLL_CON0_LOCKED		(1 << 29)

	pll_con0 = __raw_readl(EXYNOS5430_AUD_PLL_CON0);
	pll_con0 &= ~PLL_CON0_ENABLE;
	pll_con0 |= on ? PLL_CON0_ENABLE : 0;
	__raw_writel(pll_con0, EXYNOS5430_AUD_PLL_CON0);

	if (on) {
		/* wait_lock_time */
		do {
			cpu_relax();
			pll_con0 = __raw_readl(EXYNOS5430_AUD_PLL_CON0);
		} while (!(pll_con0 & PLL_CON0_LOCKED));
	}
}

static void lpass_retention_pad(void)
{
	struct subip_info *si;

	/* Powerdown mode for gpio */
	list_for_each_entry(si, &subip_list, node) {
		if (si->cb != NULL)
			(*si->cb)(si->dev);
	}

	/* Set PAD retention */
	writel(1, EXYNOS5430_GPIO_MODE_AUD_SYS_PWR_REG);
}

static void lpass_release_pad(void)
{
	struct subip_info *si;

	/* Restore gpio */
	list_for_each_entry(si, &subip_list, node) {
		if (si->cb != NULL)
			(*si->cb)(si->dev);
	}

	/* Release PAD retention */
	writel(1 << 28, EXYNOS_PAD_RET_MAUDIO_OPTION);
	writel(1, EXYNOS5430_GPIO_MODE_AUD_SYS_PWR_REG);
}

static void ass_enable(void)
{
	lpass_reg_restore();

	/* ASS_MUX_SEL */
	clk_set_parent(lpass.clk_mout_dpll_ctrl, lpass.clk_fout_dpll);
	clk_set_parent(lpass.clk_mout_mau_epll_clk, lpass.clk_mout_dpll_ctrl);
	clk_set_parent(lpass.clk_mout_mau_epll_clk_user, lpass.clk_mout_mau_epll_clk);
	clk_set_parent(lpass.clk_mout_ass_clk, lpass.clk_mout_mau_epll_clk_user);
	clk_set_parent(lpass.clk_mout_ass_i2s, lpass.clk_mout_ass_clk);

	clk_prepare_enable(lpass.clk_dmac);
	clk_prepare_enable(lpass.clk_timer);

	lpass.enabled = true;
}

static void lpass_enable(void)
{
	if (!lpass.valid) {
		pr_debug("%s: LPASS is not available", __func__);
		return;
	}

	if (is_old_ass()) {
		ass_enable();
		return;
	}

	/* Enable AUD_PLL */
	lpass_pll_enable(true);

	lpass_reg_restore();

#ifdef CONFIG_SOC_EXYNOS5430_REV_0
	/* AUD0 */
	clk_set_parent(lpass.clk_mout_aud_pll_user, lpass.clk_mout_aud_pll);
	clk_set_parent(lpass.clk_mout_aud_pll_sub, lpass.clk_mout_aud_pll_user);
#else
	/* AUD0 */
	clk_set_parent(lpass.clk_mout_aud_pll_user, lpass.clk_fout_aud_pll);
#endif
	/* TOP1 */
	clk_set_parent(lpass.clk_mout_aud_pll, lpass.clk_fout_aud_pll);
	clk_set_parent(lpass.clk_mout_aud_pll_user_top, lpass.clk_mout_aud_pll);

	clk_prepare_enable(lpass.clk_dmac);
	clk_prepare_enable(lpass.clk_sramc);

	lpass_reset_toggle(LPASS_IP_MEM);
	lpass_reset_toggle(LPASS_IP_I2S);
	lpass_reset_toggle(LPASS_IP_DMA);

	clk_disable_unprepare(lpass.clk_dmac);

	/* PAD */
	lpass_release_pad();

	lpass.enabled = true;
}

static void ass_disable(void)
{
	lpass.enabled = false;

	clk_disable_unprepare(lpass.clk_dmac);
	clk_disable_unprepare(lpass.clk_timer);

	lpass_reg_save();

	/* ASS_MUX_SEL */
	clk_set_parent(lpass.clk_mout_ass_clk, lpass.clk_fin_pll);
}

static void lpass_disable(void)
{
	if (!lpass.valid) {
		pr_debug("%s: LPASS is not available", __func__);
		return;
	}

	if (is_old_ass()) {
		ass_disable();
		return;
	}

	lpass.enabled = false;

	/* PAD */
	lpass_retention_pad();

	clk_disable_unprepare(lpass.clk_sramc);

	lpass_reg_save();

	/* TOP1 */
	clk_set_parent(lpass.clk_mout_aud_pll, lpass.clk_fin_pll);
	clk_set_parent(lpass.clk_mout_aud_pll_user_top, lpass.clk_fin_pll);

#ifdef CONFIG_SOC_EXYNOS5430_REV_0
	/* AUD0 */
	clk_set_parent(lpass.clk_mout_aud_pll_user, lpass.clk_fin_pll);
	clk_set_parent(lpass.clk_mout_aud_pll_sub, lpass.clk_mout_aud_pll_user);
#else
	/* AUD0 */
	clk_set_parent(lpass.clk_mout_aud_pll_user, lpass.clk_fin_pll);
#endif

	/* Enable clocks */
	writel(0x000007FF, EXYNOS5430_ENABLE_PCLK_AUD);
	writel(0x00000003, EXYNOS5430_ENABLE_SCLK_AUD0);
	writel(0x0000003F, EXYNOS5430_ENABLE_SCLK_AUD1);
	writel(0x00003FFF, EXYNOS5430_ENABLE_IP_AUD0);
	writel(0x0000003F, EXYNOS5430_ENABLE_IP_AUD1);

	/* Disable AUD_PLL */
	lpass_pll_enable(false);
}

static int clk_set_heirachy_ass(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	lpass.clk_fout_dpll = clk_get(NULL,"fout_dpll");
	if (IS_ERR_OR_NULL(lpass.clk_fout_dpll)) {
		dev_err(dev, "fout_dpll clk not found\n");
		goto err0;
	}

	lpass.clk_mout_dpll_ctrl = clk_get(dev,"mout_dpll_ctrl");
	if (IS_ERR_OR_NULL(lpass.clk_mout_dpll_ctrl)) {
		dev_err(dev, "mout_dpll_ctrl clk not found\n");
		goto err1;
	}

	lpass.clk_mout_mau_epll_clk = clk_get(dev,"mout_mau_epll_clk");
	if (IS_ERR_OR_NULL(lpass.clk_mout_mau_epll_clk)) {
		dev_err(dev, "mout_mau_epll_clk clk not found\n");
		goto err2;
	}

	lpass.clk_mout_mau_epll_clk_user = clk_get(dev,"mout_mau_epll_clk_user");
	if (IS_ERR_OR_NULL(lpass.clk_mout_mau_epll_clk_user)) {
		dev_err(dev, "mout_mau_epll_clk_user clk not found\n");
		goto err3;
	}

	lpass.clk_mout_ass_clk = clk_get(dev,"mout_ass_clk");
	if (IS_ERR_OR_NULL(lpass.clk_mout_ass_clk)) {
		dev_err(dev, "clk_mout_ass_clk clk not found\n");
		goto err4;
	}

	lpass.clk_mout_ass_i2s = clk_get(dev,"mout_ass_i2s");
	if (IS_ERR_OR_NULL(lpass.clk_mout_ass_i2s)) {
		dev_err(dev, "mout_ass_i2s clk not found\n");
		goto err5;
	}

	lpass.clk_dmac = clk_get(dev, "dmac");
	if (IS_ERR(lpass.clk_dmac)) {
		dev_err(dev, "dmac clk not found\n");
		goto err6;
	}

	lpass.clk_timer = clk_get(dev, "timer");
	if (IS_ERR(lpass.clk_timer)) {
		dev_err(dev, "timer clk not found\n");
		goto err7;
	}

	lpass.clk_fin_pll = clk_get(NULL, "fin_pll");
	if (IS_ERR(lpass.clk_fin_pll)) {
		dev_err(dev, "fin_pll clk not found\n");
		goto err8;
	}

	return 0;

err8:
	clk_put(lpass.clk_timer);
err7:
	clk_put(lpass.clk_dmac);
err6:
	clk_put(lpass.clk_mout_ass_i2s);
err5:
	clk_put(lpass.clk_mout_ass_clk);
err4:
	clk_put(lpass.clk_mout_mau_epll_clk_user);
err3:
	clk_put(lpass.clk_mout_mau_epll_clk);
err2:
	clk_put(lpass.clk_mout_dpll_ctrl);
err1:
	clk_put(lpass.clk_fout_dpll);
err0:
	return -1;
}

static int clk_set_heirachy(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	if (is_old_ass())
		return clk_set_heirachy_ass(pdev);

	lpass.clk_dmac = clk_get(dev, "dmac");
	if (IS_ERR(lpass.clk_dmac)) {
		dev_err(dev, "dmac clk not found\n");
		goto err0;
	}

	lpass.clk_sramc = clk_get(dev, "sramc");
	if (IS_ERR(lpass.clk_sramc)) {
		dev_err(dev, "sramc clk not found\n");
		goto err1;
	}

	lpass.clk_intr = clk_get(dev, "intr");
	if (IS_ERR(lpass.clk_intr)) {
		dev_err(dev, "intr clk not found\n");
		goto err2;
	}

	lpass.clk_timer = clk_get(dev, "timer");
	if (IS_ERR(lpass.clk_timer)) {
		dev_err(dev, "timer clk not found\n");
		goto err3;
	}

	lpass.clk_fin_pll = clk_get(dev, "fin_pll");
	if (IS_ERR(lpass.clk_fin_pll)) {
		dev_err(dev, "fin_pll clk not found\n");
		goto err4;
	}

	lpass.clk_fout_aud_pll = clk_get(dev, "fout_aud_pll");
	if (IS_ERR(lpass.clk_fout_aud_pll)) {
		dev_err(dev, "fout_aud_pll clk not found\n");
		goto err5;
	}

	lpass.clk_mout_aud_pll = clk_get(dev, "mout_aud_pll");
	if (IS_ERR(lpass.clk_mout_aud_pll)) {
		dev_err(dev, "mout_aud_pll clk not found\n");
		goto err6;
	}

	lpass.clk_mout_aud_pll_user_top = clk_get(dev, "mout_aud_pll_user_top");
	if (IS_ERR(lpass.clk_mout_aud_pll_user_top)) {
		dev_err(dev, "mout_aud_pll_user_top clk not found\n");
		goto err7;
	}

	lpass.clk_mout_aud_pll_user = clk_get(dev, "mout_aud_pll_user");
	if (IS_ERR(lpass.clk_mout_aud_pll_user)) {
		dev_err(dev, "mout_aud_pll_user clk not found\n");
		goto err8;
	}
#ifdef CONFIG_SOC_EXYNOS5430_REV_0
	lpass.clk_mout_aud_pll_sub = clk_get(dev, "mout_aud_pll_sub");
	if (IS_ERR(lpass.clk_mout_aud_pll_sub)) {
		dev_err(dev, "mout_aud_pll_sub clk not found\n");
		goto err9;
	}
#endif

#ifdef CONFIG_SOC_EXYNOS5430_REV_0
	clk_set_parent(lpass.clk_mout_aud_pll, lpass.clk_fout_aud_pll);
	clk_set_parent(lpass.clk_mout_aud_pll_user, lpass.clk_mout_aud_pll);
	clk_set_parent(lpass.clk_mout_aud_pll_sub, lpass.clk_mout_aud_pll_user);
#else
	clk_set_parent(lpass.clk_mout_aud_pll, lpass.clk_fout_aud_pll);
	clk_set_parent(lpass.clk_mout_aud_pll_user, lpass.clk_fout_aud_pll);
#endif

	return 0;

#ifdef CONFIG_SOC_EXYNOS5430_REV_0
err9:
	clk_put(lpass.clk_mout_aud_pll_user);
#endif
err8:
	clk_put(lpass.clk_mout_aud_pll_user_top);
err7:
	clk_put(lpass.clk_mout_aud_pll);
err6:
	clk_put(lpass.clk_fout_aud_pll);
err5:
	clk_put(lpass.clk_fin_pll);
err4:
	clk_put(lpass.clk_timer);
err3:
	clk_put(lpass.clk_intr);
err2:
	clk_put(lpass.clk_sramc);
err1:
	clk_put(lpass.clk_dmac);
err0:
	return -1;
}

static void clk_init(void)
{
	u32 val;

	val = readl(EXYNOS5430_ENABLE_IP_AUD0);
	val &= ~INIT_CLK_GATE_MASK;
	writel(val, EXYNOS5430_ENABLE_IP_AUD0);
}

static void clk_put_all_ass(void)
{
	clk_put(lpass.clk_dmac);
	clk_put(lpass.clk_timer);
}

static void clk_put_all(void)
{
	if (is_old_ass()) {
		clk_put_all_ass();
		return;
	}

	clk_put(lpass.clk_dmac);
	clk_put(lpass.clk_sramc);
	clk_put(lpass.clk_intr);
	clk_put(lpass.clk_timer);
}

static void lpass_add_suspend_reg(void __iomem *reg)
{
	struct device *dev = &lpass.pdev->dev;
	struct aud_reg *ar;

	ar = devm_kzalloc(dev, sizeof(struct aud_reg), GFP_KERNEL);
	if (!ar)
		return;

	ar->reg = reg;
	list_add(&ar->node, &reg_list);
}

static void lpass_init_reg_list_ass(void)
{
	lpass_add_suspend_reg(EXYNOS_CLKSRC_AUDSS);
	lpass_add_suspend_reg(EXYNOS_CLKDIV_AUDSS);
	lpass_add_suspend_reg(EXYNOS_CLKGATE_AUDSS);
}

static void lpass_init_reg_list(void)
{
	if (is_old_ass()) {
		lpass_init_reg_list_ass();
		return;
	}

	lpass_add_suspend_reg(EXYNOS5430_SRC_ENABLE_AUD0);
	lpass_add_suspend_reg(EXYNOS5430_SRC_ENABLE_AUD1);
	lpass_add_suspend_reg(EXYNOS5430_DIV_AUD0);
	lpass_add_suspend_reg(EXYNOS5430_DIV_AUD1);
	lpass_add_suspend_reg(EXYNOS5430_ENABLE_IP_AUD0);
	lpass_add_suspend_reg(EXYNOS5430_ENABLE_IP_AUD1);

	lpass_add_suspend_reg(lpass.regs + LPASS_INTR_CA5_MASK);
	lpass_add_suspend_reg(lpass.regs + LPASS_INTR_CPU_MASK);
}

static int lpass_proc_show(struct seq_file *m, void *v) {
	struct subip_info *si;
	int pmode = exynos_check_aud_pwr();

	seq_printf(m, "power: %s\n", lpass.enabled ? "on" : "off");
	seq_printf(m, "canbe: %s\n",
			(pmode == AUD_PWR_SLEEP) ? "sleep" :
			(pmode == AUD_PWR_LPA) ? "lpa" :
			(pmode == AUD_PWR_ALPA) ? "alpa" :
			(pmode == AUD_PWR_AFTR) ? "aftr" : "unknown");

	list_for_each_entry(si, &subip_list, node) {
		seq_printf(m, "subip: %s (%d)\n",
				si->name, atomic_read(&si->use_cnt));
	}

	seq_printf(m, "strm: %d\n", atomic_read(&lpass.stream_cnt));
	seq_printf(m, "uhqa: %s\n", lpass.uhqa_on ? "on" : "off");
#ifdef USE_AUD_DEVFREQ
	seq_printf(m, "cpu: %d, kfc: %d\n",
			lpass.cpu_qos / 1000, lpass.kfc_qos / 1000);
	seq_printf(m, "mif: %d, int: %d\n",
			lpass.mif_qos / 1000, lpass.int_qos / 1000);
#endif
	return 0;
}

static int lpass_proc_open(struct inode *inode, struct  file *file) {
	return single_open(file, lpass_proc_show, NULL);
}

static const struct file_operations lpass_proc_fops = {
	.owner = THIS_MODULE,
	.open = lpass_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

#ifdef CONFIG_PM_SLEEP
static int lpass_suspend(struct device *dev)
{
	pr_debug("%s entered\n", __func__);

#ifdef CONFIG_PM_RUNTIME
	if (atomic_read(&lpass.use_cnt) > 0)
		lpass_disable();
#else
	lpass_disable();
#endif
	return 0;
}

static int lpass_resume(struct device *dev)
{
	pr_debug("%s entered\n", __func__);

#ifdef CONFIG_PM_RUNTIME
	if (atomic_read(&lpass.use_cnt) > 0)
		lpass_enable();
#else
	lpass_enable();
#endif
	return 0;
}
#else
#define lpass_suspend NULL
#define lpass_resume  NULL
#endif

#ifdef CONFIG_OF
static const struct of_device_id exynos_lpass_match[];

static int lpass_get_ver(struct device_node *np)
{
	const struct of_device_id *match;
	int ver;

	if (np) {
		match = of_match_node(exynos_lpass_match, np);
		ver = *(int *)(match->data);
	} else {
		ver = LPASS_VER_000100;
	}

	return ver;
}
#else
static int lpass_get_ver(struct device_node *np)
{
	return LPASS_VER_000100;
}
#endif

static void lpass_update_qos(void)
{
#ifdef USE_AUD_DEVFREQ
	int cpu_qos_new, kfc_qos_new, mif_qos_new, int_qos_new;

	if (!lpass.enabled) {
		cpu_qos_new = 0;
		kfc_qos_new = 0;
		mif_qos_new = 0;
		int_qos_new = 0;
	} else if (lpass.uhqa_on) {
		cpu_qos_new = AUD_CPU_FREQ_UHQA;
		kfc_qos_new = AUD_KFC_FREQ_UHQA;
		mif_qos_new = AUD_MIF_FREQ_UHQA;
		int_qos_new = AUD_INT_FREQ_UHQA;
	} else if (atomic_read(&lpass.stream_cnt) > 1) {
		cpu_qos_new = AUD_CPU_FREQ_HIGH;
		kfc_qos_new = AUD_KFC_FREQ_HIGH;
		mif_qos_new = AUD_MIF_FREQ_HIGH;
		int_qos_new = AUD_INT_FREQ_HIGH;
	} else {
		cpu_qos_new = AUD_CPU_FREQ_NORM;
		kfc_qos_new = AUD_KFC_FREQ_NORM;
		mif_qos_new = AUD_MIF_FREQ_NORM;
		int_qos_new = AUD_INT_FREQ_NORM;
	}

	if (lpass.cpu_qos != cpu_qos_new) {
		lpass.cpu_qos = cpu_qos_new;
		pm_qos_update_request(&lpass.aud_cpu_qos, lpass.cpu_qos);
		pr_debug("%s: cpu_qos = %d\n", __func__, lpass.cpu_qos);
	}

	if (lpass.kfc_qos != kfc_qos_new) {
		lpass.kfc_qos = kfc_qos_new;
		pm_qos_update_request(&lpass.aud_kfc_qos, lpass.kfc_qos);
		pr_debug("%s: kfc_qos = %d\n", __func__, lpass.kfc_qos);
	}

	if (lpass.mif_qos != mif_qos_new) {
		lpass.mif_qos = mif_qos_new;
		pm_qos_update_request(&lpass.aud_mif_qos, lpass.mif_qos);
		pr_debug("%s: mif_qos = %d\n", __func__, lpass.mif_qos);
	}

	if (lpass.int_qos != int_qos_new) {
		lpass.int_qos = int_qos_new;
		pm_qos_update_request(&lpass.aud_int_qos, lpass.int_qos);
		pr_debug("%s: int_qos = %d\n", __func__, lpass.int_qos);
	}
#endif
}

static int lpass_fb_state_chg(struct notifier_block *nb,
		unsigned long val, void *data)
{
	struct fb_event *evdata = data;
	unsigned int blank;

	if (val != FB_EVENT_BLANK)
		return 0;

	blank = *(int *)evdata->data;

	switch (blank) {
	case FB_BLANK_POWERDOWN:
		lpass.display_on = false;
		lpass_update_qos();
		break;
	case FB_BLANK_UNBLANK:
		lpass.display_on = true;
		lpass_update_qos();
		break;
	default:
		break;
	}

	return NOTIFY_OK;
}

static struct notifier_block fb_noti_block = {
	.notifier_call = lpass_fb_state_chg,
};

static char banner[] =
	KERN_INFO "Samsung Low Power Audio Subsystem driver, "\
		  "(c)2013 Samsung Electronics\n";

static int lpass_probe(struct platform_device *pdev)
{
	struct resource *res;
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	int ret = 0;

	printk(banner);

	lpass.pdev = pdev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(dev, "Unable to get LPASS SFRs\n");
		return -ENXIO;
	}

	lpass.regs = devm_request_and_ioremap(&pdev->dev, res);
	if (!lpass.regs) {
		dev_err(dev, "SFR ioremap failed\n");
		return -ENOMEM;
	}
	pr_debug("%s: regs_base = %08X (%08X bytes)\n",
		__func__, res->start, resource_size(res));

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!res) {
		dev_err(dev, "Unable to get LPASS SRAM\n");
		return -ENXIO;
	}

	lpass.mem = devm_request_and_ioremap(&pdev->dev, res);
	if (!lpass.mem) {
		dev_err(dev, "SRAM ioremap failed\n");
		return -ENOMEM;
	}
	pr_debug("%s: sram_base = %08X (%08X bytes)\n",
		__func__, res->start, resource_size(res));

	/* LPASS version */
	lpass.ver = lpass_get_ver(np);

	/* Set clock hierarchy for audio subsystem */
	ret = clk_set_heirachy(pdev);
	if (ret) {
		dev_err(dev, "failed to set clock hierachy\n");
		return -ENXIO;
	}
	clk_init();

#ifdef CONFIG_SND_SAMSUNG_IOMMU
	lpass.domain = iommu_domain_alloc(pdev->dev.bus);
	if (!lpass.domain) {
		dev_err(dev, "Unable to alloc iommu domain\n");
		return -ENOMEM;
	}

	ret = iommu_attach_device(lpass.domain, dev);
	if (ret) {
		dev_err(dev, "Unable to attach iommu device: %d\n", ret);
		iommu_domain_free(lpass.domain);
		return ret;
	}
#endif

	lpass.proc_file = proc_create("driver/lpass", 0,
					NULL, &lpass_proc_fops);
	if (!lpass.proc_file)
		pr_info("Failed to register /proc/driver/lpadd\n");

	spin_lock_init(&lpass.lock);
	atomic_set(&lpass.use_cnt, 0);
	atomic_set(&lpass.stream_cnt, 0);
	lpass_init_reg_list();

	/* unmask irq source */
	writel(INTR_CA5_MASK_VAL, lpass.regs + LPASS_INTR_CA5_MASK);
	writel(INTR_CPU_MASK_VAL, lpass.regs + LPASS_INTR_CPU_MASK);

	lpass_reg_save();
	lpass.valid = true;

#ifdef CONFIG_PM_RUNTIME
	lpass_reset_toggle(LPASS_IP_MEM);
	lpass_reset_toggle(LPASS_IP_I2S);
	lpass_reset_toggle(LPASS_IP_DMA);
#else
	lpass_enable();
#endif
	lpass.display_on = true;
	fb_register_client(&fb_noti_block);

#ifdef USE_AUD_DEVFREQ
	lpass.cpu_qos = 0;
	lpass.kfc_qos = 0;
	lpass.mif_qos = 0;
	lpass.int_qos = 0;
	lpass.kfc_min_num_qos = 2;
	pm_qos_add_request(&lpass.aud_kfc_min_num_qos, PM_QOS_KFC_NUM_MIN, 0);
	pm_qos_add_request(&lpass.aud_cpu_qos, PM_QOS_CPU_FREQ_MIN, 0);
	pm_qos_add_request(&lpass.aud_kfc_qos, PM_QOS_KFC_FREQ_MIN, 0);
	pm_qos_add_request(&lpass.aud_mif_qos, PM_QOS_BUS_THROUGHPUT, 0);
	pm_qos_add_request(&lpass.aud_int_qos, PM_QOS_DEVICE_THROUGHPUT, 0);
#endif
	return 0;
}

static int lpass_remove(struct platform_device *pdev)
{
#ifdef CONFIG_SND_SAMSUNG_IOMMU
	iommu_detach_device(lpass.domain, &pdev->dev);
	iommu_domain_free(lpass.domain);
#endif
#ifdef CONFIG_PM_RUNTIME
	pm_runtime_disable(&pdev->dev);
#else
	lpass_disable();
#endif
	clk_put_all();

	return 0;
}

#ifdef CONFIG_PM_RUNTIME
static int lpass_runtime_suspend(struct device *dev)
{
	pr_debug("%s entered\n", __func__);

	lpass_disable();

	return 0;
}

static int lpass_runtime_resume(struct device *dev)
{
	pr_debug("%s entered\n", __func__);

	lpass_enable();

	return 0;
}
#endif

static const int lpass_ver_data[] = {
	[LPASS_VER_000100] = LPASS_VER_000100,
	[LPASS_VER_010100] = LPASS_VER_010100,
	[LPASS_VER_100100] = LPASS_VER_100100,
	[LPASS_VER_110100] = LPASS_VER_110100,
	[LPASS_VER_370100] = LPASS_VER_370100,
};

static struct platform_device_id lpass_driver_ids[] = {
	{
		.name	= "samsung-lpass",
	}, {
		.name	= "samsung-audss",
	}, {},
};
MODULE_DEVICE_TABLE(platform, lpass_driver_ids);

#ifdef CONFIG_OF
static const struct of_device_id exynos_lpass_match[] = {
	{
		.compatible	= "samsung,exynos5430-lpass",
		.data		= &lpass_ver_data[LPASS_VER_370100],
	}, {
		.compatible	= "samsung,exynos5-audss",
		.data		= &lpass_ver_data[LPASS_VER_110100],
	}, {},
};
MODULE_DEVICE_TABLE(of, exynos_lpass_match);
#endif

static const struct dev_pm_ops lpass_pmops = {
	SET_SYSTEM_SLEEP_PM_OPS(
		lpass_suspend,
		lpass_resume
	)
	SET_RUNTIME_PM_OPS(
		lpass_runtime_suspend,
		lpass_runtime_resume,
		NULL
	)
};

static struct platform_driver lpass_driver = {
	.probe		= lpass_probe,
	.remove		= lpass_remove,
	.id_table	= lpass_driver_ids,
	.driver		= {
		.name	= "samsung-lpass",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(exynos_lpass_match),
		.pm	= &lpass_pmops,
	},
};

static int __init lpass_driver_init(void)
{
	return platform_driver_register(&lpass_driver);
}
subsys_initcall(lpass_driver_init);

/* Module information */
MODULE_AUTHOR("Yeongman Seo, <yman.seo@samsung.com>");
MODULE_DESCRIPTION("Samsung Low Power Audio Subsystem Interface");
MODULE_ALIAS("platform:samsung-lpass");
MODULE_LICENSE("GPL");
