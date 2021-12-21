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

   Source File Name = dmlRequest.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef SDB_VESSEL_DML_REQUEST_H_
#define SDB_VESSEL_DML_REQUEST_H_

#include "vessel/slice.h"
#include "vessel/recordID.h"
#include "ossMemPool.hpp"
#include "dmsEngineOptions.hpp"

namespace engine
{
namespace vessel
{
   class dmlInsertRequest : public SDBObject
   {
      public:
         dmlInsertRequest(){}
         ~dmlInsertRequest(){}
         dmlInsertRequest(const dmlInsertRequest &o) = delete;
         dmlInsertRequest &operator=(const dmlInsertRequest &) = delete;

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return record.isValid();
         }
      public:
         slice record;
         dmsInsertRecordOptions o;

   }; /// end of class dmlInsertRequest

   class dmlBatchInsertRequest : public SDBObject
   {
      public:
         dmlBatchInsertRequest(){}
         ~dmlBatchInsertRequest(){}
         dmlBatchInsertRequest(const dmlBatchInsertRequest &o) = delete;
         dmlBatchInsertRequest &operator=(const dmlBatchInsertRequest &) = delete;

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return !batch.empty();
         }
      public:
         ossPoolVector<slice> batch;
         dmsInsertRecordOptions o;

   }; /// end of class dmlInsertRequest

   class dmlUpdateRequest : public SDBObject
   {
      public:
         dmlUpdateRequest(){}
         ~dmlUpdateRequest(){}
         dmlUpdateRequest(const dmlUpdateRequest &) = delete;
         dmlUpdateRequest &operator=(const dmlUpdateRequest &) = delete;

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return rid.isValid();
         }
      public:
         recordID rid;
         dmsUpdateRecordOptions o;
         

   };//class dmlUpdateRequest

   class dmlRemoveRequest : SDBObject
   {
      public:
         dmlRemoveRequest(){}
         ~dmlRemoveRequest(){}
         dmlRemoveRequest(const dmlRemoveRequest &) = delete;
         dmlRemoveRequest &operator=(const dmlRemoveRequest &) = delete;

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return rid.isValid();
         }

      public:
         recordID rid;
         dmsDeleteRecordOptions o;
   };//class dmlRemoveRequest

}//namespace vessel
}//namespace engine

#endif//SDB_VESSEL_DML_REQUEST_H_