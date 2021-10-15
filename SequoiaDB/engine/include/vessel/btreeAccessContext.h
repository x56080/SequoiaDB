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

#include "vessel/recordID.h"
#include "ixmKey.hpp"
#include "utilArray.hpp"
#include "vessel/btreeAccessPathNode.h"
#include "vessel/btreeNode.h"

namespace engine
{
namespace vessel
{
   class logicalPageBuffer;
   class indexContext;

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
         OSS_INLINE const ixmKey &getKey()const
         {
            return _key;
         }
         OSS_INLINE const recordID &getRid()const
         {
            return _rid;
         }
         OSS_INLINE const DPS_TRANS_ID &getTransID()const
         {
            return _transID;
         }

         OSS_INLINE BOOLEAN isPathEmpty()const
         {
            return _path.empty();
         }
         

      public:
         void init(const indexContext *ic,
                   const ixmKey &key,
                   const recordID *rid=NULL,
                   const DPS_TRANS_ID *transID=NULL);
         void fini();
         logicalPageBuffer *allocateBuffer();
         void releaseBuffer(logicalPageBuffer *buffer);

         INT32 pushIntoPath(logicalPageBuffer *buffer);

         void clearAccessPath();

         btreeNode getEndNodeInPath();

      private:
         typedef ossPoolVector<logicalPageBuffer *> _FREE_BUFFERS;

      private:
         const indexContext *_ic = NULL;
         ixmKey _key;
         recordID _rid;
         DPS_TRANS_ID _transID;

         _utilArray<btreeAccessPathNode, 4> _path;
         _FREE_BUFFERS _free;
   };//class btreeAccessContext
} // namespace vessel

} // namespace engine


#endif//VESSEL_BTREE_ACCESS_CONTEXT_H_