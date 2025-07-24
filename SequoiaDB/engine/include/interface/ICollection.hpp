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

   Source File Name = ICollection.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SDB_I_COLLECTION_HPP_
#define SDB_I_COLLECTION_HPP_

#include "sdbInterface.hpp"
#include "interface/IIndex.hpp"
#include "interface/ICursor.hpp"
#include "dms.hpp"
#include "dmsRecord.hpp"
#include "dmsMetadata.hpp"
#include "dmsOprtOptions.hpp"
#include "utilPooledObject.hpp"

namespace engine
{

   /*
      ICollection define
    */
   class ICollection : public _utilPooledObject,
                       public std::enable_shared_from_this<ICollection>
   {
   public:
      ICollection() = default ;
      virtual ~ICollection() = default ;
      ICollection( const ICollection & ) = delete ;
      ICollection &operator =( const ICollection & ) = delete ;

      virtual const dmsCLMetadata &getMetadata() const = 0 ;
      virtual dmsCLMetadata &getMetadata() = 0 ;

      virtual UINT64 fetchSnapshotID() = 0 ;

      virtual INT32 createIndex( const dmsIdxMetadata &metadata,
                                 const dmsCreateIdxOptions &options,
                                 IExecutor *executor ) = 0 ;
      virtual INT32 dropIndex( const dmsIdxMetadata &metadata,
                               const dmsDropIdxOptions &options,
                               IExecutor *executor ) = 0 ;
      virtual INT32 truncate( const dmsTruncCLOptions &options,
                              IExecutor *executor ) = 0 ;
      virtual INT32 compact( const dmsCompactCLOptions &options,
                             IExecutor *executor ) = 0 ;

      virtual INT32 getIndex( const dmsIdxMetadataKey &metadataKey,
                              IExecutor *executor,
                              std::shared_ptr<IIndex> &idxPtr ) = 0 ;
      virtual INT32 loadIndex( const dmsIdxMetadata &metadata,
                               IExecutor *executor,
                               std::shared_ptr<IIndex> &idxPtr ) = 0 ;

      virtual INT32 insertRecord( const dmsRecordID &rid,
                                  const dmsRecordData &recordData,
                                  IExecutor *executor ) = 0 ;
      virtual INT32 updateRecord( const dmsRecordID &rid,
                                  const dmsRecordData &recordData,
                                  IExecutor *executor ) = 0 ;
      virtual INT32 removeRecord( const dmsRecordID &rid,
                                  IExecutor *executor ) = 0 ;
      virtual INT32 extractRecord( const dmsRecordID &rid,
                                   dmsRecordData &recordData,
                                   IExecutor *executor ) = 0 ;

      virtual INT32 popRecords( const dmsRecordID &rid,
                                INT32 direction,
                                IExecutor *executor,
                                UINT64 &popCount,
                                UINT64 &popSize ) = 0 ;

      virtual INT32 createDataCursor( std::unique_ptr<IDataCursor> &cursor,
                                      const dmsRecordID &startRID,
                                      BOOLEAN afterStartRID,
                                      BOOLEAN isForward,
                                      IExecutor *executor ) = 0 ;
      virtual INT32 createDataSampleCursor( std::unique_ptr<IDataCursor> &cursor,
                                            UINT64 sampleNum,
                                            IExecutor *executor ) = 0 ;

      virtual INT32 getCount( UINT64 &count,
                              BOOLEAN isFast,
                              IExecutor *executor ) = 0 ;
      virtual INT32 getDataStats( UINT64 &totalSize,
                                  UINT64 &freeSize,
                                  BOOLEAN isFast,
                                  IExecutor *executor ) = 0 ;
      virtual INT32 getIndexStats( UINT64 &totalSize,
                                   UINT64 &freeSize,
                                   BOOLEAN isFast,
                                   IExecutor *executor ) = 0 ;

      virtual INT32 getMinRecordID( dmsRecordID &rid, IExecutor *executor ) = 0 ;
      virtual INT32 getMaxRecordID( dmsRecordID &rid, IExecutor *executor ) = 0 ;

      virtual INT32 validateData( IExecutor *executor ) = 0 ;

      dmsCLMetadataKey getMetadataKey() const
      {
         return getMetadata().getCLKey() ;
      }
   } ;

}


#endif // SDB_I_COLLECTION_HPP_
