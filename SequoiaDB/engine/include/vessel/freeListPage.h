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

   Source File Name = freeListPage.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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
         :_chunk(UINT32(-1)),
          _buf(0)
         {}

         OSS_INLINE freeListPage(const freeListPage &r)
         :_chunk(r._chunk), _buf(r._buf)
         {}

         OSS_INLINE freeListPage(UINT32 chunk, ossValuePtr buf)
         :_chunk(chunk), _buf(buf){}
         
         OSS_INLINE ~freeListPage(){}

         OSS_INLINE freeListPage &operator=(const freeListPage &r)
         {
            _chunk = r._chunk;
            _buf = r._buf;
            return *this;
         }

         OSS_INLINE BOOLEAN operator<(const freeListPage &r)
         {
            if (_chunk < r._chunk)
            {
               return TRUE;
            }
            else if (_chunk > r._chunk)
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
            _chunk = UINT32(-1);
            _buf = 0;
         }

         OSS_INLINE BOOLEAN valid()const
         {
            return 0 != _buf;
         }

         OSS_INLINE void set(UINT32 chunk, ossValuePtr buf)
         {
            _chunk = chunk;
            _buf = buf;
         }

         OSS_INLINE UINT32 chunk()const
         {
            return _chunk;
         }

         OSS_INLINE ossValuePtr buf()const
         {
            return _buf;
         }

      private:
         UINT32 _chunk;
         ossValuePtr _buf;
   }; /// end of class freeListPage

   
} /// end of namespace vessel
} /// end of namespace engine

#endif//VESSEL_FREE_LIST_PAGE_H_