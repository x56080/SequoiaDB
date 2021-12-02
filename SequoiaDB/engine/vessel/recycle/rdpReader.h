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

   Source File Name = rdpReader.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_RDP_READER_H_
#define VESSEL_RDP_READER_H_

#include "vessel/recordID.h"
#include "vessel/recordDataPage.h"
#include "vessel/slice.h"
#include "vessel/logicalPageBuffer.h"

namespace engine
{
namespace vessel
{
   class requestContext;

   class rdpReader : public SDBObject
   {
      public:
         rdpReader(){}
         ~rdpReader(){}
         rdpReader(const rdpReader &) = delete;
         rdpReader &operator=(const rdpReader &) = delete;

      public:
         OSS_INLINE BOOLEAN isOpen()const
         {
            return NULL != _context;
         }
         INT32 open(requestContext *context,
                    PAGE_ID lpid,
                    const ossSharedLatchMode &mode=
                    ossSharedLatchMode(OSS_SHARED_LATCH_MODE_ENUM_SHARED));
         void close();

         UINT32 getTotalSlotCount()const;

         INT32 getSlot(RECORD_SLOT_ID pos, recordSlot &rs)const;

         INT32 getNormalRecordHead(RECORD_SLOT_ID pos,
                                   const recordHead **rh)const;

         INT32 getNormalRecordBody(UINT32 pos,
                                   slice &data)const;

      public:
         const recordDataPageHead &getPageHead()const
         {
            return _header;
         }
         const logicalPageBuffer &getPageBuffer()const
         {
            return _lpb;
         }
      private:
         requestContext *_context = NULL;
         logicalPageBuffer _lpb;
         recordDataPageHead _header;
   };
}//namespace vessel
}//namespace engine

#endif//VESSEL_RDP_READER_H_