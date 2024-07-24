/****************************************/
/* CoreOS/Copyleft Yukio Mituiwa,2003	*/
/*					*/
/*  2003/12/12 first release		*/
/*					*/
/****************************************/
#define DATA0_WR    0x07   // (Arm+Enable+tranmist to Host+DATA0)
#define DATA1_WR    0x47   // (Arm+Enable+tranmist to Host on DATA1)
#define ZDATA0_WR   0x05   // (Arm+Transaction Ignored+tranmist to Host+DATA0)
#define ZDATA1_WR   0x45   // (Arm+Transaction Ignored+tranmist to Host+DATA1)
#define DATA0_RD    0x03   // (Arm+Enable+received from Host+DATA0)
#define DATA1_RD    0x43   // (Arm+Enable+received from Host+DATA1)

#define PID_SETUP   0xd0
#define PID_SOF     0x50
#define PID_IN      0x90
#define PID_OUT     0x10

#define SL11H_HOSTCTLREG        0
#define SL11H_BUFADDRREG        1
#define SL11H_BUFLNTHREG        2
#define SL11H_PKTSTATREG        3	/* read */
#define SL11H_PIDEPREG          3	/* write */
#define SL11H_XFERCNTREG        4	/* read */
#define SL11H_DEVADDRREG        4	/* write */
#define SL11H_CTLREG1           5
#define SL11H_INTENBLREG        6

#define SL11H_HOSTCTLREG_B      8
#define SL11H_BUFADDRREG_B      9
#define SL11H_BUFLNTHREG_B      10
#define SL11H_PKTSTATREG_B      11	/* read */
#define SL11H_PIDEPREG_B        11	/* write */
#define SL11H_XFERCNTREG_B      12	/* read */
#define SL11H_DEVADDRREG_B      12	/* write */

#define SL11H_INTSTATREG        13	/* write clears bitwise */
#define SL11H_HWREVREG          14	/* read */
#define SL11H_SOFLOWREG         14	/* write */
#define SL11H_SOFTMRREG         15	/* read */
#define SL11H_CTLREG2           15	/* write */
#define SL11H_DATA_START        16

/* Host control register bits (addr 0) */

#define SL11H_HCTLMASK_ARM      1
#define SL11H_HCTLMASK_ENBLEP   2
#define SL11H_HCTLMASK_WRITE    4
#define SL11H_HCTLMASK_ISOCH    0x10
#define SL11H_HCTLMASK_AFTERSOF 0x20
#define SL11H_HCTLMASK_SEQ      0x40
#define SL11H_HCTLMASK_PREAMBLE 0x80

/* Packet status register bits (addr 3) */

#define SL11H_STATMASK_ACK      1
#define SL11H_STATMASK_ERROR    2
#define SL11H_STATMASK_TMOUT    4
#define SL11H_STATMASK_SEQ      8
#define SL11H_STATMASK_SETUP    0x10
#define SL11H_STATMASK_OVF      0x20
#define SL11H_STATMASK_NAK      0x40
#define SL11H_STATMASK_STALL    0x80

/* Control register 1 bits (addr 5) */
#define SL11H_CTL1MASK_DSBLSOF  1
#define SL11H_CTL1MASK_NOTXEOF2 4
#define SL11H_CTL1MASK_DSTATE   0x18
#define SL11H_CTL1MASK_NSPD     0x20
#define SL11H_CTL1MASK_SUSPEND  0x40
#define SL11H_CTL1MASK_CLK12    0x80

#define SL11H_CTL1VAL_RESET     8

/* Interrut enable (addr 6) and interrupt status register bits (addr 0xD) */

#define SL11H_INTMASK_XFERDONE  1
#define SL11H_INTMASK_SOFINTR   0x10
#define SL11H_INTMASK_INSRMV    0x20
#define SL11H_INTMASK_USBRESET  0x40
#define SL11H_INTMASK_DSTATE    0x80	/* only in status reg */

/* HW rev and SOF lo register bits (addr 0xE) */

#define SL11H_HWRMASK_HWREV     0xF0

/* SOF counter and control reg 2 (addr 0xF) */

#define SL11H_CTL2MASK_SOFHI    0x3F
#define SL11H_CTL2MASK_DSWAP    0x40
#define SL11H_CTL2MASK_HOSTMODE 0xae
