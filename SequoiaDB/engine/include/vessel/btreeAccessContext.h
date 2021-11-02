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

   Source File Name = btreeAccessContext.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_BTREE_ACCESS_CONTEXT_H_
#define VESSEL_BTREE_ACCESS_CONTEXT_H_

#include "utilArray.hpp"
#include "vessel/btreeAccessPathNode.h"
#include "vessel/btreeNode.h"

namespace engine
{
namespace vessel
{
   class requestContext;
   class logicalPageBuffer;
   class indexContext;
   class indexSpace;

   class btreeAccessContext : public SDBObject
   {
      public:
         btreeAccessContext(){}
         ~btreeAccessContext();
         btreeAccessContext(const btreeAccessContext &) = delete;
         btreeAccessContext &operator=(const btreeAccessContext &) = delete;

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return NULL != _ic;
         }

         OSS_INLINE BOOLEAN isPathEmpty()const
         {
            return _path.empty();
         }
         OSS_INLINE BOOLEAN isPessimistic()const
         {
            return _pessimistic;
         }
         OSS_INLINE void setPessimistic(BOOLEAN v)
         {
            _pessimistic = v;
         }

         OSS_INLINE BOOLEAN isReadonly()const
         {
            return _readonly;
         }
         OSS_INLINE void setReadonly(BOOLEAN v)
         {
            _readonly = v;
         }
         OSS_INLINE indexContext *getIndexContext()
         {
            return _ic;
         }

      public:
         void init(indexContext *ic,
                   requestContext *context,
                   indexSpace *is);
         void fini();

         INT32 pushRootIntoPath(btreeNode *node=NULL);

         INT32 pushChildNodeIntoPath(PAGE_ID lpid, btreeNode *node=NULL);
         
         void clearAccessPath();

         void endToAccessNonPathEndNodes();

         void popEnd();

         btreeNode getEndNodeInPath();
         UINT32 getPathSize()const;
         btreeNode getNodeInPath(UINT32 depth);
         BOOLEAN isStillAccessing(UINT32 depth)const;

      private:
         INT32 pushIntoPath(logicalPageBuffer *buffer);
         INT32 validateBtreePage(const logicalPageBuffer &buffer)const;
         ossSharedLatchMode estimateRootMode()const;
         ossSharedLatchMode estimateChildMode()const;
         logicalPageBuffer *allocateBuffer();
         void releaseBuffer(logicalPageBuffer *buffer);
      private:
         typedef ossPoolVector<logicalPageBuffer *> _FREE_BUFFERS;

      private:
         indexContext *_ic = NULL;
         requestContext *_context = NULL;
         indexSpace *_is = NULL;

         BOOLEAN _readonly = TRUE;
         BOOLEAN _pessimistic = FALSE;

         _utilArray<btreeAccessPathNode, 4> _path;
         _FREE_BUFFERS _free;

         
   };//class btreeAccessContext
} // namespace vessel

} // namespace engine


#endif//VESSEL_BTREE_ACCESS_CONTEXT_H_