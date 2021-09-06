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

namespace engine
{
namespace vessel
{
   class logicalPageBuffer;
   class requestContext;
   class btreeNodePath;

   class btreeNode : public SDBObject
   {
      friend class btreeNodePath;
      public:
         btreeNode(){}
         ~btreeNode(){}
         btreeNode(const btreeNode &o):
         _buffer(o._buffer),
         _path(o._path),
         _depth(o._depth)
         {}
         btreeNode &operator=(const btreeNode &o)
         {
            _buffer = o._buffer;
            _path = o._path;
            _depth = o._depth;
            return *this;
         }

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return NULL != _buffer;
         }

      public:

         void fini();

         BOOLEAN isRoot()const;

         BOOLEAN hasExtNode()const;

         INT32 insert(requestContext *context,
                      const ixmKey &key,
                      const recordID &rid,
                      DPS_LSN_OFFSET lsn,
                      const DPS_TRANS_ID &transID);

      private:
         explicit btreeNode(btreeNodePath *path,
                            logicalPageBuffer *lpb,
                            UINT32 depth);

         const btreeNodePageHead *getReadbleHead()const;
      private:
         logicalPageBuffer *_buffer = NULL;
         btreeNodePath *_path = NULL;
         UINT32 _depth = 0;
      
   };//class btreeNode
} // namespace vessel

} // namespace engine


#endif//VESSEL_BTREE_NODE_H_