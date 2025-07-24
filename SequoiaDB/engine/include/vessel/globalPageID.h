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

   Source File Name = globalPageID.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_GLOBAL_PAGE_ID_H_
#define VESSEL_GLOBAL_PAGE_ID_H_

#include "vessel/vesselIdDef.h"
#include "vessel/pageDef.h"
#include "vessel/vesselFileDef.h"
#include "xxHashInc.h"
#include "../../bson/util/builder.h"

namespace engine
{
namespace vessel
{

#pragma pack(4)

class globalPageID
{
   public:
      OSS_INLINE globalPageID(){}
      OSS_INLINE ~globalPageID(){}

      OSS_INLINE explicit globalPageID(SPACE_ID sid,
                                       SPACE_TYPE spaceType,
                                       FILE_TYPE fileType,
                                       PAGE_ID pid)
      :_sid(sid),
       _spaceType(spaceType),
       _fileType(fileType),
       _pid(pid)
      {}

      OSS_INLINE globalPageID(const globalPageID &r)
      :_sid(r._sid),
       _spaceType(r._spaceType),
       _fileType(r._fileType),
       _pid(r._pid)
      {}

      OSS_INLINE globalPageID &operator=(const globalPageID &r)
      {
         *((UINT64 *)(this)) = *((const UINT64 *)(&r));
         return *this;
      }

      OSS_INLINE BOOLEAN isValid()const
      {
         return INVALID_SPACE_ID != _sid &&
                INVALID_SPACE_TYPE != _spaceType &&
                INVALID_FILE_TYPE != _fileType &&
                INVALID_PAGE_ID != _pid;
      }

      OSS_INLINE void reset(SPACE_ID sid,
                            SPACE_TYPE spaceType,
                            FILE_TYPE fileType,
                            PAGE_ID pid)
      {
         _sid = sid;
         _spaceType = spaceType;
         _fileType = fileType;
         _pid = pid;
         return;
      }

      OSS_INLINE void reset()
      {
         reset(INVALID_SPACE_ID, INVALID_SPACE_TYPE,
               INVALID_FILE_TYPE, INVALID_PAGE_ID);
         return;
      }

      OSS_INLINE UINT32 hash()const
      {
         return XXH3_64bits(this, sizeof(globalPageID));
         //return _sid + _pid;
      }

      OSS_INLINE BOOLEAN operator==(const globalPageID &r)const
      {
         return *((const UINT64 *)(this)) == *((const UINT64 *)(&r));
      }

      OSS_INLINE BOOLEAN operator!=(const globalPageID &r)const
      {
         return *((const UINT64 *)(this)) != *((const UINT64 *)(&r));
      }

      OSS_INLINE INT32 compare(const globalPageID &r)const
      {
         ///Do not cast to uint64 to compare.
         ///Ordered columns in turns. If we scan the gpids of
         /// specified space id in a ordered map, we can know
         /// where to stop.
         INT32 res = 0;
         if (_sid < r._sid)
         {
            res = -1;
            goto done;
         }
         else if (_sid > r._sid)
         {
            res = 1;
            goto done;
         }

         if (_spaceType < r._spaceType)
         {
            res = -1;
            goto done;
         }
         else if (_spaceType > r._spaceType)
         {
            res = 1;
            goto done;
         }

         if (_fileType < r._fileType)
         {
            res = -1;
            goto done;
         }
         else if (_fileType > r._fileType)
         {
            res = 1;
            goto done;
         }

         if (_pid < r._pid)
         {
            res = -1;
         }
         else if (_pid > r._pid)
         {
            res = 1;
         }
      done:
         return res;
      }

      OSS_INLINE BOOLEAN operator<(const globalPageID &r)const
      {
         return compare(r) < 0;
      }

      OSS_INLINE SPACE_ID space() const
      {
         return _sid;
      }
      OSS_INLINE SPACE_ID getSpaceId()const
      {
         return _sid;
      }

      OSS_INLINE FILE_TYPE getFileType() const
      {
         return _fileType;
      }

      OSS_INLINE PAGE_ID page()const
      {
         return _pid;
      }

      OSS_INLINE PAGE_ID getPageId()const
      {
         return _pid;
      }
      OSS_INLINE SPACE_TYPE getSpaceType()const
      {
         return _spaceType;
      }

      ossPoolString toString()const
      {
         bson::StringBuilder builder(128);
         builder << "{sid:" << _sid << ",stype:" << _spaceType
                 << ",ftype:" << _fileType << ",pid:" << _pid << '}';
         return std::move(builder.poolStr());
      }

   public:
      SPACE_ID _sid = INVALID_SPACE_ID;
      SPACE_TYPE _spaceType = INVALID_SPACE_TYPE;
      FILE_TYPE _fileType = INVALID_FILE_TYPE;
      PAGE_ID _pid = INVALID_PAGE_ID;
}; /// end of globalPageID

#pragma pack()
static const UINT32 GLOBAL_PAGE_ID_SIZE = sizeof(globalPageID);

typedef globalPageID GLOBAL_PAGE_ID;

struct GLOBAL_PAGE_ID_LESS
{
   OSS_INLINE BOOLEAN operator()(const GLOBAL_PAGE_ID &l,
                                 const GLOBAL_PAGE_ID &r)const
   {
      return l < r;
   }
};//struct GLOBAL_PAGE_ID_LESS

} /// end of namespace vessel
} /// end of namespace engine

#endif//VESSEL_GLOBAL_PAGE_ID_H_
