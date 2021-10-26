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

   Source File Name = btreeNodePageIniter.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_BTREE_NODE_PAGE_INITER_H_
#define VESSEL_BTREE_NODE_PAGE_INITER_H_

#include "vessel/pageInitializer.h"
#include "dms.hpp"
#include "vessel/indexDef.h"
#include "vessel/slice.h"

namespace engine
{
namespace vessel
{
   class btreeRootPageIniter : public pageInitializer
   {
      public:
         btreeRootPageIniter(){}
         virtual ~btreeRootPageIniter(){}

      public:
         virtual INT32 initPage(requestContext *context,
                                PAGE_ID lpid,
                                PAGE_SNAPSHOT_VERION psv,
                                runtimePageBuffer *rpb);

         void set(UINT32 clid, UINT32 indexId);

      private:
         UINT32 _logicalCLID = DMS_INVALID_LOGICCLID;
         UINT32 _indexId = INVALID_LOGICAL_INDEX_ID;
   };//class btreeRootPageIniter

   class btreeNodePageSplitIniter : public pageInitializer
   {
      public:
         btreeNodePageSplitIniter(){}
         virtual ~btreeNodePageSplitIniter(){}

      public:
         virtual INT32 initPage(requestContext *context,
                                PAGE_ID lpid,
                                PAGE_SNAPSHOT_VERION psv,
                                runtimePageBuffer *rpb);

         void set(const slice &s)
         {
            _data = s.getReadableSlice();
         }

      private:
         slice _data;
   };//class btreeNodePageSplitIniter
} // namespace vessel


} // namespace engine

#endif//VESSEL_BTREE_NODE_PAGE_INITER_H_
