/*****************************************************************************\G
 *  Copyright (C) 2012 Jim Garlick
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
#include <string.h>
#include <stdlib.h>

#define _XTAL_FREQ 64000000UL

#if defined(_18F14K22)
__CONFIG (1, FOSC_IRC & PLLEN_ON);  /* PLLEN_ON: system clock is HFOSC*4 */
__CONFIG (2, BOREN_OFF & WDTEN_OFF);
__CONFIG (3, HFOFST_OFF & MCLRE_OFF);
__CONFIG (4, LVP_OFF);
#else
#error code assumes 18F14K22 chip.
#endif

/* N.B. read-modify-write should use LATC (output latch reg) not PORTC */
#define LCD_DATA4       PORTCbits.RC0
#define LCD_DATA5       PORTCbits.RC1
#define LCD_DATA6       PORTCbits.RC2 
#define LCD_DATA7       PORTCbits.RC3 

#define LCD_DATA_INPUT  PORTC
#define LCD_DATA        LATC

#define LCD_EN          LATCbits.LATC4
#define LCD_RS          LATCbits.LATC5
#define LCD_RW          LATCbits.LATC6

#define LCD_MAXCOL      16
#define LCD_MAXROW      2

#define islcd(c)        ((c) >= 0x20 && (c) <= 0x7d)

#define ENC_RA_A        PORTAbits.RA0
#define ENC_RA_B        PORTAbits.RA1
#define ENC_DEC_A       PORTAbits.RA2
#define ENC_DEC_B       PORTAbits.RA3

#define ENC_RA_RES      2160  // convenient: 360*6 (1 tic = 10')
#define ENC_DEC_RES     2160

#define SERIAL_BAUD     115200

#define CBUF_SIZE       80

#define CBUF_PLUSONE(x) ((x) < CBUF_SIZE - 1 ? (x) + 1 : 0)
#define CBUF_INC(x)     ((x) = CBUF_PLUSONE(x))

#define CBUF_FULL(c)    (CBUF_PLUSONE((c)->head) == (c)->tail)
#define CBUF_EMPTY(c)   ((c)->head == (c)->tail)

typedef struct {
    volatile unsigned char   buf[CBUF_SIZE];
    volatile unsigned char   head;
    volatile unsigned char   tail;
} cbuf_t;

static cbuf_t serial_in = { "", 0, 0 };
static cbuf_t serial_out = { "", 0, 0 };

void enc_tic (void);

static int enc_dec;
static int enc_ra;

void
serial_recv (void)
{
    if (CBUF_FULL(&serial_in))
        CBUF_INC(serial_in.tail);
    serial_in.buf[CBUF_INC(serial_in.head)] = RCREG;
}

void
serial_xmit (void)
{
    if (!CBUF_EMPTY(&serial_out) && TXSTAbits.TRMT)
        TXREG = serial_out.buf[CBUF_INC(serial_out.tail)];
    if (CBUF_EMPTY(&serial_out))
        PIE1bits.TXIE = 0;
}

void
serial_getc (unsigned char *cp)
{
    while (CBUF_EMPTY(&serial_in))
        ;
    PIE1bits.RCIE = 0;
    *cp = serial_in.buf[CBUF_INC(serial_in.tail)];
    PIE1bits.RCIE = 1;
}

void
serial_putc (unsigned char c)
{
    while (CBUF_FULL(&serial_out))
        ;
    PIE1bits.TXIE = 0;
    serial_out.buf[CBUF_INC(serial_out.head)] = c;
    PIE1bits.TXIE = 1;
}

void
serial_gets (unsigned char *buf, int len)
{
    unsigned char c;
    int i = 0;

    do {
        serial_getc (&c);
        if (i < len - 1 && c != '\r' && c != '\n')
            buf[i++] = c;
    } while (c != '\n');
    buf[i] = '\0';
}

void
serial_puts (const char *s)
{
    while (*s)
        serial_putc (*s++);
    serial_putc ('\n');
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

char
serial_checkerr (void)
{
    char errors = 0;

    if (RCSTAbits.OERR) { /* overrun */
        RCSTAbits.CREN = 0;
        RCSTAbits.CREN = 1;
        errors++;
    }
    if (RCSTAbits.FERR) { /* framing error */
        unsigned char dummy = RCREG;
        errors++;
    }
    return errors;
}

void interrupt 
isr (void)
{
    serial_checkerr ();
    if (PIE1bits.RCIE && RCIF) {
        serial_recv ();
    }
    if (PIE1bits.TXIE && TXIF) {
        serial_xmit ();
    }
    if (INTCONbits.RABIE && INTCONbits.RABIF) {
        enc_tic ();
        INTCONbits.RABIF = 0;
    }
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

    LCD_EN = 1;
    __delay_us (1);
    c = LCD_DATA_INPUT & 0x0f;
    LCD_EN = 0;

    return c;
}

unsigned char 
lcd_read (char rs)
{
    unsigned char c;

    LCD_RS = rs;
    c = lcd_read_nybble () << 4;
    c |= lcd_read_nybble ();

    return c;
}

void
lcd_wait_busy (void)
{
    unsigned char c;

    TRISC = 0x0f;
    LCD_RW = 1;
    __delay_us (5);         /* value determined experimentally */
    do {
        c = lcd_read (0);
    } while ((c & 0x80));
    LCD_RW = 0;
    TRISC = 0;
    __delay_us (5);         /* value determined experimentally */
}

void
lcd_write_nybble (unsigned char c)
{
    LCD_DATA = (LCD_DATA & 0xf0) | (c & 0x0f);
    lcd_strobe ();
}

void
lcd_write (char rs, unsigned char c)
{
    lcd_wait_busy ();
    LCD_RS = rs; 
    lcd_write_nybble (c >> 4);
    lcd_write_nybble (c);
}

void
lcd_clear (void)
{
    lcd_write (0, 0x1);
}

void
lcd_putc (unsigned char c)
{
    lcd_write (1, c);
}

void
lcd_goto (unsigned char x)
{
    lcd_write (0, 0x80 + x);
}

void
lcd_puts (const char *s)
{
    while (*s != '\0')
        lcd_putc (*s++);
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
    LCD_DATA = 2;               /* select 4 bit mode */
    lcd_strobe ();

    lcd_write (0, 0x28);        /* set interface length */
    lcd_write (0, 0xc);         /* display on, cursor off, cursor no blink */
    lcd_clear;                  /* clear screen */
    lcd_write (0, 0x6);         /* set entry mode */
}

void
enc_tic_update (unsigned char old, unsigned char new, int *count)
{
    if (old != new) {
        if (((old >> 1) ^ new) & 0x01)
            (*count)++;
        else
            (*count)--;
    }
}

void
enc_tic (void)
{
    static unsigned char old = 0xff;
    unsigned char new = PORTA;

    enc_tic_update (old & 0x3, new & 0x3, &enc_ra);
    enc_tic_update ((old >> 2) & 0x3, (new >> 2) & 0x3, &enc_dec);
    old = new;
}

void
enc_init (void)
{
    TRISAbits.RA0 = 1;      /* inputs */
    TRISAbits.RA1 = 1;
    TRISAbits.RA2 = 1;
    //TRISAbits.RA3 = 1;    /* RA3/^MCLR has no output capability */

    WPUAbits.WPUA0 = 1;     /* weak pullups */
    WPUAbits.WPUA1 = 1;
    WPUAbits.WPUA2 = 1;
    WPUAbits.WPUA3 = 1;

    IOCAbits.IOCA0 = 1;     /* interrupt on change */
    IOCAbits.IOCA1 = 1;
    IOCAbits.IOCA2 = 1;
    IOCAbits.IOCA3 = 1;

    INTCONbits.RABIF = 0;   /* clear IOC flag */
}

void
main(void)
{
    static unsigned char line[17];
    int i;

    OSCCONbits.IRCF = 7;        /* system clock HFOSC 16 MHz (x 4 with PLL) */

    ANSEL = 0;                  /* disable all ADC inputs */
    ANSELH = 0;
    TRISA = 0;                  /* disable all digital inputs */
    TRISB = 0;
    TRISC = 0;
    INTCON2bits.RABPU = 0;      /* enable pullups that have WPU* bits set */
    WPUA = 0;                   /* disable all pullups */
    WPUB = 0;
    IOCA = 0;                   /* disable all interrupt on change bits */
    IOCB = 0; 

    lcd_init ();
    serial_init ();
    enc_init ();

    INTCONbits.PEIE = 1;        /* enable peripheral interrupts */
    INTCONbits.GIE = 1;         /* enable global interrupts */
    PIE1bits.RCIE = 1;          /* enable USART receive interrupts */
    INTCONbits.RABIE = 1;       /* enable interrupt on change interrupts */

    enc_dec = 0;
    enc_ra = 0;
#if 0
    for (;;) {
        serial_gets (line, sizeof (line));
        if (!strcmp (line, "::foo")) {
            lcd_putline (1, "got foo!");
            serial_puts ("got foo!");
        } else if (!strcmp (line, "::bar")) {
            lcd_putline (1, "got bar!");
            serial_puts ("got bar!");
        } else {
            lcd_putline (0, line);
            lcd_putline (1, "");
        }
    }
#endif
#if 1
    lcd_putline (0, "hello");

    for (;;) {
        INTCONbits.RABIE = 0;
        if (enc_ra >= 0) {
            line[0] = '+';
            itoa (&line[1], enc_ra, 10);
        } else 
            itoa (&line[0], enc_ra, 10);
        for (i = 0; i < 8; i++) {
            if (line[i] == '\0')
                line[i] = ' ';
        }
        if (enc_dec >= 0) {
            line[8] = '+';
            itoa (&line[9], enc_dec, 10);
        } else
            itoa (&line[8], enc_dec, 10);
        INTCONbits.RABIE = 1;

        lcd_putline (1, line);

        __delay_ms (10);
    }       
#endif
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
