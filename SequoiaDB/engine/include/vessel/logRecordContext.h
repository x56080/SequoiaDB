/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = logRecordContext.h

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_LOG_RECORD_CONTEXT
#define VESSEL_LOG_RECORD_CONTEXT

#include "dpsLogRecord.hpp"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
   class logRecordContext
   {
      public:
         OSS_INLINE logRecordContext():
         _originalLen(0),
         _dpsBufLen(0),
         _needFullDump(FALSE)
         {}

         OSS_INLINE ~logRecordContext()
         {
         }

      public:
         OSS_INLINE void prepush(UINT32 len)
         {
            SDB_ASSERT(0 < len, "can not be zero");
            SDB_ASSERT(len <= DPS_MAX_TAGV_LEN, "element too long");
            SDB_ASSERT(!prepared(), "can not be prepared");

            _originalLen += len;
            _originalLen += 4;/// 1byte for tag and 3bytes for len.
            return;
         }

         OSS_INLINE void prepushDone()
         {
            SDB_ASSERT(!prepared(), "can not be prepared");
            _head._length = ossAlign4(_originalLen);
            return;
         }

         OSS_INLINE void close()
         {
            _head.clear();
            _originalLen = 0;
            _dpsBufLen = 0;
            _needFullDump = FALSE;
            return;
         }

         OSS_INLINE BOOLEAN prepared()const
         {
            return DPS_INVALID_LSN_OFFSET != _head._lsn;
         }

         OSS_INLINE DPS_LSN_OFFSET getLsn()const
         {
            return _head._lsn;
         }
         OSS_INLINE DPS_LSN_VER getVersion()const
         {
            return _head._version;
         }

         OSS_INLINE UINT32 getDPSBufLen()const
         {
            return _dpsBufLen;
         }

         OSS_INLINE dpsLogRecordHeader &getHead()
         {
            return _head;
         }

         OSS_INLINE const dpsLogRecordHeader &getHead()const
         {
            return _head;
         }

         OSS_INLINE void setNeedFullDump()
         {
            _needFullDump = TRUE;
         }

         OSS_INLINE BOOLEAN needFullDump()const
         {
            return _needFullDump;
         }
      private:
         dpsLogRecordHeader _head;
         UINT32 _originalLen;
         UINT32 _dpsBufLen;
         BOOLEAN _needFullDump;

   };//class logRecordContext
}//namespace vessel
}//namespace engine

#endif//VESSEL_LOG_RECORD_CONTEXT