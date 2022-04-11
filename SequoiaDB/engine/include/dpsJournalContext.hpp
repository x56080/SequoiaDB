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

   Source File Name = dpsJournalContext.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef DPS_JOURNAL_ENTRY_HPP_
#define DPS_JOURNAL_ENTRY_HPP_

#include "dpsLogDef.hpp"
#include "dpsLogRecord.hpp"

namespace engine
{
#pragma pack(4)
   class dpsJournalContext : public SDBObject
   {
      public:
         dpsJournalContext(){}
         ~dpsJournalContext();
         dpsJournalContext(const dpsJournalContext &) = delete;
         dpsJournalContext &operator=(const dpsJournalContext &) = delete;

      public:
         enum class STATUS : UINT16
         {
            INVALID = 0,
            PREPARING = 1,
            PREPARED = 2,
            COMMITED = 3,
            ABORTED = 4,
         };//enum class STATUS

      public:
         OSS_INLINE STATUS getStatus()const {return _status;}
         OSS_INLINE UINT16 getType()const {return _type;}
         OSS_INLINE UINT32 getOriginalSize()const {return _originalSize;}
         OSS_INLINE UINT32 getElementCount()const {return _elementCount;}
         OSS_INLINE UINT32 getFlags()const {return _flags;}
         OSS_INLINE DPS_LSN_OFFSET getOplistLSN()const {return _oplist;}

         OSS_INLINE DPS_LSN_OFFSET getLSN()const {return _lsn;}
         OSS_INLINE UINT32 getAlignedSize()const {return _alignedSize;}
         OSS_INLINE UINT32 getPushedDataSize()const {return _bufferPos;}

      public:
         void reset();

      public:
         void prepare(UINT16 type);
         void setFlag(UINT32 flags);
         void setOplistLSN(DPS_LSN_OFFSET oplist);
         void prepushElement(UINT32 size);

      public:
         void pushElement(DPS_TAG tag,
                          UINT32 size,
                          const void *data);

         void pushInt64Ele(DPS_TAG tag,
                           INT64 data);

         void pushInt32Ele(DPS_TAG tag,
                           INT32 data);

         void pushInt16Ele(DPS_TAG tag,
                           INT16 data);

         void pushInt8Ele(DPS_TAG tag,
                          INT8 data);

      private:
         STATUS _status = STATUS::INVALID;
         UINT16 _type = LOG_TYPE_DUMMY;
         UINT32 _originalSize = 0;
         UINT32 _elementCount = 0;
         UINT32 _flags = 0;
         DPS_LSN_OFFSET _oplist = DPS_INVALID_LSN_OFFSET;

         /// prepared 
         DPS_LSN_OFFSET _lsn = DPS_INVALID_LSN_OFFSET;
         UINT32 _alignedSize = 0;

   };//class dpsJournalContext

#pragma pack()
} // namespace engine


#endif//DPS_JOURNAL_ENTRY_HPP_