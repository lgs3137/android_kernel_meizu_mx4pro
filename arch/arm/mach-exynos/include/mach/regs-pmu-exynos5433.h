/*
 * Copyright (c) 2014 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * EXYNOS - Power management unit definition
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __ASM_ARCH_REGS_PMU_EXYNOS5433_H
#define __ASM_ARCH_REGS_PMU_EXYNOS5433 H __FILE__

#include <mach/map.h>

#define EXYNOS_PMUREG(x)					(S5P_VA_PMU + (x))

#define EXYNOS5433_PMU_SYNC_CTRL				EXYNOS_PMUREG(0x0020)

#define EXYNOS5433_CPU_RESET_DISABLE_FROM_WDTRESET		EXYNOS_PMUREG(0x0414)

#define EXYNOS5433_WAKEUP_STAT					EXYNOS_PMUREG(0x0600)
#define EXYNOS5433_WAKEUP_STAT1					EXYNOS_PMUREG(0x0604)
#define EXYNOS5433_WAKEUP_STAT2					EXYNOS_PMUREG(0x0608)
#define EXYNOS5433_EINT_WAKEUP_MASK				EXYNOS_PMUREG(0x060C)
#define EXYNOS5433_WAKEUP_MASK					EXYNOS_PMUREG(0x0610)
#define EXYNOS5433_WAKEUP_MASK1					EXYNOS_PMUREG(0x0614)
#define EXYNOS5433_WAKEUP_MASK2					EXYNOS_PMUREG(0x0618)
#define EXYNOS5433_WAKEUP_INTERRUPT				EXYNOS_PMUREG(0x061C)
#define EXYNOS5433_WAKEUP_STAT_MIF				EXYNOS_PMUREG(0x0620)
#define EXYNOS5433_EINT_WAKEUP_MASK_MIF				EXYNOS_PMUREG(0x0624)
#define EXYNOS5433_WAKEUP_MASK_MIF				EXYNOS_PMUREG(0x0628)
#define EXYNOS5433_EINT_WAKEUP_MASK1				EXYNOS_PMUREG(0x062C)

#define EXYNOS5433_LLI_PHY_CON					EXYNOS_PMUREG(0x0720)
#define EXYNOS5433_UFS_PHY_CON					EXYNOS_PMUREG(0x0724)
#define EXYNOS5433_USBHOST30_PHY_CON				EXYNOS_PMUREG(0x0728)
#define EXYNOS5433_EDP_PHY_CON					EXYNOS_PMUREG(0x072C)
#define EXYNOS5433_PCIE_PHY_CON					EXYNOS_PMUREG(0x0730)

#define EXYNOS5433_BB_CON0					EXYNOS_PMUREG(0x0780)
#define EXYNOS5433_BB_CON1					EXYNOS_PMUREG(0x0784)
#define EXYNOS5433_BB_CON2					EXYNOS_PMUREG(0x0788)
#define EXYNOS5433_BB_CON3					EXYNOS_PMUREG(0x078C)
#define EXYNOS5433_BB_CON4					EXYNOS_PMUREG(0x0790)
#define EXYNOS5433_BB_CON5					EXYNOS_PMUREG(0x0794)

#define EXYNOS5433_DREX_CALN0					EXYNOS_PMUREG(0x09A0)
#define EXYNOS5433_DREX_CALN1					EXYNOS_PMUREG(0x09A4)
#define EXYNOS5433_DREX_CALN2					EXYNOS_PMUREG(0x09A8)

#define EXYNOS5433_EAGLE_NONCPU_SYS_PWR_REG			EXYNOS_PMUREG(0x1080)
#define EXYNOS5433_KFC_NONCPU_SYS_PWR_REG			EXYNOS_PMUREG(0x1084)
#define EXYNOS5433_A5IS_SYS_PWR_REG				EXYNOS_PMUREG(0x10B0)
#define EXYNOS5433_DIS_IRQ_A5IS_LOCAL_SYS_PWR_REG		EXYNOS_PMUREG(0x10B4)
#define EXYNOS5433_DIS_IRQ_A5IS_CENTRAL_SYS_PWR_REG		EXYNOS_PMUREG(0x10B8)
#define EXYNOS5433_KFC_L2_SYS_PWR_REG				EXYNOS_PMUREG(0x10C4)
#define EXYNOS5433_CLKSTOP_CMU_TOP_SYS_PWR_REG			EXYNOS_PMUREG(0x1100)
#define EXYNOS5433_CLKRUN_CMU_TOP_SYS_PWR_REG			EXYNOS_PMUREG(0x1104)
#define EXYNOS5433_RESET_CMU_TOP_SYS_PWR_REG			EXYNOS_PMUREG(0x110C)
#define EXYNOS5433_RESET_CPUCLKSTOP_SYS_PWR_REG			EXYNOS_PMUREG(0x111C)
#define EXYNOS5433_CLKSTOP_CMU_MIF_SYS_PWR_REG			EXYNOS_PMUREG(0x1120)
#define EXYNOS5433_CLKRUN_CMU_MIF_SYS_PWR_REG			EXYNOS_PMUREG(0x1124)
#define EXYNOS5433_RESET_CMU_MIF_SYS_PWR_REG			EXYNOS_PMUREG(0x112C)
#define EXYNOS5433_DISABLE_PLL_CMU_TOP_SYS_PWR_REG		EXYNOS_PMUREG(0x1140)
#define EXYNOS5433_DISABLE_PLL_AUD_PLL_SYS_PWR_REG		EXYNOS_PMUREG(0x1144)
#define EXYNOS5433_DISABLE_PLL_MIF_SYS_PWR_REG			EXYNOS_PMUREG(0x1160)
#define EXYNOS5433_TOP_BUS_MIF_SYS_PWR_REG			EXYNOS_PMUREG(0x1190)
#define EXYNOS5433_TOP_RETENTION_MIF_SYS_PWR_REG		EXYNOS_PMUREG(0x1194)
#define EXYNOS5433_TOP_PWR_MIF_SYS_PWR_REG			EXYNOS_PMUREG(0x1198)
#define EXYNOS5433_SLEEP_RESET_SYS_PWR_REG			EXYNOS_PMUREG(0x11A8)
#define EXYNOS5433_LOGIC_RESET_MIF_SYS_PWR_REG			EXYNOS_PMUREG(0x11B0)
#define EXYNOS5433_OSCCLK_GATE_MIF_SYS_PWR_REG			EXYNOS_PMUREG(0x11B4)
#define EXYNOS5433_SLEEP_RESET_MIF_SYS_PWR_REG			EXYNOS_PMUREG(0x11B8)
#define EXYNOS5433_MEMORY_TOP_SYS_PWR_REG			EXYNOS_PMUREG(0x11C0)
#define EXYNOS5433_PAD_RETENTION_JTAG_SYS_PWR_REG		EXYNOS_PMUREG(0x1208)
#define EXYNOS5433_PAD_RETENTION_TOP_SYS_PWR_REG		EXYNOS_PMUREG(0x1220)
#define EXYNOS5433_PAD_RETENTION_UART_SYS_PWR_REG		EXYNOS_PMUREG(0x1224)
#define EXYNOS5433_PAD_RETENTION_EBIA_SYS_PWR_REG		EXYNOS_PMUREG(0x1230)
#define EXYNOS5433_PAD_RETENTION_EBIB_SYS_PWR_REG		EXYNOS_PMUREG(0x1234)
#define EXYNOS5433_PAD_RETENTION_SPI_SYS_PWR_REG		EXYNOS_PMUREG(0x1238)
#define EXYNOS5433_PAD_RETENTION_MIF_SYS_PWR_REG		EXYNOS_PMUREG(0x123C)
#define EXYNOS5433_PAD_RETENTION_USBXTI_SYS_PWR_REG		EXYNOS_PMUREG(0x1244)
#define EXYNOS5433_PAD_RETENTION_BOOTLDO_SYS_PWR_REG		EXYNOS_PMUREG(0x1248)
#define EXYNOS5433_PAD_ISOLATION_MIF_SYS_PWR_REG		EXYNOS_PMUREG(0x1250)
#define EXYNOS5433_PAD_RETENTION_FSYSGENIO_SYS_PWR_REG		EXYNOS_PMUREG(0x1254)
#define EXYNOS5433_XXTI26_SYS_PWR_REG				EXYNOS_PMUREG(0x1288)
#define EXYNOS5433_GPIO_MODE_FSYS_SYS_PWR_REG			EXYNOS_PMUREG(0x1304)
#define EXYNOS5433_GPIO_MODE_MIF_SYS_PWR_REG			EXYNOS_PMUREG(0x1320)
#define EXYNOS5433_GPIO_MODE_AUD_SYS_PWR_REG			EXYNOS_PMUREG(0x1340)
#define EXYNOS5433_GSCL_SYS_PWR_REG				EXYNOS_PMUREG(0x1400)
#define EXYNOS5433_CAM0_SYS_PWR_REG				EXYNOS_PMUREG(0x1404)
#define EXYNOS5433_MSCL_SYS_PWR_REG				EXYNOS_PMUREG(0x1408)
#define EXYNOS5433_G3D_SYS_PWR_REG				EXYNOS_PMUREG(0x140C)
#define EXYNOS5433_DISP_SYS_PWR_REG				EXYNOS_PMUREG(0x1410)
#define EXYNOS5433_CAM1_SYS_PWR_REG				EXYNOS_PMUREG(0x1414)
#define EXYNOS5433_AUD_SYS_PWR_REG				EXYNOS_PMUREG(0x1418)
#define EXYNOS5433_FSYS_SYS_PWR_REG				EXYNOS_PMUREG(0x141C)
#define EXYNOS5433_BUS2_SYS_PWR_REG				EXYNOS_PMUREG(0x1420)
#define EXYNOS5433_G2D_SYS_PWR_REG				EXYNOS_PMUREG(0x1424)
#define EXYNOS5433_ISP_SYS_PWR_REG				EXYNOS_PMUREG(0x1428)
#define EXYNOS5433_MFC_SYS_PWR_REG				EXYNOS_PMUREG(0x1430)
#define EXYNOS5433_HEVC_SYS_PWR_REG				EXYNOS_PMUREG(0x1438)
#define EXYNOS5433_DISABLE_PLL_CMU_GSCL_SYS_PWR_REG		EXYNOS_PMUREG(0x14C0)
#define EXYNOS5433_DISABLE_PLL_CMU_CAM0_SYS_PWR_REG		EXYNOS_PMUREG(0x14C4)
#define EXYNOS5433_DISABLE_PLL_CMU_MSCL_SYS_PWR_REG		EXYNOS_PMUREG(0x14C8)
#define EXYNOS5433_DISABLE_PLL_CMU_G3D_SYS_PWR_REG		EXYNOS_PMUREG(0x14CC)
#define EXYNOS5433_DISABLE_PLL_CMU_DISP_SYS_PWR_REG		EXYNOS_PMUREG(0x14D0)
#define EXYNOS5433_DISABLE_PLL_CMU_CAM1_SYS_PWR_REG		EXYNOS_PMUREG(0x14D4)
#define EXYNOS5433_DISABLE_PLL_CMU_AUD_SYS_PWR_REG		EXYNOS_PMUREG(0x14D8)
#define EXYNOS5433_DISABLE_PLL_CMU_FSYS_SYS_PWR_REG		EXYNOS_PMUREG(0x14DC)
#define EXYNOS5433_DISABLE_PLL_CMU_BUS2_SYS_PWR_REG		EXYNOS_PMUREG(0x14E0)
#define EXYNOS5433_DISABLE_PLL_CMU_G2D_SYS_PWR_REG		EXYNOS_PMUREG(0x14E4)
#define EXYNOS5433_DISABLE_PLL_CMU_ISP_SYS_PWR_REG		EXYNOS_PMUREG(0x14E8)
#define EXYNOS5433_DISABLE_PLL_CMU_MFC0_SYS_PWR_REG		EXYNOS_PMUREG(0x14F0)
#define EXYNOS5433_DISABLE_PLL_CMU_HEVC_SYS_PWR_REG		EXYNOS_PMUREG(0x14F8)
#define EXYNOS5433_RESET_CMU_FSYS_SYS_PWR_REG			EXYNOS_PMUREG(0x159C)
#define EXYNOS5433_RESET_CMU_BUS2_SYS_PWR_REG			EXYNOS_PMUREG(0x15A0)
#define EXYNOS5433_RESET_SLEEP_FSYS_SYS_PWR_REG			EXYNOS_PMUREG(0x15DC)
#define EXYNOS5433_RESET_SLEEP_BUS2_SYS_PWR_REG			EXYNOS_PMUREG(0x15E0)

#define EXYNOS5433_EAGLE_NONCPU_CONFIGURATION			EXYNOS_PMUREG(0x2400)
#define EXYNOS5433_EAGLE_NONCPU_STATUS				EXYNOS_PMUREG(0x2404)
#define EXYNOS5433_EAGLE_NONCPU_OPTION				EXYNOS_PMUREG(0x2408)
#define EXYNOS5433_KFC_NONCPU_CONFIGURATION			EXYNOS_PMUREG(0x2420)
#define EXYNOS5433_KFC_NONCPU_STATUS				EXYNOS_PMUREG(0x2424)
#define EXYNOS5433_KFC_NONCPU_OPTION				EXYNOS_PMUREG(0x2428)
#define EXYNOS5433_EAGLE_CPUSEQ_CONFIGURATION			EXYNOS_PMUREG(0x2480)
#define EXYNOS5433_EAGLE_CPUSEQ_STATUS				EXYNOS_PMUREG(0x2484)
#define EXYNOS5433_EAGLE_CPUSEQ_OPTION				EXYNOS_PMUREG(0x2488)
#define EXYNOS5433_KFC_CPUSEQ_CONFIGURATION			EXYNOS_PMUREG(0x24A0)
#define EXYNOS5433_KFC_CPUSEQ_STATUS				EXYNOS_PMUREG(0x24A4)
#define EXYNOS5433_KFC_CPUSEQ_OPTION				EXYNOS_PMUREG(0x24A8)
#define EXYNOS5433_A5IS_CONFIGURATION				EXYNOS_PMUREG(0x2580)
#define EXYNOS5433_A5IS_STATUS					EXYNOS_PMUREG(0x2584)
#define EXYNOS5433_A5IS_OPTION					EXYNOS_PMUREG(0x2588)
#define EXYNOS5433_EAGLE_L2_STATUS				EXYNOS_PMUREG(0x2604)
#define EXYNOS5433_KFC_L2_STATUS				EXYNOS_PMUREG(0x2624)
#define EXYNOS5433_OSC_DURATION					EXYNOS_PMUREG(0x2D3C)

#define EXYNOS5433_PAD_RETENTION_MMC2_OPTION			EXYNOS_PMUREG(0x30C8)
#define EXYNOS5433_PAD_RETENTION_TOP_OPTION			EXYNOS_PMUREG(0x3108)
#define EXYNOS5433_PAD_RETENTION_UART_OPTION			EXYNOS_PMUREG(0x3128)
#define EXYNOS5433_PAD_RETENTION_MMC0_OPTION			EXYNOS_PMUREG(0x3148)
#define EXYNOS5433_PAD_RETENTION_MMC1_OPTION			EXYNOS_PMUREG(0x3168)
#define EXYNOS5433_PAD_RETENTION_EBIA_OPTION			EXYNOS_PMUREG(0x3188)
#define EXYNOS5433_PAD_RETENTION_EBIB_OPTION			EXYNOS_PMUREG(0x31A8)
#define EXYNOS5433_PAD_RETENTION_SPI_OPTION			EXYNOS_PMUREG(0x31C8)
#define EXYNOS5433_PAD_RETENTION_MIF_OPTION			EXYNOS_PMUREG(0x31E8)
#define EXYNOS5433_PAD_RETENTION_USBXTI_OPTION			EXYNOS_PMUREG(0x3228)
#define EXYNOS5433_PAD_RETENTION_BOOTLDO_OPTION			EXYNOS_PMUREG(0x3248)
#define EXYNOS5433_PAD_RETENTION_UFS_OPTION			EXYNOS_PMUREG(0x3268)
#define EXYNOS5433_PAD_RETENTION_FSYSGENIO_OPTION		EXYNOS_PMUREG(0x32A8)

#define EXYNOS5433_PS_HOLD_CONTROL				EXYNOS_PMUREG(0x330C)

#define EXYNOS5433_GSCL_CONFIGURATION				EXYNOS_PMUREG(0x4000)
#define EXYNOS5433_GSCL_STATUS					EXYNOS_PMUREG(0x4004)
#define EXYNOS5433_GSCL_OPTION					EXYNOS_PMUREG(0x4008)
#define EXYNOS5433_CAM0_CONFIGURATION				EXYNOS_PMUREG(0x4020)
#define EXYNOS5433_CAM0_STATUS					EXYNOS_PMUREG(0x4024)
#define EXYNOS5433_CAM0_OPTION					EXYNOS_PMUREG(0x4028)
#define EXYNOS5433_MSCL_CONFIGURATION				EXYNOS_PMUREG(0x4040)
#define EXYNOS5433_MSCL_STATUS					EXYNOS_PMUREG(0x4044)
#define EXYNOS5433_MSCL_OPTION					EXYNOS_PMUREG(0x4048)
#define EXYNOS5433_G3D_CONFIGURATION				EXYNOS_PMUREG(0x4060)
#define EXYNOS5433_G3D_STATUS					EXYNOS_PMUREG(0x4064)
#define EXYNOS5433_G3D_OPTION					EXYNOS_PMUREG(0x4068)
#define EXYNOS5433_DISP_CONFIGURATION				EXYNOS_PMUREG(0x4080)
#define EXYNOS5433_DISP_STATUS					EXYNOS_PMUREG(0x4084)
#define EXYNOS5433_DISP_OPTION					EXYNOS_PMUREG(0x4088)
#define EXYNOS5433_CAM1_CONFIGURATION				EXYNOS_PMUREG(0x40A0)
#define EXYNOS5433_CAM1_STATUS					EXYNOS_PMUREG(0x40A4)
#define EXYNOS5433_CAM1_OPTION					EXYNOS_PMUREG(0x40A8)
#define EXYNOS5433_AUD_CONFIGURATION				EXYNOS_PMUREG(0x40C0)
#define EXYNOS5433_AUD_STATUS					EXYNOS_PMUREG(0x40C4)
#define EXYNOS5433_AUD_OPTION					EXYNOS_PMUREG(0x40C8)
#define EXYNOS5433_FSYS_CONFIGURATION				EXYNOS_PMUREG(0x40E0)
#define EXYNOS5433_FSYS_STATUS					EXYNOS_PMUREG(0x40E4)
#define EXYNOS5433_FSYS_OPTION					EXYNOS_PMUREG(0x40E8)
#define EXYNOS5433_BUS2_CONFIGURATION				EXYNOS_PMUREG(0x4100)
#define EXYNOS5433_BUS2_STATUS					EXYNOS_PMUREG(0x4104)
#define EXYNOS5433_BUS2_OPTION					EXYNOS_PMUREG(0x4108)
#define EXYNOS5433_G2D_CONFIGURATION				EXYNOS_PMUREG(0x4120)
#define EXYNOS5433_G2D_STATUS					EXYNOS_PMUREG(0x4124)
#define EXYNOS5433_G2D_OPTION					EXYNOS_PMUREG(0x4128)
#define EXYNOS5433_ISP_CONFIGURATION				EXYNOS_PMUREG(0x4140)
#define EXYNOS5433_ISP_STATUS					EXYNOS_PMUREG(0x4144)
#define EXYNOS5433_ISP_OPTION					EXYNOS_PMUREG(0x4148)
#define EXYNOS5433_MFC_CONFIGURATION				EXYNOS_PMUREG(0x4180)
#define EXYNOS5433_MFC_STATUS					EXYNOS_PMUREG(0x4184)
#define EXYNOS5433_MFC_OPTION					EXYNOS_PMUREG(0x4188)
#define EXYNOS5433_HEVC_CONFIGURATION				EXYNOS_PMUREG(0x41C0)
#define EXYNOS5433_HEVC_STATUS					EXYNOS_PMUREG(0x41C4)
#define EXYNOS5433_HEVC_OPTION					EXYNOS_PMUREG(0x41C8)

#define EXYNOS5433_PMU_LPI_CAM0_REG(x)				(EXYNOS5430_VA_PMU_LPI_CAM0 + (x))
#define EXYNOS5433_LPI_MASK_CAM0_BUSMASTER			EXYNOS5433_PMU_LPI_CAM0_REG(0x0000)
#define EXYNOS5433_LPI_MASK_CAM0_BUSMASTER_FIMC_LITE_A		(1 << 0)
#define EXYNOS5433_LPI_MASK_CAM0_BUSMASTER_FIMC_LITE_B		(1 << 1)
#define EXYNOS5433_LPI_MASK_CAM0_BUSMASTER_FIMC_LITE_D		(1 << 2)
#define EXYNOS5433_LPI_MASK_CAM0_BUSMASTER_FIMC_3AA_0		(1 << 3)
#define EXYNOS5433_LPI_MASK_CAM0_BUSMASTER_FIMC_3AA_1		(1 << 4)
#define EXYNOS5433_LPI_MASK_CAM0_BUSMASTER_ALL			(0x1F)
#define EXYNOS5433_LPI_MASK_CAM0_ASYNCBRIDGE			EXYNOS5433_PMU_LPI_CAM0_REG(0x0020)
#define EXYNOS5433_LPI_MASK_CAM0_ASYNCBRIDGE_CAM1_CAM0		(1 << 0)
#define EXYNOS5433_LPI_MASK_CAM0_ASYNCBRIDGE_ALL		(0x1)
#define EXYNOS5433_LPI_MASK_CAM0_NOCBUS				EXYNOS5433_PMU_LPI_CAM0_REG(0x0040)
#define EXYNOS5433_LPI_MASK_CAM0_NOCBUS_D			(1 << 0)
#define EXYNOS5433_LPI_MASK_CAM0_NOCBUS_P			(1 << 1)
#define EXYNOS5433_LPI_MASK_CAM0_NOCBUS_ALL			(0x3)

#define EXYNOS5433_PMU_LPI_CAM1_REG(x)				(EXYNOS5433_VA_PMU_LPI_CAM1 + (x))
#define EXYNOS5433_LPI_MASK_CAM1_BUSMASTER			EXYNOS5433_PMU_LPI_CAM1_REG(0x0000)
#define EXYNOS5433_LPI_MASK_CAM1_BUSMASTER_FIMC_LITE_C		(1 << 0)
#define EXYNOS5433_LPI_MASK_CAM1_BUSMASTER_FIMC_FD		(1 << 1)
#define EXYNOS5433_LPI_MASK_CAM1_BUSMASTER_ALL			(0x3)
#define EXYNOS5433_LPI_MASK_CAM1_ASYNCBRIDGE			EXYNOS5433_PMU_LPI_CAM1_REG(0x0020)
#define EXYNOS5433_LPI_MASK_CAM1_ASYNCBRIDGE_ISP_CAM1_ISPEX	(1 << 0)
#define EXYNOS5433_LPI_MASK_CAM1_ASYNCBRIDGE_CAM1_CAM0_IS0X	(1 << 1)
#define EXYNOS5433_LPI_MASK_CAM1_ASYNCBRIDGE_CAM1_ISP_AXI0X	(1 << 2)
#define EXYNOS5433_LPI_MASK_CAM1_ASYNCBRIDGE_CAM1_ISP_AXI1X	(1 << 3)
#define EXYNOS5433_LPI_MASK_CAM1_ASYNCBRIDGE_CAM1_ISP_APB1P	(1 << 4)
#define EXYNOS5433_LPI_MASK_CAM1_ASYNCBRIDGE_CAM1_ISP_APB2P	(1 << 4)
#define EXYNOS5433_LPI_MASK_CAM1_ASYNCBRIDGE_ALL		(0x3F)
#define EXYNOS5433_LPI_MASK_CAM1_NOCBUS				EXYNOS5433_PMU_LPI_CAM1_REG(0x0040)
#define EXYNOS5433_LPI_MASK_CAM1_NOCBUS_D			(1 << 0)
#define EXYNOS5433_LPI_MASK_CAM1_NOCBUS_P			(1 << 1)
#define EXYNOS5433_LPI_MASK_CAM1_NOCBUS_ALL			(0x3)

#define EXYNOS5433_PMU_LPI_ISP_REG(x)				(EXYNOS5433_VA_PMU_LPI_ISP + (x))
#define EXYNOS5433_LPI_MASK_ISP_BUSMASTER			EXYNOS5433_PMU_LPI_ISP_REG(0x0000)
#define EXYNOS5433_LPI_MASK_ISP_BUSMASTER_FIMC_ISP		(1 << 0)
#define EXYNOS5433_LPI_MASK_ISP_BUSMASTER_FIMC_DRC		(1 << 1)
#define EXYNOS5433_LPI_MASK_ISP_BUSMASTER_FIMC_SCC		(1 << 2)
#define EXYNOS5433_LPI_MASK_ISP_BUSMASTER_FIMC_DIS		(1 << 3)
#define EXYNOS5433_LPI_MASK_ISP_BUSMASTER_FIMC_TDNR		(1 << 4)
#define EXYNOS5433_LPI_MASK_ISP_BUSMASTER_FIMC_SCP		(1 << 5)
#define EXYNOS5433_LPI_MASK_ISP_BUSMASTER_ALL			(0x3F)
#define EXYNOS5433_LPI_MASK_ISP_ASYNCBRIDGE			EXYNOS5433_PMU_LPI_ISP_REG(0x0020)
#define EXYNOS5433_LPI_MASK_ISP_ASYNCBRIDGE_CAM1_ISP_1P		(1 << 0)
#define EXYNOS5433_LPI_MASK_ISP_ASYNCBRIDGE_CAM1_ISP_2P		(1 << 1)
#define EXYNOS5433_LPI_MASK_ISP_ASYNCBRIDGE_ISP_CAM1		(1 << 2)
#define EXYNOS5433_LPI_MASK_ISP_ASYNCBRIDGE_CAM1_ISP_ISPSSCON2	(1 << 3)
#define EXYNOS5433_LPI_MASK_ISP_ASYNCBRIDGE_CAM1_ISP_APB1P	(1 << 4)
#define EXYNOS5433_LPI_MASK_ISP_ASYNCBRIDGE_ALL			(0x1F)
#define EXYNOS5433_LPI_MASK_ISP_NOCBUS				EXYNOS5433_PMU_LPI_ISP_REG(0x0040)
#define EXYNOS5433_LPI_MASK_ISP_NOCBUS_D			(1 << 0)
#define EXYNOS5433_LPI_MASK_ISP_NOCBUS_ALL			(0x1)

#define EXYNOS5433_SYS_WDTRESET_EGL				(1 << 23)
#define EXYNOS5433_SYS_WDTRESET_KFC				(1 << 24)
#endif /* __ASM_ARCH_REGS_PMU_EXYNOS5433_H */
