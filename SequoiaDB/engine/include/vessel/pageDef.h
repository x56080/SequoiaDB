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
#include "ossTypes.hpp"
#include "vessel/vesselIdDef.h"
#include "vessel/pageIdentifier.h"

namespace engine
{
namespace vessel
{
   constexpr UINT16 INVALID_PAGE_VERSION = 0;
   constexpr UINT16 PAGE_VERSION_1 = 1;

   constexpr UINT32 PAGE_COUNT_IN_EXTENT = 8;

   typedef UINT16 PAGE_TYPE;
   constexpr PAGE_TYPE INVALID_PAGE_TYPE = 65535;

   constexpr PAGE_TYPE PAGE_TYPE_CS_META = 0;
   constexpr PAGE_TYPE PAGE_TYPE_CL_META = 1;
   constexpr PAGE_TYPE PAGE_TYPE_ROUTE = 2;
   constexpr PAGE_TYPE PAGE_TYPE_RECORD = 3;
   constexpr PAGE_TYPE PAGE_TYPE_BTREE_ENTRY = 1000;
   constexpr PAGE_TYPE PAGE_TYPE_BTREE_NODE = 1001;

   OSS_INLINE void getPageEyeCatcher(CHAR &e0, CHAR &e1)
   {
      e0 = 'P';
      e1 = 'H';
      return;
   }

#pragma pack(4)
   struct pageHead
   {
      pageHead() = default;
      ~pageHead() = default;

      OSS_INLINE pageHead(const pageHead &o):
      version(o.version),
      checksum(o.checksum),
      type(o.type),
      flags(o.flags),
      size(o.size),
      pid(o.pid),
      lpid(o.lpid),
      psv(o.psv),
      lsn(o.lsn),
      reserved(o.reserved)
      {
         eyeCatcher[0] = o.eyeCatcher[0];
         eyeCatcher[1] = o.eyeCatcher[1];
      }

      OSS_INLINE pageHead &operator=(const pageHead &o)
      {
         eyeCatcher[0] = o.eyeCatcher[0];
         eyeCatcher[1] = o.eyeCatcher[1];
         version = o.version;
         checksum = o.checksum;
         type = o.type;
         flags = o.flags;
         size = o.size;
         pid = o.pid;
         lpid = o.lpid;
         psv = o.psv;
         lsn = o.lsn;
         reserved = o.reserved;
         return *this;
      }

      OSS_INLINE void reset()
      {
         eyeCatcher[0] = 0;
         eyeCatcher[1] = 0;
         version = INVALID_PAGE_VERSION;
         checksum = 0;
         type = INVALID_PAGE_TYPE;
         flags = 0;
         size = 0;
         pid = INVALID_PAGE_ID;
         lpid = INVALID_PAGE_ID;
         lsn = DPS_INVALID_LSN_OFFSET;
         psv = INVALID_PAGE_SNAPSHOT_VERSION;
         reserved = 0;
         return;
      }

      CHAR eyeCatcher[2] = {};
      UINT16 version = 0;
      UINT32 checksum = 0;
      UINT16 type = INVALID_PAGE_TYPE;
      UINT16 flags = 0;
      UINT32 size = 0;
      UINT32 pid = INVALID_PAGE_ID;
      UINT32 lpid = INVALID_PAGE_ID;
      UINT32 psv = INVALID_PAGE_SNAPSHOT_VERSION;
      UINT64 lsn = DPS_INVALID_LSN_OFFSET;
      UINT64 reserved = 0;
   };// struct pageHead
#pragma pack()
   constexpr UINT32 PAGE_HEAD_SIZE = sizeof(pageHead);
   constexpr UINT32 PAGE_TAIL_SIZE = sizeof(UINT64);

   UINT32 getPageBodySize(UINT32 pageSize);

   BOOLEAN isValidPageSize(UINT32 pageSize);

   BOOLEAN isPageCrashed(ossValuePtr ptr, UINT32 pageSize);

   INT32 validatePage(ossValuePtr ptr,
                        PAGE_TYPE type,
                        UINT32 pageSize,
                        PAGE_ID pid,
                        PAGE_ID lpid,
                        PAGE_SNAPSHOT_VERION psv);

   BOOLEAN initCommonPage(UINT16 pageType,
                          UINT32 pageSize,
                          PAGE_ID pid,
                          PAGE_ID lpid,
                          PAGE_SNAPSHOT_VERION psv,
                          void *buf);

   /// page head valid.
   BOOLEAN updatePageLsn(ossValuePtr ptr,
                         DPS_LSN_OFFSET lsn);

}/// end of namespace vessel
} /// end of namespace engine

#endif // VESSEL_PAGE_DEF_H_
