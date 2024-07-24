void init_ne();
int32_t open_ne(uint16_t, int32_t, int32_t, int32_t);
int32_t close_ne(uint16_t, int32_t);
int32_t write_ne(int32_t, STRING, int32_t);
int32_t read_ne(int32_t, STRING, int32_t);
int32_t seek_ne(int32_t, int32_t);
int32_t ioctl_ne(int32_t, int32_t, int32_t);

typedef struct {
	Byte	 *base;
	int16_t	 id, fd;
	Byte	 mac[6], irq;
	uint16_t start, size;
	uint16_t rx_start, rx_end;
	void	 (*handle)(int32_t);
} NEInfo;

typedef union {
	Byte BYTE;
	struct {
		Bits DFR :1;
		Bits DIS :1;
		Bits PHY :1;
		Bits MPA :1;
		Bits FO  :1;
		Bits FAE :1;
		Bits CRC :1;
		Bits PRX :1;
	} BIT;
} RSR;

typedef union {
	Byte BYTE;
	struct {
		Bits RST :1;
		Bits RDC :1;
		Bits CNT :1;
		Bits OVW :1;
		Bits TXE :1;
		Bits RXE :1;
		Bits PTX :1;
		Bits PRX :1;
	} BIT;
} NE_ISR;

#define NE_P0_COMMAND	0
#define NE_P0_PSTART	1
#define NE_P0_PSTOP	2
#define NE_P0_BNRY	3	/* Boundary Reg */
#define NE_P0_TSR	4	/* Transmit Status Reg */
#define NE_P0_TPSR	4	/* Transmit Page Start Address Reg */
#define NE_P0_NCR	5	/* Nubmer of Collisions */
#define NE_P0_TBCR0	5	/* Transmit Byte Counter Reg */
#define NE_P0_TBCR1	6
#define NE_P0_ISR	7	/* Interrupt Status Reg */
#define NE_P0_RSAR0	8	/* Remote Start Address Reg */
#define NE_P0_RSAR1	9
#define NE_P0_RBCR0	10	/* Remote Byte Count Reg */
#define NE_P0_RBCR1	11
#define NE_P0_RSR	12	/* Receive Status Reg */
#define NE_P0_RCR	12	/* Receive Configuration Reg */
#define NE_P0_TCR	13	/* Transmit Configuration Reg */
#define NE_P0_DCR	14	/* Data Configuration Reg */
#define NE_P0_IMR	15	/* Interrupt Mask Reg */
#define NE_P0_CNTR0	13
#define NE_P0_CNTR1	14
#define NE_P0_CNTR2	15
#define NE_P1_COMMAND	0
#define NE_P1_PAR0	1		/* Physical Addres Reg */
#define NE_P1_PAR1	2
#define NE_P1_PAR2	3
#define NE_P1_PAR3	4
#define NE_P1_PAR4	5
#define NE_P1_PAR5	6
#define NE_P1_CURR	7		/* Current Page Reg */
#define NE_P1_MAR0	8		/* Multicast Address Reg */
#define NE_P1_MAR1	9
#define NE_P1_MAR2	10
#define NE_P1_MAR3	11
#define NE_P1_MAR4	12
#define NE_P1_MAR5	13
#define NE_P1_MAR6	14
#define NE_P1_MAR7	15
#define NE_P3_COMMAND	0
#define NE_P3_9346CR	1
#define NE_P3_BPAGE	2
#define NE_P3_CONFIG0	3
#define NE_P3_CONFIG1	4
#define NE_P3_CONFIG2	5
#define NE_P3_CONFIG3	6
#define NE_ASIC_DATA	16	/* data port */
#define NE_ASIC_RESET	24	/* reset port */

/* NE2000 定数定義 */
#define	NE_CR_PS0	0x00	/* Page 0 Select   */
#define	NE_CR_PS1	0x40	/* Page 1 Select   */
#define	NE_CR_PS2	0x80	/* Page 2 Select   */
#define	NE_CR_PS3	0xc0	/* Page 3 Select   */
#define	NE_CR_PSMASK	0xc0	/* Page Mask       */
#define	NE_CR_RD0	0x08	/********************** 000:DON'T, 001:read  */
#define	NE_CR_RD1	0x10	/* Remote DMA Control * 010:write, 011:trans */
#define NE_CR_RD2	0x20	/********************** 1xx:disable          */
#define NE_CR_TXP	0x04	/* Transmit packet */
#define NE_CR_STA	0x02	/* start	   */
#define	NE_CR_STP	0x01	/* stop		   */

#define	NE_RCR_SEP	0x01	/* accept error packet */
#define	NE_RCR_AR	0x02	/* accept tiny packet */
#define	NE_RCR_AB	0x04	/* accept broadcast */
#define	NE_RCR_MUL	0x08	/* accept multicast */
#define	NE_RCR_PRO	0x10	/* accept all */
#define	NE_RCR_MON	0x20	/* monitor mode */

#define	NE_DCR_FT0	0x20	/* fifo  */
#define	NE_DCR_FT1	0x40	/* fifo  */
#define	NE_DCR_AR	0x10	/* Auto remote */
#define	NE_DCR_WTS	0x01	/* Word Transfer select */
#define	NE_DCR_BOS	0x02	/* Byte Transfer select */
#define	NE_DCR_LAS	0x04	/* Long Transfer select */
#define	NE_DCR_LS	0x08	/* Loopback select */

#define	NE_ISR_PRX	0x01	/* successful receive */
#define	NE_ISR_PTX	0x02	/* successful trasmit */
#define	NE_ISR_RXE	0x04	/* receive error */
#define	NE_ISR_TXE	0x08	/* transmit error */
#define	NE_ISR_OVW	0x10	/* overflow */
#define	NE_ISR_CNT	0x20	/* counter overflow */
#define	NE_ISR_RDC	0x40	/* Remote DMA complete */
#define	NE_ISR_RST	0x80	/* reset */

#define	NE_IMR_PRXE	0x01	/* successful receive */
#define	NE_IMR_PTXE	0x02	/* successful trasmit */
#define	NE_IMR_RXEE	0x04	/* receive error */
#define	NE_IMR_TXEE	0x08	/* transmit error */
#define	NE_IMR_OVWE	0x10	/* overflow */
#define	NE_IMR_CNTE	0x20	/* counter overflow */
#define	NE_IMR_RDCE	0x40	/* Remote DMA complete */

#define	NE_RSTAT_PRX	0x01	/* successful receive */
#define	NE_RSTAT_CRC	0x02	/* CRC error */
#define	NE_RSTAT_FAE	0x04	/* Frame alignment error */
#define	NE_RSTAT_OVER	0x08	/* overflow */

#define	NE_TSR_PTX	0x01	/* Transmit complete */
#define	NE_TSR_ABT	0x08	/* Transmit Aborted */

#define	NE_TCR_LB0	0x02	/* Internal loopback */
#define	NE_TCR_LB1	0x04	

/* NE2000 領域定義 */
#define	NE_PAGE_SIZE	 0x100	/* 1ページのバイト数 */
#define	NE_TX_PAGE_SIZE	 0x600	/* 送信バッファページ数 */

/* イーサネット定数定義 */
#define	ETHER_HEADER_SIZE	14	/* ヘッダーサイズ */
#define	ETHER_MIN_PACKET	64	/* 最小パケットサイズ */
#define	ETHER_MAX_PACKET	1514	/* 最大パケットサイズ */
