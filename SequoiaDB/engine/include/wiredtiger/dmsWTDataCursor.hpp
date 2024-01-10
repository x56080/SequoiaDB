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

   Source File Name = dmsWTDataCursor.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMS_WT_DATA_CURSOR_HPP_
#define DMS_WT_DATA_CURSOR_HPP_

#include "interface/ICursor.hpp"
#include "restAdaptor.hpp"
#include "wiredtiger/dmsWTCursorHolder.hpp"

namespace engine
{
namespace wiredtiger
{

   /*
      _dmsWTDataCursor define
    */
   class _dmsWTDataCursor : public IDataCursor, public _dmsWTCursorHolder
   {
   public:
      _dmsWTDataCursor( dmsWTSession &session ) ;
      virtual ~_dmsWTDataCursor() = default ;
      _dmsWTDataCursor( const _dmsWTDataCursor & ) = delete ;
      _dmsWTDataCursor &operator =( const _dmsWTDataCursor & ) = delete ;

      virtual BOOLEAN isOpened() const
      {
         return _isOpened ;
      }

      virtual BOOLEAN isClosed() const
      {
         return _isClosed ;
      }

      virtual BOOLEAN isForward() const
      {
         return _isForward ;
      }

      virtual BOOLEAN isBackward() const
      {
         return !_isForward ;
      }

      virtual BOOLEAN isSample() const
      {
         return _isSample ;
      }

      virtual BOOLEAN isEOF() const
      {
         return _isEOF ;
      }

      virtual INT32 open( std::shared_ptr<ICollection> collPtr,
                          const dmsRecordID &startRID,
                          BOOLEAN isAfterStartRID,
                          BOOLEAN isForward,
                          UINT64 snapshotID,
                          IExecutor *executor ) ;
      virtual INT32 open( std::shared_ptr<ICollection> collPtr,
                          UINT64 sampleNum,
                          UINT64 snapshotID,
                          IExecutor *executor ) ;

      virtual INT32 close()
      {
         _resetCache() ;
         return _close() ;
      }

      virtual INT32 advance( IExecutor *executor )
      {
         _resetCache() ;
         return _advance( executor ) ;
      }

      virtual INT32 locate( const dmsRecordID &rid,
                            BOOLEAN isAfterRID,
                            IExecutor *executor,
                            BOOLEAN &isFound ) ;

      virtual INT32 pause( IExecutor *executor ) ;

      virtual INT32 getCurrentRecordID( dmsRecordID &recordID ) ;
      virtual INT32 getCurrentRecord( dmsRecordData &data ) ;

      virtual UINT64 getSnapshotID() const
      {
         return _snapshotID ;
      }

      virtual void resetSnapshotID( UINT64 snapshotID )
      {
         _snapshotID = snapshotID ;
      }

   protected:
      void _resetCache()
      {
         _recordIDCache.reset() ;
      }

   protected:
      std::shared_ptr<ICollection> _collPtr ;
      dmsRecordID _recordIDCache ;
   } ;

   typedef class _dmsWTDataCursor dmsWTDataCursor ;

   /*
      _dmsWTDataAsyncCursor define
    */
   class _dmsWTDataAsyncCursor : public _dmsWTDataCursor
   {
   public:
      _dmsWTDataAsyncCursor() ;
      virtual ~_dmsWTDataAsyncCursor() ;
      _dmsWTDataAsyncCursor( const _dmsWTDataAsyncCursor & ) = delete ;
      _dmsWTDataAsyncCursor &operator =( const _dmsWTDataAsyncCursor & ) = delete ;

      virtual INT32 open( std::shared_ptr<ICollection> collPtr,
                          const dmsRecordID &startRID,
                          BOOLEAN isAfterStartRID,
                          BOOLEAN isForward,
                          UINT64 snapshotID,
                          IExecutor *executor ) ;
      virtual INT32 open( std::shared_ptr<ICollection> collPtr,
                          UINT64 sampleNum,
                          UINT64 snapshotID,
                          IExecutor *executor ) ;

   protected:
      dmsWTSession _asyncSession ;
   } ;

   typedef class _dmsWTDataAsyncCursor dmsWTDataAsyncCursor ;

}
}

#endif // DMS_WT_DATA_CURSOR_HPP_
