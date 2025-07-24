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

   Source File Name = dmsWTIndex.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMS_WT_INDEX_HPP_
#define DMS_WT_INDEX_HPP_

#include "interface/IIndex.hpp"
#include "dmsMetadata.hpp"
#include "wiredtiger/dmsWTStoreHolder.hpp"
#include "keystring/utilKeyStringBuilder.hpp"

namespace engine
{
namespace wiredtiger
{

   /*
      _dmsWTIndex define
    */
   class _dmsWTIndex : public IIndex,
                       public _dmsWTStoreHolder
   {
   public:
      _dmsWTIndex( const dmsIdxMetadata &metadata,
                   dmsWTStorageEngine &engine,
                   const dmsWTStore &indexStore )
      : _dmsWTStoreHolder( engine, indexStore ),
        _metadata( metadata, TRUE )
      {
      }

      virtual ~_dmsWTIndex() = default ;

      virtual const dmsIdxMetadata &getMetadata() const
      {
         return _metadata ;
      }

      virtual dmsIdxMetadata &getMetadata()
      {
         return _metadata ;
      }

      virtual UINT64 fetchSnapshotID()
      {
         return _metadata.fetchSnapshotID() ;
      }

      virtual INT32 truncate( const dmsTruncateIdxOptions &options,
                              IExecutor *executor ) ;
      virtual INT32 compact( const dmsCompactIdxOptions &options,
                             IExecutor *executor ) ;

      virtual INT32 index( const bson::BSONObj &key,
                           const dmsRecordID &rid,
                           BOOLEAN allowDuplicated,
                           IExecutor *executor,
                           utilWriteResult *result ) ;
      virtual INT32 unindex( const bson::BSONObj &key,
                             const dmsRecordID &rid,
                             IExecutor *executor ) ;

      virtual INT32 createIndexCursor( std::unique_ptr<IIndexCursor> &cursor,
                                       const keystring::keyString &startKey,
                                       BOOLEAN isAfterStartKey,
                                       BOOLEAN isForward,
                                       IExecutor *executor ) ;
      virtual INT32 createIndexSampleCursor( std::unique_ptr<IIndexCursor> &cursor,
                                             UINT64 sampleNum,
                                             IExecutor *executor ) ;

      virtual INT32 getIndexStats( UINT64 &totalSize,
                                   UINT64 &freeSize,
                                   BOOLEAN isFast,
                                   IExecutor *executor ) ;

      static INT32 buildIdxConfigString( const dmsWTEngineOptions &options,
                                         const dmsCreateIdxOptions &createIdxOptions,
                                         ossPoolString &configString ) ;

      static INT32 buildIdxURI( utilCSUniqueID csUID,
                                utilCLInnerID clInnerID,
                                UINT32 clLID,
                                utilIdxInnerID idxInnerID,
                                ossPoolString &idxURI ) ;

   protected:
      INT32 _buildKeyString( const bson::BSONObj &key,
                             const dmsRecordID &rid,
                             keystring::keyStringBuilderImpl &builder,
                             BOOLEAN *isAllUndefined = nullptr ) ;

      INT32 _getKey( const keystring::keyString &ks,
                     dmsWTItem &keyItem ) ;

      INT32 _getKeyAndValue( const keystring::keyString &ks,
                             BOOLEAN isRIDInValue,
                             dmsWTItem &keyItem,
                             dmsWTItem &valueItem ) ;

      INT32 _getRecordID( const dmsWTItem &value,
                          BOOLEAN isAtEnd,
                          dmsRecordID &rid ) ;

      virtual INT32 _index( dmsWTCursor &cursor,
                            const bson::BSONObj &key,
                            const dmsRecordID &rid,
                            BOOLEAN allowDuplicated,
                            IExecutor *executor,
                            utilWriteResult *result ) ;
      virtual INT32 _unindex( dmsWTCursor &cursor,
                              const bson::BSONObj &key,
                              const dmsRecordID &rid,
                              IExecutor *executor ) ;

      INT32 _insertStrictUnique( dmsWTCursor &cursor,
                                 const keystring::keyString &ks,
                                 const dmsRecordID &rid,
                                 utilWriteResult *result ) ;
      INT32 _insertStrictUnique( dmsWTCursor &cursor,
                                 const dmsWTItem &keyItem,
                                 const dmsWTItem &valueItem,
                                 const dmsRecordID &rid,
                                 utilWriteResult *result ) ;

      INT32 _insertUnique( dmsWTCursor &cursor,
                           const keystring::keyString &ks,
                           const dmsRecordID &rid,
                           BOOLEAN isAllUndefined,
                           utilWriteResult *result ) ;

      INT32 _insertStandard( dmsWTCursor &cursor,
                             const keystring::keyString &ks,
                             const dmsRecordID &rid,
                             utilWriteResult *result ) ;

      INT32 _checkUnique( dmsWTCursor &cursor,
                          const keystring::keyString &ks,
                          const dmsRecordID &rid,
                          utilWriteResult *result ) ;

      INT32 _createIndexCursor( unique_ptr<IIndexCursor> &cursor,
                                IExecutor *executor ) ;

   protected:
      dmsIdxMetadata _metadata ;
   } ;

   typedef class _dmsWTIndex dmsWTIndex ;

}
}

#endif // DMS_WT_INDEX_HPP_
