#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/jz_dwc.h>
#include <soc/base.h>
#include <soc/extal.h>
#include <soc/cpm.h>

#define USBRDT_VBFIL_LD_EN		25
#define USBPCR_TXPREEMPHTUNE		6
#define USBPCR_POR			22
#define USBPCR_USB_MODE			31
#define USBPCR_COMMONONN		25
#define USBPCR_VBUSVLDEXT		24
#define USBPCR_VBUSVLDEXTSEL		23
#define USBPCR_OTG_DISABLE		20
#define USBPCR_SIDDQ			21
#define USBPCR_IDPULLUP_MASK		28
#define OPCR_SPENDN0			7
#define USBPCR1_USB_SEL			28
#define USBPCR1_WORD_IF0		19
#define USBPCR1_WORD_IF1		18
#define SRBC_USB_SR			12

int cpm_start_ehci(void)
{
	static int has_reset = 0;
	int tmp;

	cpm_clear_bit(20, CPM_USBPCR);

	/* The PLL uses CLKCORE as reference */
	tmp = cpm_inl(CPM_USBPCR1);
	tmp |= (0x3<<26);
	cpm_outl(tmp,CPM_USBPCR1);

	/* selects the reference clock frequency 48M */
	tmp = cpm_inl(CPM_USBPCR1);
	tmp &= ~(0x3<<24);
	switch(JZ_EXTAL) {
		case 24000000:
			tmp |= (1<<24);break;
		case 48000000:
			tmp |= (2<<24);break;
		case 19200000:
			tmp |= (3<<24);break;
		case 12000000:
		default:
			tmp |= (0<<24);break;
	}
	cpm_outl(tmp,CPM_USBPCR1);

	/* port1(uhc) hasn't forced to entered SUSPEND mode */
	cpm_set_bit(6, CPM_OPCR);

	/* The pull-down resistance on D-/D+ of port1 */
	tmp = cpm_inl(CPM_USBPCR1);
	tmp |= (0x3<<22);
	cpm_outl(tmp,CPM_USBPCR1);

	/* select utmi data bus width of port1 to 16bit/30M */
	cpm_set_bit(18, CPM_USBPCR1);

	/* select utmi data bus width of controller to 16bit */
	*((volatile int *) 0xb34900b0) |= (1 << 6);

	/* phy reset */
	cpm_set_bit(22, CPM_USBPCR);
	udelay(30);
	cpm_clear_bit(22, CPM_USBPCR);
	udelay(300);

	if(!has_reset) {
		/* UHC soft reset */
		cpm_set_bit(14, CPM_SRBC);
		udelay(300);
		cpm_clear_bit(14, CPM_SRBC);
		udelay(300);
		has_reset = 1;
	}

	return 0;
}
EXPORT_SYMBOL(cpm_start_ehci);

int cpm_stop_ehci(void)
{
	cpm_clear_bit(6, CPM_OPCR);
	return 0;
}
EXPORT_SYMBOL(cpm_stop_ehci);

int cpm_start_ohci(void)
{
	return cpm_start_ehci();
}
EXPORT_SYMBOL(cpm_start_ohci);

int cpm_stop_ohci(void)
{
	return cpm_stop_ehci();
}
EXPORT_SYMBOL(cpm_stop_ohci);


void jz_otg_ctr_reset(void)
{
	cpm_set_bit(SRBC_USB_SR, CPM_SRBC);
	mdelay(100);
	cpm_clear_bit(SRBC_USB_SR, CPM_SRBC);
}

void jz_otg_phy_reset(void)
{
	cpm_set_bit(USBPCR_POR, CPM_USBPCR);
	mdelay(1);
	cpm_clear_bit(USBPCR_POR, CPM_USBPCR);
	mdelay(1);
}

void jz_otg_phy_init(otg_mode_t mode)
{
	unsigned int ref_clk_div = CONFIG_EXTAL_CLOCK / 24;
	unsigned int usbpcr1;

	/* select dwc otg */
	cpm_set_bit(USBPCR1_USB_SEL, CPM_USBPCR1);

	/* select utmi data bus width of port0 to 16bit/30M */
	cpm_set_bit(USBPCR1_WORD_IF0, CPM_USBPCR1);

	usbpcr1 = cpm_inl(CPM_USBPCR1);
	usbpcr1 &= ~(0x3 << 24 | 1 << 30);
	usbpcr1 |= (ref_clk_div << 24);
	cpm_outl(usbpcr1, CPM_USBPCR1);

	/* fil */
	cpm_outl(0, CPM_USBVBFIL);

	/* rdt */
	cpm_outl(0x96, CPM_USBRDT);

	/* rdt - filload_en */
	cpm_set_bit(USBRDT_VBFIL_LD_EN, CPM_USBRDT);

	/* TXRISETUNE & TXVREFTUNE. */
	//cpm_outl(0x3f, CPM_USBPCR);
	//cpm_outl(0x35, CPM_USBPCR);

	/* enable tx pre-emphasis */
	//cpm_set_bit(USBPCR_TXPREEMPHTUNE, CPM_USBPCR);

	/* OTGTUNE adjust */
	//cpm_outl(7 << 14, CPM_USBPCR);

	cpm_outl(0x8380385F, CPM_USBPCR);

	if (mode == DEVICE_ONLY) {
		pr_info("DWC IN DEVICE ONLY MODE\n");
		cpm_clear_bit(USBPCR_USB_MODE, CPM_USBPCR);
		cpm_clear_bit(USBPCR_OTG_DISABLE, CPM_USBPCR);
		cpm_clear_bit(USBPCR_SIDDQ, CPM_USBPCR);
	} else {
		unsigned int tmp;
		pr_info("DWC IN OTG MODE\n");
		tmp = cpm_inl(CPM_USBPCR);
		tmp |= 1 << USBPCR_USB_MODE | 1 << USBPCR_COMMONONN;
		tmp &= ~(1 << USBPCR_OTG_DISABLE | 1 << USBPCR_SIDDQ |
				0x03 << USBPCR_IDPULLUP_MASK | 1 << USBPCR_VBUSVLDEXT |
				1 << USBPCR_VBUSVLDEXTSEL);
		cpm_outl(tmp, CPM_USBPCR);
	}

	cpm_set_bit(USBPCR_POR, CPM_USBPCR);
	mdelay(1);
	cpm_clear_bit(USBPCR_POR, CPM_USBPCR);
	mdelay(1);
}
EXPORT_SYMBOL(jz_otg_phy_init);

int jz_otg_phy_is_suspend(void)
{
	return (!(cpm_test_bit(7, CPM_OPCR)));
}
EXPORT_SYMBOL(jz_otg_phy_is_suspend);

void jz_otg_phy_suspend(int suspend)
{
	if (!suspend && jz_otg_phy_is_suspend()) {
		printk("EN PHY\n");
		cpm_set_bit(7, CPM_OPCR);
	} else if (suspend && !jz_otg_phy_is_suspend()) {
		printk("DIS PHY\n");
		cpm_clear_bit(7, CPM_OPCR);
	}
}
EXPORT_SYMBOL(jz_otg_phy_suspend);

void jz_otg_phy_powerdown(void)
{
	cpm_set_bit(USBPCR_OTG_DISABLE,CPM_USBPCR);
	cpm_set_bit(USBPCR_SIDDQ ,CPM_USBPCR);
}
EXPORT_SYMBOL(jz_otg_phy_powerdown);
