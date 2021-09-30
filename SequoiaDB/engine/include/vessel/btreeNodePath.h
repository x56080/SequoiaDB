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

   Source File Name = btreeNodePath.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_BTREE_NODE_PATH_H_
#define VESSEL_BTREE_NODE_PATH_H_

#include "vessel/btreeNode.h"
#include "ossMemPool.hpp"
#include "vessel/logicalPageBuffer.h"

namespace engine
{
namespace vessel
{
   class indexContext;

   class btreeNodePath : public SDBObject
   {
      public:
         btreeNodePath() = delete;
         btreeNodePath(const indexContext *ic);
         ~btreeNodePath();
         btreeNodePath(const btreeNodePath &) = delete;
         btreeNodePath &operator=(const btreeNodePath &) = delete;

      private:
         class _pathNode : public SDBObject
         {
            public:
               _pathNode(){}
               explicit _pathNode(logicalPageBuffer *lpb,
                                  UINT32 splitedTimes);
               ~_pathNode();
               _pathNode(const _pathNode &o):
               _lpid(o._lpid),
               _splitedTimes(o._splitedTimes),
               _lpb(o._lpb){}
               _pathNode &operator=(const _pathNode &o)
               {
                  _lpid = o._lpid;
                  _splitedTimes = o._splitedTimes;
                  _lpb = o._lpb;
                  return *this;
               }

            public:
               void fini();
               OSS_INLINE BOOLEAN isAccessing()const
               {
                  return NULL != _lpb;
               }

            public:
               PAGE_ID _lpid = INVALID_PAGE_ID;
               UINT32 _splitedTimes = 0;
               logicalPageBuffer *_lpb = NULL;
               
         };// class _pathNode

      public:
         OSS_INLINE UINT32 getSize()const
         {
            return _size;
         }
         OSS_INLINE BOOLEAN isEmpty()const
         {
            return 0 == _size;
         }

      public:
         static const UINT32 ROOT_DEPTH = 0;

      public:
         void fini();

         logicalPageBuffer *allocateBuffer();
         void releaseBuffer(logicalPageBuffer *buffer);

         void clearPath();

         INT32 push(logicalPageBuffer *buffer,
                    btreeNode *out=NULL);

         btreeNode getCurrentEndNodeInPath()const;

      private:
         _pathNode &getPathNode(UINT32 i);
         const _pathNode &getPathNode(UINT32 i)const;

      private:
         const indexContext *_ic = NULL;
         typedef ossPoolVector<logicalPageBuffer *> _FREE_BUFFERS;
         _FREE_BUFFERS _free;

         static const UINT32 _DEFAULT_CAPACITY = 4;
         UINT32 _size = 0;
         _pathNode _staticNodes[_DEFAULT_CAPACITY];
         ossPoolVector<_pathNode> _dynamicNodes;
   };//class btreeNodePath
} // namespace vessel

} // namespace engine


#endif//VESSEL_BTREE_NODE_PATH_H_