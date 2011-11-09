/*****************************************************************************\
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

/* i2c_slave.c - code for i2c slave */

/* Simple protocol based on 1-byte and 2-byte registers:
 * To write a register, master writes 2 <regnum> <lsb> [<msb>]
 * To read a register, master writes 1 <regnum>, then reads <lsb> [<msb>].
 * After a read or a write, another read re-samples the same <regnum>.
 *
 * N.B. expects reg_set () and reg_get () to be defined externally
 */

/* NOTE: compiled with hi-tech C pro V9.80 */

#include <htc.h>
#include "i2c_slave.h"

static unsigned char
i2c_read (void)
{
    return SSPBUF;
}

static void
i2c_write (unsigned char c)
{
    SSPCON1bits.CKP = 0;
    SSPBUF = c;
    SSPCON1bits.CKP = 1;
    (void)i2c_read();
}

/* See AN734b for 18F series I2C slave state descriptions
 */
static unsigned char
i2c_getstate (void)
{
    unsigned char state = 0;

    // State 1: I2C write operation, last byte was an address byte
    // SSPSTAT bits: S = 1, D_A = 0, R_W = 0, BF = 1
    if (SSPSTATbits.S == 1 && SSPSTATbits.D_A == 0
                           && SSPSTATbits.R_W == 0
                           && SSPSTATbits.BF == 1) {
        state = 1;
    }
    // State 2: I2C write operation, last byte was a data byte
    // SSPSTAT bits: S = 1, D_A = 1, R_W = 0, BF = 1
    if (SSPSTATbits.S == 1 && SSPSTATbits.D_A == 1
                                  && SSPSTATbits.R_W == 0
                                  && SSPSTATbits.BF == 1) {
        state = 2;
    }
    // State 3: I2C read operation, last byte was an address byte
    // SSPSTAT bits: S = 1, D_A = 0, R_W = 1
    if (SSPSTATbits.S == 1 && SSPSTATbits.D_A == 0
                           && SSPSTATbits.R_W == 1) {

        state = 3;
    }
    // State 4: I2C read operation, last byte was a data byte
    // SSPSTAT bits: S = 1, D_A = 1, R_W = 1, BF = 0
    if (SSPSTATbits.S == 1 && SSPSTATbits.D_A == 1
                           && SSPSTATbits.R_W == 1
                           && SSPSTATbits.BF == 0) {

        state = 4;
    
    }
    // State 5: Slave I2C logic reset by NACK from master
    // SSPSTAT bits: S = 1, D_A = 1, BF = 0, CKP = 1
    if (SSPSTATbits.S == 1 && SSPSTATbits.D_A == 1
                           && SSPSTATbits.BF == 0
                           && SSPCON1bits.CKP == 1) {
        state = 5;
    }
    return state;
}

void
i2c_interrupt (void)
{
    static unsigned char count = 0;
    static unsigned char cmd = I2C_CMD_READ;
    static unsigned char regnum = 0;

    switch (i2c_getstate ()) {
        case 1: /* write addr */
            (void)i2c_read();
            count = 0;
            break;
        case 2: /* write 1..Nth byte */
            if (count == 0) {
               cmd = i2c_read ();
            } else if (count == 1) {
               regnum = i2c_read ();
            } else if (count == 2 && cmd == I2C_CMD_WRITE) {
               reg_set (regnum, i2c_read (), REG_LSB);
            } else if (count == 3 && cmd == I2C_CMD_WRITE) {
               reg_set (regnum, i2c_read (), REG_MSB);
            } else {
               (void)i2c_read();
            }
            count++;
            break;
        case 3: /* read 1st byte */
            count = 0;
            i2c_write (reg_get (regnum, REG_LSB));
            count++;
            break;
        case 4: /* read 2..Nth byte */
            if (count == 1)
                i2c_write (reg_get (regnum, REG_MSB));
            else
                i2c_write (0);
            count++;
            break;
        case 5:  /* NAK */
            count = 0;
            (void)i2c_read();
            break;
        default:
            break;
    }

    if (SSPCON1bits.SSPOV) {
        SSPCON1bits.SSPOV = 0;
        (void)i2c_read();
    }
}

void
i2c_init (unsigned char addr)
{
    TRISBbits.RB4 = 1;          /* SDA */
    TRISBbits.RB6 = 1;          /* SCL */
    SSPSTAT = 0;
    SSPCON1 = 0;
    SSPCON1bits.SSPM = 6;       /* I2C slave mode, 7-bit address */
    SSPCON1bits.CKP = 1;        /* release clock */
    SSPCON2 = 0;
    SSPCON2bits.GCEN = 1;       /* disable general call */
    SSPCON2bits.SEN = 0;        /* disable clock stretching */
    SSPADD = addr << 1;         /* set I2C slave address */
    SSPCON1bits.SSPEN = 1;      /* enable MSSP peripheral */
    (void)i2c_read();
    SSPCON1bits.SSPOV = 0;      /* clear buffer overflow */
    SSPCON2bits.RCEN = 1;       /* receive enable */
    SSPIF = 0;                  /* clear SSP interrupt flag */
    SSPIE = 1;                  /* enable SSP interrupt */
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
