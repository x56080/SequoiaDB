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

   Source File Name = dmlIndexRequest.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_DML_INDEX_REQUEST_H_
#define VESSEL_DML_INDEX_REQUEST_H_

#include "vessel/indexHandle.h"
#include "../bson/bson.hpp"
#include "ossMemPool.hpp"
#include "utilPooledObject.hpp"
#include "vessel/indexContext.h"

namespace engine
{
namespace vessel
{
   class dmlIndexRequest : public _utilPooledObject
   {
      public:
         dmlIndexRequest(){}
         ~dmlIndexRequest(){}
         dmlIndexRequest(const dmlIndexRequest &) = delete;
         dmlIndexRequest &operator=(const dmlIndexRequest &)const;

      private:
         static constexpr UINT32 FLAG_EXECUTED = 0x01;

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return NULL != _index;
         }

         void init(indexContext *index,
                   const bson::BSONObjSet &keys);

         void fini();

         const ossPoolList<bson::BSONObj> &getKeys()const
         {
            return _keys;
         }

         indexContext *getContext()const
         {
            return _index;
         }

         OSS_INLINE void setExecuted()
         {
            OSS_BIT_SET(_flags, FLAG_EXECUTED);
         }

         OSS_INLINE BOOLEAN isExecuted()const
         {
            return 0 != OSS_BIT_TEST(_flags, FLAG_EXECUTED);
         }

         OSS_INLINE BOOLEAN withConstraint()const
         {
            SDB_ASSERT(isValid(), "must be valid");
            return _index->getObj().getParams().isUnique &&
                   _index->isNormal();
         }
      private:
         indexContext *_index = NULL;
         ossPoolList<bson::BSONObj> _keys;
         UINT32 _flags = 0;
   };//class dmlIndexRequest

   class dmlIndexRequestArray : public SDBObject
   {
      public:
         dmlIndexRequestArray(){}
         ~dmlIndexRequestArray();
         dmlIndexRequestArray(const dmlIndexRequestArray &) = delete;
         dmlIndexRequestArray &operator=(const dmlIndexRequestArray &) = delete;

      public:
         /// Not all elements are not null.
         OSS_INLINE UINT32 getSize()const
         {
            return _requests.size();
         }
         OSS_INLINE BOOLEAN isEmpty()const
         {
            return _requests.empty();
         }

         dmlIndexRequest *get(UINT32 i);

         const dmlIndexRequest *get(UINT32 i)const;

         void clear();

         ///The appending better to be orderd as index slot.
         INT32 append(indexContext *index,
                      const bson::BSONObjSet &keys);

         BOOLEAN withConstraint()const
         {
            return 0 < _constraintIndexCount;
         }
         BOOLEAN hasBuildingIndex()const
         {
            return 0 < _building;
         }
      private:
         ossPoolVector<dmlIndexRequest *> _requests;
         UINT32 _constraintIndexCount = 0;
         UINT32 _building = 0;
   };//class dmlIndexRequestArray
}//namespace vessel
}//nameapace engine

#endif//VESSEL_DML_INDEX_REQUEST_H_