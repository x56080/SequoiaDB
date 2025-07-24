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

   Source File Name = freeListPage.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_FREE_LIST_PAGE_H_
#define VESSEL_FREE_LIST_PAGE_H_

#include "oss.hpp"
#include "ossTypes.h"

namespace engine
{
namespace vessel
{
   class freeListPage : public SDBObject
   {
      public:
         OSS_INLINE freeListPage()
         :_chunkId(UINT32(-1)),
          _buf(0)
         {}

         OSS_INLINE freeListPage(const freeListPage &r)
         :_chunkId(r._chunkId), _buf(r._buf)
         {}

         OSS_INLINE freeListPage(UINT32 chunk, ossValuePtr buf, INT32 bitPos)
         :_chunkId(chunk), _buf(buf){}
         
         OSS_INLINE ~freeListPage(){}

         OSS_INLINE freeListPage &operator=(const freeListPage &r)
         {
            _chunkId = r._chunkId;
            _buf = r._buf;
            return *this;
         }

         OSS_INLINE BOOLEAN operator<(const freeListPage &r)
         {
            if (_chunkId < r._chunkId)
            {
               return TRUE;
            }
            else if (_chunkId > r._chunkId)
            {
               return FALSE;
            }
            else
            {
               return _buf < r._buf;
            }
         }

         OSS_INLINE void reset()
         {
            _chunkId = UINT32(-1);
            _buf = 0;
         }

         OSS_INLINE BOOLEAN valid()const
         {
            return 0 != _buf;
         }

         OSS_INLINE void set(UINT32 chunkid, ossValuePtr buf)
         {
            _chunkId = chunkid;
            _buf = buf;
         }

         OSS_INLINE UINT32 getChunkId()const
         {
            return _chunkId;
         }

         OSS_INLINE ossValuePtr getBuf()const
         {
            return _buf;
         }

      private:
         UINT32 _chunkId;
         ossValuePtr _buf;
   }; /// end of class freeListPage

   
} /// end of namespace vessel
} /// end of namespace engine

#endif//VESSEL_FREE_LIST_PAGE_H_