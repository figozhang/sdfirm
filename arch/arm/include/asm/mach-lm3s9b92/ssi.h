#ifndef __SSI_LM3S9B92_H_INCLUDE__
#define __SSI_LM3S9B92_H_INCLUDE__

#include <target/config.h>
#include <target/generic.h>
#include <asm/reg.h>
#include <asm/mach/clk.h>
#include <asm/mach/pm.h>
#include <asm/mach/gpio.h>

#ifdef CONFIG_SSI_LM3S9B92
#ifndef ARCH_HAVE_SPI
#define ARCH_HAVE_SPI		1
#else
#error "Multiple SPI controller defined"
#endif
#endif

extern uint16_t ssi_nr;
extern uint8_t ssi_mode;
extern uint32_t ssi_khz;

#define SSI_BASE		0x40008000
#define SSI(offset)		(SSI_BASE + offset)			

#define SPI_HW_FREQ		2000

/* SSI Control 0 */
#define SSICR0(n)		SSI(0x##n##000)
#define __SSI_SCR_OFFSET	8
#define __SSI_SCR_MASK		0xff
#define SPH			7
#define SPO			6
#define __SSI_MODE_OFFSET	6
#define __SSI_MODE_MASK		0x3
#define __SSI_FRF_OFFSET	4
#define __SSI_FRF_MASK		0x3
#define __SSI_FRF_FREESCALE	0x0
#define __SSI_FRF_TI		0x1
#define __SSI_FRF_MICROWIRE	0x2
#define __SSI_DSS_OFFSET	0
#define __SSI_DSS_MASK		0xf
#define __SSI_DSS_8BIT		0x7

/* SSI Control 1 */
#define SSICR1(n)		SSI(0x##n##004)
#define EOT			4
#define SOD			3
#define MS			2
#define	SSE			1
#define LBM			0

/* SSI Data */
#define SSIDR(n)		SSI(0x##n##008)

/* SSI Status */
#define SSISR(n)		SSI(0x##n##00C)
#define BSY			4
#define RFF			3
#define RNE			2
#define TNF			1
#define TFE			0

/* SSI Clock Prescale */
#define SSICPSR(n)		SSI(0x##n##010)

/* SSI Interrupt Mask */
#define SSIIM(n)		SSI(0x##n##014)
#define TXIM			3
#define RXIM			2
#define	RTIM			1
#define RORIM			0

/* SSI Raw Interrupt Status */
#define SSIRIS(n)		SSI(0x##n##018)
#define TXRIS			3
#define RXRIS			2
#define RTRIS			1
#define RORRIS			0

/* SSI Masked Interrupt Status */
#define SSIMIS(n)		SSI(0x##n##01C)
#define TXMIS			3
#define RXMIS			2
#define RTMIS			1
#define RORMIS			0

/* SSI Interrupt Clear */
#define SSIICR(n)		SSI(0x##n##020)
#define RTIC			1
#define RORIC			0

/* SSI DMA Control */
#define SSIDMACTL(n)		SSI(0x##n##024)
#define TXDMAE			1
#define RXDMAE			0

/* SSI Peripheral Identification 0~7 */
#define SSIPeriphID4(n)		SSI(0x##n##FD0)
#define SSIPeriphID5(n)		SSI(0x##n##FD4)
#define SSIPeriphID6(n)		SSI(0x##n##FD8)
#define SSIPeriphID7(n)		SSI(0x##n##FDC)
#define SSIPeriphID0(n)		SSI(0x##n##FE0)
#define SSIPeriphID1(n)		SSI(0x##n##FE4)
#define SSIPeriphID2(n)		SSI(0x##n##FE8)
#define SSIPeriphID3(n)		SSI(0x##n##FEC)

/* SSI PrimeCell Identification 0~3 */
#define SSIPCellID0(n)		SSI(0x##n##FF0)
#define SSIPCellID1(n)		SSI(0x##n##FF4)
#define SSIPCellID2(n)		SSI(0x##n##FF8)
#define SSIPCellID3(n)		SSI(0x##n##FFC)

#define LM3S9B92_SSI(n)							\
static inline void __ssi##n##_hw_config_clk_phase(uint8_t phase)	\
{									\
	__raw_writel_mask(phase<<(__SSI_SCR_OFFSET),			\
				  __SSI_SCR_MASK<<(__SSI_SCR_OFFSET),	\
				  			  SSICR0(n));	\
}									\
static inline void __ssi##n##_hw_config_frame_type(uint8_t type)	\
{									\
	__raw_writel_mask(type<<(__SSI_FRF_OFFSET),			\
				  __SSI_FRF_MASK<<(__SSI_FRF_OFFSET),	\
				  			  SSICR0(n));	\
}									\
static inline void __ssi##n##_hw_config_size_8bit(void)			\
{									\
	__raw_writel_mask(__SSI_DSS_8BIT<<(__SSI_DSS_OFFSET),		\
				  __SSI_DSS_MASK<<(__SSI_DSS_OFFSET),	\
				  			  SSICR0(n));	\
}									\
static inline void __ssi##n##_hw_enable(void)				\
{									\
	__raw_setl_atomic(SSE, SSICR1(n));				\
}									\
static inline void __ssi##n##_hw_master_mode(void)			\
{									\
	__raw_setl_atomic(MS, SSICR1(n));				\
}									\
static inline void __ssi##n##_hw_loopback_mode(void)			\
{									\
	__raw_setl_atomic(LBM, SSICR1(n));				\
}									\
static inline void __ssi##n##_hw_write_byte(uint8_t byte)		\
{									\
	__raw_writel(byte, SSIDR(n));					\
}									\
static inline uint8_t __ssi##n##_hw_read_byte(void)			\
{									\
	return (uint8_t)__raw_readl(SSIDR(n));				\
}									\
static inline void __ssi##n##_hw_config_clk_prescale(uint8_t prescale)	\
{									\
	__raw_writel(prescale, SSICPSR(n));				\
}									\
static inline  void __ssi##n##_hw_irq_enable_all(void)			\
{									\
	__raw_setl_atomic((TXIM | RXIM |				\
				  RTIM | RORIM), SSIIM(n));		\
}									\
static inline void __ssi##n##_hw_irq_disable_all(void)			\
{									\
	__raw_clearl_atomic((TXIM | RXIM |				\
				    RTIM | RORIM), SSIIM(n));		\
}									\
static inline void __ssi##n##_hw_irq_enable(uint8_t irq)		\
{									\
	__raw_setl_atomic(irq, SSIIM(n));				\
}									\
static inline void __ssi##n##_hw_irq_disable(uint8_t irq)		\
{									\
	__raw_clearl_atomic(irq, SSIIM(n));				\
}									\
static inline boolean __ssi##n##_hw_irq_raw(uint8_t irq)		\
{									\
	return __raw_testl_atomic(irq, SSIRIS(n));			\
}									\
static inline boolean __ssi##n##_hw_irq_masked(uint8_t irq)		\
{									\
	return __raw_testl_atomic(irq, SSIMIS(n));			\
}									\
static inline void __ssi##n##_hw_irq_clear(uint8_t irq)			\
{									\
	__raw_setl_atomic(irq, SSIICR(n));				\
}									\
static inline void __ssi##n##_hw_ctrl_disable(void)			\
{									\
	while (__raw_testl_atomic(SSE, SSICR1(n)))			\
		__raw_clearl_atomic(SSE, SSICR1(n));			\
}

/* SPI Freescale modes */
#if 0
#define SPI_SPH_LEADING		(0)
#define SPI_SPH_TRAILING	(_BV(SPH))
#define SPI_SPO_LEADING		(0)
#define SPI_SPO_TRAILING	(_BV(SPO))
#define SPI_MODE_0_SPCR		(SPI_SPO_LEADING | SPI_SPH_LEADING)
#define SPI_MODE_1_SPCR		(SPI_SPO_LEADING |SPI_SPH_TRAILING)
#define SPI_MODE_2_SPCR		(SPI_SPO_TRAILING | SPI_SPH_LEADING)
#define SPI_MODE_3_SPCR		(SPI_SPO_TRAILING | SPI_SPH_TRAILING)

#define spi_hw_write_byte(byte)
#define spi_hw_read_byte()			(0)
#endif

void spi_hw_write_byte(uint8_t byte);
uint8_t spi_hw_read_byte(void);
void spi_hw_config_mode(uint8_t mode);
void spi_hw_config_freq(uint32_t khz);
void spi_hw_chip_select(uint8_t chip);
void spi_hw_deselect_chips(void);

void spi_hw_ctrl_start(void);
void spi_hw_ctrl_stop(void);
void spi_hw_ctrl_init(void);

void ssi_hw_config_mode(uint8_t mode);
void ssi_hw_config_freq(uint32_t khz);
void ssi_hw_ctrl_start(void);

#endif /* __SSI_LM3S9B92_H_INCLUDE__ */

