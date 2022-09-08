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

#include "vessel/indexObject.h"
#include "../bson/bson.hpp"
#include "ossMemPool.hpp"
#include "utilPooledObject.hpp"


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
         dmlIndexRequest &operator=(const dmlIndexRequest &) = delete;

      private:
         static constexpr UINT32 FLAG_EXECUTED = 0x01;

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return NULL != _index;
         }
         OSS_INLINE BOOLEAN isEmpty()const
         {
            return _toInsert.empty() && _toRemove.empty();
         }

         INT32 init(indexObject *index,
                    const bson::BSONObjSet *toInsert,
                    const bson::BSONObjSet *toRemove);

         void fini();

         const ossPoolList<bson::BSONObj> &getKeysToInsert()const
         {
            return _toInsert;
         }
         const ossPoolList<bson::BSONObj> &getKeysToRemove()const
         {
            return _toRemove;
         }

         const indexObject *getObject()const
         {
            return _index;
         }

         indexObject *getMutableObject() const {return _index;}

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
            return _index->getProperties().isUnique() &&
                   _index->isNormal() &&
                   !_toInsert.empty();
         }
      private:
         mutable indexObject *_index = nullptr;
         ossPoolList<bson::BSONObj> _toInsert;
         ossPoolList<bson::BSONObj> _toRemove;
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

         INT32 append(indexObject *index,
                      const bson::BSONObjSet *keysToInsert,
                      const bson::BSONObjSet *keysToRemove);

         BOOLEAN withConstraint()const
         {
            return 0 < _constraintIndexCount;
         }
         BOOLEAN hasBuildingIndex()const
         {
            return 0 < _building;
         }

         const ossPoolVector<dmlIndexRequest *> &getRequests()const {return _requests;}
      private:
         ossPoolVector<dmlIndexRequest *> _requests;
         UINT32 _constraintIndexCount = 0;
         UINT32 _building = 0;
   };//class dmlIndexRequestArray
}//namespace vessel
}//nameapace engine

#endif//VESSEL_DML_INDEX_REQUEST_H_