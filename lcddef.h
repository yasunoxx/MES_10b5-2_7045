#define	D0	lcd[num].masks[0]
#define	D1	lcd[num].masks[1]
#define	D2	lcd[num].masks[2]
#define	D3	lcd[num].masks[3]
#define	D4	lcd[num].masks[0]
#define	D5	lcd[num].masks[1]
#define	D6	lcd[num].masks[2]
#define	D7	lcd[num].masks[3]
#define	RS	lcd[num].masks[4]
#define	CS	lcd[num].masks[5]
#define DATA	lcd[num].masks164[0]
#define CLK	lcd[num].masks164[1]
#define ENA	lcd[num].masks164[2]
#define	NONE	0

#define LCDINC		D1
#define LCDSHIFT	D0
#define LCDDISPLAY	D2
#define LCDCURSOR	D1
#define LCDBLINK	D0
#define LCDDISPSHIFT	D3
#define LCDRIGHT	D2
#define LCDLEFT		NONE
#define SerialMode	lcd[num].masks164[0]

#ifdef	SH2
  #define IOREG	volatile uint16_t
  #define WAITSCALE	4
#else
  #define IOREG	volatile uint8_t
  #define WAITSCALE	1
#endif
