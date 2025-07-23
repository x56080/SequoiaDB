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

   Source File Name = rtnAlter.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/03/2018  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#include "rtnAlter.hpp"
#include "pd.hpp"
#include "rtn.hpp"
#include "rtnTrace.hpp"
#include "pdTrace.hpp"
#include "pmdEDU.hpp"
#include "dpsLogWrapper.hpp"
#include "dpsOp2Record.hpp"
#include "rtnAlterJob.hpp"
#include "../bson/bson.hpp"

using namespace bson ;
using namespace std ;

namespace engine
{

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNALTER_TASK, "rtnAlter" )
   INT32 rtnAlter ( const CHAR * name,
                    const rtnAlterTask * task,
                    const rtnAlterOptions * options,
                    _pmdEDUCB * cb,
                    _dpsLogWrapper * dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNALTER_TASK ) ;

      switch ( task->getActionType() )
      {
         case RTN_ALTER_CL_CREATE_ID_INDEX :
         {
            rc = rtnCreateIDIndex( name, task, options, cb, dpsCB ) ;
            break ;
         }
         case RTN_ALTER_CL_DROP_ID_INDEX :
         {
            rc = rtnDropIDIndex( name, task, options, cb, dpsCB ) ;
            break ;
         }
         case RTN_ALTER_CL_ENABLE_SHARDING :
         {
            rc = rtnEnableSharding( name, task, options, cb, dpsCB ) ;
            break ;
         }
         case RTN_ALTER_CL_DISABLE_SHARDING :
         {
            rc = rtnDisableSharding( name, task, options, cb, dpsCB ) ;
            break ;
         }
         case RTN_ALTER_CL_ENABLE_COMPRESS :
         {
            rc = rtnEnableCompress( name, task, options, cb, dpsCB ) ;
            break ;
         }
         case RTN_ALTER_CL_DISABLE_COMPRESS :
         {
            rc = rtnDisableCompress( name, task, options, cb, dpsCB ) ;
            break ;
         }
         case RTN_ALTER_CL_SET_ATTRIBUTES :
         {
            rc = rtnAlterCLSetAttributes( name, task, options, cb, dpsCB ) ;
            break ;
         }
         case RTN_ALTER_CS_SET_ATTRIBUTES :
         {
            rc = rtnAlterCSSetAttributes( name, task, options, cb, dpsCB ) ;
            break ;
         }
         case RTN_ALTER_CL_CREATE_AUTOINC_FLD:
         case RTN_ALTER_CL_DROP_AUTOINC_FLD:
         {
            //TODO: data group should do nothing
            break ;
         }
         default :
         {
            rc = SDB_INVALIDARG ;
            break ;
         }
      }

      PD_RC_CHECK( rc, PDERROR, "Failed to run alter task [%s], rc: %d",
                   task->getActionName(), rc ) ;

      rc = rtnAlter2DPSLog( name, task, options, dpsCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to write DPS log, rc: %d", rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_RTNALTER_TASK, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNALTER, "rtnAlter" )
   INT32 rtnAlter ( const CHAR * name,
                    RTN_ALTER_OBJECT_TYPE objectType,
                    BSONObj alterObject,
                    _pmdEDUCB * cb,
                    _dpsLogWrapper * dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNALTER ) ;

      rtnAlterJob alterJob ;

      rc = alterJob.initialize( NULL, objectType, alterObject ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to initialize alter job, rc: %d", rc ) ;

      PD_CHECK( 0 == ossStrcmp( name, alterJob.getObjectName() ),
                SDB_INVALIDARG, error, PDERROR,
                "Failed to initialize alter job, rc: %d", rc ) ;

      if ( !alterJob.isEmpty() )
      {
         const rtnAlterOptions * options = alterJob.getOptions() ;
         const RTN_ALTER_TASK_LIST & alterTasks = alterJob.getAlterTasks() ;

         for ( RTN_ALTER_TASK_LIST::const_iterator iter = alterTasks.begin() ;
               iter != alterTasks.end() ;
               ++ iter )
         {
            const rtnAlterTask * task = ( *iter ) ;

            rc = rtnAlter( name, task, options, cb, dpsCB ) ;

            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to run alter task [%s], rc: %d",
                       task->getActionName(), rc ) ;
               if ( options->isIgnoreException() )
               {
                  rc = SDB_OK ;
                  continue ;
               }
               else
               {
                  goto error ;
               }
            }
         }
      }

   done :
      PD_TRACE_EXITRC( SDB_RTNALTER, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNALTER2DPSLOG, "rtnAlter2DPSLog" )
   INT32 rtnAlter2DPSLog ( const CHAR * name,
                           const rtnAlterTask * task,
                           const rtnAlterOptions * options,
                           _dpsLogWrapper * dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNALTER2DPSLOG ) ;

      SDB_ASSERT( NULL != name, "name is invalid" ) ;
      SDB_ASSERT( NULL != task, "task is invalid" ) ;

      if ( NULL != dpsCB )
      {
         dpsMergeInfo info ;
         info.setInfoEx( ~0, ~0, DMS_INVALID_EXTENT, NULL ) ;
         dpsLogRecord & record = info.getMergeBlock().record() ;

         BSONObj alterOptions ;
         BSONObj alterObject;

         if ( NULL != options )
         {
            alterOptions = options->toBSON() ;
         }
         alterObject = task->toBSON( name, alterOptions ) ;

         rc = dpsAlter2Record( name, task->getObjectType(), alterObject, record ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build alter log, rc: %d", rc ) ;

         rc = dpsCB->prepare(info ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to prepare analyze log, rc: %d", rc ) ;

         dpsCB->writeData( info ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_RTNALTER2DPSLOG, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCREATEIDINDEX, "rtnCreateIDIndex" )
   INT32 rtnCreateIDIndex ( const CHAR * name,
                            const rtnAlterTask * task,
                            const rtnAlterOptions * options,
                            _pmdEDUCB * cb,
                            _dpsLogWrapper * dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNCREATEIDINDEX ) ;

      SDB_ASSERT( NULL != name, "name is invalid" ) ;
      SDB_ASSERT( NULL != cb, "cb is invalid" ) ;
      SDB_ASSERT( NULL != task &&
                  RTN_ALTER_CL_CREATE_ID_INDEX == task->getActionType(),
                  "task is invalid" ) ;

      SDB_DMSCB * dmsCB = sdbGetDMSCB() ;

      dmsStorageUnit * su = NULL ;
      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      BOOLEAN writable = FALSE ;
      const CHAR * collectionShortName = NULL ;
      dmsMBContext * mbContext = NULL ;

      const rtnCLCreateIDIndexTask * localTask = dynamic_cast<const rtnCLCreateIDIndexTask *>( task ) ;
      PD_CHECK( NULL != localTask, SDB_SYS, error, PDERROR,
                "Failed to get create id index task" ) ;

      rc = dmsCB->writable( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc = %d", rc ) ;
      writable = TRUE ;

      rc = rtnResolveCollectionNameAndLock ( name, dmsCB, &su,
                                             &collectionShortName, suID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to resolve collection name %s, rc: %d",
                   name, rc ) ;

      rc = su->data()->getMBContext( &mbContext, collectionShortName, EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to lock mb context [%s], rc: %d",
                   name, rc ) ;

      PD_CHECK( NULL != localTask, SDB_INVALIDARG, error, PDERROR,
                "Failed to get create id index task" ) ;

      rc = su->createIndex( collectionShortName, ixmGetIDIndexDefine(), cb, dpsCB,
                            TRUE, mbContext, localTask->getSortBufferSize() ) ;
      if ( SDB_IXM_REDEF == rc || SDB_IXM_EXIST_COVERD_ONE == rc )
      {
         /// already exists
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to create id index on collection [%s], "
                   "rc: %d", name, rc ) ;

   done :
      if ( NULL != mbContext )
      {
         su->data()->releaseMBContext( mbContext ) ;
      }
      if ( DMS_INVALID_SUID != suID )
      {
         dmsCB->suUnlock( suID ) ;
      }
      if ( writable )
      {
         dmsCB->writeDown( cb ) ;
      }
      PD_TRACE_EXITRC( SDB_RTNCREATEIDINDEX, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNDROPIDINDEX, "rtnDropIDIndex" )
   INT32 rtnDropIDIndex ( const CHAR * name,
                          const rtnAlterTask * task,
                          const rtnAlterOptions * options,
                          _pmdEDUCB * cb,
                          _dpsLogWrapper * dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNDROPIDINDEX ) ;

      SDB_ASSERT( NULL != name, "name is invalid" ) ;
      SDB_ASSERT( NULL != cb, "cb is invalid" ) ;
      SDB_ASSERT( NULL != task &&
                  RTN_ALTER_CL_DROP_ID_INDEX == task->getActionType(),
                  "task is invalid" ) ;

      SDB_DMSCB * dmsCB = sdbGetDMSCB() ;

      dmsStorageUnit * su = NULL ;
      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      BOOLEAN writable = FALSE ;
      const CHAR * collectionShortName = NULL ;
      dmsMBContext * mbContext = NULL ;

      rc = dmsCB->writable( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc = %d", rc ) ;
      writable = TRUE ;

      rc = rtnResolveCollectionNameAndLock ( name, dmsCB, &su,
                                             &collectionShortName, suID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to resolve collection name %s, rc: %d",
                   name, rc ) ;

      rc = su->data()->getMBContext( &mbContext, collectionShortName, EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to lock mb context [%s], rc: %d",
                   name, rc ) ;

      rc = su->dropIndex( collectionShortName, IXM_ID_KEY_NAME, cb, dpsCB,
                          TRUE, mbContext ) ;
      if ( SDB_IXM_NOTEXIST == rc )
      {
         // Already dropped
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to drop id index on collection [%s], "
                   "rc: %d", name, rc ) ;

   done :
      if ( NULL != mbContext )
      {
         su->data()->releaseMBContext( mbContext ) ;
      }
      if ( DMS_INVALID_SUID != suID )
      {
         dmsCB->suUnlock( suID ) ;
      }
      if ( writable )
      {
         dmsCB->writeDown( cb ) ;
      }
      PD_TRACE_EXITRC( SDB_RTNDROPIDINDEX, rc ) ;
      return rc ;

   error :
      goto done ;
   }


   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNENABLESHARDING, "rtnEnableSharding" )
   INT32 rtnEnableSharding ( const CHAR * name,
                             const rtnAlterTask * task,
                             const rtnAlterOptions * options,
                             _pmdEDUCB * cb,
                             _dpsLogWrapper * dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNENABLESHARDING ) ;

      SDB_ASSERT( NULL != name, "name is invalid" ) ;
      SDB_ASSERT( NULL != cb, "cb is invalid" ) ;
      SDB_ASSERT( NULL != task && RTN_ALTER_CL_ENABLE_SHARDING == task->getActionType(),
                  "task is invalid" ) ;

      SDB_DMSCB * dmsCB = sdbGetDMSCB() ;

      dmsStorageUnit * su = NULL ;
      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      BOOLEAN writable = FALSE ;
      const CHAR * collectionShortName = NULL ;
      dmsMBContext * mbContext = NULL ;

      rtnCLShardingArgument argument ;

      const rtnCLEnableShardingTask * localTask =
                     dynamic_cast<const rtnCLEnableShardingTask *>( task ) ;
      PD_CHECK( NULL != localTask, SDB_SYS, error, PDERROR,
                "Failed to get enable sharding task" ) ;

      argument = localTask->getShardingArgument() ;

      rc = dmsCB->writable( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc = %d", rc ) ;
      writable = TRUE ;

      rc = rtnResolveCollectionNameAndLock ( name, dmsCB, &su,
                                             &collectionShortName, suID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to resolve collection name %s, rc: %d",
                   name, rc ) ;

      rc = su->data()->getMBContext( &mbContext, collectionShortName, EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to lock mb context [%s], rc: %d",
                   name, rc ) ;

      rc = rtnCollectionCheckSharding( name, argument, cb, mbContext, su, dmsCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check sharding, rc: %d", rc ) ;

      rc = rtnCollectionSetSharding( name, argument, cb, mbContext, su, dmsCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to set sharding, rc: %d", rc ) ;

   done :
      if ( NULL != mbContext )
      {
         su->data()->releaseMBContext( mbContext ) ;
      }
      if ( DMS_INVALID_SUID != suID )
      {
         dmsCB->suUnlock( suID ) ;
      }
      if ( writable )
      {
         dmsCB->writeDown( cb ) ;
      }
      PD_TRACE_EXITRC( SDB_RTNENABLESHARDING, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNDISABLESHARDING, "rtnDisableSharding" )
   INT32 rtnDisableSharding ( const CHAR * name,
                              const rtnAlterTask * task,
                              const rtnAlterOptions * options,
                              _pmdEDUCB * cb,
                              _dpsLogWrapper * dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNDISABLESHARDING ) ;

      SDB_ASSERT( NULL != name, "name is invalid" ) ;
      SDB_ASSERT( NULL != cb, "cb is invalid" ) ;
      SDB_ASSERT( NULL != task &&
                  RTN_ALTER_CL_DISABLE_SHARDING == task->getActionType(),
                  "task is invalid" ) ;

      SDB_DMSCB * dmsCB = sdbGetDMSCB() ;

      dmsStorageUnit * su = NULL ;
      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      BOOLEAN writable = FALSE ;
      const CHAR * collectionShortName = NULL ;
      dmsMBContext * mbContext = NULL ;

      rtnCLShardingArgument argument ;
      argument.setEnsureShardingIndex( FALSE ) ;

      rc = dmsCB->writable( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc = %d", rc ) ;
      writable = TRUE ;

      rc = rtnResolveCollectionNameAndLock ( name, dmsCB, &su,
                                             &collectionShortName, suID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to resolve collection name %s, rc: %d",
                   name, rc ) ;

      rc = su->data()->getMBContext( &mbContext, collectionShortName, EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to lock mb context [%s], rc: %d",
                   name, rc ) ;

      PD_CHECK( DMS_STORAGE_NORMAL == su->type(),
                SDB_OPTION_NOT_SUPPORT, error, PDERROR,
                "Failed to check collection for sharding: "
                "should not be capped" ) ;

      rc = rtnCollectionSetSharding( name, argument, cb, mbContext, su, dmsCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to set sharding, rc: %d", rc ) ;

   done :
      if ( NULL != mbContext )
      {
         su->data()->releaseMBContext( mbContext ) ;
      }
      if ( DMS_INVALID_SUID != suID )
      {
         dmsCB->suUnlock( suID ) ;
      }
      if ( writable )
      {
         dmsCB->writeDown( cb ) ;
      }
      PD_TRACE_EXITRC( SDB_RTNDISABLESHARDING, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNSETSHARD, "rtnCollectionSetSharding" )
   INT32 rtnCollectionSetSharding ( const CHAR * collection,
                                    const rtnCLShardingArgument & argument,
                                    _pmdEDUCB * cb,
                                    _dmsMBContext * mbContext,
                                    _dmsStorageUnit * su,
                                    _SDB_DMSCB * dmsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNSETSHARD ) ;

      SDB_ASSERT( NULL != mbContext, "mbContext is invalid" ) ;
      SDB_ASSERT( NULL != su, "su is invalid" ) ;
      SDB_ASSERT( NULL != dmsCB, "dmsCB is invalid" ) ;

      const CHAR * collectionShortName = NULL ;
      dmsMB * mb = NULL ;
      BOOLEAN dropIndex = FALSE ;
      BOOLEAN createIndex = TRUE ;
      monIndex shardIndex ;

      mb = mbContext->mb() ;
      SDB_ASSERT( NULL != mb, "mb is invalid" ) ;

      collectionShortName = mb->_collectionName ;

      rc = su->getIndex( mbContext, IXM_SHARD_KEY_NAME, shardIndex ) ;
      if ( SDB_OK == rc )
      {
         if ( argument.getShardingKey().isEmpty() )
         {
            dropIndex = TRUE ;
            createIndex = FALSE ;
         }
         else if ( 0 == shardIndex.getKeyPattern().woCompare( argument.getShardingKey() ) )
         {
            dropIndex = FALSE ;
            createIndex = FALSE ;
         }
         else
         {
            dropIndex = TRUE ;
            createIndex = TRUE ;
         }
      }
      else if ( SDB_IXM_NOTEXIST == rc )
      {
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get index [%s] on collection [%s], rc: %d",
                   IXM_SHARD_KEY_NAME, collection, rc ) ;

      if ( dropIndex )
      {
         rc = su->dropIndex( collectionShortName, IXM_SHARD_KEY_NAME, cb, NULL,
                             TRUE, mbContext ) ;
         if ( SDB_IXM_NOTEXIST == rc )
         {
            rc = SDB_OK ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to drop id index on collection [%s], "
                      "rc: %d", collection, rc ) ;
      }

      if ( argument.isEnsureShardingIndex() && createIndex )
      {
         BSONObj indexDef = BSON( IXM_FIELD_NAME_KEY << argument.getShardingKey() <<
                                  IXM_FIELD_NAME_NAME << IXM_SHARD_KEY_NAME <<
                                  IXM_V_FIELD << 0 ) ;

         rc = su->createIndex( collectionShortName, indexDef, cb, NULL, TRUE,
                               mbContext, 0 ) ;
         if ( SDB_IXM_REDEF == rc || SDB_IXM_EXIST_COVERD_ONE == rc )
         {
            /// sharding key index already exists.
            rc = SDB_OK ;
            goto done ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to create %s index on "
                      "collection [%s], rc: %d", IXM_SHARD_KEY_NAME,
                      collection, rc ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_RTNSETSHARD, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCHKSHARD, "rtnCollectionCheckSharding" )
   INT32 rtnCollectionCheckSharding ( const CHAR * collection,
                                      const rtnCLShardingArgument & argument,
                                      _pmdEDUCB * cb,
                                      _dmsMBContext * mbContext,
                                      _dmsStorageUnit * su,
                                      _SDB_DMSCB * dmsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNCHKSHARD ) ;

      SDB_ASSERT( NULL != mbContext, "mbContext is invalid" ) ;
      SDB_ASSERT( NULL != su, "su is invalid" ) ;
      SDB_ASSERT( NULL != dmsCB, "dmsCB is invalid" ) ;

      dmsMB * mb = NULL ;
      MON_IDX_LIST indexes ;

      BSONObj shardingKey = argument.getShardingKey() ;

      PD_CHECK( DMS_STORAGE_NORMAL == su->type(),
                SDB_OPTION_NOT_SUPPORT, error, PDERROR,
                "Failed to check collection for sharding: "
                "should not be capped" ) ;

      mb = mbContext->mb() ;
      SDB_ASSERT( NULL != mb, "mb is invalid" ) ;

      rc = su->getIndexes( mbContext, indexes ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get index on collection [%s], rc: %d",
                   collection, rc ) ;

      for ( MON_IDX_LIST::iterator iterIndex = indexes.begin() ;
            iterIndex != indexes.end() ;
            iterIndex ++ )
      {
         monIndex & index = (*iterIndex) ;
         UINT16 indexType = IXM_EXTENT_TYPE_NONE ;

         rc = index.getIndexType( indexType ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get index type, rc: %d", rc ) ;

         // Exclude non-unique index, text index and system index
         if ( !index.isUnique() ||
              index.getIndexName()[ 0 ] == '$' ||
              IXM_EXTENT_TYPE_TEXT == indexType )
         {
            continue ;
         }

         // Check the sharding key which should be included in keys of
         // unique index
         try
         {
            BSONObj indexKey = index.getKeyPattern() ;
            BSONObjIterator shardingItr ( shardingKey ) ;
            while ( shardingItr.more() )
            {
               BSONElement sk = shardingItr.next() ;
               PD_CHECK( indexKey.hasField( sk.fieldName() ),
                         SDB_SHARD_KEY_NOT_IN_UNIQUE_KEY, error, PDERROR,
                         "All fields in sharding key must be included in "
                         "unique index, missing field: %s, shardingKey: %s, "
                         "indexKey: %s, collection: %s",
                         sk.fieldName(), shardingKey.toString().c_str(),
                         indexKey.toString().c_str(), collection ) ;
            }
         }
         catch( exception & e )
         {
            PD_LOG( PDWARNING, "Occur exception: %s", e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }

   done :
      PD_TRACE_EXITRC( SDB_RTNCHKSHARD, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNSETCOMPRESS_ARG, "rtnCollectionSetCompress" )
   INT32 rtnCollectionSetCompress ( const CHAR * collection,
                                    const rtnCLCompressArgument & argument,
                                    _pmdEDUCB * cb,
                                    _dmsMBContext * mbContext,
                                    _dmsStorageUnit * su,
                                    SDB_DMSCB * dmsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNSETCOMPRESS_ARG ) ;

      if ( argument.isCompressed() )
      {
         // Alter the attribute only in below cases :
         // 1. collection is no compressed
         // 2. altering the compressor type, and old type of collection is
         //    different
         if ( !OSS_BIT_TEST( mbContext->mb()->_attributes, DMS_MB_ATTR_COMPRESSED ) ||
              ( argument.testArgumentMask( UTIL_CL_COMPRESSTYPE_FIELD ) &&
                mbContext->mb()->_compressorType != argument.getCompressorType() ) )
         {
            rc = rtnCollectionSetCompress( collection, argument.getCompressorType(),
                                           cb, mbContext, su, dmsCB ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to set compressor on "
                         "collection [%s], rc: %d", collection, rc ) ;
         }
      }
      else
      {
         rc = rtnCollectionSetCompress( collection, UTIL_COMPRESSOR_INVALID,
                                        cb, mbContext, su, dmsCB ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to set compressor on "
                      "collection [%s], rc: %d", collection, rc ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_RTNSETCOMPRESS_ARG, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNSETCOMPRESS, "rtnCollectionSetCompress" )
   INT32 rtnCollectionSetCompress ( const CHAR * collection,
                                    UTIL_COMPRESSOR_TYPE compressorType,
                                    _pmdEDUCB * cb,
                                    _dmsMBContext * mbContext,
                                    _dmsStorageUnit * su,
                                    SDB_DMSCB * dmsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNSETCOMPRESS ) ;

      SDB_ASSERT( NULL != mbContext, "mbContext is invalid" ) ;
      SDB_ASSERT( NULL != su, "su is invalid" ) ;
      SDB_ASSERT( NULL != dmsCB, "dmsCB is invalid" ) ;

      dmsMB * mb = mbContext->mb() ;
      SDB_ASSERT( NULL != mb, "mb is invalid" ) ;

      rc = su->setCollectionCompressor( mb->_collectionName,
                                        compressorType, mbContext ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to set compressor on collection [%s], "
                   "rc: %d", collection, rc ) ;

      if ( OSS_BIT_TEST( mb->_attributes, DMS_MB_ATTR_COMPRESSED ) &&
           UTIL_COMPRESSOR_LZW == mb->_compressorType  &&
           DMS_INVALID_EXTENT == mb->_dictExtentID )
      {
         /// Build the dictionary for LZW compression if needed
         dmsCB->pushDictJob( dmsDictJob( su->CSID(), su->LogicalCSID(),
                                         mbContext->mbID(), mbContext->clLID() ) ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_RTNSETCOMPRESS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNSETEXTOPTS, "rtnCollectionSetExtOptions" )
   INT32 rtnCollectionSetExtOptions ( const CHAR * collection,
                                      const rtnCLExtOptionArgument & optionArgument,
                                      _pmdEDUCB * cb,
                                      _dmsMBContext * mbContext,
                                      _dmsStorageUnit * su,
                                      _SDB_DMSCB * dmsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNSETEXTOPTS ) ;

      SDB_ASSERT( NULL != mbContext, "mbContext is invalid" ) ;
      SDB_ASSERT( NULL != su, "su is invalid" ) ;
      SDB_ASSERT( NULL != dmsCB, "dmsCB is invalid" ) ;

      BSONObj curExtOptions, extOptions ;
      dmsMB * mb = mbContext->mb() ;
      SDB_ASSERT( NULL != mb, "mb is invalid" ) ;

      const CHAR * collectionShortName = mb->_collectionName ;

      rc = su->getCollectionExtOptions( collectionShortName, curExtOptions,
                                        mbContext ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get extent options on collection [%s], "
                   "rc: %d", collection, rc ) ;

      rc = optionArgument.toBSON( curExtOptions, extOptions ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to generate new ext options, rc: %d", rc ) ;

      rc = su->setCollectionExtOptions( collectionShortName, extOptions, mbContext ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to set compressor on collection [%s], "
                   "rc: %d", collection, rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_RTNSETEXTOPTS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNENABLECOMPRESS, "rtnEnableCompress" )
   INT32 rtnEnableCompress ( const CHAR * name,
                             const rtnAlterTask * task,
                             const rtnAlterOptions * options,
                             _pmdEDUCB * cb,
                             _dpsLogWrapper * dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNENABLECOMPRESS ) ;

      SDB_DMSCB * dmsCB = sdbGetDMSCB() ;

      dmsStorageUnit * su = NULL ;
      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      BOOLEAN writable = FALSE ;
      const CHAR * collectionShortName = NULL ;
      dmsMBContext * mbContext = NULL ;

      const rtnCLEnableCompressTask * localTask = NULL ;

      localTask = dynamic_cast<const rtnCLEnableCompressTask *>( task ) ;
      PD_CHECK( NULL != localTask, SDB_INVALIDARG, error, PDERROR,
                "Failed to get task" ) ;

      rc = dmsCB->writable( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc = %d", rc ) ;
      writable = TRUE ;

      rc = rtnResolveCollectionNameAndLock ( name, dmsCB, &su,
                                             &collectionShortName, suID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to resolve collection name %s, rc: %d",
                   name, rc ) ;

      rc = su->data()->getMBContext( &mbContext, collectionShortName, EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to lock mb context [%s], rc: %d",
                   name, rc ) ;

      rc = rtnCollectionSetCompress( name, localTask->getCompressArgument(),
                                     cb, mbContext, su, dmsCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to set compress on collection [%s], rc: %d",
                   name, rc ) ;

   done :
      if ( NULL != mbContext )
      {
         su->data()->releaseMBContext( mbContext ) ;
      }
      if ( DMS_INVALID_SUID != suID )
      {
         dmsCB->suUnlock( suID ) ;
      }
      if ( writable )
      {
         dmsCB->writeDown( cb ) ;
      }
      PD_TRACE_EXITRC( SDB_RTNENABLECOMPRESS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNDISABLECOMPRESS, "rtnDisableCompress" )
   INT32 rtnDisableCompress ( const CHAR * name,
                              const rtnAlterTask * task,
                              const rtnAlterOptions * options,
                              _pmdEDUCB * cb,
                              _dpsLogWrapper * dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNDISABLECOMPRESS ) ;

      SDB_DMSCB * dmsCB = sdbGetDMSCB() ;

      dmsStorageUnit * su = NULL ;
      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      BOOLEAN writable = FALSE ;
      const CHAR * collectionShortName = NULL ;
      dmsMBContext * mbContext = NULL ;

      rc = dmsCB->writable( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc = %d", rc ) ;
      writable = TRUE ;

      rc = rtnResolveCollectionNameAndLock ( name, dmsCB, &su,
                                             &collectionShortName, suID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to resolve collection name %s, rc: %d",
                   name, rc ) ;

      rc = su->data()->getMBContext( &mbContext, collectionShortName, EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to lock mb context [%s], rc: %d",
                   name, rc ) ;

      rc = rtnCollectionSetCompress( name, UTIL_COMPRESSOR_INVALID, cb, mbContext, su, dmsCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to set compress on collection [%s], rc: %d",
                   name, rc ) ;

   done :
      if ( NULL != mbContext )
      {
         su->data()->releaseMBContext( mbContext ) ;
      }
      if ( DMS_INVALID_SUID != suID )
      {
         dmsCB->suUnlock( suID ) ;
      }
      if ( writable )
      {
         dmsCB->writeDown( cb ) ;
      }
      PD_TRACE_EXITRC( SDB_RTNDISABLECOMPRESS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNALTERCLCHKATTR_INT, "rtnAlterCLCheckAttributes" )
   INT32 rtnAlterCLCheckAttributes ( const CHAR * collection,
                                     const rtnAlterTask * task,
                                     _dmsMBContext * mbContext,
                                     _dmsStorageUnit * su,
                                     _SDB_DMSCB * dmsCB,
                                     _pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNALTERCLCHKATTR_INT ) ;

      const rtnCLSetAttributeTask * localTask =
            dynamic_cast<const rtnCLSetAttributeTask *>( task ) ;
      PD_CHECK( NULL != localTask, SDB_SYS, error, PDERROR,
                "Failed to get alter task" ) ;

      // Check sharding
      if ( localTask->testArgumentMask( UTIL_CL_SHDKEY_FIELD ) )
      {
         rc = rtnCollectionCheckSharding( collection, localTask->getShardingArgument(),
                                          cb, mbContext, su, dmsCB ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to check sharding, rc: %d", rc ) ;
      }

      // Check compress
      if ( localTask->containCompressArgument() )
      {
         rc = su->canSetCollectionCompressor( mbContext ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to check compress, "
                      "rc: %d", rc ) ;
      }

      // Check ext options
      if ( localTask->containExtOptionArgument() )
      {
         PD_CHECK( DMS_STORAGE_CAPPED == su->type(),
                   SDB_OPTION_NOT_SUPPORT, error, PDERROR,
                   "Failed to check collection for setting ext options: "
                   "should be capped" ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_RTNALTERCLCHKATTR_INT, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNALTERCLSETATTR_INT, "rtnAlterCLSetAttributes" )
   INT32 rtnAlterCLSetAttributes ( const CHAR * collection,
                                   const rtnAlterTask * task,
                                   _dmsMBContext * mbContext,
                                   _dmsStorageUnit * su,
                                   _SDB_DMSCB * dmsCB,
                                   _pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNALTERCLSETATTR_INT ) ;

      SDB_ASSERT( NULL != mbContext, "mbContext is invalid" ) ;
      SDB_ASSERT( NULL != su, "su is invalid" ) ;
      SDB_ASSERT( NULL != dmsCB, "dmsCB is invalid" ) ;

      const CHAR * collectionShortName = mbContext->mb()->_collectionName ;
      const rtnCLSetAttributeTask * localTask = NULL ;

      localTask = dynamic_cast<const rtnCLSetAttributeTask *>( task ) ;
      PD_CHECK( NULL != localTask, SDB_INVALIDARG, error, PDERROR,
                "Failed to get task" ) ;

      PD_CHECK( mbContext->isMBLock( EXCLUSIVE ), SDB_SYS, error, PDERROR,
                "Failed to get mbContext: should be exclusive locked" ) ;

      rc = rtnAlterCLCheckAttributes( collection, task, mbContext, su, dmsCB, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check attributes on collection [%s], "
                   "rc: %d", collection, rc ) ;

      // $sharding index
      if ( localTask->testArgumentMask( UTIL_CL_SHDKEY_FIELD ) )
      {
         const rtnCLShardingArgument & argument = localTask->getShardingArgument() ;

         rc = rtnCollectionSetSharding( collection, argument, cb, mbContext,
                                        su, dmsCB ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to set sharding on collection [%s], "
                      "rc: %d", collection, rc ) ;
      }

      // Compress
      if ( localTask->containCompressArgument() )
      {
         const rtnCLCompressArgument & compressArgument = localTask->getCompressArgument() ;
         rc = rtnCollectionSetCompress( collectionShortName, compressArgument,
                                        cb, mbContext, su, dmsCB ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to set compressor on "
                      "collection [%s], rc: %d", collection, rc ) ;
      }

      // Ext options
      if ( localTask->containExtOptionArgument() )
      {
         rc = rtnCollectionSetExtOptions( collectionShortName,
                                          localTask->getExtOptionArgument(),
                                          cb, mbContext, su, dmsCB ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to set capped options on "
                      "collection [%s], rc: %d", collection, rc ) ;
      }

      // Autoincrement options
      // do nothing
      if ( localTask->containAutoincArgument() )
      {
         rc = SDB_OK ;
      }

      // Strict data mode
      if ( localTask->testArgumentMask( UTIL_CL_STRICTDATAMODE_FIELD ) )
      {
         rc = su->setCollectionStrictDataMode( collectionShortName,
                                               localTask->isStrictDataMode(),
                                               mbContext ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to set strict data mode "
                      "on collection [%s], rc: %d", collection, rc ) ;
      }

      // $id index
      if ( localTask->testArgumentMask( UTIL_CL_AUTOIDXID_FIELD ) )
      {
         if ( localTask->isAutoIndexID() )
         {
            rc = su->createIndex( collection, ixmGetIDIndexDefine(), cb, NULL,
                                  TRUE, mbContext ) ;
            if ( SDB_IXM_REDEF == rc || SDB_IXM_EXIST_COVERD_ONE == rc )
            {
               rc = SDB_OK ;
            }
            PD_RC_CHECK( rc, PDERROR, "Failed to create id index" ) ;
         }
         else
         {
            rc = su->dropIndex( collection, IXM_ID_KEY_NAME, cb, NULL,
                                TRUE, mbContext ) ;
            if ( SDB_IXM_NOTEXIST == rc )
            {
               rc = SDB_OK ;
            }
            PD_RC_CHECK( rc, PDERROR, "Failed to drop id index" ) ;
         }
      }

   done :
      PD_TRACE_EXITRC( SDB_RTNALTERCLSETATTR_INT, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNALTERCLSETATTR, "rtnAlterCLSetAttributes" )
   INT32 rtnAlterCLSetAttributes ( const CHAR * name,
                                   const rtnAlterTask * task,
                                   const rtnAlterOptions * options,
                                   _pmdEDUCB * cb,
                                   _dpsLogWrapper * dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNALTERCLSETATTR ) ;

      SDB_DMSCB * dmsCB = sdbGetDMSCB() ;

      dmsStorageUnit * su = NULL ;
      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      BOOLEAN writable = FALSE ;
      const CHAR * collectionShortName = NULL ;
      dmsMBContext * mbContext = NULL ;

      rc = dmsCB->writable( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc: %d", rc ) ;
      writable = TRUE ;

      rc = rtnResolveCollectionNameAndLock ( name, dmsCB, &su,
                                             &collectionShortName, suID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to resolve collection name [%s], rc: %d",
                   name, rc ) ;

      rc = su->data()->getMBContext( &mbContext, collectionShortName, EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to lock mb context [%s], rc: %d",
                   name, rc ) ;

      rc = rtnAlterCLSetAttributes( name, task, mbContext, su, dmsCB, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to set attributes on collection [%s], "
                   "rc: %d", name, rc ) ;

   done :
      if ( NULL != mbContext )
      {
         su->data()->releaseMBContext( mbContext ) ;
      }
      if ( DMS_INVALID_SUID != suID )
      {
         dmsCB->suUnlock( suID ) ;
      }
      if ( writable )
      {
         dmsCB->writeDown( cb ) ;
      }
      PD_TRACE_EXITRC( SDB_RTNALTERCLSETATTR, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNALTERCSSETATTR_INT, "rtnAlterCSSetAttributes" )
   INT32 rtnAlterCSSetAttributes ( const CHAR * collectionSpace,
                                   const rtnAlterTask * task,
                                   _dmsStorageUnit * su,
                                   _SDB_DMSCB * dmsCB,
                                   _pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNALTERCSSETATTR_INT ) ;

      SDB_ASSERT( NULL != su, "su is invalid" ) ;
      SDB_ASSERT( NULL != dmsCB, "dmsCB is invalid" ) ;

      const rtnCSSetAttributeTask * localTask = NULL ;
      INT32 lobPageSize = DMS_DEFAULT_LOB_PAGE_SZ ;

      if ( !task->testArgumentMask( UTIL_CS_LOBPAGESIZE_FIELD ) )
      {
         goto done ;
      }

      localTask = dynamic_cast< const rtnCSSetAttributeTask * >( task ) ;
      PD_CHECK( NULL != localTask, SDB_SYS, error, PDERROR,
                "Failed to get set attributes task on collection space [%s]",
                collectionSpace ) ;

      lobPageSize = localTask->getLobPageSize() ;

      if ( lobPageSize != su->getLobPageSize() )
      {
         rc = su->setLobPageSize( (UINT32)lobPageSize ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to set LOB page size on "
                      "storage unit [%s], rc: %d", collectionSpace, rc ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_RTNALTERCSSETATTR_INT, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNALTERCSSETATTR, "rtnAlterCSSetAttributes" )
   INT32 rtnAlterCSSetAttributes ( const CHAR * name,
                                   const rtnAlterTask * task,
                                   const rtnAlterOptions * options,
                                   _pmdEDUCB * cb,
                                   _dpsLogWrapper * dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNALTERCSSETATTR ) ;

      SDB_DMSCB * dmsCB = sdbGetDMSCB() ;

      dmsStorageUnit * su = NULL ;
      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      BOOLEAN writable = FALSE ;

      rc = dmsCB->writable( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc: %d", rc ) ;
      writable = TRUE ;

      rc = dmsCB->nameToSUAndLock( name, suID, &su, EXCLUSIVE, OSS_ONE_SEC ) ;
      if ( SDB_TIMEOUT == rc )
      {
         rc = SDB_LOCK_FAILED ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to lock storage unit [%s], rc: %d",
                   name, rc ) ;

      rc = rtnAlterCSSetAttributes( name, task, su, dmsCB, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to set attributes on collection space "
                   "[%s], rc: %d", name, rc ) ;

   done :
      if ( DMS_INVALID_SUID != suID )
      {
         dmsCB->suUnlock( suID, EXCLUSIVE ) ;
      }
      if ( writable )
      {
         dmsCB->writeDown( cb ) ;
      }
      PD_TRACE_EXITRC( SDB_RTNALTERCSSETATTR, rc ) ;
      return rc ;

   error :
      goto done ;
   }
}
