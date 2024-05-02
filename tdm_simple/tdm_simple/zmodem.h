//  zmodem.h - ZModem implementation

#ifndef ZMODEM_H
#define ZMODEM_H

#define ZDIAG   1

#if ZDIAG
void zdiag (const char *psFmt,...);
#endif
void yreceive (int mode, const char *pfname);
void ysend (int mode, const char *pfname);
void zreceive (const char *pfname, const char *pcmd);
void zsend (const char *pfname);

#endif
