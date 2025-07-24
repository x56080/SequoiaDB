/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = dmlRequest.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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