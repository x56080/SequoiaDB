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

   Source File Name = IIndex.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SDB_I_INDEX_HPP_
#define SDB_I_INDEX_HPP_

#include "sdbInterface.hpp"
#include "interface/ICursor.hpp"
#include "keystring/utilKeyString.hpp"
#include "dms.hpp"
#include "dmsRecord.hpp"
#include "dmsOprtOptions.hpp"
#include "utilInsertResult.hpp"
#include "utilPooledObject.hpp"

namespace engine
{

   /*
      _IIndex define
    */
   class IIndex : public _utilPooledObject,
                  public std::enable_shared_from_this<IIndex>
   {
   public:
      IIndex() = default ;
      virtual ~IIndex() = default ;
      IIndex( const IIndex & ) = delete ;
      IIndex &operator =( const IIndex & ) = delete ;

      virtual const dmsIdxMetadata &getMetadata() const = 0 ;
      virtual dmsIdxMetadata &getMetadata() = 0 ;
      virtual UINT64 fetchSnapshotID() = 0 ;

      virtual INT32 truncate( const dmsTruncateIdxOptions &options,
                              IExecutor *executor ) = 0 ;
      virtual INT32 compact( const dmsCompactIdxOptions &options,
                             IExecutor *executor ) = 0 ;

      virtual INT32 index( const bson::BSONObj &key,
                           const dmsRecordID &rid,
                           BOOLEAN allowDuplicated,
                           IExecutor *executor,
                           utilWriteResult *result ) = 0 ;
      virtual INT32 unindex( const bson::BSONObj &key,
                             const dmsRecordID &rid,
                             IExecutor *executor ) = 0 ;

      virtual INT32 createIndexCursor( std::unique_ptr<IIndexCursor> &cursor,
                                       const keystring::keyString &startKey,
                                       BOOLEAN isAfterStartKey,
                                       BOOLEAN isForward,
                                       IExecutor *executor ) = 0 ;
      virtual INT32 createIndexSampleCursor( std::unique_ptr<IIndexCursor> &cursor,
                                             UINT64 sampleNum,
                                             IExecutor *executor ) = 0 ;
      virtual INT32 getIndexStats( UINT64 &totalSize,
                                   UINT64 &freeSize,
                                   BOOLEAN isFast,
                                   IExecutor *executor ) = 0 ;

      dmsIdxMetadataKey getMetadataKey() const
      {
         return getMetadata().getIdxKey() ;
      }
   } ;

}


#endif // SDB_I_INDEX_HPP_