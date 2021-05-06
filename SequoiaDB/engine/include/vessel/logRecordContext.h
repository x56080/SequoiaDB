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
         OSS_INLINE logRecordContext()
         {}

         ~logRecordContext();

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

         void close();

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

         OSS_INLINE BOOLEAN needFullDump()const
         {
            return 0 < _fullDumpDataSize;
         }

         INT32 fullDumpPage(UINT32 size, const void *data);

         OSS_INLINE const CHAR *getFullDumpBuffer()const
         {
            return _fullDumpBuffer;
         }
         OSS_INLINE UINT32 getFullDumpDataSize()const
         {
            return _fullDumpDataSize;
         }

      private:
         INT32 ensureBuffer(UINT32 size);
      private:
         dpsLogRecordHeader _head;
         UINT32 _originalLen = 0;
         UINT32 _dpsBufLen = 0;
         CHAR *_fullDumpBuffer = NULL;
         UINT32 _fullDumpBufferSize = 0;
         UINT32 _fullDumpDataSize = 0;

   };//class logRecordContext
}//namespace vessel
}//namespace engine

#endif//VESSEL_LOG_RECORD_CONTEXT