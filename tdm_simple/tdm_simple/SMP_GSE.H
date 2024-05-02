/* ------------------------------------------------------------------------ */
/* IDENT E1474.026-00    Scimitar FA2100 Aviation Recorder Project          */
/* ------------------------------------------------------------------------ */

/* ------------------------------------------------------------------------ */
/* NOTICE:  All rights reserved.   This material contains the trade secrets */
/* and  confidential  information of L3 Communications, L3 Communications   */
/* Aviation Recorders Division,  which  embody substantial creative effort, */
/* ideas  and  expressions.  No part of this material  may be reproduced or */
/* transmitted  in  any  form or  by  any  means,  electronic,  mechanical, */
/* optical  or  otherwise,  including  photocopying  and  recording  or  in */
/* connection with  any  information  storage  or retrieval system, without */
/* specific written permission from  L3 Communications, L3 Communications   */
/* Aviation Recorders Division.                                             */
/* ------------------------------------------------------------------------ */

/* ------------------------------------------------------------------------ */
/*                                                                          */
/* Project   : Scimitar FA2100 Aviation Recorder                            */
/* Subsystem : Various FA2100 Subsystems                                    */
/*                                                                          */
/* Filename  : smp_gse.h                                                    */
/* Author    : Bill Hertz                                                   */
/* Revision  : 2.9                                                          */
/* Updated   : 04-Apr-98                                                    */
/*                                                                          */
/* This module contains definitions that pertain to the SMP-GSE interface.  */
/*                                                                          */
/* ------------------------------------------------------------------------ */

/* ------------------------------------------------------------------------ */
/*                                                                          */
/* Revision Log:                                                            */
/*                                                                          */
/* Rev 1.0  : 14-Aug-97, wbh -- Original version created.                   */
/*                                                                          */
/* Rev 1.1  : 24-Aug-97, wbh -- Moved "TDM_S2G_UART" definitions from the   */
/*          : "tdm.h" file, and renamed them as "Trace" words.              */
/*                                                                          */
/* Rev 1.2  : 05-Sep-97, tns -- Added definitions for "FDR Record Control"  */
/*          : and "Force Test Condition" commands.                          */
/*                                                                          */
/* Rev 1.3  : 19-Sep-97, wbh -- Added definitions for "SMP-Retransmitted    */
/*          : Flight Data Stream" messages.                                 */
/*                                                                          */
/* Rev 1.4  : 24-Sep-97, wbh -- Added definitions for "SMP-to-GSE" FDR      */
/*          : Indices" messages.                                            */
/*                                                                          */
/* Rev 1.5  : 28-Sep-97, wbh -- Added definitions for "GSE-to-SMP" "Clear   */
/*          : Latched Fault" message.                                       */
/*                                                                          */
/* Rev 1.6  : 30-Sep-97, wbh -- Fixed definitions for several "SMP-to-GSE"  */
/*          : "exception" messages. These had been inadvertantly assigned   */
/*          : values which conflicted with the "FDR Indices" messages.      */
/*          :                                                               */
/*          : Added messages for "GSE-to-SMP" FDP Status query and "SMP-to  */
/*          : GSE" response message.                                        */
/*          :                                                               */
/*          : Added messages for "GSE-to-SMP" Dump Time specification.      */
/*                                                                          */
/* Rev 1.7  : 30-Oct-97, wbh -- Modified some of the "Force Test Condition" */
/*          : messages to support FDR and CVR Fault On/Off.                 */
/*          :                                                               */
/*          : Added defintitions for Dumping and Clearing the Fault Log.    */
/*                                                                          */
/* Rev 1.8  : 21-Nov-97, wbh -- Added definitions for "SMP-to-GSE" ASCII    */
/*          : text as required for the generation of Dump-File Hdrs, etc.   */
/*                                                                          */
/* Rev 1.9  : 04-Dec-97, wbh -- Added definitions for "Set Marker" command, */
/*          : GSE-to-SMP, and "Marker Num" response, SMP-to-GSE.            */
/*          :                                                               */
/*          : Added definitions for "Version Request" message, GSE-to-SMP,  */
/*          : and the "Version Responses", SMP-to-GSE.                      */
/*                                                                          */
/* Rev 2.0  : 12-Dec-97, wbh -- Added definition for "Dump Block Pairs" msg */
/*          : from SMP-to-GSE during Dump startup exchange. Also deleted    */
/*          : the "Bad Dump Cmd" message definition because it is obsolete. */
/*          : A "Bad Dump Cmd" is now indicated by a "Zero Block Pairs"     */
/*          : response from the SMP.                                        */
/*                                                                          */
/* Rev 2.1  : 12-Dec-97, wbh -- Fixed "QMASK" for "Dump Block Pairs" msg.   */
/*                                                                          */
/* Rev 2.2  : 16-Dec-97, wbh -- Deleted the obsolete "FDR Indices" message  */
/*          : definitions, and fixed the value for "S2G_CMD_NOT_IF_RECORD". */
/*                                                                          */
/* Rev 2.3  : 17-Dec-97, wbh -- Added "G2S_DUMP_PKT16X_WDS" definitions to  */
/*          : allow for a 16X specifier in the Dump Packet size from GSE.   */
/*                                                                          */
/* Rev 2.4  : 01-Jan-98, wbh -- Added "CVR Playback" message definitions.   */
/*                                                                          */
/* Rev 2.5  : 05-Jan-98, wbh -- Added "CVR Set Absolute" time definitions.  */
/*                                                                          */
/* Rev 2.6  : 06-Jan-98, wbh -- Added FA2100 "Type Query" msg definitions.  */
/*                                                                          */
/* Rev 2.7  : 09-Jan-98, wbh -- Moved the "SMP Fault Status" definitions to */
/*          : this file from "fa2100.h" because it was realized that these  */
/*          : are meaningful only to the SMP-GSE interface.                 */
/*                                                                          */
/* Rev 2.8  : 10-Jan-98, wbh -- Added "CVR Position" Query and Response     */
/*          : message definitions.                                          */
/*                                                                          */
/* Rev 2.9  : 29-Mar-98, wbh -- Added "CVR Dump" message definitions, and   */
/*          : the "CVR_DUMP_FRMS_PER_PKT" definition.                       */
/*                                                                          */
/* ------------------------------------------------------------------------ */

#ifndef SMP_GSE_H
#define SMP_GSE_H

/* ------------------------------------------------------------------------ */
/* Miscellaneous Interface Constants and Definitions                        */
/* ------------------------------------------------------------------------ */

#define CVR_DUMP_FRMS_PER_PKT    7       /* Num CVR Frms per CVR Dump Pkt   */

/* ------------------------------------------------------------------------ */
/* These definitions correspond to the protocol between the SMP and the GSE */
/* over the TDM bus.                                                        */
/* ------------------------------------------------------------------------ */

#define G2S_FDR_DUMP_MASK        0xff00  /* Mask  for "FDR Dump" command    */
#define G2S_FDR_DUMP             0x0000  /* Patrn for "FDR Dump" command    */

#define G2S_FDR_DUMP_QUAL_MASK   0x00ff  /* Mask  for FDR Dump qualifier    */
#define G2S_FDR_FULL_DUMP        0x0000  /* Patrn for FDR Full-Dump request */
#define G2S_FDR_TIME_DUMP        0x00ff  /* Patrn for FDR Time-Dump request */

#define G2S_FDR_DUMP_TIME_MASK   0xff00  /* Mask  for "FDR Dmp Time" cmd    */
#define G2S_FDR_DUMP_TIME        0x1100  /* Patrn for "FDR Dmp Time" cmd    */

#define G2S_CVR_DUMP_MASK        0xff00  /* Mask  for "CVR Dump" command    */
#define G2S_CVR_DUMP             0x0f00  /* Patrn for "CVR Dump" command    */
#define G2S_CVR_DUMP_QUAL_MASK   0x00ff  /* Mask  for "CVR Dump" qualifier  */

#define G2S_DUMP_PKT_WDS_MASK    0xff00  /* Mask  for "Dump Pkt Size" cmd   */
#define G2S_DUMP_PKT_WDS         0x1200  /* Patrn for "Dump Pkt Size" cmd   */

#define G2S_DUMP_PKT16X_WDS_MASK 0xff00  /* Mask  for "Dump Pkt 16X" cmd    */
#define G2S_DUMP_PKT16X_WDS      0x5100  /* Patrn for "Dump Pkt 16X" cmd    */

#define G2S_DUMP_CMD_MASK        0xffff  /* Dump-Command Mask,   GSE to SMP */
#define G2S_NEXT_PKT_GO          0x1301  /* "Next Pkt Go" cmd,   GSE to SMP */
#define G2S_PREV_PKT_GO          0x1302  /* "Prev Pkt Go" cmd,   GSE to SMP */
#define G2S_KEEP_ALIVE           0x13f0  /* "Keep Alive"  cmd,   GSE to SMP */
#define G2S_TERMINATE_DUMP       0x13ff  /* "Trmnt Dump"  cmd,   GSE to SMP */

#define G2S_SET_MARKER_MASK      0xfff0  /* Set Marker Cmd Mask  GSE to SMP */
#define G2S_SET_MARKER           0x1800  /* Set Marker Cmd Patrn GSE to SMP */
#define G2S_SET_MARKER_QMASK     0x000f  /* Set Marker Cmd Qmask GSE to SMP */

#define G2S_DUMP_FLT_LOG_MASK    0xfff0  /* Dmp FltLog Cmd Mask  GSE to SMP */
#define G2S_DUMP_FLT_LOG         0x1820  /* Dmp FltLog Cmd Patrn GSE to SMP */
#define G2S_DUMP_FLT_LOG_QMASK   0x000f  /* Dmp FltLog Cmd Qmask GSE to SMP */

#define G2S_CVR_PLAY_MASK        0xffff  /* Play control Mask,   GSE to SMP */
#define G2S_CVR_PLAY             0x2b02  /* Play control Cmd,    GSE to SMP */
#define G2S_CVR_STOP_MASK        0xffff  /* Stop control Mask,   GSE to SMP */
#define G2S_CVR_STOP             0x2b03  /* Stop control Mask,   GSE to SMP */

#define G2S_CVR_ADJ_SECS_MASK    0xfe00  /* CVR Adj Secs Mask,   GSE to SMP */
#define G2S_CVR_ADJ_SECS         0x1c00  /* CVR Adj Secs Cmd,    GSE to SMP */
#define G2S_CVR_ADJ_SECS_HI      0x0100  /* CVR Adj Secs Hi Flg, GSE to SMP */

#define G2S_CVR_ABS_SECS_MASK    0xfe00  /* CVR Abs Secs Mask,   GSE to SMP */
#define G2S_CVR_ABS_SECS         0x1e00  /* CVR Abs Secs Cmd,    GSE to SMP */
#define G2S_CVR_ABS_SECS_HI      0x0100  /* CVR Abs Secs Hi Flg, GSE to SMP */

#define G2S_CVR_QUERY_MASK       0xffff  /* CVR Query Mask,      GSE to SMP */
#define G2S_CVR_QUERY            0xa000  /* CVR Query Cmd,       GSE to SMP */

#define G2S_FDR_RECD_CTL_MASK    0xffff  /* Record control mask, GSE to SMP */
#define G2S_FDR_RECD_ENBL        0x2b05  /* Enable  FDR Rec cmd, GSE to SMP */
#define G2S_FDR_RECD_DSBL        0x2b04  /* Disable FDR Rec cmd, GSE to SMP */

#define G2S_FORCE_TST_COND_MASK  0xfff0  /* Force Test cnd mask, GSE to SMP */
#define G2S_FORCE_TST_COND       0x2b10  /* "Force Test Cond",   GSE to SMP */
#define G2S_FORCE_TST_COND_QMASK 0x000f  /* "Force Test Cond",   GSE to SMP */

#define G2S_FORCE_TST_LED_ON     0x0000  /* "SMP LED On",        GSE to SMP */
#define G2S_FORCE_TST_LED_OFF    0x0001  /* "SMP LED Off",       GSE to SMP */

#define G2S_FORCE_TST_FDRFLT_ON  0x0002  /* "FDR Fault Ind On"   GSE to SMP */
#define G2S_FORCE_TST_FDRFLT_OFF 0x0003  /* "FDR Fault Ind Off"  GSE to SMP */
#define G2S_FORCE_TST_FDRFLT_RLS 0x0004  /* "FDR Fault Ind Rls"  GSE to SMP */

#define G2S_FORCE_TST_CVRFLT_ON  0x0005  /* "CVR Fault Ind On"   GSE to SMP */
#define G2S_FORCE_TST_CVRFLT_OFF 0x0006  /* "CVR Fault Ind Off"  GSE to SMP */
#define G2S_FORCE_TST_CVRFLT_RLS 0x0007  /* "CVR Fault Ind Rls"  GSE to SMP */

#define G2S_CLR_FLT_LOG_MASK     0xffff  /* "Clr FltLog" mask    GSE to SMP */
#define G2S_CLR_FLT_LOG          0x2b1e  /* "Clr FltLog" cmd     GSE to SMP */

#define G2S_CLR_LTCHD_FLT_MASK   0xffff  /* "Clr Ltchd Flt" mask GSE to SMP */
#define G2S_CLR_LTCHD_FLT        0x2b1f  /* "Clr Ltchd Flt" cmd  GSE to SMP */

#define G2S_VERSN_RQST_MASK      0xffff  /* "Version Rqst" mask  GSE to SMP */
#define G2S_VERSN_RQST           0x600e  /* "Version Rqst" cmd   GSE to SMP */

#define G2S_STATUS_RQST_MASK     0xffff  /* "Status Rqst" mask   GSE to SMP */
#define G2S_STATUS_RQST          0x600f  /* "Status Rqst" cmd    GSE to SMP */

#define G2S_TYPE_QUERY_MASK      0xff00  /* "Type Query" mask,   GSE to SMP */
#define G2S_TYPE_QUERY           0x5400  /* "Type Query" cmd,    GSE to SMP */

#define G2S_CVR_POS_QRY_MASK     0xff00  /* "Postn Query" mask,  GSE to SMP */
#define G2S_CVR_POS_QRY          0x5500  /* "Postn Query" cmd,   GSE to SMP */

#define S2G_TYPE_RSPNS_MASK      0xff00  /* "Type Rspns" mask,   SMP to GSE */
#define S2G_TYPE_RSPNS           0x5400  /* "Type Rspns" msg,    SMP to GSE */

#define S2G_CVR_POS_RSPNS_MASK   0xff00  /* "Postn Rspns" mssk,  SMP to GSE */
#define S2G_CVR_POS_RSPNS        0x5500  /* "Postn Rspns" msg,   SMP to GSE */

#define S2G_TYPE_FDR             0x0080  /* FDR Type flag        SMP to GSE */
#define S2G_TYPE_CVR             0x0040  /* CVR Type flag        SMP to GSE */
#define S2G_TYPE_120M            0x0008  /* CVR 120-Min flag     SMP to GSE */
#define S2G_TYPE_IN_SITU         0x0002  /* CVR In-Situ Pbk flg  SMP to GSE */
#define S2G_TYPE_FDR_RATE_SHIFT       4  /* Num Bits to shft rt to lo postn */

#define S2G_FDP_STATUS_MASK      0xfc00  /* FDP Status mask      SMP to GSE */
#define S2G_FDP_STATUS           0x6800  /* FDP Status           SMP to GSE */
#define S2G_FDP_STATUS_BITS      0x03ff  /* Mask  for "FDP Status" bits     */

#define S2G_SMP_STATUS_MASK      0xfc00  /* SMP Status mask      SMP to GSE */
#define S2G_SMP_STATUS           0x6400  /* SMP Status           SMP to GSE */
#define S2G_SMP_STATUS_BITS      0x03ff  /* Mask  for "SMP Status" bits     */

#define S2G_FA2100_REV_MASK      0xff00  /* SMP FA2100 Rev mask  SMP to GSE */
#define S2G_FA2100_REV           0x6e00  /* SMP FA2100 Rev msg   SMP to GSE */

#define S2G_FDP_CHKSUM_MASK      0xfe00  /* FDP Chksum mask      SMP to GSE */
#define S2G_FDP_CHKSUM           0x2c00  /* FDP Chksum msg       SMP to GSE */
#define S2G_FDP_CHKSUM_HIBYTE    0x0100  /* Mask for Hibyte flag SMP to GSE */

#define S2G_SMP_CHKSUM_MASK      0xfe00  /* SMP Chksum mask      SMP to GSE */
#define S2G_SMP_CHKSUM           0x2e00  /* SMP Chksum msg       SMP to GSE */
#define S2G_SMP_CHKSUM_HIBYTE    0x0100  /* Mask for Hibyte flag SMP to GSE */

#define S2G_FDP_VERSN_MASK       0xf000  /* FDP Intrnl Vrsn mask SMP to GSE */
#define S2G_FDP_VERSN            0x7000  /* FDP Intrnl Vrsn msg  SMP to GSE */

#define S2G_SMP_VERSN_MASK       0xf000  /* SMP Intrnl Vrsn mask SMP to GSE */
#define S2G_SMP_VERSN            0x9000  /* SMP Intrnl Vrsn msg  SMP to GSE */

#define S2G_ASCII_TEXT_MASK      0xff00  /* ASCII Text Msg mask  SMP to GSE */
#define S2G_ASCII_TEXT           0x6c00  /* ASCII Text Msg       SMP to GSE */
#define S2G_ASCII_TEXT_END_FLAG  0x0080  /* ASCII Text Last Char SMP to GSE */

#define S2G_DUMP_BLK_PAIRS_MASK  0xf000  /* "Dump Blk Prs" mask  SMP to GSE */
#define S2G_DUMP_BLK_PAIRS       0x0000  /* "Dump Blk Prs" msg   SMP to GSE */
#define S2G_DUMP_BLK_PAIRS_QMASK 0x0fff  /* "Dump Blk Prs" Qmask SMP to GSE */

#define S2G_RDY_TO_DUMP_MASK     0xffff  /* "Rdy to Dump" mask,  SMP to GSE */
#define S2G_RDY_TO_DUMP          0x1300  /* "Rdy to Dump" cmd,   SMP to GSE */

#define S2G_BLOCK_CRC_MASK       0xfc00  /* "Block CRC" mask,    SMP to GSE */
#define S2G_BLOCK_CRC            0x1400  /* "Block CRC" msg,     SMP to GSE */
#define S2G_CRC_HI_FLAG          0x0100  /* Blk CRC, Hi-byte flg SMP to GSE */
#define S2G_CRC_DONE_FLAG        0x0200  /* Blk CRC, Done flg    SMP to GSE */

#define S2G_FD_RTRNSMT_DATA_MASK 0xf000  /* FD Rtrnsmt Data mask SMP to GSE */
#define S2G_FD_RTRNSMT_DATA      0x8000  /* FD Rtrnsmt Data cmd  SMP to GSE */
#define S2G_FD_RTRNSMT_STRM_BIT  0x0400  /* FD Rtrnsmt Strm bit  SMP to GSE */
#define S2G_FD_RTRNSMT_HIBYT_BIT 0x0200  /* FD Rtrnsmt HiByt bit SMP to GSE */
#define S2G_FD_RTRNSMT_PGTOP_BIT 0x0100  /* FD Rtrnsmt PgTop bit SMP to GSE */

#define S2G_MARKER_NUM_MASK      0xfff0  /* Marker Num Msg Mask  SMP to GSE */
#define S2G_MARKER_NUM           0x1800  /* Marker Num Msg Patrn SMP to GSE */
#define S2G_MARKER_NUM_QMASK     0x000f  /* Marker Num Msg Qmask SMP to GSE */

#define S2G_CVR_RSPNS_MASK       0xf000  /* CVR Response Mask,   SMP to GSE */
#define S2G_CVR_RSPNS            0xa000  /* CVR Response Msg,    SMP to GSE */
#define S2G_CVR_RSPNS_120M       0x0800  /* CVR 120 Min Flg,     SMP to GSE */
#define S2G_CVR_RSPNS_RECD       0x0400  /* CVR Record Mode Flg  SMP to GSE */
#define S2G_CVR_RSPNS_PLAY       0x0200  /* CVR Play Mode Flg    SMP to GSE */
#define S2G_CVR_RSPNS_HIBYTE     0x0100  /* CVR Play Mode Flg    SMP to GSE */

#define S2G_FDR_NOT_SUPPORTED    0x6000  /* "FDR Not Supprtd" ,  SMP to GSE */
#define S2G_CVR_NOT_SUPPORTED    0x6001  /* "CVR Not Supprtd" ,  SMP to GSE */
#define S2G_ILLEGAL_CMD          0x6002  /* "Illegal Command" ,  SMP to GSE */
#define S2G_CMD_OUT_OF_CONTEXT   0x6003  /* "Cmd Out of Cntxt",  SMP to GSE */
#define S2G_CMD_NOT_IF_RECORD    0x6004  /* "Not Valid in Recd", SMP to GSE */

#define S2G_TDM_TRC_MASK         0xff00  /* TDM Trace wd mask,   SMP to GSE */
#define S2G_TDM_TRC_WORD         0x1000  /* TDM Trace word ID,   SMP to GSE */

/* ------------------------------------------------------------------------ */
/* The bit definitions in this section pertain to the common FA2100 system  */
/* standard for a Fault Subsystem Status Word. This word format is used in  */
/* internal SMP ITC messages as well as in messages from the SMP to the GSE */
/* to pass this status along.                                               */
/* ------------------------------------------------------------------------ */

#define SMP_LTCHD_FLT_FLG        0x0100  /* SMP Latched Fault Flag          */
#define SMP_FDR_FLT_FLG          0x0080  /* FDR Fault Flag                  */
#define SMP_FDR_NONLTCHD_FLT_FLG 0x0040  /* FDR Nonlatched Fault Flag       */
#define SMP_FDR_GSE_FLT_ON_FLG   0x0020  /* FDR GSE-Cmd "Fault On" Flag     */
#define SMP_FDR_GSE_FLT_OFF_FLG  0x0010  /* FDR GSE-Cmd "Fault Off" Flag    */
#define SMP_CVR_FLT_FLG          0x0008  /* CVR Fault Flag                  */
#define SMP_CVR_NONLTCHD_FLT_FLG 0x0004  /* CVR Nonlatched Fault Flag       */
#define SMP_CVR_GSE_FLT_ON_FLG   0x0002  /* CVR GSE-Cmd "Fault On" Flag     */
#define SMP_CVR_GSE_FLT_OFF_FLG  0x0001  /* CVR GSE-Cmd "Fault Off" Flag    */

#endif  /* SMP_GSE_H */
