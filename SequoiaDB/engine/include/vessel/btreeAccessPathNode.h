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

   Source File Name = btreeAccessPathNode.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_BTREE_ACCESS_PATH_NODE_H_
#define VESSEL_BTREE_ACCESS_PATH_NODE_H_

#include "vessel/pageIdentifier.h"
#include "ossSharedLatch.hpp"
#include "vessel/logicalPageBuffer.h"
#include "vessel/btreePathFootprint.h"

namespace engine
{
namespace vessel
{
   class btreeAccessPathNode : public SDBObject
   {
      public:
         btreeAccessPathNode(){}
         ~btreeAccessPathNode(){}
         explicit btreeAccessPathNode(logicalPageBuffer *lpb);
         btreeAccessPathNode(const btreeAccessPathNode &o):
         _lpid(o._lpid),
         _splitedTimes(o._splitedTimes),
         _lpb(o._lpb),
         _footprint(o._footprint){}
         btreeAccessPathNode &operator=(const btreeAccessPathNode &o)
         {
            _lpid = o._lpid;
            _splitedTimes = o._splitedTimes;
            _lpb = o._lpb;
            _footprint = o._footprint;
            return *this;
         }

      public:
         OSS_INLINE PAGE_ID getLogicalPageId()const
         {
            return _lpid;
         }
         OSS_INLINE UINT32 getSplitedTimes()const
         {
            return _splitedTimes;
         }
         OSS_INLINE logicalPageBuffer *getPageBuffer()
         {
            return _lpb;
         }
         OSS_INLINE const logicalPageBuffer *getPageBuffer()const
         {
            return _lpb;
         }
      
         OSS_INLINE BOOLEAN isValid()const
         {
            return INVALID_PAGE_ID != _lpid;
         }
         OSS_INLINE BOOLEAN isAccessing()const
         {
            return NULL != _lpb;
         }
         OSS_INLINE void endToAccess()
         {
            _lpb = NULL;
         }
         void reaccess(logicalPageBuffer *lpb);
         
         OSS_INLINE ossSharedLatchMode getNodeMode()const
         {
            SDB_ASSERT(isAccessing(), "must be accessing");
            return _lpb->getLockingMode();
         }

         OSS_INLINE const btreePathFootprint &getChildFootprint()const
         {
            return _footprint;
         }
         void setChildFootprint(const btreePathFootprint &fp);
         void clearChildFootprint()
         {
            _footprint = btreePathFootprint();
         }
         
      private:
         PAGE_ID _lpid = INVALID_PAGE_ID;
         UINT32 _splitedTimes = 0;
         logicalPageBuffer *_lpb = NULL;
         btreePathFootprint _footprint;
   };//btreeAccessPathNode
} // namespace vessel

} // namespace engine


#endif//VESSEL_BTREE_ACCESS_PATH_NODE_H_