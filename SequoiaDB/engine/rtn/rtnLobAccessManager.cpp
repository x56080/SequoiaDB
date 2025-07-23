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

   Source File Name = rtnLobAccessManager.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/28/2017  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "rtnLobAccessManager.hpp"
#include "msgDef.h"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"

namespace engine
{
   _rtnLobAccessInfo::_rtnLobAccessInfo( const bson::OID& oid,
                                         UINT32 mode,
                                         INT64 accessId )
   : _mode( mode ),
     _refCount( 0 ),
     _metaCache( NULL ),
     _lockSections( NULL )
   {
      _oid = oid ;
      if ( SDB_LOB_MODE_CREATEONLY == mode ||
           SDB_LOB_MODE_REMOVE == mode ||
           SDB_LOB_MODE_TRUNCATE == mode )
      {
         _accessId = accessId ;
      }
      else
      {
         _accessId = -1 ;
      }
   }

   _rtnLobAccessInfo::~_rtnLobAccessInfo()
   {
      SAFE_OSS_DELETE( _metaCache ) ;
      SAFE_OSS_DELETE( _lockSections ) ;
   }

   void _rtnLobAccessInfo::setMetaCache( _rtnLobMetaCache* metaCache )
   {
      SDB_ASSERT( NULL == _metaCache, "_metaCache is not null" ) ;

      _metaCache = metaCache ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOBACCESSINFO_LOCKSECTION, "_rtnLobAccessInfo::lockSection" )
   INT32 _rtnLobAccessInfo::lockSection( const _rtnLobSection& section )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOBACCESSINFO_LOCKSECTION ) ;

      if ( SDB_LOB_MODE_WRITE != _mode )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Only support in write mode, rc=%d", rc );
         goto error ;
      }

      // whole lob is already locked
      if ( -1 != _accessId )
      {
         if ( section.accessId != _accessId )
         {
            rc = SDB_LOB_LOCK_CONFLICTED ;
            PD_LOG( PDERROR, "Whole LOB[%s] is already locked by [%lld], rc=%d",
                    _oid.str().c_str(), _accessId, rc ) ;
            goto error ;
         }
         else
         {
            // whole lob is locked by the same accessId
            goto done ;
         }
      }

      // lock whole lob
      if ( 0 == section.offset && OSS_SINT64_MAX == section.length )
      {
         if ( NULL != _lockSections &&
              _lockSections->conflicted( section.accessId ) )
         {
            rc = SDB_LOB_LOCK_CONFLICTED ;
            PD_LOG( PDERROR, "Failed to lock whole LOB[%s], rc=%d",
                    _oid.str().c_str(), rc ) ;
            goto error ;
         }

         _accessId = section.accessId ;
      }

      if ( NULL == _lockSections )
      {
         _lockSections = SDB_OSS_NEW _rtnLobSections() ;
         if ( NULL == _lockSections )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Failed to create lob sections, rc=%d", rc ) ;
            goto error ;
         }
      }

      rc = _lockSections->addSection( section ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to add section, rc=%d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNLOBACCESSINFO_LOCKSECTION, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOBACCESSINFO_UNLOCKSECTIONBYACCESSID, "_rtnLobAccessInfo::unlockSectionByAccessId" )
   INT32 _rtnLobAccessInfo::unlockSectionByAccessId( INT64 accessId )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOBACCESSINFO_UNLOCKSECTIONBYACCESSID ) ;

      if ( SDB_LOB_MODE_WRITE != _mode )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Only support in write mode, rc=%d", rc );
         goto error ;
      }

      if ( -1 != _accessId && accessId == _accessId )
      {
         _accessId = -1 ;
      }

      if ( NULL == _lockSections )
      {
         goto done ;
      }

      _lockSections->delSectionById( accessId ) ;

   done:
      PD_TRACE_EXITRC( SDB_RTNLOBACCESSINFO_UNLOCKSECTIONBYACCESSID, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   _rtnLobAccessManager::_rtnLobAccessManager()
   {
   }

   _rtnLobAccessManager::~_rtnLobAccessManager()
   {
      FOR_EACH_CMAP_BUCKET_X(RTN_LOB_MAP, _lobMap)
      {
         for ( RTN_LOB_MAP::map_const_iterator lobIter = bucket.begin() ;
               lobIter != bucket.end() ;
               lobIter++ )
         {
            _rtnLobAccessInfo* lobAccessInfo = lobIter->second ;
            SDB_OSS_DEL lobAccessInfo ;
         }

         bucket.clear() ;
      }
      FOR_EACH_CMAP_BUCKET_END
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOBACCESSMGR_GETACCESSPRIVILEGE, "_rtnLobAccessManager::getAccessPrivilege" )
   INT32 _rtnLobAccessManager::getAccessPrivilege( const std::string& clName,
                                                   const bson::OID& oid,
                                                   UINT32 mode,
                                                   INT64 accessId,
                                                   _rtnLobAccessInfo** accessInfo )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOBACCESSMGR_GETACCESSPRIVILEGE ) ;

      if ( !SDB_IS_VALID_LOB_MODE( mode ) )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Invalid LOB access mode: %u", mode ) ;
         goto error ;
      }

      if ( SDB_LOB_MODE_WRITE == mode && accessId <= -1 )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Invalid LOB accessId[%lld] in write mode",
                 accessId ) ;
         goto error ;
      }

      {
         _rtnLobAccessKey key( clName, oid ) ;

         RTN_LOB_MAP::Bucket& bucket = _lobMap.getBucket( key ) ;
         BUCKET_XLOCK( bucket ) ;

         RTN_LOB_MAP::map_const_iterator lobIter = bucket.find( key ) ;
         if ( lobIter != bucket.end() )
         {
            _rtnLobAccessInfo* lobAccessInfo = lobIter->second ;
            switch ( lobAccessInfo->getMode() )
            {
            case SDB_LOB_MODE_CREATEONLY:
               // pass through
            case SDB_LOB_MODE_REMOVE:
               // pass through
            case SDB_LOB_MODE_TRUNCATE:
               rc = SDB_LOB_IS_IN_USE ;
               PD_LOG( PDDEBUG, "LOB[%s] is owned by [%lld] in mode[%u] refCount[%d], rc=%d",
                    lobAccessInfo->getOID().str().c_str(),
                    lobAccessInfo->getAccessId(),
                    lobAccessInfo->getMode(),
                    lobAccessInfo->getRefCount(), rc ) ;
               goto error ;
            case SDB_LOB_MODE_READ:
               if ( SDB_LOB_MODE_READ != mode )
               {
                  rc = SDB_LOB_IS_IN_USE ;
                  PD_LOG( PDDEBUG, "LOB[%s] is owned by [%lld] in mode[%u] refCount[%d], rc=%d",
                    lobAccessInfo->getOID().str().c_str(),
                    lobAccessInfo->getAccessId(),
                    lobAccessInfo->getMode(),
                    lobAccessInfo->getRefCount(), rc ) ;
                  goto error ;
               }
               else
               {
                  lobAccessInfo->lock();
                  lobAccessInfo->incRefCount() ;
                  lobAccessInfo->unlock();
                  break ;
               }
            case SDB_LOB_MODE_WRITE:
               if ( SDB_LOB_MODE_WRITE != mode )
               {
                  rc = SDB_LOB_IS_IN_USE ;
                  PD_LOG( PDDEBUG, "LOB[%s] is owned by [%lld] in mode[%u] refCount[%d], rc=%d",
                    lobAccessInfo->getOID().str().c_str(),
                    lobAccessInfo->getAccessId(),
                    lobAccessInfo->getMode(),
                    lobAccessInfo->getRefCount(), rc ) ;
                  goto error ;
               }

               lobAccessInfo->lock();
               lobAccessInfo->incRefCount() ;
               lobAccessInfo->unlock();
               break ;
            default:
               SDB_ASSERT( FALSE, "invalid mode" ) ;
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Invalid LOB access mode: %u",
                       lobAccessInfo->getMode() ) ;
               goto error ;
            }

            if ( NULL != accessInfo )
            {
               *accessInfo = lobAccessInfo ;
            }

            PD_LOG( PDDEBUG, "Get privilege of LOB[%s] by [%lld] in mode[%u] refCount[%d]",
                    oid.str().c_str(),
                    accessId,
                    mode,
                    lobAccessInfo->getRefCount() ) ;
         }
         else
         {
            _rtnLobAccessInfo* lobAccessInfo =
               SDB_OSS_NEW _rtnLobAccessInfo( oid, mode, accessId ) ;
            if ( NULL == lobAccessInfo )
            {
               rc = SDB_OOM ;
               PD_LOG( PDERROR, "Failed to alloc _rtnLobAccessInfo, rc=%d", rc ) ;
               goto error ;
            }

            if ( SDB_LOB_MODE_READ == mode ||
                 SDB_LOB_MODE_WRITE == mode )
            {
               lobAccessInfo->incRefCount() ;
            }

            try
            {
               bucket.insert( RTN_LOB_MAP::value_type( key, lobAccessInfo ) ) ;
            }
            catch ( std::exception& e )
            {
               rc = SDB_SYS ;
               SAFE_OSS_DELETE( lobAccessInfo ) ;
               PD_LOG( PDERROR, "Unexpected error happened: %s", e.what() ) ;
               goto error ;
            }

            if ( NULL != accessInfo )
            {
               *accessInfo = lobAccessInfo ;
            }

            PD_LOG( PDDEBUG, "Get privilege of LOB[%s] by [%lld] in mode[%u] refCount[%d]",
                    oid.str().c_str(),
                    accessId,
                    mode,
                    lobAccessInfo->getRefCount() ) ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNLOBACCESSMGR_GETACCESSPRIVILEGE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOBACCESSMGR_RELEASEACCESSPRIVILEGE, "_rtnLobAccessManager::releaseAccessPrivilege" )
   INT32 _rtnLobAccessManager::releaseAccessPrivilege( const std::string& clName,
                                                       const bson::OID& oid,
                                                       UINT32 mode,
                                                       INT64 accessId )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOBACCESSMGR_RELEASEACCESSPRIVILEGE ) ;
      _rtnLobAccessKey key( clName, oid ) ;

      RTN_LOB_MAP::Bucket& bucket = _lobMap.getBucket( key ) ;
      BUCKET_XLOCK( bucket ) ;

      RTN_LOB_MAP::map_const_iterator lobIter = bucket.find( key ) ;
      if ( lobIter != bucket.end() )
      {
         _rtnLobAccessInfo* lobAccessInfo = lobIter->second ;

         SDB_ASSERT( oid == lobAccessInfo->getOID(), "incorrect oid" ) ;

         if ( mode != lobAccessInfo->getMode() )
         {
            SDB_ASSERT( mode == lobAccessInfo->getMode(), "incorrect mode" ) ;
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Invalid LOB access mode: %u, %u",
                    mode, lobAccessInfo->getMode() ) ;
            goto error ;
         }

         // lobAccessInfo->getAccessId() can be not -1 in write mode
         // when a context lock the whole LOB,
         // but the refCount should be decrease
         if ( -1 != accessId &&
              -1 != lobAccessInfo->getAccessId() &&
              accessId != lobAccessInfo->getAccessId() &&
              SDB_LOB_MODE_WRITE != lobAccessInfo->getMode() )
         {
            SDB_ASSERT( accessId != lobAccessInfo->getAccessId(), 
                        "incorrect accessId" ) ;
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Invalid LOB access id: %lld, %lld",
                    accessId, lobAccessInfo->getAccessId() ) ;
            goto error ;
         }

         switch ( lobAccessInfo->getMode() )
         {
         case SDB_LOB_MODE_CREATEONLY:
            // pass through
         case SDB_LOB_MODE_REMOVE:
            // pass through
         case SDB_LOB_MODE_TRUNCATE:
            bucket.erase( key ) ;
            PD_LOG( PDDEBUG, "Release privilege of LOB[%s] by [%lld] in mode[%u] refCount[%d]",
                    oid.str().c_str(),
                    accessId,
                    mode,
                    lobAccessInfo->getRefCount() ) ;
            SAFE_OSS_DELETE( lobAccessInfo ) ;
            break ;
         case SDB_LOB_MODE_READ:
            lobAccessInfo->lock();
            lobAccessInfo->decRefCount() ;
            PD_LOG( PDDEBUG, "Release privilege of LOB[%s] by [%lld] in mode[%u] refCount[%d]",
                    oid.str().c_str(),
                    accessId,
                    mode,
                    lobAccessInfo->getRefCount() ) ;
            if ( lobAccessInfo->getRefCount() <= 0 )
            {
               bucket.erase( key ) ;
               lobAccessInfo->unlock();
               SAFE_OSS_DELETE( lobAccessInfo ) ;
            }
            else
            {
               lobAccessInfo->unlock();
            }
            break ;
         case SDB_LOB_MODE_WRITE:
            if ( accessId <= -1 )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Invalid LOB accessId[%lld] in write mode",
                       accessId ) ;
               goto error ;
            }
            lobAccessInfo->lock();
            rc = lobAccessInfo->unlockSectionByAccessId( accessId ) ;
            if ( SDB_OK != rc )
            {
               lobAccessInfo->unlock();
               PD_LOG( PDERROR, "Failed to unlock LOB section by access id: %lld, rc=%d",
                       accessId, rc ) ;
               goto error ;
            }
            lobAccessInfo->decRefCount() ;
            PD_LOG( PDDEBUG, "Release privilege of LOB[%s] by [%lld] in mode[%u] refCount[%d]",
                    oid.str().c_str(),
                    accessId,
                    mode,
                    lobAccessInfo->getRefCount() ) ;
            if ( lobAccessInfo->getRefCount() <= 0 )
            {
               bucket.erase( key ) ;
               lobAccessInfo->unlock();
               SAFE_OSS_DELETE( lobAccessInfo ) ;
            }
            else
            {
               lobAccessInfo->unlock();
            }
            break ;
         default:
            SDB_ASSERT( FALSE, "invalid mode" ) ;
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Invalid LOB access mode: %u",
                    lobAccessInfo->getMode() ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNLOBACCESSMGR_RELEASEACCESSPRIVILEGE, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}

