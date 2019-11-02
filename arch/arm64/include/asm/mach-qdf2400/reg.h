#ifndef __REG_QDF2400_H_INCLUDE__
#define __REG_QDF2400_H_INCLUDE__

#define ARM_ROM_OFFSET		0x850

/* Firmware only programs the system fabric region */
#define SYS_FABRIC_START	0xFF00000000
#define SYS_FABRIC_SIZE		0x0080000000
#define SYS_FABRIC_END		(SYS_FABRIC_START + SYS_FABRIC_SIZE)

/* System fabric sub-regions */

/* HS5 includes QGIC GICs & QGIC GITS */
#define QGIC_GICS_START		0xFF7F000000
#define QGIC_GICS_SIZE		0x0001000000
#define QGIC_GICS_END		(QGIC_GICS_START + QGIC_GICS_SIZE)

#define QGIC_GITS_START		0xFF7EF00000
#define QGIC_GITS_SIZE		0x0000100000
#define QGIC_GITS_END		(QGIC_GITS_START + QGIC_GITS_SIZE)

#define HMSS_START		0xFF7E000000
#define HMSS_SIZE		0x0000F00000
#define HMSS_END		(HMSS_START + HMSS_SIZE)

#define MSG_IMEM_START		0xFF70400000
#define MSG_IMEM_SIZE		0x0000010000
#define MSG_IMEM_END		(MSG_IMEM_START + MSG_IMEM_SIZE)

#define SYS_IMEM_START		0xFF70000000
#define SYS_IMEM_SIZE		0x0000040000
#define SYS_IMEM_END		(SYS_IMEM_START + SYS_IMEM_SIZE)

#define IMC_FABRIC_START	0xFF6F800000
#define IMC_FABRIC_SIZE		0x0000800000
#define IMC_FABRIC_END		(IMC_FABRIC_START + IMC_FABRIC_SIZE)

#define QDSS_STM_START		0xFF63000000
#define QDSS_STM_SIZE		0x0001000000
#define QDSS_STM_END		(QDSS_STM_START + QDSS_STM_SIZE)

/* Skip system fabric sub-regions:
 * GMAC-1 (HS2):    0xFF61800000 - 0xFF61D00000
 * GMAC-0 (HS1):    0xFF61000000 - 0xFF61400000
 * Crypto5-1 (HS4): 0xFF60800000 - 0xFF60D00000
 * Crypto5-0 (HS3): 0xFF60000000 - 0xFF60500000
 */

#define CFG_FABRIC_START	0xFF00000000
#define CFG_FABRIC_SIZE		0x0060000000

/* IMC fabric sub-regions */
#define IMC_ROM_START		0xFF6FF00000
#define IMC_ROM_SIZE		0x0000100000
#define IMC_ROM_END		(IMC_ROM_START + IMC_ROM_SIZE)

/* Skip IMC fabric sub-regions:
 * RAM5 Cfg: 0xFF6FE40000 - 0xFF6FE80000
 * RAM4 Cfg: 0xFF6FE00000 - 0XFF6FE40000
 * RAM3 Cfg: 0xFF6FDC0000 - 0xFF6FE00000
 * RAM2 Cfg: 0xFF6FD80000 - 0xFF6FDC0000
 * RAM1 Cfg: 0xFF6FD40000 - 0xFF6FD80000
 * RAM0 Cfg: 0xFF6FD00000 - 0xFF6FD40000
 */

#define CLK_CTRL_START		0xFF6FC40000
#define CLK_CTRL_SIZE		0x0000040000
#define CLK_CTRL_END		(CLK_CTRL_START + CLK_CTRL_SIZE)

#define IMC_CFG_START		0xFF6FC00000
#define IMC_CFG_SIZE		0x0000040000
#define IMC_CFG_END		(IMC_CFG_START + IMC_CFG_SIZE)

#define CSR_GPIO_START		0xFF6FBC0000
#define CSR_GPIO_SIZE		0x0000040000
#define CSR_GPIO_END		(CSR_GPIO_START + CSR_GPIO_SIZE)

#define QTIMER_RF_START		0xFF6FB80000
#define QTIMER_RF_SIZE		0x0000040000
#define QTIMER_RF_END		(QTIMER_RF_START + QTIMER_RF_SIZE)

#define QTIMER_CF_START		0xFF6FB40000
#define QTIMER_CF_SIZE		0x0000040000
#define QTIMER_CF_END		(QTIMER_CF_START + QTIMER_CF_SIZE)

#define QTIMER_GPV1_FRAME_START	0xFF6FB00000
#define QTIMER_GPV1_FRAME_SIZE	0x0000040000
#define QTIMER_GPV1_FRAME_END	(QTIMER_GPV1_FRAME_START + QTIMER_GPV1_FRAME_SIZE)

#define QTIMER_ACC_CTL_START	0xFF6FAC0000
#define QTIMER_ACC_CTL_SIZE	0x0000040000
#define QTIMER_ACC_CTL_END	(QTIMER_ACC_CTL_START + QTIMER_ACC_CTL_SIZE)

#define QGIC_MSI_NS_START	0xFF6FA80000
#define QGIC_MSI_NS_SIZE	0x0000040000
#define QGIC_MSI_NS_END		(QGIC_MSI_NS_START + QGIC_MSI_NS_SIZE)

#define QGIC_START		0xFF6FA40000
#define QGIC_SIZE		0x0000040000
#define QGIC_END		(QGIC_START + QGIC_SIZE)

/* Skip IMC fabric sub-regions:
 * AHB S0 APU Cfg: 0xFF6F000000 - 0xFF6FA40000
 */

#define IMC_RAM_START		0xFF6F800000
#define IMC_RAM_SIZE		0x0000180000
#define IMC_RAM_END		(IMC_RAM_START + IMC_RAM_SIZE)

/* TCM default region */
#define TCM_START		0xFFF8000000
#define TCM_SIZE		0x1E00000 /* about 32MB available */
#define TCM_END			(TCM_START + TCM_SIZE)

/* TESTOS image related definitions */
#define ROM_BASE		IMC_ROM_START
#define ROMEND			IMC_ROM_END

#define RAM_BASE		IMC_RAM_START
#define RAMEND			IMC_RAM_END

#endif /* __REG_QDF2400_H_INCLUDE__ */
