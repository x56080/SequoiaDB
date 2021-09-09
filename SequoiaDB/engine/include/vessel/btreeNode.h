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

   Source File Name = btreeNode.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_BTREE_NODE_H_
#define VESSEL_BTREE_NODE_H_

#include "vessel/btreeNodePage.h"
#include "ixmKey.hpp"
#include "vessel/btreeIndexTuple.h"
#include "vessel/btreeIndexDef.h"
#include "vessel/logicalPageBuffer.h"

namespace engine
{
namespace vessel
{
   class requestContext;
   class indexContext;

   class btreeNode : public SDBObject
   {
      friend class btreeNodePath;
      public:
         btreeNode(){}
         ~btreeNode(){}
         btreeNode(const btreeNode &o):
         _buffer(o._buffer),
         _depth(o._depth)
         {}
         btreeNode &operator=(const btreeNode &o)
         {
            _buffer = o._buffer;
            _depth = o._depth;
            return *this;
         }

      private:
         explicit btreeNode(logicalPageBuffer *buffer,
                            indexContext *ic,
                            UINT32 depth);

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return NULL != _buffer && _buffer->isValid();
         }

         OSS_INLINE UINT32 getDepth()const
         {
            return _depth;
         }
      public:
         void reset();

         BOOLEAN isRoot()const;

      public:/// node must be active
         BOOLEAN hasExtNode()const;

         BOOLEAN isLeaf()const;

         INT32 search(const ixmKey &key,
                      const recordID &rid,
                      RECORD_SLOT_ID &slotNo,
                      BOOLEAN &identical)const;

         INT32 getIndexTuple(RECORD_SLOT_ID slotNo,
                             btreeIndexTuple &tuple)const;

      private:
         const btreeNodePageHead *getReadbleHead()const;
      private:
         logicalPageBuffer *_buffer = NULL;
         indexContext *_ic = NULL;
         UINT32 _depth = 0;
      
   };//class btreeNode
} // namespace vessel

} // namespace engine


#endif//VESSEL_BTREE_NODE_H_