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

   Source File Name = dmsWTCollection.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMS_WT_COLLECTION_HPP_
#define DMS_WT_COLLECTION_HPP_

#include "interface/ICollection.hpp"
#include "dmsMetadata.hpp"
#include "ossRWMutex.hpp"
#include "wiredtiger/dmsWTStoreHolder.hpp"
#include "wiredtiger/dmsWTIndex.hpp"
#include <memory>

namespace engine
{
namespace wiredtiger
{

   /*
      _dmsWTCollection define
    */
   class _dmsWTCollection : public ICollection,
                            public _dmsWTStoreHolder
   {
   public:
      _dmsWTCollection( const dmsCLMetadata &metadata,
                        dmsWTStorageEngine &engine,
                        const dmsWTStore &dataStore )
      : dmsWTStoreHolder( engine, dataStore),
        _metadata( metadata )
      {
      }

      virtual ~_dmsWTCollection() = default ;

      virtual const dmsCLMetadata &getMetadata() const
      {
         return _metadata ;
      }

      virtual dmsCLMetadata &getMetadata()
      {
         return _metadata ;
      }

      virtual UINT64 fetchSnapshotID()
      {
         return _metadata.fetchSnapshotID() ;
      }

      virtual INT32 createIndex( const dmsIdxMetadata &metadata,
                                 const dmsCreateIdxOptions &options,
                                 IExecutor *executor ) ;
      virtual INT32 dropIndex( const dmsIdxMetadata &metadata,
                               const dmsDropIdxOptions &options,
                               IExecutor *executor ) ;
      virtual INT32 truncate( const dmsTruncCLOptions &options,
                              IExecutor *executor ) ;
      virtual INT32 compact( const dmsCompactCLOptions &options,
                             IExecutor *executor ) ;

      virtual INT32 getIndex( const dmsIdxMetadataKey &metadataKey,
                              IExecutor *executor,
                              std::shared_ptr<IIndex> &idxPtr ) ;
      virtual INT32 loadIndex( const dmsIdxMetadata &metadata,
                               IExecutor *executor,
                               std::shared_ptr<IIndex> &idxPtr ) ;

      virtual INT32 insertRecord( const dmsRecordID &rid,
                                  const dmsRecordData &recordData,
                                  IExecutor *executor ) ;
      virtual INT32 updateRecord( const dmsRecordID &rid,
                                  const dmsRecordData &recordData,
                                  IExecutor *executor ) ;
      virtual INT32 removeRecord( const dmsRecordID &rid,
                                  IExecutor *executor ) ;
      virtual INT32 extractRecord( const dmsRecordID &rid,
                                   dmsRecordData &recordData,
                                   IExecutor *executor ) ;

      virtual INT32 popRecords( const dmsRecordID &rid,
                                INT32 direction,
                                IExecutor *executor,
                                UINT64 &popCount,
                                UINT64 &popSize ) ;

      virtual INT32 createDataCursor( std::unique_ptr<IDataCursor> &cursor,
                                      const dmsRecordID &startRID,
                                      BOOLEAN afterStartRID,
                                      BOOLEAN isForward,
                                      IExecutor *executor ) ;
      virtual INT32 createDataSampleCursor( std::unique_ptr<IDataCursor> &cursor,
                                            UINT64 sampleNum,
                                            IExecutor *executor ) ;

      virtual INT32 getCount( UINT64 &count,
                              BOOLEAN isFast,
                              IExecutor *executor ) ;
      virtual INT32 getDataStats( UINT64 &totalSize,
                                  UINT64 &freeSize,
                                  BOOLEAN isFast,
                                  IExecutor *executor ) ;
      virtual INT32 getIndexStats( UINT64 &totalSize,
                                   UINT64 &freeSize,
                                   BOOLEAN isFast,
                                   IExecutor *executor ) ;

      virtual INT32 getMinRecordID( dmsRecordID &rid, IExecutor *executor ) ;
      virtual INT32 getMaxRecordID( dmsRecordID &rid, IExecutor *executor ) ;

      virtual INT32 validateData( IExecutor *executor ) ;

      static INT32 buildDataConfigString( const dmsWTEngineOptions &options,
                                          const dmsCreateCLOptions &createCLOptions,
                                          ossPoolString &configString ) ;
      static INT32 buildDataURI( utilCSUniqueID csUID,
                                 utilCLInnerID clInnerID,
                                 UINT32 clLID,
                                 ossPoolString &dataURI ) ;

   protected:
      INT32 _addIndex( const dmsIdxMetadata &metadata,
                       const dmsWTStore &store,
                       std::shared_ptr<IIndex> &idxPtr ) ;
      void _removeIndex( const dmsIdxMetadataKey &metadataKey ) ;
      std::shared_ptr<IIndex> _getIndex( const dmsIdxMetadataKey &metadataKey ) ;
      std::shared_ptr<IIndex> _getNextIndex( std::shared_ptr<IIndex> &idxPtr ) ;

      INT32 _getMinRecordID( dmsWTSession &session,
                             dmsRecordID &rid,
                             IExecutor *executor ) ;
      INT32 _getMaxRecordID( dmsWTSession &session,
                             dmsRecordID &rid,
                             IExecutor *executor ) ;

      INT32 _createDataCursor( unique_ptr<IDataCursor> &cursor,
                               IExecutor *executor ) ;

   protected:
      dmsCLMetadata _metadata ;

      typedef ossPoolMap<dmsIdxMetadataKey,
                         std::shared_ptr<IIndex>> _dmsWTIdxMap ;
      typedef _dmsWTIdxMap::iterator _dmsWTIdxMapIter ;
      _dmsWTIdxMap _idxMap ;
      ossRWMutex _idxMapMutex ;
   } ;

   typedef class _dmsWTCollection dmsWTCollection ;

}
}

#endif // DMS_WT_COLLECTION_HPP_
