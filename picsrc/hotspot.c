/*****************************************************************************\G
 *  Copyright (C) 2011 Jim Garlick
 *  
 *  This file is part of ultima8drivecorrector, replacement base electronics
 *  for the Celestron Ultima 8 telescope.  For details, see
 *  <http://code.google.com/p/ultima8drivecorrector>.
 *  
 *  ultima8drivecorrector is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as published
 *  by the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *  
 *  ultima8drivecorrector is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 *  Public License *  for more details.
 *  
 *  You should have received a copy of the GNU General Public License along
 *  with ultima8drivecorrector; if not, write to the Free Software Foundation,
 *  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
\*****************************************************************************/

/* hotspot.c - firmware for Ultima 8 WiFi hotspot board */

/* NOTE: compiled with hi-tech C pro V9.80 */

#include <htc.h>
#include <ctype.h>
#include <string.h>

#define _XTAL_FREQ 64000000UL

#if defined(_18F14K22)
__CONFIG (1, FOSC_IRC & PLLEN_ON);  /* PLLEN_ON: system clock is HFOSC*4 */
__CONFIG (2, BOREN_OFF & WDTEN_OFF);
__CONFIG (3, HFOFST_OFF & MCLRE_OFF);
__CONFIG (4, LVP_OFF);
#else
#error code assumes 18F14K22 chip.
#endif

#define LCD_DATA4       PortCbits.RC0
#define LCD_DATA5       PortCbits.RC1
#define LCD_DATA6       PortCbits.RC2 
#define LCD_DATA7       PortCbits.RC3 

#define LCD_EN          PORTAbits.RA0
#define LCD_RS          PORTAbits.RA1
#define LCD_RW          PORTAbits.RA2

#define LCD_DATA        PORTC

#define LCD_MAXCOL      16
#define LCD_MAXROW      2

#define islcd(c)        ((c) >= 0x20 && (c) <= 0x7d)

#define SERIAL_BAUD     115200

typedef struct {
    volatile unsigned char   buf[256]; /* MAX_CHAR so no wrap logic needed */
    volatile unsigned char   head;
    volatile unsigned char   tail;
} cbuf_t;

static cbuf_t serial_in = { "", 0, 0 };

/* overwrites tail */
void
cbuf_putc (cbuf_t *cbuf, unsigned char c)
{
    if (cbuf->head + 1 == cbuf->tail)
        cbuf->tail++;
    cbuf->buf[cbuf->head++] = c;
}

/* spins until data available */
unsigned char
cbuf_getc (cbuf_t *cbuf)
{
    while (cbuf->head == cbuf->tail)
        ;
    return cbuf->buf[cbuf->tail++];
}

/* Spins until line is read.
 * Consume up to \n, but return at most length, always NULL terminated.
 * Do not return \n or \r characters.
 */
int
cbuf_gets (cbuf_t *cbuf, unsigned char *buf, int len)
{
    unsigned char c;
    int i = 0;

    do {
        c = cbuf_getc (cbuf);
        if (i < len - 1 && c != '\r' && c != '\n')
            buf[i++] = c;
    } while (c != '\n');
    buf[i] = '\0';
}

void
lcd_strobe (void)
{
    LCD_EN = 1;
    __delay_us (1);
    LCD_EN = 0;
}

unsigned char
lcd_read_nybble (void)
{
    unsigned char c;

    __delay_us (1);
    LCD_EN = 1;
    __delay_us (1);
    c = (LCD_DATA & 0x0f);
    LCD_EN = 0;

    return c;
}

unsigned char 
lcd_read (void)
{
    unsigned char c;

    c = lcd_read_nybble () << 4;
    c |= lcd_read_nybble ();

    return c;
}

void
lcd_waitbusy (void)
{
    TRISC = 0x0f;
    LCD_RW = 1;
    LCD_RS = 0;
    while (((lcd_read () & 0x80)))
        ;
    LCD_RW = 0;
    TRISC = 0;
}

void
lcd_write_nybble (unsigned char c)
{
    LCD_DATA = c & 0x0f;
    lcd_strobe ();
}

void
lcd_write (unsigned char c)
{
    __delay_us(40);
    lcd_write_nybble (c >> 4);
    lcd_write_nybble (c);
}

void
lcd_clear (void)
{
    LCD_RS = 0;
    lcd_write (0x1);
    __delay_ms (2);
}

void
lcd_putc (const char c)
{
    LCD_RS = 1;
    lcd_write (c);
}

void
lcd_puts (const char *s)
{
    while (*s != '\0')
        lcd_putc (*s++);
}

void
lcd_goto (unsigned char pos)
{
    LCD_RS = 0;
    lcd_write (0x80 + pos);
}

/* Overwrite the selected LCD line (0, 1, ...) with s.
 */
void
lcd_putline (int line, const char *s)
{
    int i = 0;

    if (line >= 0 && line < LCD_MAXROW) {
        lcd_goto (line * 0x40);
        while (s[i] != '\0' && i < LCD_MAXCOL) {
            if (islcd (s[i]))
                lcd_putc (s[i++]);
        }
        while (i++ < LCD_MAXCOL)
            lcd_putc (' ');
    }
}

void
lcd_init (void)
{
    LCD_RS = 0;
    LCD_EN = 0;
    LCD_RW = 0;

    //__delay_ms (15);
    __delay_ms (10); __delay_ms (5);
    LCD_DATA = 0x3;
    lcd_strobe ();
    __delay_ms (5);
    lcd_strobe ();
    __delay_us(200);
    lcd_strobe ();
    __delay_us (200);
    LCD_DATA = 2;        /* select 4 bit mode */
    lcd_strobe ();

    lcd_write (0x28);    /* set interface length */
    lcd_write (0xc);     /* display on, cursor off, cursor no blink */
    lcd_clear ();        /* clear screen */
    lcd_write (0x6);     /* set entry mode */
}

void
serial_init (void)
{
    TRISBbits.RB5 = 1; 
    TRISBbits.RB7 = 1;

    /* Set baud rate.
     * N.B. BRGH, BRG16, and divisor might needed to be changed
     * to get an accurate rate (see data sheet).
     * These settings are are for SERIAL_BAUD of 115200.
     */
    TXSTAbits.BRGH = 1;
    BAUDCONbits.BRG16 = 1;
    SPBRG = ((int)(_XTAL_FREQ/(4UL * SERIAL_BAUD) - 1));
    SPBRGH = 0;

    TXSTAbits.SYNC = 0;
    RCSTAbits.SPEN = 1;
    RCSTAbits.CREN = 1;
    TXSTAbits.TXEN = 1;
}

void
serial_putc(unsigned char byte)
{
    while(!TRMT)
        ;
    TXREG = byte;
}

unsigned char
serial_getc (void)
{
    if (OERR) {
        CREN = 0;
        CREN = 1;
    }
    while(!RCIF)
        ;
    return RCREG;
}

unsigned char *
serial_gets (unsigned char *buf, int len)
{
    unsigned char c;
    int i = 0;

    do {
        c = serial_getc ();
        if (i < len - 1 && islcd(c))
            buf[i++] = c;
    } while (c != '\n');
    buf[i] = '\0';

    return buf;
}

void interrupt 
isr (void)
{
    if (PIE1bits.RCIE && RCIF) {
        cbuf_putc (&serial_in, RCREG);
    }
    if (PIE1bits.TXIE && TXIF) {
        /* FIXME: set TXREG */
        TXIE = 0;
    }
}

void
main(void)
{
    static unsigned char line[17];

    OSCCONbits.IRCF = 7;        /* system clock HFOSC 16 MHz (x 4 with PLL) */

    ANSEL = 0;                  /* disable all ADC inputs */
    ANSELH = 0;
    TRISA = 0;                  /* disable all digital inputs */
    TRISB = 0;
    TRISC = 0;
    WPUA = 0;                   /* disable all pullups */
    WPUB = 0;
    IOCA = 0;                   /* disable all interrupt on change bits */
    IOCB = 0; 

    lcd_init ();
    serial_init ();

    INTCONbits.PEIE = 1;        /* enable peripheral interrupts */
    INTCONbits.GIE = 1;         /* enable global interrupts */
    PIE1bits.RCIE = 1;          /* enable USART receive interrupts */

    for (;;) {
        cbuf_gets (&serial_in, line, sizeof (line));
        lcd_putline (0, line);
    }
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
