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
#include "utilArray.hpp"
#include "utilPooledObject.hpp"
#include "vessel/indexObject.h"

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

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return isValidIndexSlot(_indexSlot);
         }

         void init(INT32 indexSlot,
                   const indexObject &obj,
                   const bson::BSONObjSet &keys,
                   BOOLEAN getOwned=TRUE);

         void fini();


         OSS_INLINE INT32 getIndexSlot()const
         {
            return _indexSlot;
         }
         OSS_INLINE INDEX_TYPE getIndexType()const
         {
            return _obj.getIndexType();
         }
         OSS_INLINE BOOLEAN isUnique()const
         {
            return _obj.getParams().isUnique;
         }

         const ossPoolList<bson::BSONObj> &getKeys()const
         {
            return _keys;
         }

         const indexObject &getIndexObj()const
         {
            return _obj;
         }

         OSS_INLINE void setIngnored()
         {
            _ignored = TRUE;
         }
         OSS_INLINE BOOLEAN isIgnored()const
         {
            return _ignored;
         }
      private:
         INT32 _indexSlot = -1;
         ossPoolList<bson::BSONObj> _keys;
         indexObject _obj;
         BOOLEAN _ignored = FALSE;
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
            return 0 == _requests.size();
         }
         OSS_INLINE UINT32 getUniqueIndexCount()const
         {
            return _uniqueIndexCount;
         }

         dmlIndexRequest *get(UINT32 i)const;

         void clear();

         ///The appending better to be orderd as index slot.
         INT32 append(INT32 indexSlot,
                      const indexObject &obj,
                      const bson::BSONObjSet &keys,
                      BOOLEAN getOwned=TRUE);
      private:
         UINT32 _uniqueIndexCount = 0;
         _utilArray<dmlIndexRequest *> _requests;
   };//class dmlIndexRequestArray
}//namespace vessel
}//nameapace engine

#endif//VESSEL_DML_INDEX_REQUEST_H_