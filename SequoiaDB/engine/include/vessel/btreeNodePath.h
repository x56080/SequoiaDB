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
   class indexSpace;

   class btreeNodePath : public SDBObject
   {
      public:
         btreeNodePath(){}
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

            public:
               PAGE_ID _lpid = INVALID_PAGE_ID;
               UINT32 _splitedTimes = 0;
               logicalPageBuffer *_lpb = NULL;
               
         };// class _pathNode

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return NULL != _ic && NULL != _is;
         }
         OSS_INLINE indexContext *getIndexContext()
         {
            return _ic;
         }
         OSS_INLINE indexSpace *getIndexSpace()
         {
            return _is;
         }
         OSS_INLINE UINT32 getSize()const
         {
            return _size;
         }
         OSS_INLINE BOOLEAN isEmpty()const
         {
            return 0 == _size;
         }

         void init(indexContext *ic,
                   indexSpace *is);
         void fini();

         INT32 ensureRootToWrite(requestContext *context,
                                 btreeNode &rootNode);

         void clearWholePath();

         INT32 pushNode(requestContext *context,
                        PAGE_ID lpid,
                        const ossSharedLatchMode &mode,
                        btreeNode &out);

         BOOLEAN isRoot(const btreeNode &bn)const;

      private:
         logicalPageBuffer *allocateBuffer();
         void releaseBuffer(logicalPageBuffer *buffer);
         INT32 validateBtreePage(requestContext *context,
                                 logicalPageBuffer *buffer,
                                 const btreeNodePageHead **out=NULL)const;

      private:
         typedef ossPoolVector<logicalPageBuffer *> _FREE_BUFFER_LIST;
         static const UINT32 _DEFAULT_CAPACITY = 4;

      private:
         _pathNode &getPathNode(UINT32 i);

         INT32 createRoot(requestContext *context,
                          PAGE_ID &out);

         INT32 splitRootAndCreateNewOne(requestContext *context,
                                        btreeNode &root,
                                        btreeNode *newRoot=NULL);

      private:
         indexContext *_ic = NULL;
         indexSpace *_is = NULL;
         logicalPageBuffer _entryPage;
         _FREE_BUFFER_LIST _free;
         UINT32 _size = 0;
         _pathNode _staticNodes[_DEFAULT_CAPACITY];
         ossPoolVector<_pathNode> _dynamicNodes;
   };//class btreeNodePath
} // namespace vessel

} // namespace engine


#endif//VESSEL_BTREE_NODE_PATH_H_