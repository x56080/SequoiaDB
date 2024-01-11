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

   Source File Name = ICursor.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SDB_I_CURSOR_HPP_
#define SDB_I_CURSOR_HPP_

#include "sdbInterface.hpp"
#include "interface/IStorageSession.hpp"
#include "utilPooledObject.hpp"
#include "dms.hpp"
#include "dmsRecord.hpp"
#include "keystring/utilKeyString.hpp"
#include <memory>

namespace engine
{

   // forward declaration
   class ICollection ;
   class IIndex ;
   class ILob ;
   typedef struct _dmsLobRecord dmsLobRecord ;
   typedef struct _dmsLobInfoOnPage dmsLobInfoOnPage ;

   /*
      ICursor define
    */
   class ICursor : public _utilPooledObject
   {
   public:
      ICursor() = default ;
      virtual ~ICursor() = default ;
      ICursor( const ICursor & ) = delete ;
      ICursor &operator =( const ICursor & ) = delete ;

   public:
      virtual BOOLEAN isOpened() const = 0 ;
      virtual BOOLEAN isClosed() const = 0 ;
      virtual BOOLEAN isForward() const = 0 ;
      virtual BOOLEAN isBackward() const = 0 ;
      virtual BOOLEAN isSample() const = 0 ;
      virtual BOOLEAN isEOF() const = 0 ;

      virtual INT32 close() = 0 ;

      virtual INT32 advance( IExecutor *executor ) = 0 ;

      virtual UINT64 getSnapshotID() const = 0 ;
      virtual void resetSnapshotID( UINT64 snapshotID ) = 0 ;

      virtual BOOLEAN isAsync() const = 0 ;
      virtual IStorageSession *getSession() = 0 ;
   } ;

   /*
      IDataCursor define
    */
   class IDataCursor : public ICursor
   {
   public:
      IDataCursor() = default ;
      virtual ~IDataCursor() = default ;
      IDataCursor( const IDataCursor & ) = delete ;
      IDataCursor &operator =( const IDataCursor & ) = delete ;

   public:
      virtual INT32 open( std::shared_ptr<ICollection> collPtr,
                          const dmsRecordID &startRID,
                          BOOLEAN isAfterStartRID,
                          BOOLEAN isForward,
                          UINT64 snapshotID,
                          IExecutor *executor ) = 0 ;
      virtual INT32 open( std::shared_ptr<ICollection> collPtr,
                          UINT64 sampleNum,
                          UINT64 snapshotID,
                          IExecutor *executor ) = 0 ;

      virtual INT32 locate( const dmsRecordID &rid,
                            BOOLEAN isAfterStartRID,
                            IExecutor *executor,
                            BOOLEAN &isFound ) = 0 ;

      virtual INT32 pause( IExecutor *executor ) = 0 ;

      virtual INT32 getCurrentRecordID( dmsRecordID &recordID ) = 0 ;
      virtual INT32 getCurrentRecord( dmsRecordData &data ) = 0 ;
   } ;

   /*
      IIndexCursor define
    */
   class IIndexCursor : public ICursor
   {
   public:
      IIndexCursor() = default ;
      virtual ~IIndexCursor() = default ;
      IIndexCursor( const IIndexCursor & ) = delete ;
      IIndexCursor &operator =( const IIndexCursor & ) = delete ;

   public:
      virtual INT32 open( std::shared_ptr<IIndex> idxPtr,
                          const keystring::keyString &startKey,
                          BOOLEAN isAfterStartKey,
                          BOOLEAN isForward,
                          UINT64 snapshotID,
                          IExecutor *executor ) = 0 ;
      virtual INT32 open( std::shared_ptr<IIndex> idxPtr,
                          UINT64 sampleNum,
                          UINT64 snapshotID,
                          IExecutor *executor ) = 0 ;

      virtual INT32 locate( const bson::BSONObj &key,
                            const dmsRecordID &recordID,
                            BOOLEAN isAfterStartKey,
                            IExecutor *executor,
                            BOOLEAN &isFound ) = 0 ;

      virtual INT32 locate( const keystring::keyString &key,
                            BOOLEAN isAfterStartKey,
                            IExecutor *executor,
                            BOOLEAN &isFound ) = 0 ;

      virtual INT32 pause( IExecutor *executor ) = 0 ;

      virtual INT32 getCurrentKeyString( keystring::keyString &key ) = 0 ;
      virtual INT32 getCurrentKey( bson::BSONObj &key ) = 0 ;
      virtual INT32 getCurrentRecordID( dmsRecordID &recordID ) = 0 ;
      virtual INT32 getCurrentRecord( dmsRecordData &data ) = 0 ;
   } ;

   /*
      ILobCursor define
    */
   class ILobCursor : public ICursor
   {
   public:
      ILobCursor() = default ;
      virtual ~ILobCursor() = default ;
      ILobCursor( const ILobCursor & ) = delete ;
      ILobCursor &operator=( const ILobCursor & ) = delete ;

   public:
      virtual INT32 open( std::shared_ptr< ILob > lobPtr,
                          const dmsLobRecord &startKey,
                          BOOLEAN isAfterStartKey,
                          BOOLEAN isForward,
                          UINT64 snapshotID,
                          IExecutor *executor ) = 0 ;

      virtual INT32 open( std::shared_ptr< ILob > lobPtr,
                          UINT64 sampleNum,
                          UINT64 snapshotID,
                          IExecutor *executor ) = 0 ;

      virtual INT32 locate( const dmsLobRecord &rid,
                            BOOLEAN isAfterStartKey,
                            IExecutor *executor,
                            BOOLEAN &isFound ) = 0 ;

      virtual INT32 getCurrentLobRecord( dmsLobInfoOnPage &info, const CHAR **data ) = 0 ;
   } ;
}


#endif // SDB_I_CURSOR_HPP_
