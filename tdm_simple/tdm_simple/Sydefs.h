/* ------------------------------------------------------------------------ */
/* IDENT E1474.027-00    Scimitar FA2100, Store Manager Processor (SMP)     */
/* ------------------------------------------------------------------------ */

/* ------------------------------------------------------------------------ */
/* NOTICE:  All rights reserved.   This material contains the trade secrets */
/* and  confidential  information of L3 Communications, L3 Communications   */
/* Aviation Recorders Division,  which  embody substantial creative effort, */
/* ideas  and  expressions.  No part of this material  may be reproduced or */
/* transmitted  in  any  form or  by  any  means,  electronic,  mechanical, */
/* optical  or  otherwise,  including  photocopying  and  recording  or  in */
/* connection with  any  information  storage  or retrieval system, without */
/* specific written permission from  L3 Communcations, L3 Communications    */
/* Aviation Recorders Division.                                             */
/* ------------------------------------------------------------------------ */

/* ------------------------------------------------------------------------ */
/*                                                                          */
/* Project   : Scimitar FA2100 Aviation Recorder                            */
/* Subsystem : SMP (Store Manager Processor)                                */
/*                                                                          */
/* Filename  : sysdefs.h                                                    */
/* Author    : Dan Cunningham, Bill Hertz                                   */
/* Revision  : 1.4                                                          */
/* Updated   : 14-Jan-98                                                    */
/*                                                                          */
/* This module contains system-level definitions such as synthetic data     */
/* types and standard constants.                                            */
/*                                                                          */
/* ------------------------------------------------------------------------ */

/* ------------------------------------------------------------------------ */
/*                                                                          */
/* Revision Log:                                                            */
/*                                                                          */
/* Rev 1.0  : 19-Feb-97, wbh -- Original version created.                   */
/*                                                                          */
/* Rev 1.1  : 15-Jul-97, wbh -- Added typedef for "lword".                  */
/*                                                                          */
/* Rev 1.2  : 05-Sep-97, wbh -- Added ON/OFF definitions.                   */
/*                                                                          */
/* Rev 1.3  : 09-Jan-98, wbh -- Added "BooL" definitions as an alternate to */
/*          : "bool" for compatibility with certain "PC-based" 'C' compiler */
/*          : environments.                                                 */
/*                                                                          */
/* Rev 1.4  : 14-Jan-97, wbh -- Added several definitions which Karl Hahn   */
/*          : uses in the "PI" software so that his "stdtypes.h" file could */
/*          : be eliminated.                                                */
/*                                                                          */
/* ------------------------------------------------------------------------ */

#ifndef SYSDEFS_H
#define SYSDEFS_H

/* ------------------------------------------------------------------------ */
/* Synthetic data types used by the FA2100 software. Note that the smallest */
/* addressable unit on the C50 is 16 bits, so all "char" and "byte" data    */
/* types are actually 16-bit objects in the C50 environment. See the "Data  */
/* Types" section in the C50 C Compiler User's Guide for more information.  */
/* ------------------------------------------------------------------------ */

typedef unsigned char  byte;
typedef unsigned short word;
typedef unsigned char  boolt;
typedef int            BooL;
typedef unsigned long  ulong;
typedef unsigned long  lword;

/* ------------------------------------------------------------------------ */
/* Miscellaneous "logical" definitions and other commonly-used constants    */
/* ------------------------------------------------------------------------ */

   #ifndef TRUE

#define TRUE  ((boolt)1)
#define FALSE ((boolt)0)

   #endif

   #ifndef ON

#define ON    ((boolt)1)
#define OFF   ((boolt)0)

   #endif

   #ifndef NIL

#define NIL 0

   #endif

   #ifndef NULL

#define NULL 0

   #endif

   #ifndef FOREVER

#define FOREVER TRUE

   #endif

#endif  /* SYSDEFS_H */
