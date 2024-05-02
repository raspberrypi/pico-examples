
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <pico/stdio.h>
#include <pico/time.h>
#include "crctab.h"
#include "ff.h"
#include "ssd1306.h" 

#include "zerror.h"

#define DISPLAY_ADDRESS 0x3c // 011110+SA0+RW - 0x3C or 0x3D
extern ssd1306_t disp;


//#include "lfswrap.h"

#define YDEBUG  0       // Diagnostics for XModem & YModem
#define ZDEBUG  0       // Diagnostics for ZModem

#if YDEBUG
#define YDIAG(...)  zdiag (__VA_ARGS__)
#else
#define YDIAG(...)
#endif
#if ZDEBUG
#define ZDIAG(...)  zdiag (__VA_ARGS__)
#else
#define ZDIAG(...)
#endif

int anykey (unsigned char *key, int tmo);
// HACK
void error (int iErr, const char *psMsg) {printf("%s",psMsg);}

char *setup (char *dst, const char *src, char *ext, char term, unsigned char *pflag);
#define MAX_PATH    260

#define SOH         0x01
#define EOT         0x04
#define ACK         0x06
#define CR          0x0D
#define DLE         0x10
#define XON         0x11
#define XOFF        0x13
#define NAK         0x15
#define CAN         0x18

#define ZHDR_BIN    0x101       /* Binary header */
#define ZHDR_HEX    0x102       /* Hex header */
#define ZHDR_B32    0x103       /* Binary header with 32-bit CRC */

#define ZCRC_16     ZHDR_BIN    /* 16-bit CRC */
#define ZCRC_32     ZHDR_B32    /* 32-bit CRC */

#define ZHT_RQINIT	    0	/* Request receive init */
#define ZHT_RINIT	    1	/* Receive init */
#define ZHT_SINIT       2	/* Send init sequence (optional) */
#define ZHT_ACK         3	/* ACK to above */
#define ZHT_FILE        4	/* File name from sender */
#define ZHT_SKIP        5	/* To sender: skip this file */
#define ZHT_NAK         6	/* Last packet was garbled */
#define ZHT_ABORT       7	/* Abort batch transfers */
#define ZHT_FIN         8	/* Finish session */
#define ZHT_RPOS        9	/* Resume data trans at this position */
#define ZHT_DATA        10	/* Data packet(s) follow */
#define ZHT_EOF         11	/* End of file */
#define ZHT_FERR        12	/* Fatal Read or Write error Detected */
#define ZHT_CRC         13	/* Request for file CRC and response */
#define ZHT_CHALLENGE   14	/* Receiver's Challenge */
#define ZHT_COMPL       15	/* Request is complete */
#define ZHT_CAN         16	/* Other end canned session with CAN*5 */
#define ZHT_FREECNT     17	/* Request for free bytes on filesystem */
#define ZHT_COMMAND     18	/* Command from sending program */
#define ZHT_STDERR      19  /* Output to standard error, data follows */
#define ZST_NEWHDR      20  /* Read a new header */
#define ZST_PEEKHDR     21  /* Check for a new header, continue if none */
#define ZST_QUIT        22  /* Exit */


#define ZFE_CRCE    ( 'h' | 0x100 )	/* 360 CRC next, frame ends, header packet follows */
#define ZFE_CRCG    ( 'i' | 0x100 )	/* 361 CRC next, frame continues nonstop */
#define ZFE_CRCQ    ( 'j' | 0x100 )	/* 362 CRC next, frame continues, ZACK expected */
#define ZFE_CRCW    ( 'k' | 0x100 )	/* 363 CRC next, ZACK expected, end of frame */

#define ZERR_NONE    0      // No error
#define ZERR_BAD    -1      // Invalid char sequence
#define ZERR_TOUT   -2      // Timeout
#define ZERR_ABT    -3      // Abort (CAN * 5)
#define ZERR_CRC    -4      // CRC failure
#define ZERR_ORUN   -5      // Overrun buffer
#define ZERR_FULL   -6      // Storage full (or other write error)
#define ZERR_NSYNC  -7      // File position out of sync
#define ZERR_INFO   -8      // Error generating file information
#define ZERR_UNIMP  -9      // Command not implemented
#define BUFFER_SIZE 2048  // Increase buffer size

#define ZFLG_ESCALL 0x40    // Escape all control characters
static unsigned char txflg;



//static FILE *pf = NULL;
FIL FileZ;
FIL *fpZ = NULL;
int nfpos = 0;

#if YDEBUG || ZDEBUG
#define DIAG_HW 1
#if DIAG_HW
#include <hardware/uart.h>
#include <hardware/gpio.h>
#endif

void zdiag (const char *psFmt,...)
    {
    static FILE *pfd = NULL;
    char sMsg[128];
    va_list va;
    va_start (va, psFmt);
    vsprintf (sMsg, psFmt, va);
    va_end (va);
#if DIAG_HW
    if ( pfd == NULL )
        {
        uart_init (uart1, 115200);
        uart_set_format (uart1, 8, 1, UART_PARITY_EVEN);
        gpio_set_function (4, GPIO_FUNC_UART);
        gpio_set_function (5, GPIO_FUNC_UART);
        uart_set_hw_flow (uart1, false, false);
        pfd = (FILE *) 1;
        }
    uart_write_blocking (uart1, sMsg, strlen (sMsg));
#else
    if ( pfd == NULL ) pfd = fopen ("/dev/uart.baud=115200 parity=N data=8 stop=1 tx=4 rx=5", "w");
    fwrite ((void *)sMsg, 1, strlen (sMsg), pfd);
#endif
    }
#endif

int nCan = 0;
static int zrdchr (int timeout)
//int zrdchr (int timeout)
    {
    int key = 0;
//    unsigned char kb;
    int kb;
//    int nCan = 0;
    absolute_time_t twait = make_timeout_time_ms (timeout);
    do
        {
//        if ( anykey (&kb, 100 * timeout) )
        if ((kb = getchar_timeout_us(100 * timeout)) !=  PICO_ERROR_TIMEOUT)        
            {
            key = kb & 0xff;
            switch (key)
                {
                case CAN:
                    ++nCan;
                    if ( nCan >= 5 ) return ZERR_ABT;
                    break;
                case XON:
                case XOFF:
                    nCan=0;
                    break;
                case 'h':
                case 'i':
                case 'j':
                case 'k':
                    if ( nCan > 0 ) key |= 0x100;
                    nCan=0;
                    return key;
                default:
                    if ( nCan > 0 )
                        {
                        nCan=0;
                        if (( key & 0x60 ) == 0x40 ) return (key ^ 0x40) | 0x100;
                        return ZERR_BAD;
                        }
                    nCan=0;
                    return key;
                }
            }
        }
    while ( to_ms_since_boot(get_absolute_time ()) < to_ms_since_boot(twait) );
    return ZERR_TOUT;
    }

static bool zpeekchr (int pchr, int tmo)
    {
    int key;
    ZDIAG ("zpeekchr:");
    do
        {
        key = zrdchr (tmo);
        ZDIAG (" %02X", key);
        }
    while (( key != pchr ) && ( key != ZERR_TOUT ));
    ZDIAG ("\r\n");
    return ( key == pchr );
    }

static inline void zcrcini (int type, long *pcrc)
    {
    if ( type == ZHDR_B32 )
        {
        *pcrc = 0xFFFFFFFFL;
        }
    else
        {
        *pcrc = 0;
        }
    }

static inline void zcrcupd (int type, long *pcrc, unsigned char b)
    {
    if ( type == ZHDR_B32 )
        {
        *pcrc = UPDC32 (b, *pcrc);
        }
    else
        {
        *pcrc = updcrc (b, *pcrc);
        }
    }

static int zchkhdr (int type, const unsigned char *hdr)
    {
    long chk;
    zcrcini (type, &chk);
    for (int i = 0; i < 5; ++i) zcrcupd (type, &chk, hdr[i]);
    if ( type == ZHDR_B32 )
        {
        for (int i = 5; i < 9; ++i)
            {
            if ( hdr[i] != (chk & 0xFF) ) return ZERR_CRC;
            chk >>= 8;
            }
        }
    else
        {
        zcrcupd (type, &chk, 0);
        zcrcupd (type, &chk, 0);
        if (( hdr[5] != (chk >> 8) ) || ( hdr[6] != (chk & 0xFF) )) return ZERR_CRC;
        }
    return hdr[0];
    }

static int zrdhdr (unsigned char *hdr)
    {
    int type;
    int key;
    bool bHavePad = false;
    while (true)
        {
        type = zrdchr (60000);
        if ( type < ZERR_BAD ) return type;
        if ( bHavePad && ( type >= ZHDR_BIN ) && ( type <= ZHDR_B32 )) break;
        else if ( type == '*' ) bHavePad = true;
        else bHavePad = false;
        }
    int nhdr = 7;
    if ( type == ZHDR_HEX )
        {
        for (int i = 0; i < nhdr; ++i)
            {
            key = zrdchr (1000);
            if ( key < 0 ) return key;
            int hx1 = key;
            if (( hx1 >= '0' ) && ( hx1 <= '9' ))   hx1 -= '0';
            else if (( hx1 >= 'a' ) && ( hx1 <= 'f' )) hx1 -= 'a' - 10;
            else ZERR_BAD;
            key = zrdchr (1000);
            if ( key < 0 ) return key;
            if (( key >= '0' ) && ( key <= '9' ))   key -= '0';
            else if (( key >= 'a' ) && ( key <= 'f' )) key -= 'a' - 10;
            else ZERR_BAD;
            hdr[i] = ( hx1 << 4 ) | key;
            }
        }
    else
        {
        if ( type == ZHDR_B32 ) nhdr = 9;
        for (int i = 0; i < nhdr; ++i)
            {
            key = zrdchr (1000);
            if ( key < 0 ) return key;
            hdr[i] = key & 0xFF;
            }
        }
    return zchkhdr (type, hdr);
    }

static int ztxthdr (const char *ps, unsigned char *hdr)
    {
    int hx1;
    int hx2;
    for (int i = 0; i < 7; ++i)
        {
        hx1 = *ps;
        if (( hx1 >= '0' ) && ( hx1 <= '9' )) hx1 -= '0';
        else if (( hx1 >= 'a' ) && ( hx1 <= 'f' )) hx1 -= 'a' - 10;
        else return ZERR_BAD;
        ++ps;
        hx2 = *ps;
        if (( hx2 >= '0' ) && ( hx2 <= '9' )) hx2 -= '0';
        else if (( hx2 >= 'a' ) && ( hx2 <= 'f' )) hx2 -= 'a' - 10;
        else return ZERR_BAD;
        ++ps;
        hdr[i] = ( hx1 << 4 ) | hx2;
        }
    return zchkhdr (ZHDR_HEX, hdr);
    }

static void zwrchr (int chr)
    {
    static bool bAt = false;
    if ( chr >= 0x100 )
        {
        putchar_raw (CAN);
        chr = ( chr & 0xFF ) | 0x40;
        }
    else if ( (chr & 0x7F) < 0x20 )
        {
        if ( txflg & ZFLG_ESCALL )
            {
            putchar_raw (CAN);
            chr |= 0x40;
            }
        else
            {
            switch (chr & 0x7F)
                {
                case DLE:
                case XON:
                case XOFF:
                case CAN:
                    putchar_raw (CAN);
                    chr |= 0x40;
                    break;
                case CR:
                    if ( bAt )
                        {
                        putchar_raw (CAN);
                        chr |= 0x40;
                        }
                    break;
                default:
                    break;
                }
            }
        }
    putchar_raw (chr);
    bAt = (chr == '@');
    }

static void zwrhex (int chr)
    {
    int hx = chr >> 4;
    if ( hx < 10 ) hx += '0';
    else hx += 'a' - 10;
    zwrchr (hx);
    hx = chr & 0x0F;
    if ( hx < 10 ) hx += '0';
    else hx += 'a' - 10;
    zwrchr (hx);
    }

static void zwrhdr (int type, unsigned char *hdr)
    {
    zwrchr ('*');
    if ( type == ZHDR_HEX ) zwrchr ('*');
    zwrchr (type);
    long chk;
    zcrcini (type, &chk);
    ZDIAG ("zwrhdr:");
    for (int i = 0; i < 5; ++i)
        {
        if ( type == ZHDR_HEX )
            {
            zwrhex (hdr[i]);
            }
        else
            {
            zwrchr (hdr[i]);
            }
        zcrcupd (type, &chk, hdr[i]);
        ZDIAG (" %02X=%04X", hdr[i], chk);
        }
    if ( type == ZHDR_BIN )
        {
        long test = chk;
        zcrcupd (type, &chk, 0);
        ZDIAG (" 0=%04X", chk);
        zcrcupd (type, &chk, 0);
        ZDIAG (" 0=%04X", chk);
        zwrchr (chk >> 8);
        zcrcupd (type, &test, chk >> 8);
        ZDIAG (" c=%04X", test);
        zwrchr (chk & 0xFF);
        zcrcupd (type, &test, chk & 0xFF);
        ZDIAG (" c=%04X\r\n", test);
        }
    else if ( type == ZHDR_HEX )
        {
        long test = chk;
        zcrcupd (type, &chk, 0);
        ZDIAG (" 0=%04X", chk);
        zcrcupd (type, &chk, 0);
        ZDIAG (" 0=%04X", chk);
        zwrhex (chk >> 8);
        zcrcupd (type, &test, chk >> 8);
        ZDIAG (" c=%04X", test);
        zwrhex (chk & 0xFF);
        zcrcupd (type, &test, chk & 0xFF);
        ZDIAG (" c=%04X\r\n", test);
        zwrchr (0x0D);
        zwrchr (0x0A);
        long crc = chk;
        }
    if ( type == ZHDR_B32 )
        {
        for (int i = 0; i < 4; ++i)
            {
            zwrchr (chk & 0xFF);
            chk >>= 8;
            }
        }
    }

static int zchkcrc (long chk, int state)
    {
    zcrcupd (ZCRC_16, &chk, 0);
    zcrcupd (ZCRC_16, &chk, 0);
    int crc1 = zrdchr (1000);
    if ( crc1 < 0 ) return crc1;
    crc1 &= 0xFF;
    int crc2 = zrdchr (1000);
    if ( crc2 < 0 ) return crc2;
    crc2 &= 0xFF;
    if ((( chk >> 8 ) != crc1 ) || (( chk & 0xFF ) != crc2 )) return ZERR_CRC;
    return state;
    }

static int zrddata (void *ptr, int nlen)
    {
    unsigned char *buffer;
    int nalloc = 0;
    int ndata = 0;
    long chk = 0;
    int key;
    if ( nlen < 0 )
        {
        nlen = -nlen;
        nalloc = nlen;
        buffer = (unsigned char *) malloc(nlen);
        *((unsigned char **)ptr) = buffer;
        if ( buffer == NULL ) return ZERR_ORUN;
        }
    else
        {
        buffer = (unsigned char *)ptr;
        }
    zcrcini (ZCRC_16, &chk);
    while (true)
        {
        key = zrdchr (1000);
        if ( key < 0 ) return key;
        zcrcupd (ZCRC_16, &chk, (unsigned char) key);
        if ( key >= ZFE_CRCE ) break;
        key &= 0xFF;
        if (( ndata >= nlen ) && ( nalloc > 0 ))
            {
            unsigned char *bnew = realloc (buffer, ndata + nalloc);
            if ( bnew != NULL )
                {
                buffer = bnew;
                ndata += nalloc;
                *((unsigned char **)ptr) = buffer;
                }
            else
                {
                nalloc = -nalloc;
                }
            }
        if ( ndata < nlen ) buffer[ndata] = key;
        ++ndata;
        }
    if ( ndata > nlen ) return ZERR_ORUN;
    return zchkcrc (chk, key);
    }

static FILE *pathopen (char *path, const char *pfname)
    {
    char *ps = strrchr (path, '/');
    if ( ps == NULL ) ps = path;
    else ++ps;
    int nch = strlen (pfname) + (ps - path);
    if ( nch > MAX_PATH ) return NULL;
    strcpy (ps, pfname);
    if (f_open (&FileZ, path, FA_WRITE) == FR_OK)
        return (&FileZ);
    else
        return (NULL);
    }

static int zrdfinfo (unsigned char *hdr, char *path)
    {
    unsigned char *pinfo = NULL;
    int state = zrddata (&pinfo, -64);
    ZDIAG ("\r\nFile = %s\r\n", pinfo);
    if ( state < 0 )
        {
        if ( pinfo != NULL ) free (pinfo);
        return state;
        }
    if ( fpZ == NULL )
        {
        fpZ = pathopen (path, pinfo);
        if ( fpZ == NULL )
            {
            free (pinfo);
            return ZHT_FERR;
            }
        }
    nfpos = 0;
    free (pinfo);
    return ZERR_NSYNC; // Force a ZHT_POS response
    }

static inline int hdrint (unsigned char *hdr)
    {
    return ( hdr[1] | ( hdr[2] << 8 ) | ( hdr[3] << 16 ) | ( hdr[4] << 24 ));
    }

static void zsethdr (unsigned char *hdr, int ht, int val)
    {
    hdr[0] = ht;
    hdr[1] = val & 0xFF;
    val >>= 8;
    hdr[2] = val & 0xFF;
    val >>= 8;
    hdr[3] = val & 0xFF;
    val >>= 8;
    hdr[4] = val;
    }

static bool zsvbuff (int npos, int ndata, const unsigned char *data)
    {
    int nfst = nfpos - npos;
    int nwrt = ndata - nfst;
    ZDIAG ("npos = %d ndata = %d nfst = %d nwrt = %d", npos, ndata, nfst, nwrt);
    if ( nwrt > 0 )
        {
        int nsave;
        f_write (fpZ,(void *)&data[nfst], nwrt, &nsave);
        nfpos += nsave;
        ZDIAG (" nwrt = %d, nsave = %d, nfpos = %d", nwrt, nsave, nfpos);
        if ( nsave < ndata ) return false;
        }
    ZDIAG ("\r\n");
    return true;
    }

static int zsvdata (unsigned char *hdr)
    {
    long chk;
    int key;
    // unsigned char data[64];
    unsigned char data[1024];
    int ndata = 0;
    int npos = hdrint (hdr);
    ZDIAG ("zsvdata: nfpos = %d, hdr = %d\r\n", nfpos, npos);
    if ( npos > nfpos ) return ZERR_NSYNC;
    zcrcini (ZCRC_16, &chk);
    while (true)
        {
        key = zrdchr (1000);
        if ( key < 0 ) return key;
        zcrcupd (ZCRC_16, &chk, key);
        if ( key >= ZFE_CRCE ) break;
        key &= 0xFF;
        data[ndata] = key;
        if ( ++ndata >= sizeof (data) )
            {
            if ( ! zsvbuff (npos, ndata, data) ) return ZERR_FULL;
            npos += ndata;
            ndata = 0;
            }
        }
    if ( ndata > 0 )
        {
        if ( ! zsvbuff (npos, ndata, data) ) return ZERR_FULL;
        npos += ndata;
        ndata = 0;
        }
    zsethdr (hdr, hdr[0], npos);    // Record new position for continuing data
    return zchkcrc (chk, key);
    }

// static int zsvdata (unsigned char *hdr) {
//     long chk;
//     int key;
//     unsigned char data[1024];  // Increased buffer size
//     int ndata = 0;
//     int npos = hdrint(hdr);

//     if (npos > nfpos) return ZERR_NSYNC;

//     zcrcini(ZCRC_16, &chk);
//     while (true) {
//         key = zrdchr(1000);
//         if (key < 0) return key;
//         zcrcupd(ZCRC_16, &chk, key);
//         if (key >= ZFE_CRCE) break;
//         key &= 0xFF;
//         data[ndata++] = key;
//         if (ndata >= sizeof(data)) {
//             if (!zsvbuff(npos, ndata, data)) return ZERR_FULL;
//             npos += ndata;
//             ndata = 0;
//         }
//     }
//     if (ndata > 0) {
//         if (!zsvbuff(npos, ndata, data)) return ZERR_FULL;
//     }
//     zsethdr(hdr, hdr[0], npos);  // Record new position for continuing data
//     return zchkcrc(chk, key);
// }

static void zwrdata (int type, const unsigned char *pdata, int ndata, int fend)
    {
    long chk;
    zcrcini (type, &chk);
    ZDIAG ("zwrdata:");
    for (int i = 0; i < ndata; ++i)
        {
        zwrchr (*pdata);
        zcrcupd (type, &chk, *pdata);
        ZDIAG (" %02X=%04X", *pdata, chk);
        ++pdata;
        }
    zwrchr (fend);
    zcrcupd (type, &chk, fend & 0xFF);
    ZDIAG (" %02X=%04X", fend & 0xFF, chk);
    long test = chk;
    zcrcupd (type, &chk, 0);
    ZDIAG (" 0=%04X", chk);
    zcrcupd (type, &chk, 0);
    ZDIAG (" 0=%04X", chk);
    zwrchr (chk >> 8);
    zcrcupd (type, &test, chk >> 8);
    ZDIAG (" c=%04X", test);
    zwrchr (chk & 0xFF);
    zcrcupd (type, &test, chk & 0xFF);
    ZDIAG (" c=%04X\r\n", test);
    }

static int zwrfinfo (int type, const char *pfn)
    {
    ZDIAG ("pfn = %s pf = %p\r\n", pfn, pf);
    int nch = strlen (pfn);
    unsigned char *pinfo = (unsigned char *) malloc (nch + 12);
    if ( pinfo == NULL ) return ZERR_INFO;
    strcpy (pinfo, pfn);
    ZDIAG ("Seek to file end\r\n");
//    if ( f_lseek (pf, 0, SEEK_END) != 0 ) return ZERR_INFO;
//    if ( f_lseek (pf, 1000) != 0 ) return ZERR_INFO;
    if (f_lseek (fpZ, -1) != FR_OK)
        return ZERR_INFO;
    long nlen = f_tell (fpZ);
    ZDIAG ("nlen = %d\r\n", nlen);
    if ( nlen < 0 ) return ZERR_INFO;
//    if ( f_lseek (pf, 0, SEEK_SET) != 0 ) return ZERR_INFO;
    if (f_lseek (fpZ, 0) != FR_OK)
        return ZERR_INFO;
    ZDIAG ("Rewound file\r\n");
    nfpos = 0;
    sprintf (&pinfo[nch+1], "%d", nlen);
    nch += strlen (&pinfo[nch+1]) + 2;
    zwrdata (type, pinfo, nch, ZFE_CRCW);
    free (pinfo);
    return ZST_NEWHDR;
    }

static int zwrfile (int type, unsigned char *hdr)
    {
    int state;
    int fend;
    // unsigned char data[64];
    unsigned char data[1024];
    int npos = hdrint (hdr);
    ZDIAG ("zwrfile: npos = %d nfpos = %d\r\n", npos, nfpos);
    if ( npos != nfpos )
        {
//        if ( f_lseek (pf, npos, SEEK_SET) < 0 )
        if (f_lseek (fpZ, npos) != FR_OK)
            {
            return ZHT_FERR;
            }
        nfpos = npos;
        }
//    int ndata = fread (data, 1, sizeof (data), pf);
    int ndata;
    f_read (fpZ, data, sizeof (data), &ndata);
    nfpos += ndata;
    zsethdr (hdr, hdr[0], nfpos);
    if ( ndata == sizeof (data) )
        {
        state = ZST_PEEKHDR;
        fend = ZFE_CRCG;
        // state = ZST_NEWHDR;
        // fend = ZFE_CRCQ;
        }
    else
        {
        ZDIAG ("zwrfile: ndata = %d nfpos = %d\r\n", ndata, nfpos);
        state = ZHT_EOF;
        fend = ZFE_CRCE;
        }
    zwrdata (type, data, ndata, fend);
    return state;
    }


static int zwrbreak (unsigned char *hdr, unsigned char *attn)
    {
    unsigned char *pa = attn;
    ZDIAG ("ATTN\r\n");
    while ( *pa )
        {
        switch (*pa)
            {
            case 0xDD:
                // Should be a break
                sleep_ms (1000);
                break;
            case 0xDE:
                // A pause
                sleep_ms (1000);
                break;
            default:
                putchar_raw (*pa);
                break;
            }
        ++pa;
        }
    zwrhdr (ZHDR_HEX, hdr);
    return ZST_NEWHDR;
    }

static void zclose (void)
    {
    ZDIAG ("zclose\r\n");
    if ( &fpZ != NULL ) f_close (fpZ);
    fpZ = NULL;
    }

void zreceive (const char *pfname, const char *pcmd)
    {
    unsigned char attn[32] = { 0 };
	char path[MAX_PATH+1];
	unsigned char flag;
    ZDIAG ("zreceive (%s, %s)\r\n", pfname, pcmd);
//    setup (path, pfname, ".bbc", ' ', &flag);
//    ZDIAG ("path = %s flag = %02X\r\n", path, flag);
//    if ( flag & 0x01 )
//        {
//        pf = fopen (path, "w");
//        if ( pf == NULL ) error (204, "Cannot create file");
//        }
    strcpy (path,pfname);
    if (f_open (&FileZ, pfname, FA_WRITE) != FR_OK)
        error (214, "Cannot open file");
    fpZ = &FileZ;

    int state = ZHT_RQINIT;
    int curst;
    int ntmo = 10;
    int nnak = 10;
    unsigned char hdr[9];
    if ( pcmd != NULL )
        {
        pcmd += 2;
        state = ztxthdr (pcmd, hdr);
        }
    do
        {
        ZDIAG ("state = %d\r\n", state);
        if (( state >= 0 ) && ( state < ZST_QUIT )) curst = state;
        switch (state)
            {
            case ZST_NEWHDR:
                state = zrdhdr (hdr);
                break;
            case ZHT_RQINIT:    /* 0 - Request receive init */
                zsethdr (hdr, ZHT_RINIT, 0);
                zwrhdr (ZHDR_HEX, hdr);
                state = ZST_NEWHDR;
                break;
            case ZHT_SINIT:     /* 2 - Send init sequence (optional) */
                txflg = hdr[4];
                state = zrddata (attn, sizeof (attn));
                break;
            case ZHT_FILE:      /* 4 - File name from sender */
                state = zrdfinfo (hdr, path);
                break;
            case ZHT_SKIP:      /* 5 - To sender: skip this file */
                state = ZERR_UNIMP;
                break;
            case ZHT_NAK:       /* 6 - Last packet was garbled */
                if ( --nnak == 0 ) state = ZST_QUIT;
                else state = curst;
                break;
            case ZHT_FIN:       /* 8 - Finish session */
                zwrhdr (ZHDR_HEX, hdr);
                ZDIAG ("ZHT_FIN: ");
                if ( zpeekchr ('O', 1000) )
                    {
                    zpeekchr ('O', 1000);
                    }
                state = ZST_QUIT;
                break;
            case ZHT_DATA:      /* 10 - Data packet(s) follow */
                state = zsvdata (hdr);
                break;
            case ZHT_EOF:       /* 11 - End of file */
                ZDIAG ("EOF: nfpos = %d, hdr = %d\r\n", nfpos, hdrint (hdr));
                if ( hdrint (hdr) != nfpos ) state = ZERR_NSYNC;
                if ( fpZ != NULL )
                    {
                    f_close (fpZ);
                    fpZ = NULL;
                    }
                nfpos = 0;
                state = ZHT_RQINIT;
                break;
            case ZHT_FERR:      /* 12 - Fatal Read or Write error Detected */
                state = ZHT_RQINIT;
                break;
            case ZHT_RINIT:     /* 1 - Receive init - Echoed from dead client */
            case ZHT_CAN:       /* 16 - Other end canned session with CAN*5 */
                state = ZST_QUIT;
                break;
            case ZHT_FREECNT:   /* 17 - Request for free bytes on filesystem */
                zsethdr (hdr, ZHT_ACK, 0);
                zwrhdr (ZHDR_HEX, hdr);
                state = ZST_NEWHDR;
                break;
            case ZHT_ACK:       /* 3 - ACK to above */
            case ZHT_ABORT:     /* 7 - Abort batch transfers */
            case ZHT_RPOS:      /* 9 - Resume data trans at this position */
            case ZHT_CRC:       /* 13 - Request for file CRC and response */
            case ZHT_CHALLENGE: /* 14 - Receiver's Challenge */
            case ZHT_COMPL:     /* 15 - Request is complete */
            case ZHT_COMMAND:   /* 18 - Command from sending program */
            case ZHT_STDERR:    /* 19 - Output to standard error, data follows */
                state = ZERR_UNIMP;
                break;
            case ZFE_CRCE:      /* CRC next, frame ends, header packet follows */
                state = ZST_NEWHDR;
                break;
            case ZFE_CRCG:       /* CRC next, frame continues nonstop */
                state = curst;
                break;
            case ZFE_CRCQ:      /* CRC next, frame continues, ZACK expected */
                zsethdr (hdr, ZHT_ACK, nfpos);
                zwrhdr (ZHDR_HEX, hdr);
                state = curst;
                break;
            case ZFE_CRCW:      /* CRC next, ZACK expected, end of frame */
                zsethdr (hdr, ZHT_ACK, nfpos);
                zwrhdr (ZHDR_HEX, hdr);
                state = ZST_NEWHDR;
                break;
            case ZERR_TOUT:
                ZDIAG ("ntmo = %d\r\n", ntmo);
                if ( --ntmo == 10 )
                    {
                    state = ZST_QUIT;
                    }
                else
                    {
                    zsethdr (hdr, ZHT_NAK, 0);
                    zwrhdr (ZHDR_HEX, hdr);
                    state = ZST_NEWHDR;
                    }
                break;
            case ZERR_ABT:
                state = ZST_QUIT;
                break;
            case ZERR_FULL:
                zsethdr (hdr, ZHT_FERR, nfpos);
                state = zwrbreak (hdr, attn);
                break;
            case ZERR_NSYNC:
                zsethdr (hdr, ZHT_RPOS, nfpos);
                state = zwrbreak (hdr, attn);
                break;
            default:
                zsethdr (hdr, ZHT_NAK, nfpos);
                state = zwrbreak (hdr, attn);
                break;
            }
        }
    while ( state != ZST_QUIT );
    zclose ();
    }

void zsend (const char *pfname)
    {
	char path[MAX_PATH+1];
    bool bDone = false;
    int state = ZHT_RQINIT;
    int curst;
    int prvst = 0;
    int ntmo = 10;
    int nnak = 10;
    unsigned char hdr[9];
	unsigned char flag;
    ZDIAG ("zsend\r\n");
//    setup (path, pfname, ".bbc", ' ', &flag);
//    pf = fopen (path, "r");
    strcpy (path,pfname);
    if (f_open (&FileZ, pfname, FA_READ) != FR_OK)
        error (214, "Cannot open file");
    fpZ = &FileZ;

    state = ZHT_RQINIT;
    do
        {
        ZDIAG ("state = %d\r\n", state);
        prvst = curst;
        if (( state >= 0 ) && ( state < ZST_QUIT )) curst = state;
       
        switch (state)
            {
            case ZST_PEEKHDR:
                if ( zpeekchr ('*', 0) ) state = ZST_NEWHDR;
                else state = ZHT_ACK;
                break;
            case ZST_NEWHDR:
                state = zrdhdr (hdr);
                break;
            case ZHT_RQINIT:    /* 0 - Request receive init */
                zsethdr (hdr, ZHT_RQINIT, 0);
                zwrhdr (ZHDR_HEX, hdr);
                state = ZST_NEWHDR;
                break;
            case ZHT_RINIT:     /* 1 - Receive init */
                ZDIAG ("ZHT_RINIT: %02X %02X %02X %02X\r\n", hdr[1], hdr[2], hdr[3], hdr[4]);
                if ( bDone )
                    {
                    zsethdr (hdr, ZHT_FIN, 0);
                    zwrhdr (ZHDR_HEX, hdr);
                    state = ZST_NEWHDR;
                    }
                else
                    {
                    zsethdr (hdr, ZHT_FILE, 0);
                    zwrhdr (ZHDR_BIN, hdr);
                    state = zwrfinfo (ZHDR_BIN, path);
                    }
                break;
            case ZHT_ACK:       /* 3 - ACK to above */
                ZDIAG ("ZHT_ACK: hdr = %d nfpos = %d\r\n", hdrint (hdr), nfpos);
                state = zwrfile (ZHDR_BIN, hdr);
                break;
            case ZHT_SKIP:      /* 5 - To sender: skip this file */
                zsethdr (hdr, ZHT_FIN, 0);
                zwrhdr (ZHDR_HEX, hdr);
                state = ZST_NEWHDR;
                break;
            case ZHT_NAK:       /* 6 - Last packet was garbled */
                if ( --nnak == 0 )
                    {
                    zclose ();
                    return;
                    }
                curst = prvst;
                state = prvst;
                break;
            case ZHT_FIN:       /* 8 - Finish session */
                zwrchr ('O');
                zwrchr ('O');
                state = ZST_QUIT;
                break;
            case ZHT_RPOS:      /* 9 - Resume data trans at this position */
                hdr[0] = ZHT_DATA;
                zwrhdr (ZHDR_BIN, hdr);
                state = zwrfile (ZHDR_BIN, hdr);
                break;
            case ZHT_EOF:       /* 11 - End of file */
                zsethdr (hdr, ZHT_EOF, nfpos);
                zwrhdr (ZHDR_HEX, hdr);
                bDone = true;
                state = ZST_NEWHDR;
                break;
            case ZHT_FERR:      /* 12 - Fatal Read or Write error Detected */
                zsethdr (hdr, ZHT_FIN, 0);
                zwrhdr (ZHDR_HEX, hdr);
                state = ZST_NEWHDR;
                displayError("Fatal Read or Write Error", "Error", "Restart");
                break;
            case ZHT_CAN:       /* 16 - Other end canned session with CAN*5 */
                ZDIAG ("Cancelled by remote\r\n");
                zclose ();
                return;
            case ZHT_FREECNT:   /* 17 - Request for free bytes on filesystem */
                zsethdr (hdr, ZHT_ACK, 0);
                zwrhdr (ZHDR_HEX, hdr);
                state = ZST_NEWHDR;
                break;
            case ZHT_SINIT:     /* 2 - Send init sequence (optional) */
            case ZHT_FILE:      /* 4 - File name from sender */
            case ZHT_ABORT:     /* 7 - Abort batch transfers */
            case ZHT_DATA:      /* 10 - Data packet(s) follow */
            case ZHT_CRC:       /* 13 - Request for file CRC and response */
            case ZHT_CHALLENGE: /* 14 - Receiver's Challenge */
            case ZHT_COMPL:     /* 15 - Request is complete */
            case ZHT_COMMAND:   /* 18 - Command from sending program */
            case ZHT_STDERR:    /* 19 - Output to standard error, data follows */
                state = ZERR_UNIMP;
                displayError("Standard Error", "Error", "Restart");
                break;
            case ZERR_ABT:
                state = ZST_QUIT;
                displayError("Aborting batch transfer", "Error", "Restart");
                break;
            case ZERR_TOUT:
                ZDIAG("ntmo = %d\r\n", ntmo);
                if (--ntmo == 0) {
                state = ZST_QUIT;
                displayError("Timeout", "Error", "Restart");
                } else {
                    state = curst;
                    displayError("Timeout", "Continuing", "Check");
                }
                break;

            case ZERR_INFO:
                zsethdr (hdr, ZHT_FERR, 0);
                zwrhdr (ZHDR_HEX, hdr);
                state = ZST_NEWHDR;
                displayError("Error generating info", "Error", "Restart");
                break;
            default:
                zsethdr (hdr, ZHT_NAK, nfpos);
                zwrhdr (ZHDR_HEX, hdr);
                state = ZST_NEWHDR;

                break;
            }
        }
    while ( state != ZST_QUIT );
    zclose ();
    }

