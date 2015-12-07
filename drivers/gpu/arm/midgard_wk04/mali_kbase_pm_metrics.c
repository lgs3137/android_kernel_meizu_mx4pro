/*
 *
 * (C) COPYRIGHT ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * A copy of the licence is included with the program, and can also be obtained
 * from Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 */





/**
 * @file mali_kbase_pm_metrics.c
 * Metrics for power management
 */

#include <mali_kbase.h>
#include <mali_kbase_pm.h>
#include <platform/mali_kbase_platform.h>

/* When VSync is being hit aim for utilisation between 70-90% */
#define KBASE_PM_VSYNC_MIN_UTILISATION          70
#define KBASE_PM_VSYNC_MAX_UTILISATION          90
/* Otherwise aim for 10-40% */
#define KBASE_PM_NO_VSYNC_MIN_UTILISATION       10
#define KBASE_PM_NO_VSYNC_MAX_UTILISATION       40

/* Shift used for kbasep_pm_metrics_data.time_busy/idle - units of (1 << 8) ns
   This gives a maximum period between samples of 2^(32+8)/100 ns = slightly under 11s.
   Exceeding this will cause overflow */
#define KBASE_PM_TIME_SHIFT			8

#if defined(SLSI_SUBSTITUTE)
static void dvfs_callback(unsigned long __data)
#else
static enum hrtimer_restart dvfs_callback(struct hrtimer *timer)
#endif
{
	unsigned long flags;
	kbase_pm_dvfs_action action;
	kbasep_pm_metrics_data *metrics;
#if defined(SLSI_SUBSTITUTE)
	struct exynos_context *platform;
	struct timer_list *tlist = (struct timer_list *)__data;

	KBASE_DEBUG_ASSERT(tlist != NULL);

	metrics = container_of(tlist, kbasep_pm_metrics_data, tlist);
	platform = (struct exynos_context *)metrics->kbdev->platform_context;
#else
	KBASE_DEBUG_ASSERT(timer != NULL);

	metrics = container_of(timer, kbasep_pm_metrics_data, timer);
#endif

	action = kbase_pm_get_dvfs_action(metrics->kbdev);

	spin_lock_irqsave(&metrics->lock, flags);

	if (metrics->timer_active) {
#if defined(SLSI_SUBSTITUTE)
		metrics->tlist.function = dvfs_callback;
		metrics->tlist.expires = jiffies + msecs_to_jiffies(platform->polling_speed);
		add_timer_on(&metrics->tlist, 0);
#else
		hrtimer_start(timer,
					  HR_TIMER_DELAY_MSEC(metrics->kbdev->pm.platform_dvfs_frequency),
					  HRTIMER_MODE_REL);
#endif
	}

	spin_unlock_irqrestore(&metrics->lock, flags);
#if !defined(SLSI_SUBSTITUTE)
	return HRTIMER_NORESTART;
#endif
}

mali_error kbasep_pm_metrics_init(kbase_device *kbdev)
{
#if defined(SLSI_SUBSTITUTE)
	struct exynos_context *platform;
	platform = (struct exynos_context *)kbdev->platform_context;
#endif
	KBASE_DEBUG_ASSERT(kbdev != NULL);

	kbdev->pm.metrics.kbdev = kbdev;
	kbdev->pm.metrics.vsync_hit = 0;
	kbdev->pm.metrics.utilisation = 0;

	kbdev->pm.metrics.time_period_start = ktime_get();
	kbdev->pm.metrics.time_busy = 0;
	kbdev->pm.metrics.time_idle = 0;
	/* MALI_SEC */
#ifdef SEPERATED_UTILIZATION
	kbdev->pm.metrics.gpu_active = MALI_FALSE;
#else
	kbdev->pm.metrics.gpu_active = MALI_TRUE;
#endif /* SEPERATED_UTILIZATION */
	kbdev->pm.metrics.timer_active = MALI_TRUE;

	spin_lock_init(&kbdev->pm.metrics.lock);
#if defined(SLSI_SUBSTITUTE)
	/* use timer with core affinity */
	init_timer(&kbdev->pm.metrics.tlist);
	kbdev->pm.metrics.tlist.function = dvfs_callback;
	kbdev->pm.metrics.tlist.expires = jiffies + msecs_to_jiffies(platform->polling_speed);
	kbdev->pm.metrics.tlist.data = (unsigned long)&kbdev->pm.metrics.tlist;
	add_timer_on(&kbdev->pm.metrics.tlist, 0);
#else

	hrtimer_init(&kbdev->pm.metrics.timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	kbdev->pm.metrics.timer.function = dvfs_callback;

	hrtimer_start(&kbdev->pm.metrics.timer, HR_TIMER_DELAY_MSEC(kbdev->pm.platform_dvfs_frequency), HRTIMER_MODE_REL);
#endif

	kbase_pm_register_vsync_callback(kbdev);
#if defined(CL_UTILIZATION_BOOST_BY_TIME_WEIGHT)
	atomic_set(&kbdev->pm.metrics.time_compute_jobs, 0);atomic_set(&kbdev->pm.metrics.time_vertex_jobs, 0);atomic_set(&kbdev->pm.metrics.time_fragment_jobs, 0);
#endif

	return MALI_ERROR_NONE;
}

KBASE_EXPORT_TEST_API(kbasep_pm_metrics_init)

void kbasep_pm_metrics_term(kbase_device *kbdev)
{
	unsigned long flags;
	KBASE_DEBUG_ASSERT(kbdev != NULL);

	spin_lock_irqsave(&kbdev->pm.metrics.lock, flags);
	kbdev->pm.metrics.timer_active = MALI_FALSE;
	spin_unlock_irqrestore(&kbdev->pm.metrics.lock, flags);
#if defined(SLSI_SUBSTITUTE)
	del_timer(&kbdev->pm.metrics.tlist);
#else
	hrtimer_cancel(&kbdev->pm.metrics.timer);
#endif
	kbase_pm_unregister_vsync_callback(kbdev);
}

KBASE_EXPORT_TEST_API(kbasep_pm_metrics_term)

void kbasep_pm_record_gpu_idle(kbase_device *kbdev)
{
	unsigned long flags;
	ktime_t now;
	ktime_t diff;

	KBASE_DEBUG_ASSERT(kbdev != NULL);

	spin_lock_irqsave(&kbdev->pm.metrics.lock, flags);

	KBASE_DEBUG_ASSERT(kbdev->pm.metrics.gpu_active == MALI_TRUE);

	kbdev->pm.metrics.gpu_active = MALI_FALSE;

	now = ktime_get();
	diff = ktime_sub(now, kbdev->pm.metrics.time_period_start);

	kbdev->pm.metrics.time_busy += (u32) (ktime_to_ns(diff) >> KBASE_PM_TIME_SHIFT);
	kbdev->pm.metrics.time_period_start = now;

	spin_unlock_irqrestore(&kbdev->pm.metrics.lock, flags);
}

KBASE_EXPORT_TEST_API(kbasep_pm_record_gpu_idle)

void kbasep_pm_record_gpu_active(kbase_device *kbdev)
{
	unsigned long flags;
	ktime_t now;
	ktime_t diff;

	KBASE_DEBUG_ASSERT(kbdev != NULL);

	spin_lock_irqsave(&kbdev->pm.metrics.lock, flags);

	KBASE_DEBUG_ASSERT(kbdev->pm.metrics.gpu_active == MALI_FALSE);

	kbdev->pm.metrics.gpu_active = MALI_TRUE;

	now = ktime_get();
	diff = ktime_sub(now, kbdev->pm.metrics.time_period_start);

	kbdev->pm.metrics.time_idle += (u32) (ktime_to_ns(diff) >> KBASE_PM_TIME_SHIFT);
	kbdev->pm.metrics.time_period_start = now;

	spin_unlock_irqrestore(&kbdev->pm.metrics.lock, flags);
}

KBASE_EXPORT_TEST_API(kbasep_pm_record_gpu_active)

#ifdef SEPERATED_UTILIZATION
void kbase_pm_record_gpu_state(struct kbase_device *kbdev, mali_bool is_active)
{
    unsigned long flags;
    ktime_t now;
    ktime_t diff;

    KBASE_DEBUG_ASSERT(kbdev != NULL);

    wake_lock(&kbdev->pm.kbase_wake_lock);
    mutex_lock(&kbdev->pm.lock);

    spin_lock_irqsave(&kbdev->pm.metrics.lock, flags);

    now = ktime_get();
    diff = ktime_sub(now, kbdev->pm.metrics.time_period_start);

    /* Note: We cannot use kbdev->pm.gpu_powered for debug checks that
     * we're in the right state because:
     * 1) we may be doing a delayed poweroff, in which case gpu_powered
     *    might (or might not, depending on timing) still be true soon after
     *    the call to kbase_pm_context_idle()
     * 2) hwcnt collection keeps the GPU powered
     */
    if (!kbdev->pm.metrics.gpu_active && is_active) {
        /* Going from idle to active, and not already recorded.
         * Log current time spent idle so far */

        kbdev->pm.metrics.time_idle += (u32) (ktime_to_ns(diff) >> KBASE_PM_TIME_SHIFT);
    } else if (kbdev->pm.metrics.gpu_active && !is_active) {
        /* Going from active to idle, and not already recorded.
         * Log current time spent active so far */

        kbdev->pm.metrics.time_busy += (u32) (ktime_to_ns(diff) >> KBASE_PM_TIME_SHIFT);
    }
    kbdev->pm.metrics.time_period_start = now;
    kbdev->pm.metrics.gpu_active = is_active;

    spin_unlock_irqrestore(&kbdev->pm.metrics.lock, flags);

    mutex_unlock(&kbdev->pm.lock);
    wake_unlock(&kbdev->pm.kbase_wake_lock);
}
KBASE_EXPORT_TEST_API(kbase_pm_record_gpu_state)
#endif

void kbase_pm_report_vsync(kbase_device *kbdev, int buffer_updated)
{
	unsigned long flags;
	KBASE_DEBUG_ASSERT(kbdev != NULL);

	spin_lock_irqsave(&kbdev->pm.metrics.lock, flags);
	kbdev->pm.metrics.vsync_hit = buffer_updated;
	spin_unlock_irqrestore(&kbdev->pm.metrics.lock, flags);
}

KBASE_EXPORT_TEST_API(kbase_pm_report_vsync)

#if defined(CL_UTILIZATION_BOOST_BY_TIME_WEIGHT)
/*
* peak_flops: 100/85
* sobel: 100/50
*/
#define COMPUTE_JOB_WEIGHT (10000/50)
#endif

/*caller needs to hold kbdev->pm.metrics.lock before calling this function*/
int kbase_pm_get_dvfs_utilisation(kbase_device *kbdev)
{
	int utilisation = 0;
#if defined(CL_UTILIZATION_BOOST_BY_TIME_WEIGHT)
	int compute_time = 0, vertex_time = 0, fragment_time = 0, total_time = 0, compute_time_rate = 0;
#endif

	ktime_t now = ktime_get();
	ktime_t diff;

	KBASE_DEBUG_ASSERT(kbdev != NULL);

	diff = ktime_sub(now, kbdev->pm.metrics.time_period_start);

	if (kbdev->pm.metrics.gpu_active) {
		kbdev->pm.metrics.time_busy += (u32) (ktime_to_ns(diff) >> KBASE_PM_TIME_SHIFT);
		kbdev->pm.metrics.time_period_start = now;
	} else {
		kbdev->pm.metrics.time_idle += (u32) (ktime_to_ns(diff) >> KBASE_PM_TIME_SHIFT);
		kbdev->pm.metrics.time_period_start = now;
	}

	if (kbdev->pm.metrics.time_idle + kbdev->pm.metrics.time_busy == 0) {
		/* No data - so we return NOP */
		utilisation = -1;
		goto out;
	}

	utilisation = (100 * kbdev->pm.metrics.time_busy) / (kbdev->pm.metrics.time_idle + kbdev->pm.metrics.time_busy);
	kbdev->pm.metrics.time_idle = 0;
	kbdev->pm.metrics.time_busy = 0;
#if defined(CL_UTILIZATION_BOOST_BY_TIME_WEIGHT)
	compute_time = atomic_read(&kbdev->pm.metrics.time_compute_jobs);
	vertex_time = atomic_read(&kbdev->pm.metrics.time_vertex_jobs);
	fragment_time = atomic_read(&kbdev->pm.metrics.time_fragment_jobs);
	total_time = compute_time + vertex_time + fragment_time;

	if (compute_time > 0 && total_time > 0)
	{
		compute_time_rate = (100 * compute_time) / total_time;
		utilisation = utilisation * (COMPUTE_JOB_WEIGHT * compute_time_rate + 100 * (100 - compute_time_rate));
		utilisation /= 10000;

		if (utilisation >= 100) utilisation = 100;
	}
#endif

	return utilisation;

 out:

	kbdev->pm.metrics.time_idle = 0;
	kbdev->pm.metrics.time_busy = 0;

	return utilisation;
}

kbase_pm_dvfs_action kbase_pm_get_dvfs_action(kbase_device *kbdev)
{
	unsigned long flags;
	int utilisation;
	kbase_pm_dvfs_action action;

	KBASE_DEBUG_ASSERT(kbdev != NULL);

	spin_lock_irqsave(&kbdev->pm.metrics.lock, flags);

	utilisation = kbase_pm_get_dvfs_utilisation(kbdev);

	if (utilisation < 0) {
		action = KBASE_PM_DVFS_NOP;
		utilisation = 0;
		goto out;
	}

	if (kbdev->pm.metrics.vsync_hit) {
		/* VSync is being met */
		if (utilisation < KBASE_PM_VSYNC_MIN_UTILISATION)
			action = KBASE_PM_DVFS_CLOCK_DOWN;
		else if (utilisation > KBASE_PM_VSYNC_MAX_UTILISATION)
			action = KBASE_PM_DVFS_CLOCK_UP;
		else
			action = KBASE_PM_DVFS_NOP;
	} else {
		/* VSync is being missed */
		if (utilisation < KBASE_PM_NO_VSYNC_MIN_UTILISATION)
			action = KBASE_PM_DVFS_CLOCK_DOWN;
		else if (utilisation > KBASE_PM_NO_VSYNC_MAX_UTILISATION)
			action = KBASE_PM_DVFS_CLOCK_UP;
		else
			action = KBASE_PM_DVFS_NOP;
	}

	kbdev->pm.metrics.utilisation = utilisation;
 out:
#ifdef CONFIG_MALI_T6XX_DVFS
	kbase_platform_dvfs_event(kbdev, utilisation);
#endif				/*CONFIG_MALI_T6XX_DVFS */
	kbdev->pm.metrics.time_idle = 0;
	kbdev->pm.metrics.time_busy = 0;
	spin_unlock_irqrestore(&kbdev->pm.metrics.lock, flags);

	return action;
}
KBASE_EXPORT_TEST_API(kbase_pm_get_dvfs_action)

mali_bool kbase_pm_metrics_is_active(kbase_device *kbdev)
{
	mali_bool isactive;
	unsigned long flags;

	KBASE_DEBUG_ASSERT(kbdev != NULL);

	spin_lock_irqsave(&kbdev->pm.metrics.lock, flags);
	isactive = (kbdev->pm.metrics.timer_active == MALI_TRUE);
	spin_unlock_irqrestore(&kbdev->pm.metrics.lock, flags);

	return isactive;
}
KBASE_EXPORT_TEST_API(kbase_pm_metrics_is_active)
