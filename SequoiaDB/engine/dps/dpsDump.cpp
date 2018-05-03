/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = dpsDump.cpp

   Descriptive Name = Data Protection Service Log File

   When/how to use: this program may be used on binary and text-formatted
   versions of data protection component. This file contains code logic for
   DPS transaction log file basic operations

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/12/2013  XJH  Initial Draft

   Last Changed =

*******************************************************************************/
#include "dpsDump.hpp"
#include "dpsDef.hpp"
#include "ossUtil.hpp"
#include "dpsLogFile.hpp"

namespace engine
{

   UINT32 _dpsDump::dumpLogFileHead( CHAR *inBuf, UINT32 inSize,
                                     CHAR *outBuf, UINT32 outSize,
                                     UINT32 options )
   {
      SDB_ASSERT ( inBuf, "inbuf can't be NULL" ) ;
      SDB_ASSERT ( outBuf, "outbuf can't be NULL" ) ;
      SDB_ASSERT ( DPS_LOG_HEAD_LEN == inSize,
                   "insize must be DPS_LOG_HEAD_LEN" ) ;

      UINT32 len           = 0 ;
      UINT32 hexDumpOption = 0 ;
      if ( DPS_DMP_OPT_HEX & options )
      {
         hexDumpOption |= OSS_HEXDUMP_INCLUDE_ADDR ;
         if ( !(DPS_DMP_OPT_HEX_WITH_ASCII & options ) )
         {
            hexDumpOption |= OSS_HEXDUMP_RAW_HEX_ONLY ;
         }
         ossHexDumpBuffer ( (void*)inBuf, inSize, outBuf, outSize, NULL,
                            hexDumpOption ) ;
         len = ossStrlen ( outBuf ) ;
         outBuf [ len ] = '\n' ;
         ++len ;
      }
      if ( DPS_DMP_OPT_FORMATTED & options )
      {
         dpsLogHeader *logHead = (dpsLogHeader*)inBuf ;
         /* dump output looks like
          *  Head    : SDBLOGHD
          *  FirstLSN: 0x00000000123456789(4886718345)
          *  LogID   : 10
          */
         len += ossSnprintf ( outBuf + len, outSize - len,
                              OSS_NEWLINE
                              " Head   : %c%c%c%c%c%c%c%c"OSS_NEWLINE,
                              logHead->_eyeCatcher[0],
                              logHead->_eyeCatcher[1],
                              logHead->_eyeCatcher[2],
                              logHead->_eyeCatcher[3],
                              logHead->_eyeCatcher[4],
                              logHead->_eyeCatcher[5],
                              logHead->_eyeCatcher[6],
                              logHead->_eyeCatcher[7] ) ;
         if ( ossMemcmp ( DPS_LOG_HEADER_EYECATCHER, logHead->_eyeCatcher,
                          DPS_LOG_HEADER_EYECATCHER_LEN ) != 0 )
         {
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 "Error: Invalid Eye Catcher"OSS_NEWLINE ) ;
            goto exit ;
         }
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " FirstLSN: 0x%016lx(%lld)"OSS_NEWLINE,
                              logHead->_firstLSN.offset,
                              logHead->_firstLSN.offset ) ;
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " LogID  : %d"OSS_NEWLINE,
                              logHead->_logID ) ;
      }

   exit :
      return len ;
   }

}

