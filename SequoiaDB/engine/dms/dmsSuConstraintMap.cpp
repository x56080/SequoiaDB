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

   Source File Name = dmsSuConstraintMap.cpp

   Descriptive Name = Data Management Service SU Constraint Map

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/14/2022  ZHY Initial Draft

   Last Changed =

*******************************************************************************/
#include "dmsSuConstraintMap.hpp"
#include "xxhash.h"

namespace engine
{
   _dmsSuConstraintMap::contextCreate::~contextCreate()
   {
      _release();
   }

   void _dmsSuConstraintMap::contextCreate::abort()
   {
      _release();
   }

   void _dmsSuConstraintMap::contextCreate::commit( const DMS_SU_DESCRIPTOR &desc )
   {
      SDB_ASSERT( desc && desc->isValid(), "must be valid" );
      if ( _lock.owns_lock() )
      {
         INT32 rc = SDB_OK;
         rc = _cm->addSuDescriptor( desc );
         SDB_ASSERT( rc == SDB_OK, "must success" );
      }
      else
      {
         SDB_ASSERT( FALSE, "must own lock" );
      }
      _release();
   }

   void _dmsSuConstraintMap::contextCreate::_release()
   {
      if ( UTIL_IS_VALID_CSUNIQUEID( _csUID ) )
      {
         _cm->removeTempUniqueID( _csUID );
      }
      if ( _lock.owns_lock() )
      {
         _lock.unlock();
      }
   }

   _dmsSuConstraintMap::contextDrop::~contextDrop()
   {
      _release();
   }

   void _dmsSuConstraintMap::contextDrop::abort()
   {
      _release();
   }

   const DMS_SU_DESCRIPTOR &_dmsSuConstraintMap::contextDrop::getDescriptor() const
   {
      return _desc;
   }

   void _dmsSuConstraintMap::contextDrop::commit()
   {
      if ( _desc && _lock.owns_lock() )
      {
         _cm->removeSuDescriptor( _desc->name.c_str() );
      }
      else
      {
         SDB_ASSERT( FALSE, "must own lock" );
      }
      _release();
   }

   void _dmsSuConstraintMap::contextDrop::_release()
   {
      _desc.reset();
      if ( _lock.owns_lock() )
      {
         _lock.unlock();
      }
   }

   _dmsSuConstraintMap::contextRename::~contextRename()
   {
      _release();
   }

   void _dmsSuConstraintMap::contextRename::abort()
   {
      _release();
   }

   const DMS_SU_DESCRIPTOR &_dmsSuConstraintMap::contextRename::getOldDescriptor() const
   {
      return _oldDesc;
   }

   void _dmsSuConstraintMap::contextRename::commit( const DMS_SU_DESCRIPTOR &newDesc )
   {
      SDB_ASSERT( newDesc && newDesc->isValid(), "must be valid" );
      if ( _oldDesc && newDesc && _lock1.owns_lock() && _lock2.owns_lock() )
      {
         INT32 rc = SDB_OK;
         _cm->removeSuDescriptor( _oldDesc->name.c_str() );
         rc = _cm->addSuDescriptor( newDesc );
         SDB_ASSERT( rc == SDB_OK, "must success" );
      }
      else
      {
         SDB_ASSERT( FALSE, "must own lock" );
      }
      _release();
   }

   void _dmsSuConstraintMap::contextRename::_release()
   {
      if ( UTIL_IS_VALID_CSUNIQUEID( _csUID ) )
      {
         _cm->removeTempUniqueID( _csUID );
      }
      _oldDesc.reset();
      if ( _lock1.owns_lock() )
      {
         _lock1.unlock();
      }
      if ( _lock2.owns_lock() )
      {
         _lock2.unlock();
      }
   }

   _dmsSuConstraintMap::contextChangeUniqueID::contextChangeUniqueID(
      _dmsSuConstraintMap *cm,
      std::unique_lock< std::mutex > &&lock,
      utilCSUniqueID newCSUID )
   : context( cm ), _lock( std::move( lock ) ), _csUID( newCSUID )
   {
   }

   _dmsSuConstraintMap::contextChangeUniqueID::~contextChangeUniqueID()
   {
      _release();
   }

   void _dmsSuConstraintMap::contextChangeUniqueID::_release()
   {
      if ( UTIL_IS_VALID_CSUNIQUEID( _csUID ) )
      {
         _cm->removeTempUniqueID( _csUID );
      }
      _oldDesc.reset();
      if ( _lock.owns_lock() )
      {
         _lock.unlock();
      }
   }

   const DMS_SU_DESCRIPTOR &_dmsSuConstraintMap::contextChangeUniqueID::getOldDescriptor() const
   {
      return _oldDesc;
   }

   void _dmsSuConstraintMap::contextChangeUniqueID::commit( const DMS_SU_DESCRIPTOR &newDesc )
   {
      SDB_ASSERT( newDesc && newDesc->isValid(), "must be valid" );
      if ( _oldDesc && newDesc && _lock.owns_lock() )
      {
         INT32 rc = SDB_OK;
         _cm->removeSuDescriptor( _oldDesc->name.c_str() );
         rc = _cm->addSuDescriptor( newDesc );
         SDB_ASSERT( rc == SDB_OK, "must success" );
      }
      else
      {
         SDB_ASSERT( FALSE, "must own lock" );
      }
      _release();
   }

   void _dmsSuConstraintMap::contextChangeUniqueID::abort()
   {
      _release();
   }

   DMS_SU_DESCRIPTOR _dmsSuConstraintMap::getSuDescriptor( const utilStringView &name )
   {
      DMS_SU_DESCRIPTOR desc = nullptr;
      SDB_ASSERT( !name.empty(), "collection space name can not be null" );

      try
      {
         ossScopedLock lock( &_mapLatch, SHARED );
         decltype( _nameToDesc )::iterator found = _nameToDesc.find( name );
         if ( _nameToDesc.end() != found )
         {
            desc = found->second;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDSEVERE, "occur exception: %s", e.what() );
         ossPanic();
      }
      return desc;
   }

   DMS_SU_DESCRIPTOR _dmsSuConstraintMap::getSuDescriptor( const CHAR *name )
   {
      return getSuDescriptor( utilStringView( name ) );
   }

   DMS_SU_DESCRIPTOR _dmsSuConstraintMap::getSuDescriptor( utilCSUniqueID csUID )
   {
      DMS_SU_DESCRIPTOR desc = nullptr;
      SDB_ASSERT( UTIL_IS_VALID_CSUNIQUEID( csUID ), "collection space unique id can not be null" );

      try
      {
         ossScopedLock lock( &_mapLatch, SHARED );
         decltype( _UIDToDesc )::iterator found = _UIDToDesc.find( csUID );
         if ( _UIDToDesc.end() != found )
         {
            desc = found->second;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "occur exception: %s", e.what() );
         ossPanic();
      }
      return desc;
   }

   INT32 _dmsSuConstraintMap::addSuDescriptor( const DMS_SU_DESCRIPTOR &desc )
   {
      INT32 rc = SDB_OK;
      ossScopedLock lock( &_mapLatch, EXCLUSIVE );
      rc = _upsert( desc );
      PD_RC_CHECK( rc, PDERROR, "failed to upsert descriptor of cs[%s], rc: %d",
                     desc->name.c_str(), rc );
   done:
      return rc;
   error:
      goto done;
   }

   void _dmsSuConstraintMap::removeSuDescriptor( const CHAR *name )
   {
      SDB_ASSERT( name && *name, "collection space name can not be null" );
      ossScopedLock lock( &_mapLatch, EXCLUSIVE );
      _remove( name );
   }

   void _dmsSuConstraintMap::removeSuDescriptor( utilCSUniqueID csUID )
   {
      SDB_ASSERT( UTIL_IS_VALID_CSUNIQUEID( csUID ), "collection space unique id must be valid" );
      ossScopedLock lock( &_mapLatch, EXCLUSIVE );
      _remove( csUID );
   }

   INT32 _dmsSuConstraintMap::prepareToCreate( const CHAR *name,
                                               utilCSUniqueID csUID,
                                               CONTEXT_CREATE &context )
   {
      INT32 rc = SDB_OK;
      try
      {
         std::unique_lock< std::mutex > lockCSName( _getHashLatch( name ) );
         CONTEXT_CREATE temp( SDB_OSS_NEW contextCreate( this, std::move( lockCSName ), csUID ) );
         PD_CHECK( temp, SDB_OOM, error, PDERROR, "out of memory" );
         context = std::move( temp );
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e );
         PD_LOG( PDERROR, "occur exception: %s, rc: %d", e.what(), rc );
         goto error;
      }

      {
         BOOLEAN exist = FALSE;
         ossScopedLock lock( &_mapLatch, SHARED );
         rc = _testSuDescriptor( name, csUID, exist );
         PD_RC_CHECK( rc, PDERROR, "failed to test whether collection space[%s] descriptor exists",
                      name );
         if ( exist )
         {
            rc = SDB_DMS_CS_EXIST;
            PD_LOG( PDERROR, "invalid collection space name[%s] or unique id[%d], rc: %d", name,
                    csUID, rc );
            goto error;
         }
         else
         {
            if ( UTIL_IS_VALID_CSUNIQUEID( csUID ) )
            {
               rc = _insertTempUniqueID( csUID );
               PD_RC_CHECK( rc, PDERROR, "failed to insert collection space unique id[%d]", csUID );
            }
         }
      }

   done:
      return rc;
   error:
      context.reset();
      goto done;
   }

   INT32 _dmsSuConstraintMap::prepareToDrop( const CHAR *name, CONTEXT_DROP &context )
   {
      INT32 rc = SDB_OK;
      try
      {
         std::unique_lock< std::mutex > lockCSName( _getHashLatch( name ) );
         DMS_SU_DESCRIPTOR desc = getSuDescriptor( name );
         if ( !desc )
         {
            rc = SDB_DMS_CS_NOTEXIST;
            PD_LOG( PDERROR, "the collection space[%s] does not exist", name );
            goto error;
         }
         CONTEXT_DROP temp( SDB_OSS_NEW contextDrop( this, std::move( lockCSName ), desc ) );
         PD_CHECK( temp, SDB_OOM, error, PDERROR, "out of memory" );
         context = std::move( temp );
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e );
         PD_LOG( PDERROR, "occur exception: %s, rc: %d", e.what(), rc );
         goto error;
      }

   done:
      return rc;
   error:
      context.reset();
      goto done;
   }

   INT32 _dmsSuConstraintMap::prepareToRename( const CHAR *oldName,
                                               const CHAR *newName,
                                               CONTEXT_RENAME &context )
   {
      INT32 rc = SDB_OK;
      try
      {
         std::mutex *latchSmaller = nullptr;
         std::mutex *latchLarger = nullptr;
         std::unique_lock< std::mutex > lockSmaller;
         std::unique_lock< std::mutex > lockLarger;
         _getOrderedLatches( oldName, newName, &latchSmaller, &latchLarger );
         if ( latchSmaller )
         {
            lockSmaller = std::unique_lock< std::mutex >( *latchSmaller, std::adopt_lock );
         }
         if ( latchLarger )
         {
            lockLarger = std::unique_lock< std::mutex >( *latchLarger, std::adopt_lock );
         }

         DMS_SU_DESCRIPTOR oldDesc = getSuDescriptor( oldName );
         if ( !oldDesc )
         {
            rc = SDB_DMS_CS_EXIST;
            PD_LOG( PDERROR, "the collection space[%s] does not exists, rc: %d", oldName, rc );
            goto error;
         }
         else
         {
            utilCSUniqueID csUID = oldDesc->csUID;
            CONTEXT_RENAME temp( SDB_OSS_NEW contextRename(
               this, std::move( lockSmaller ), std::move( lockLarger ), csUID, oldDesc ) );
            PD_CHECK( temp, SDB_OOM, error, PDERROR, "out of memory" );
            context = std::move( temp );

            {
               DMS_SU_DESCRIPTOR desc = getSuDescriptor( newName );
               PD_RC_CHECK( rc, PDERROR,
                            "failed to test whether collection space[%s] descriptor exists, rc: %d",
                            oldName, rc );
               if ( desc )
               {
                  rc = SDB_DMS_CS_EXIST;
                  PD_LOG( PDERROR, "the collection space[%s] already exists", newName );
                  goto error;
               }
               else
               {
                  if ( UTIL_IS_VALID_CSUNIQUEID( csUID ) )
                  {
                     rc = _insertTempUniqueID( csUID );
                     PD_RC_CHECK( rc, PDERROR, "failed to insert collection space unique id[%d]",
                                  csUID );
                  }
               }
            }
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e );
         PD_LOG( PDERROR, "occur exception: %s, rc: %d", e.what(), rc );
         goto error;
      }

   done:
      return rc;
   error:
      context.reset();
      goto done;
   }

   INT32 _dmsSuConstraintMap::prepareToChangeUniqueID( const CHAR *name,
                                                       utilCSUniqueID newCSUID,
                                                       CONTEXT_CHANGE_UNIQUE_ID &context )
   {
      INT32 rc = SDB_OK;
      try
      {
         std::unique_lock< std::mutex > lockCSName( _getHashLatch( name ) );
         DMS_SU_DESCRIPTOR desc = getSuDescriptor( name );
         if ( !desc )
         {
            rc = SDB_DMS_CS_NOTEXIST;
            PD_LOG( PDERROR, "the collection space[%s] does not exist", name );
            goto error;
         }
         if ( desc->csUID == newCSUID )
         {
            goto done;
         }
         CONTEXT_CHANGE_UNIQUE_ID temp(
            SDB_OSS_NEW contextChangeUniqueID( this, std::move( lockCSName ), newCSUID ) );
         PD_CHECK( temp, SDB_OOM, error, PDERROR, "out of memory" );
         context = std::move( temp );

         {
            DMS_SU_DESCRIPTOR testDesc = getSuDescriptor( newCSUID );
            PD_RC_CHECK( rc, PDERROR,
                         "failed to test whether collection space[%d] descriptor exists, rc: %d",
                         newCSUID, rc );
            if ( testDesc )
            {
               rc = SDB_DMS_CS_EXIST;
               PD_LOG( PDERROR, "the collection space[unique id: %d] already exists", name,
                       newCSUID );
               goto error;
            }
            else
            {
               if ( UTIL_IS_VALID_CSUNIQUEID( newCSUID ) )
               {
                  rc = _insertTempUniqueID( newCSUID );
                  PD_RC_CHECK( rc, PDERROR, "failed to insert collection space unique id[%d]",
                               newCSUID );
               }
            }
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e );
         PD_LOG( PDERROR, "occur exception: %s, rc: %d", e.what(), rc );
         goto error;
      }
   done:
      return rc;
   error:
      context.reset();
      goto done;
   }

   INT32 _dmsSuConstraintMap::_insertTempUniqueID( utilCSUniqueID csUID )
   {
      INT32 rc = SDB_OK;
      if ( !UTIL_IS_VALID_CSUNIQUEID( csUID ) )
      {
         rc = SDB_INVALIDARG;
         PD_LOG( PDERROR, "collection space unique id can not be null" );
         goto error;
      }

      try
      {
         ossScopedLock lock( &_setLatch, EXCLUSIVE );
         _tempUniqueIDs.insert( csUID );
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e );
         PD_LOG( PDERROR, "occur exception: %s, rc: %d", e.what(), rc );
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 _dmsSuConstraintMap::_testSuDescriptor( const CHAR *name,
                                                 utilCSUniqueID csUID,
                                                 BOOLEAN &exist )
   {
      INT32 rc = SDB_OK;
      if ( !name || !( *name ) )
      {
         rc = SDB_INVALIDARG;
         PD_LOG( PDERROR, "collection space name can not be null" );
         goto error;
      }

      try
      {
         decltype( _nameToDesc )::iterator found = _nameToDesc.find( name );
         if ( _nameToDesc.end() != found )
         {
            exist = TRUE;
            PD_LOG( PDERROR, "the collection space[name: %s] already exists", name );
         }
         else
         {
            decltype( _UIDToDesc )::iterator found = _UIDToDesc.find( csUID );
            if ( _UIDToDesc.end() != found )
            {
               exist = TRUE;
               PD_LOG( PDERROR, "the collection space[unique id: %d] already exists", csUID );
            }
            else
            {
               exist = FALSE;
            }
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e );
         PD_LOG( PDERROR, "occur exception: %s, rc: %d", e.what(), rc );
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   void _dmsSuConstraintMap::removeTempUniqueID( utilCSUniqueID csUID )
   {
      SDB_ASSERT( UTIL_IS_VALID_CSUNIQUEID( csUID ), "collection space unique id must be valid" );
      try
      {
         ossScopedLock lock( &_setLatch, EXCLUSIVE );
         _tempUniqueIDs.erase( csUID );
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDSEVERE, "occur exception: %s", e.what() );
         ossPanic();
      }
   }

   void _dmsSuConstraintMap::clear()
   {
      ossScopedLock lockMap( &_mapLatch, EXCLUSIVE );
      ossScopedLock lockSet( &_setLatch, EXCLUSIVE );
      _nameToDesc.clear();
      _UIDToDesc.clear();
      _tempUniqueIDs.clear();
   }

   INT32 _dmsSuConstraintMap::_upsert( const DMS_SU_DESCRIPTOR &desc )
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT( desc && desc->isValid(), "descriptor to add must be valid" );
      try
      {
         utilCSUniqueID csUID = desc->csUID;
         decltype( _nameToDesc )::value_type val = make_pair( utilStringView( desc->name ), desc );

         auto r = _nameToDesc.insert( val );
         if ( !r.second )
         {
            _nameToDesc.erase( r.first );
            r = _nameToDesc.insert( val );
            if ( !r.second )
            {
               rc = SDB_SYS;
               PD_RC_CHECK( rc, PDERROR, "failed to insert cs[%s] descriptor, rc: %d",
                            desc->name.c_str(), rc );
            }
         }

         if ( UTIL_IS_VALID_CSUNIQUEID( csUID ) )
         {
            _UIDToDesc[ csUID ] = desc;
         }
         else
         {
            if ( csUID == UTIL_UNIQUEID_NULL && !dmsIsSysCSName( desc->name.c_str() ) )
            {
               _nullCSUniqueIDCntInc();
            }
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e );
         PD_LOG( PDERROR, "occur exception: %s, rc: %d", e.what(), rc );
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   void _dmsSuConstraintMap::_remove( const CHAR *name )
   {
      SDB_ASSERT( name && *name, "collection space name can not be null" );
      try
      {
         decltype( _nameToDesc )::iterator found = _nameToDesc.find( name );
         if ( _nameToDesc.end() != found )
         {
            utilCSUniqueID csUID = found->second->csUID;
            _nameToDesc.erase( found );
            if ( UTIL_IS_VALID_CSUNIQUEID( csUID ) )
            {
               _UIDToDesc.erase( csUID );
            }
            else
            {
               if ( csUID == UTIL_UNIQUEID_NULL && !dmsIsSysCSName( name ) )
               {
                  _nullCSUniqueIDCntDec();
               }
            }
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDSEVERE, "occur exception: %s", e.what() );
         ossPanic();
      }
   }

   UINT32 _dmsSuConstraintMap::getNullCSUniqueID() const
   {
      return _nullCSUniqueIDCnt.load( std::memory_order_relaxed );
   }

   void _dmsSuConstraintMap::_remove( utilCSUniqueID csUID )
   {
      SDB_ASSERT( UTIL_IS_VALID_CSUNIQUEID( csUID ), "collection space unique id must be valid" );
      try
      {
         decltype( _UIDToDesc )::iterator found = _UIDToDesc.find( csUID );
         if ( _UIDToDesc.end() != found )
         {
            utilStringView csName = found->second->name;
            _nameToDesc.erase( csName );
            _UIDToDesc.erase( found );
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDSEVERE, "occur exception: %s", e.what() );
         ossPanic();
      }
   }

   std::mutex &_dmsSuConstraintMap::_getHashLatch( const CHAR *name )
   {

      return _hashLatches[ _getHashLatchPos( name ) ];
   }

   UINT64 _dmsSuConstraintMap::_getHashLatchPos( const CHAR *name )
   {
      return XXH64( name, ossStrlen( name ), HASH_SEED ) % HASH_LATCH_NUMBER;
   }

   void _dmsSuConstraintMap::_getOrderedLatches( const CHAR *name1,
                                                 const CHAR *name2,
                                                 std::mutex **latchSmaller,
                                                 std::mutex **latchLarger )
   {
      UINT64 pos1 = _getHashLatchPos( name1 );
      UINT64 pos2 = _getHashLatchPos( name2 );
      if ( pos1 <= pos2 )
      {
         *latchSmaller = _hashLatches + pos1;
         *latchLarger = _hashLatches + pos2;
      }
      else
      {
         *latchSmaller = _hashLatches + pos2;
         *latchLarger = _hashLatches + pos1;
      }
   }

   void _dmsSuConstraintMap::_nullCSUniqueIDCntInc()
   {
      _nullCSUniqueIDCnt.fetch_add( 1, std::memory_order_relaxed );
   }

   void _dmsSuConstraintMap::_nullCSUniqueIDCntDec()
   {
      _nullCSUniqueIDCnt.fetch_add( -1, std::memory_order_relaxed );
   }

} // namespace engine