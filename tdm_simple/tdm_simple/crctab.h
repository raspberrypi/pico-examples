//  crctab.h - CRC calculation routines taken from Linux lrzsz package

#ifndef CRCTAB_H
#define CRCTAB_H

/*
 * updcrc macro derived from article Copyright (C) 1986 Stephen Satchell. 
 *  NOTE: First srgument must be in range 0 to 255.
 *        Second argument is referenced twice.
 * 
 * Programmers may incorporate any or all code into their programs, 
 * giving proper credit within the source. Publication of the 
 * source routines is permitted so long as proper credit is given 
 * to Stephen Satchell, Satchell Evaluations and Chuck Forsberg, 
 * Omen Technology.
 */

extern unsigned short crctab[];
static inline unsigned short updcrc(unsigned char cp, unsigned short crc)
    {
    return crctab[((crc >> 8) & 255)] ^ (crc << 8) ^ cp;
    }

/*
 * Copyright (C) 1986 Gary S. Brown.  You may use this program, or
 * code or tables extracted from it, as desired without restriction.
 */

extern long cr3tab[];
static long UPDC32(unsigned char b, long c)
    {
	return (cr3tab[((int)c ^ b) & 0xff] ^ ((c >> 8) & 0x00FFFFFF));
    }

#endif
