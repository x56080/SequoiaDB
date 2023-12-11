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

   Source File Name = dmsWTIndex.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "wiredtiger/dmsWTIndex.hpp"
#include "wiredtiger/dmsWTIndexCursor.hpp"
#include "wiredtiger/dmsWTSession.hpp"
#include "wiredtiger/dmsWTUtil.hpp"
#include "dmsStorageDataCommon.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"
#include "pd.hpp"

using namespace std ;
using namespace bson ;
using namespace engine::keystring ;

namespace engine
{
namespace wiredtiger
{

namespace
{
   static const dmsWTItem s_emptyItem( nullptr, 0 ) ;
}
   /*
      _dmsWTIndex implement
    */
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTINDEX_INDEX, "_dmsWTIndex::index" )
   INT32 _dmsWTIndex::index( const keyString &keyString,
                             const dmsRecordID &rid,
                             BOOLEAN checkDuplicated,
                             IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTINDEX_INDEX ) ;

      dmsWTItem key( keyString ) ;

      rc = _engine.insertToStore( _store, key, s_emptyItem ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to insert index key to engine, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTINDEX_INDEX, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTINDEX_UNINDEX, "_dmsWTIndex::unindex" )
   INT32 _dmsWTIndex::unindex( const keyString &keyString,
                               const dmsRecordID &rid,
                               IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTINDEX_UNINDEX ) ;

      dmsWTItem key( keyString ) ;

      rc = _engine.removeFromStore( _store, key ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to remove index key from engine, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTINDEX_UNINDEX, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTINDEX_CREATEINDEXCURSOR, "_dmsWTIndex::createIndexCursor" )
   INT32 _dmsWTIndex::createIndexCursor( unique_ptr<IIndexCursor> &cursor,
                                         const keyString &startKey,
                                         BOOLEAN isAfterStartKey,
                                         BOOLEAN isForward,
                                         IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTINDEX_CREATEINDEXCURSOR ) ;

      cursor = unique_ptr<dmsWTIndexCursor>( new dmsWTIndexCursor() ) ;
      PD_CHECK( cursor, SDB_OOM, error, PDERROR, "Failed to create index cursor, rc: %d", rc ) ;

      rc = cursor->open( std::move( shared_from_this() ),
                         startKey,
                         isAfterStartKey,
                         isForward,
                         executor ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open data cursor, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTINDEX_CREATEINDEXCURSOR, rc ) ;
      return rc ;

   error:
      cursor.release() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTINDEX_BLDIDXCONFSTR, "_dmsWTIndex::buildIdxConfigString" )
   INT32 _dmsWTIndex::buildIdxConfigString( const dmsWTEngineOptions &options,
                                            const dmsCreateIdxOptions &createCLOptions,
                                            ossPoolString &configString )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTINDEX_BLDIDXCONFSTR ) ;

      try
      {
         ossPoolStringStream ss ;

         ss << "type=file,internal_page_max=16k,leaf_page_max=16k,";
         ss << "checksum=on,";
         ss << "prefix_compression=true,";
         ss << "key_format=u,";
         ss << "value_format=u,";
         ss << "app_metadata=(formatVersion=" << DMS_WT_FORMART_VER_CUR << ")," ;
         ss << "log=(enabled=true)," ;

         configString = ss.str();
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build index config string, "
                 "occur exception: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTINDEX_BLDIDXCONFSTR, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTINDEX_BLDIDXURI, "_dmsWTIndex::buildIdxURI" )
   INT32 _dmsWTIndex::buildIdxURI( utilCSUniqueID csUID,
                                   utilCLInnerID clInnerID,
                                   UINT32 clLID,
                                   utilIdxInnerID idxInnerID,
                                   ossPoolString &idxURI )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTINDEX_BLDIDXURI ) ;

      try
      {
         ossPoolStringStream ss ;

         ss << "table:" ;
         dmsWTBuildIndexIdent( csUID, clInnerID, clLID, idxInnerID, ss ) ;
         idxURI = ss.str();
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build index URI, "
                 "occur exception: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTINDEX_BLDIDXURI, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
}
