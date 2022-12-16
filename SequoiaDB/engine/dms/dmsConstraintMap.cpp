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
   INT32 _dmsSuConstraintMap::getSuDescriptor( const CHAR *name, DMS_SU_DESCRIPTOR &desc )
   {
      INT32 rc = SDB_OK;
      desc.reset();
      if ( !name || !( *name ) )
      {
         rc = SDB_INVALIDARG;
         PD_LOG( PDERROR, "collection space name can not be null" );
         goto error;
      }

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
         rc = ossException2RC( &e );
         PD_LOG( PDERROR, "occur exception: %s, rc: %d", e.what(), rc );
         goto error;
      }
   done:
      return rc;
   error:
      desc.reset();
      goto done;
   }

   INT32 _dmsSuConstraintMap::getSuDescriptor( utilCSUniqueID csUID, DMS_SU_DESCRIPTOR &desc )
   {
      INT32 rc = SDB_OK;
      desc.reset();
      if ( !UTIL_IS_VALID_CSUNIQUEID( csUID ) )
      {
         rc = SDB_INVALIDARG;
         PD_LOG( PDERROR, "collection space unique id can not be null" );
         goto error;
      }

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
         rc = ossException2RC( &e );
         PD_LOG( PDERROR, "occur exception: %s, rc: %d", e.what(), rc );
         goto error;
      }
   done:
      return rc;
   error:
      desc.reset();
      goto done;
   }

   INT32 _dmsSuConstraintMap::addSuDescriptor( const DMS_SU_DESCRIPTOR &desc )
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT( desc, "descriptor to add can not be null" );
      try
      {
         utilCSUniqueID csUID = desc->csUID;
         ossScopedLock lock( &_mapLatch, EXCLUSIVE );
         _nameToDesc.emplace( desc->name, desc );
         if ( UTIL_IS_VALID_CSUNIQUEID( csUID ) )
         {
            _UIDToDesc.emplace( csUID, desc );
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
         BOOLEAN exist;
         ossScopedLock lock( &_mapLatch );
         rc = _testSuDescriptor( name, exist );
         PD_RC_CHECK( rc, PDERROR, "failed to test whether collection space[%s] descriptor exists",
                      name );
         if ( exist )
         {
            rc = SDB_DMS_CS_EXIST;
            PD_LOG( PDERROR, "the collection space[%s] already exists", name );
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
         CONTEXT_DROP temp( SDB_OSS_NEW contextDrop( this, std::move( lockCSName ) ) );
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
         BOOLEAN exist;
         ossScopedLock lock( &_mapLatch, SHARED );
         rc = _testSuDescriptor( name, exist );
         PD_RC_CHECK( rc, PDERROR, "failed to test whether collection space[%s] descriptor exists",
                      name );
         if ( !exist )
         {
            rc = SDB_DMS_CS_NOTEXIST;
            PD_LOG( PDERROR, "the collection space[%s] does not exist", name );
            goto error;
         }
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
         std::mutex &latch1 = _getHashLatch( oldName );
         std::mutex &latch2 = _getHashLatch( newName );
         if ( &latch1 == &latch2 )
         {
            latch1.lock();
         }
         else
         {
            std::lock( latch1, latch2 );
         }
         std::unique_lock< std::mutex > lock1( latch1, std::adopt_lock );
         std::unique_lock< std::mutex > lock2( latch2, std::adopt_lock );

         DMS_SU_DESCRIPTOR oldDesc = nullptr;
         rc = getSuDescriptor( oldName, oldDesc );
         PD_RC_CHECK( rc, PDERROR, "failed to collection space descriptor[%s], rc: %d", oldName,
                      rc );
         if ( !oldDesc )
         {
            rc = SDB_DMS_CS_EXIST;
            PD_LOG( PDERROR, "the collection space[%s] does not exists, rc: %d", oldName, rc );
            goto error;
         }
         else
         {
            utilCSUniqueID csUID = oldDesc->csUID;
            CONTEXT_RENAME temp(
               SDB_OSS_NEW contextRename( this, std::move( lock1 ), std::move( lock2 ), csUID ) );
            PD_CHECK( temp, SDB_OOM, error, PDERROR, "out of memory" );
            context = std::move( temp );

            {
               BOOLEAN exist = FALSE;
               ossScopedLock lock( &_mapLatch, SHARED );
               rc = _testSuDescriptor( newName, exist );
               PD_RC_CHECK( rc, PDERROR,
                            "failed to test whether collection space[%s] descriptor exists, rc: %d",
                            oldName, rc );
               if ( exist )
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
         ossScopedLock lock( &_setLatch );
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

   INT32 _dmsSuConstraintMap::_testSuDescriptor( const CHAR *name, BOOLEAN &exist )
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
         }
         else
         {
            exist = FALSE;
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
         ossScopedLock lock( &_setLatch );
         _tempUniqueIDs.erase( csUID );
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDWARNING, "occur exception: %s", e.what() );
      }
   }

   void _dmsSuConstraintMap::clear()
   {
      ossScopedLock lockMap( &_mapLatch );
      ossScopedLock lockSet( &_setLatch );
      _nameToDesc.clear();
      _UIDToDesc.clear();
      _tempUniqueIDs.clear();
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
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDWARNING, "occur exception: %s", e.what() );
      }
   }

   void _dmsSuConstraintMap::_remove( utilCSUniqueID csUID )
   {
      SDB_ASSERT( UTIL_IS_VALID_CSUNIQUEID( csUID ), "collection space unique id must be valid" );
      try
      {
         decltype( _UIDToDesc )::iterator found = _UIDToDesc.find( csUID );
         if ( _UIDToDesc.end() != found )
         {
            utilCSUniqueID csUID = found->second->csUID;
            _UIDToDesc.erase( found );
            if ( UTIL_IS_VALID_CSUNIQUEID( csUID ) )
            {
               _UIDToDesc.erase( csUID );
            }
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDWARNING, "occur exception: %s", e.what() );
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

} // namespace engine