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

   Source File Name = pageIdentifier.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_PAGE_IDENTIFIER_H_
#define VESSEL_PAGE_IDENTIFIER_H_

#include "core.hpp"
#include "oss.hpp"

namespace engine
{
namespace vessel
{
   typedef UINT32 PAGE_ID;
   constexpr PAGE_ID INVALID_PAGE_ID = UINT32(-1);

   /// long page id.
   typedef UINT64 LONG_PAGE_ID;
   constexpr LONG_PAGE_ID INVALID_LONG_PAGE_ID = OSS_UINT64_MAX;

#pragma pack(4)

   class mappedLogicalPageId : public SDBObject
   {
      public:
         OSS_INLINE mappedLogicalPageId(){}
         OSS_INLINE mappedLogicalPageId(const mappedLogicalPageId &o):
         _lpid(o._lpid),
         _pid(o._pid){}
         OSS_INLINE explicit mappedLogicalPageId(PAGE_ID lpid, PAGE_ID pid):
         _lpid(lpid),
         _pid(pid){}
         OSS_INLINE explicit mappedLogicalPageId(UINT64 v)
         {
            _lpid = v;
            _pid = (v >> 32);
         }

         OSS_INLINE ~mappedLogicalPageId()
         {
            _lpid = INVALID_PAGE_ID;
            _pid = INVALID_PAGE_ID;
         }

         OSS_INLINE mappedLogicalPageId &operator=(const mappedLogicalPageId &o)
         {
            _lpid = o._lpid;
            _pid = o._pid;
            return *this;
         }

         OSS_INLINE mappedLogicalPageId &operator=(UINT64 v)
         {
            _lpid = v;
            _pid = (v >> 32);
            return *this;
         }

         OSS_INLINE BOOLEAN operator==(const mappedLogicalPageId &o)const
         {
            return _lpid == o._lpid &&
                   _pid == o._pid;
         }

      public:
         OSS_INLINE PAGE_ID getLpid()const {return _lpid;}
         OSS_INLINE PAGE_ID getPid()const {return _pid;}
         OSS_INLINE BOOLEAN isValid()const
         {
            return INVALID_PAGE_ID != _lpid &&
                   INVALID_PAGE_ID != _pid;
         }
         OSS_INLINE UINT64 dumpAsUint64()const
         {
            UINT64 v = _pid;
            v = v << 32;
            v |= _lpid;
            return v;
         }
         OSS_INLINE void reset(PAGE_ID lpid, PAGE_ID pid)
         {
            _lpid = lpid;
            _pid = pid;
            return;
         }
      private:
         PAGE_ID _lpid = INVALID_PAGE_ID;
         PAGE_ID _pid = INVALID_PAGE_ID;
   };//class mappedLogicalPageId

   class pageIdentifier : public SDBObject
   {
      OSS_INLINE pageIdentifier(){}
      OSS_INLINE ~pageIdentifier(){}

      OSS_INLINE pageIdentifier(const pageIdentifier &o):
      _pid(o._pid){}

      OSS_INLINE explicit pageIdentifier(PAGE_ID pid):
      _pid(pid){}

      OSS_INLINE pageIdentifier &operator=(const pageIdentifier &o)
      {
         _pid = o._pid;
         return *this;
      }

      OSS_INLINE pageIdentifier &operator=(PAGE_ID pid)
      {
         _pid = pid;
         return *this;
      }

      OSS_INLINE BOOLEAN operator==(const pageIdentifier &o)const
      {
         return _pid == o._pid;
      }

      OSS_INLINE BOOLEAN operator!=(const pageIdentifier &o)const
      {
         return _pid != o._pid;
      }

      OSS_INLINE BOOLEAN operator==(PAGE_ID pid)const
      {
         return _pid == pid;
      }

      OSS_INLINE BOOLEAN operator!=(PAGE_ID pid)const
      {
         return _pid != _pid;
      }

      OSS_INLINE void reset(PAGE_ID pid = INVALID_PAGE_ID)
      {
         _pid = pid;
         return;
      }

      OSS_INLINE BOOLEAN isValid()const
      {
         return INVALID_PAGE_ID != _pid;
      }

      private:
         PAGE_ID _pid = INVALID_PAGE_ID;
   };

#pragma pack()
}//namespace vessel
}//namespace engine


#endif//VESSEL_PAGE_IDENTIFIER_H_