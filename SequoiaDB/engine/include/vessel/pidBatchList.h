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

   Source File Name = pidBatchList.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/
#ifndef VESSEL_PID_BATCH_LIST_H_
#define VESSEL_PID_BATCH_LIST_H_

#include "vessel/pageIdentifier.h"
#include "ossMemPool.hpp"

namespace engine
{
namespace vessel
{
   class pidBatchList : public SDBObject
   {
      public:
         pidBatchList(){}
         ~pidBatchList(){}
         pidBatchList(const pidBatchList &) = delete;
         pidBatchList &operator=(const pidBatchList &) = delete;


      public:
         typedef ossPoolVector<PAGE_ID> BATCH;
         typedef ossPoolList<BATCH> BATCH_LIST;

      public:
         OSS_INLINE BATCH_LIST::const_iterator begin()const {return _bl.cbegin();}
         OSS_INLINE BATCH_LIST::const_iterator end()const {return _bl.cend();}
         OSS_INLINE BOOLEAN isEmpty()const {return _bl.empty();}
         OSS_INLINE void clear() {_bl.clear();}
         void push(PAGE_ID pid);
         void transferTo(pidBatchList &o);

      private:
         BATCH_LIST _bl;
   };//class pidBatchList
} // namespace vessel

} // namespace engine


#endif//VESSEL_PID_BATCH_LIST_H_