/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = pageIdentifier.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_PAGE_IDENTIFIER_H_
#define VESSEL_PAGE_IDENTIFIER_H_

#include "core.hpp"
#include "oss.hpp"

namespace engine
{
namespace vessel
{
   typedef UINT32 PAGE_ID;
   constexpr PAGE_ID INVALID_PAGE_ID = OSS_UINT32_MAX;

   /// long page id.
   typedef UINT64 LONG_PAGE_ID;
   constexpr LONG_PAGE_ID INVALID_LONG_PAGE_ID = OSS_UINT64_MAX;

#pragma pack(4)

   class mappedLogicalPageId : public SDBObject
   {
      public:
         mappedLogicalPageId() = default;
         explicit mappedLogicalPageId(PAGE_ID lpid, PAGE_ID pid):
         _lpid(lpid),
         _pid(pid){}
         explicit mappedLogicalPageId(UINT64 v)
         {
            _lpid = v;
            _pid = (v >> 32);
         }
         mappedLogicalPageId(const mappedLogicalPageId &o) = default;
         mappedLogicalPageId &operator=(const mappedLogicalPageId &) = default;

         mappedLogicalPageId &operator=(UINT64 v)
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
      pageIdentifier() = default;
      ~pageIdentifier() = default;

      explicit pageIdentifier(PAGE_ID pid):
      _pid(pid){}

      pageIdentifier(const pageIdentifier &) = default;
      pageIdentifier &operator=(const pageIdentifier &) = default;
      pageIdentifier &operator=(PAGE_ID pid)
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
         return _pid != pid;
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