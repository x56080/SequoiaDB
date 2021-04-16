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

   Source File Name = pageDef.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_PAGE_DEF_H_
#define VESSEL_PAGE_DEF_H_

#include "dpsDef.hpp"
#include "ossLikely.hpp"
#include "pd.hpp"

namespace engine
{
namespace vessel
{
   typedef UINT32 PAGE_ID;
   const PAGE_ID INVALID_PAGE_ID = UINT32(-1);

   const UINT16 INVALID_PAGE_VERSION = 0;
   const UINT16 PAGE_VERSION_1 = 1;

   const UINT32 PAGE_COUNT_IN_EXTENT = 8;

   typedef UINT16 PAGE_TYPE;
   const static PAGE_TYPE INVALID_PAGE_TYPE = 65535;
   const static PAGE_TYPE PAGE_TYPE_SMP = 0;
   const static PAGE_TYPE PAGE_TYPE_ID_MAP = 1;
   const static PAGE_TYPE PAGE_TYPE_RECORD = 2;
   const static PAGE_TYPE PAGE_TYPE_CS_META = 3;
   const static PAGE_TYPE PAGE_TYPE_COLLECTION_RECORD = 4;
   const static PAGE_TYPE PAGE_TYPE_ROUTE = 5;

   OSS_INLINE void getEyeCatcher(PAGE_TYPE type, CHAR &e0, CHAR &e1)
   {
      switch (type)
      {
      case PAGE_TYPE_SMP:
         e0 = 'S';
         e1 = 'P';
         break;
      case PAGE_TYPE_ID_MAP:
         e0 = 'I';
         e1 = 'P';
         break;
      case PAGE_TYPE_RECORD:
         e0 = 'R';
         e1 = 'P';
         break;
      case PAGE_TYPE_CS_META:
         e0 = 'C';
         e1 = 'S';
         break;
      case PAGE_TYPE_COLLECTION_RECORD:
         e0 = 'C';
         e1 = 'L';
         break;
      case PAGE_TYPE_ROUTE:
         e0 = 'R';
         e1 = 'O';
         break;
      default:
         e0 = 0;
         e1 = 0;
         SDB_ASSERT(FALSE, "invalid type");
      }
      return;
   }

   typedef UINT16 PAGE_FLAGS;
   const PAGE_FLAGS PAGE_FLAG_IN_USED = 0x01;

#pragma pack(4)
   struct pageHead
   {
      OSS_INLINE pageHead()
               :version(0),
               type(INVALID_PAGE_TYPE),
               flags(0),
               size(0),
               pageID(INVALID_PAGE_ID),
               lsn(DPS_INVALID_LSN_OFFSET),
               //snapshot(INVALID_SNAPSHOT_ID),
               pad(0),
               pad2(0)
               {
                  eyeCatcher[0] = 0;
                  eyeCatcher[1] = 0;
               }

      OSS_INLINE BOOLEAN inUsed()const
      {
         return OSS_BIT_TEST(flags, PAGE_FLAG_IN_USED);
      }
      OSS_INLINE void setInUsed()
      {
         OSS_BIT_SET(flags, PAGE_FLAG_IN_USED);
      }

      OSS_INLINE void reset()
      {
         eyeCatcher[0] = 0;
         eyeCatcher[1] = 0;
         version = INVALID_PAGE_VERSION;
         type = INVALID_PAGE_TYPE;
         flags = 0;
         size = 0;
         pageID = INVALID_PAGE_ID;
         lsn = DPS_INVALID_LSN_OFFSET;
         //snapshot = INVALID_SNAPSHOT_ID;
         pad = 0;
         pad2 = 0;
         return;
      }

      CHAR eyeCatcher[2];
      UINT16 version;
      UINT16 type;
      UINT16 flags;
      UINT32 size;
      UINT32 pageID;
      UINT64 lsn;
      //UINT32 snapshot;
      UINT32 pad;
      UINT64 pad2;
   };// struct pageHead
#pragma pack()
   const UINT32 PAGE_HEAD_LEN = sizeof(pageHead);
   const UINT32 PAGE_TAIL_LEN = sizeof(UINT64);

   BOOLEAN validatePageHeadAndTail(ossValuePtr ptr, UINT32 pageSize);

}/// end of namespace vessel
} /// end of namespace engine

#endif // VESSEL_PAGE_DEF_H_
