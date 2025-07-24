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

   Source File Name = dmsWTCursorHolder.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMS_WT_CURSOR_HOLDER_HPP_
#define DMS_WT_CURSOR_HOLDER_HPP_

#include "ossMemPool.hpp"
#include "wiredtiger/dmsWTCursor.hpp"
#include "wiredtiger/dmsWTSession.hpp"
#include "wiredtiger/dmsWTStorageEngine.hpp"
#include "wiredtiger/dmsWTUtil.hpp"
#include "wiredtiger/dmsWTItem.hpp"

namespace engine
{
namespace wiredtiger
{

   /*
      _dmsWTCursorHolder define
    */
   class _dmsWTCursorHolder : public _utilPooledObject
   {
   public:
      _dmsWTCursorHolder( dmsWTSession &session ) ;
      virtual ~_dmsWTCursorHolder() = default ;
      _dmsWTCursorHolder( const _dmsWTCursorHolder & ) = delete ;
      _dmsWTCursorHolder &operator =( const _dmsWTCursorHolder & ) = delete ;

   protected:
      INT32 _open( dmsWTStorageEngine &engine,
                   const ossPoolString &uri,
                   const ossPoolString &config,
                   UINT64 startKey,
                   BOOLEAN isAfterStartKey,
                   BOOLEAN isForward,
                   UINT64 snapshotID,
                   IExecutor *executor ) ;
      INT32 _open( dmsWTStorageEngine &engine,
                   const ossPoolString &uri,
                   const ossPoolString &config,
                   const dmsWTItem &startKey,
                   BOOLEAN isAfterStartKey,
                   BOOLEAN isForward,
                   UINT64 snapshotID,
                   IExecutor *executor ) ;
      INT32 _open( dmsWTStorageEngine &engine,
                   const ossPoolString &uri,
                   const ossPoolString &config,
                   BOOLEAN isForward,
                   UINT64 snapshotID,
                   IExecutor *executor ) ;
      INT32 _open( dmsWTStorageEngine &engine,
                   const ossPoolString &uri,
                   const ossPoolString &config,
                   UINT64 sampelNum,
                   UINT64 snapshotID,
                   IExecutor *executor ) ;

      INT32 _advance( IExecutor *executor ) ;

      INT32 _close() ;

   protected:
      dmsWTCursor _cursor ;
      UINT64 _snapshotID = DMS_INVALID_SNAPSHOT_ID ;
      BOOLEAN _isOpened = FALSE ;
      BOOLEAN _isClosed = FALSE ;
      BOOLEAN _isForward = TRUE ;
      BOOLEAN _isSample = FALSE ;
      BOOLEAN _isEOF = FALSE ;
   } ;

   typedef class _dmsWTCursorHolder dmsWTCursorHolder ;

}
}

#endif // DMS_WT_CURSOR_HOLDER_HPP_
