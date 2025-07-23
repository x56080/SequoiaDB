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

   Source File Name = rtnLogRecordSerializer.cpp

   Descriptive Name = Log Record Serializer

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/01/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#include "rtnLogRecordSerializer.hpp"
#include "dmsStorageDataCommon.hpp"
#include "dpsLogRecord.hpp"
#include "dpsOp2Record.hpp"
#include "ossUtil.hpp"
#include "pd.hpp"
#include "pmd.hpp"
#include "pdTrace.hpp"
#include "pmdEnv.hpp"
#include "rtnAlterTask.hpp"
#include "rtnLobPieces.hpp"
#include "rtnTrace.hpp"

using namespace std ;
using namespace bson ;

namespace engine
{

   #define RTN_LOG_TMP_STR_SIZE ( 128 )

   /*
      _rtnLogRecordSerializer implement
    */
   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNLOGRECSERIALIZER_BUILDRECORD, "_rtnLogRecordSerializer::buildRecord" )
   INT32 _rtnLogRecordSerializer::buildRecord( const dpsLogRecord &record,
                                               BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNLOGRECSERIALIZER_BUILDRECORD ) ;

      try
      {
         dpsRecordTimeInfo timeInfo ;
         dpsRecordTransInfo transInfo ;

         const dpsLogRecordHeader &header = record.head() ;

         const CHAR *opName = dpsGetOPName( header._type ) ;

         CHAR flagStr[ DPS_RECORD_FLAGS_STATUS_LEN + 1 ] = { 0 } ;
         dpsFlags2String( header._flags, flagStr, DPS_RECORD_FLAGS_STATUS_LEN ) ;

         builder.append( FIELD_NAME_CHANGE_TYPE, opName ) ;
         builder.append( FIELD_NAME_CHANGE_FLAGS, flagStr ) ;

         switch ( header._type )
         {
         case LOG_TYPE_DATA_INSERT :
         {
            rc = _buildInsertRecord( record, builder, transInfo, timeInfo ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build [%s] record, rc: %d",
                         opName, rc ) ;
            break ;
         }
         case LOG_TYPE_DATA_UPDATE :
         {
            rc = _buildUpdateRecord( record, builder, transInfo, timeInfo ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build [%s] record, rc: %d",
                         opName, rc ) ;
            break ;
         }
         case LOG_TYPE_DATA_DELETE :
         {
            rc = _buildDeleteRecord( record, builder, transInfo, timeInfo ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build [%s] record, rc: %d",
                         opName, rc ) ;
            break ;
         }
         case LOG_TYPE_DATA_POP:
         {
            rc = _buildPopRecord( record, builder, transInfo, timeInfo ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build [%s] record, rc: %d",
                         opName, rc ) ;
            break ;
         }
         case LOG_TYPE_CS_CRT:
         {
            rc = _buildCreateCSRecord( record, builder, transInfo, timeInfo ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build [%s] record, rc: %d",
                         opName, rc ) ;
            break ;
         }
         case LOG_TYPE_CS_DELETE :
         {
            rc = _buildDeleteCSRecord( record, builder, transInfo, timeInfo ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build [%s] record, rc: %d",
                         opName, rc ) ;
            break ;
         }
         case LOG_TYPE_CS_RENAME:
         {
            rc = _buildRenameCSRecord( record, builder, transInfo, timeInfo ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build [%s] record, rc: %d",
                         opName, rc ) ;
            break ;
         }
         case LOG_TYPE_CL_CRT :
         {
            rc = _buildCreateCLRecord( record, builder, transInfo, timeInfo ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build [%s] record, rc: %d",
                         opName, rc ) ;
            break ;
         }
         case LOG_TYPE_CL_DELETE :
         {
            rc = _buildDeleteCLRecord( record, builder, transInfo, timeInfo ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build [%s] record, rc: %d",
                         opName, rc ) ;
            break ;
         }
         case LOG_TYPE_IX_CRT :
         {
            rc = _buildCreateIXRecord( record, builder, transInfo, timeInfo ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build [%s] record, rc: %d",
                         opName, rc ) ;
            break ;
         }
         case LOG_TYPE_IX_DELETE :
         {
            rc = _buildDeleteIXRecord( record, builder, transInfo, timeInfo ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build [%s] record, rc: %d",
                         opName, rc ) ;
            break ;
         }
         case LOG_TYPE_CL_RENAME :
         {
            rc = _buildRenameCLRecord( record, builder, transInfo, timeInfo ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build [%s] record, rc: %d",
                         opName, rc ) ;
            break ;
         }
         case LOG_TYPE_CL_TRUNC :
         {
            rc = _buildTruncateCLRecord( record, builder, transInfo, timeInfo ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build [%s] record, rc: %d",
                         opName, rc ) ;
            break ;
         }
         case LOG_TYPE_INVALIDATE_CATA :
         {
            rc = _buildInvalidateCataRecord( record, builder, transInfo, timeInfo ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build [%s] record, rc: %d",
                         opName, rc ) ;
            break ;
         }
         case LOG_TYPE_TS_COMMIT:
         {
            rc = _buildTransCommitRecord( record, builder, transInfo, timeInfo ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build [%s] record, rc: %d",
                         opName, rc ) ;
            break ;
         }
         case LOG_TYPE_LOB_WRITE :
         {
            rc = _buildLobWriteRecord( record, builder, transInfo, timeInfo ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build [%s] record, rc: %d",
                         opName, rc ) ;
            break ;
         }
         case LOG_TYPE_LOB_REMOVE :
         {
            rc = _buildLobRemoveRecord( record, builder, transInfo, timeInfo ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build [%s] record, rc: %d",
                         opName, rc ) ;
            break ;
         }
         case LOG_TYPE_LOB_UPDATE :
         {
            rc = _buildLobUpdateRecord( record, builder, transInfo, timeInfo ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build [%s] record, rc: %d",
                         opName, rc ) ;
            break ;
         }
         case LOG_TYPE_LOB_TRUNCATE :
         {
            rc = _buildLobTruncateRecord( record, builder, transInfo, timeInfo ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build [%s] record, rc: %d",
                         opName, rc ) ;
            break ;
         }
         case LOG_TYPE_ALTER :
         {
            rc = _buildAlterRecord( record, builder, transInfo, timeInfo ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build [%s] record, rc: %d",
                         opName, rc ) ;
            break ;
         }
         case LOG_TYPE_ADDUNIQUEID :
         {
            rc = _buildAddUniqueIDRecord( record, builder, transInfo, timeInfo ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build [%s] record, rc: %d",
                         opName, rc ) ;
            break ;
         }
         case LOG_TYPE_RETURN:
         {
            rc = _buildReturnRecord( record, builder, transInfo, timeInfo ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build [%s] record, rc: %d",
                         opName, rc ) ;
            break ;
         }
         case LOG_TYPE_DUMMY :
         case LOG_TYPE_TS_ROLLBACK :
         case LOG_TYPE_SEC_KEY_CRT :
         default:
         {
            rc = _buildDefaultRecord( record, builder, transInfo, timeInfo ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build [%s] record, rc: %d",
                         opName, rc ) ;
            break ;
         }
         }

         // trans info
         _transInfoToBSON( transInfo, builder ) ;

         // time info
         _timeInfoToBSON( timeInfo, builder ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build BSON object, occured exception: %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNLOGRECSERIALIZER_BUILDRECORD, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNLOGRECSERIALIZER__CSTOBSON, "_rtnLogRecordSerializer::_collectionSpaceToBSON" )
   void _rtnLogRecordSerializer::_collectionSpaceToBSON( const CHAR *name,
                                                         BSONObjBuilder &builder )
   {
      PD_TRACE_ENTRY( SDB__RTNLOGRECSERIALIZER__CSTOBSON ) ;

      builder.append( FIELD_NAME_COLLECTIONSPACE, name ) ;

      PD_TRACE_EXIT( SDB__RTNLOGRECSERIALIZER__CSTOBSON ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNLOGRECSERIALIZER__CLTOBSON, "_rtnLogRecordSerializer::_collectionToBSON" )
   void _rtnLogRecordSerializer::_collectionToBSON( const CHAR *name,
                                                    BSONObjBuilder &builder )
   {
      PD_TRACE_ENTRY( SDB__RTNLOGRECSERIALIZER__CLTOBSON ) ;

      const CHAR *p = ossStrnchr( name, '.', DMS_COLLECTION_FULL_NAME_SZ ) ;
      if ( NULL != p )
      {
         builder.appendStrWithNoTerminating( FIELD_NAME_COLLECTIONSPACE,
                                             name, p - name ) ;
         builder.append( FIELD_NAME_COLLECTION, name ) ;
      }
      else
      {
         SDB_ASSERT( FALSE, "should not be here" ) ;
      }

      PD_TRACE_EXIT( SDB__RTNLOGRECSERIALIZER__CLTOBSON ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNLOGRECSERIALIZER__OBJKEYTOBSON, "_rtnLogRecordSerializer::_objectKeyToBSON" )
   void _rtnLogRecordSerializer::_objectKeyToBSON( const BSONObj &object,
                                                   BSONObjBuilder &builder )
   {
      PD_TRACE_ENTRY( SDB__RTNLOGRECSERIALIZER__OBJKEYTOBSON ) ;

      BSONElement ele = object.getField( DMS_ID_KEY_NAME ) ;
      if ( EOO != ele.type() )
      {
         BSONObjBuilder keyBuilder( builder.subobjStart( FIELD_NAME_DOCUMENT_KEY ) ) ;
         keyBuilder.append( ele ) ;
         keyBuilder.doneFast() ;
      }

      PD_TRACE_EXIT( SDB__RTNLOGRECSERIALIZER__OBJKEYTOBSON ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNLOGRECSERIALIZER__TRANSIDTOBSON, "_rtnLogRecordSerializer::_transIDToBSON" )
   void _rtnLogRecordSerializer::_transIDToBSON( const DPS_TRANS_ID &transID,
                                                 BSONObjBuilder &builder )
   {
      PD_TRACE_ENTRY( SDB__RTNLOGRECSERIALIZER__TRANSIDTOBSON ) ;

      if ( DPS_INVALID_TRANS_ID != transID )
      {
         CHAR strTransID[ DPS_TRANS_STR_LEN + 1 ] = { 0 } ;
         CHAR strTransAttr[ DPS_TRANS_STR_LEN + 1 ] = { 0 } ;
         dpsTransIDToString( transID, strTransID, DPS_TRANS_STR_LEN ) ;
         dpsTransIDAttrToString( transID, strTransAttr, DPS_TRANS_STR_LEN ) ;

         builder.append( FIELD_NAME_TRANS_ID, strTransID ) ;
         builder.append( FIELD_NAME_TRANS_ATTR, strTransAttr ) ;
      }

      PD_TRACE_EXIT( SDB__RTNLOGRECSERIALIZER__TRANSIDTOBSON ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNLOGRECSERIALIZER__TRANSINFOTOBSON, "_rtnLogRecordSerializer::_transInfoToBSON" )
   void _rtnLogRecordSerializer::_transInfoToBSON(
                                          const dpsRecordTransInfo &transInfo,
                                          BSONObjBuilder &builder )
   {
      PD_TRACE_ENTRY( SDB__RTNLOGRECSERIALIZER__TRANSINFOTOBSON ) ;

      if ( DPS_INVALID_TRANS_ID != transInfo._transID )
      {
         BSONObjBuilder transBuilder( builder.subobjStart( FIELD_NAME_TRANS_INFO ) ) ;
         _transIDToBSON( transInfo._transID, transBuilder ) ;
         transBuilder.doneFast() ;
      }

      PD_TRACE_EXIT( SDB__RTNLOGRECSERIALIZER__TRANSINFOTOBSON ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNLOGRECSERIALIZER__TIMEINFOTOBSON, "_rtnLogRecordSerializer::_timeInfoToBSON" )
   void _rtnLogRecordSerializer::_timeInfoToBSON(
                                             const dpsRecordTimeInfo &timeInfo,
                                             BSONObjBuilder &builder )
   {
      PD_TRACE_ENTRY( SDB__RTNLOGRECSERIALIZER__TIMEINFOTOBSON ) ;

      if ( 0 != timeInfo._realTime ||
            0 != timeInfo._logicalTime )
      {
         BSONObjBuilder timeBuilder( builder.subobjStart( FIELD_NAME_TIME_INFO ) ) ;
         if ( 0 != timeInfo._realTime )
         {
            ossTimestamp realTime( timeInfo._realTime ) ;
            timeBuilder.appendTimestamp( FIELD_NAME_REAL_TIME,
                                         realTime.time, realTime.microtm ) ;
         }
         if ( 0 != timeInfo._logicalTime )
         {
            timeBuilder.append( FIELD_NAME_LOGICAL_TIME,
                                 (INT64)( timeInfo._logicalTime ) ) ;
         }
         timeBuilder.doneFast() ;
      }

      PD_TRACE_EXIT( SDB__RTNLOGRECSERIALIZER__TIMEINFOTOBSON ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNLOGRECSERIALIZER__BLDINSTREC, "_rtnLogRecordSerializer::_buildInsertRecord" )
   INT32 _rtnLogRecordSerializer::_buildInsertRecord(
                                                const dpsLogRecord &record,
                                                BSONObjBuilder &builder,
                                                dpsRecordTransInfo &transInfo,
                                                dpsRecordTimeInfo &timeInfo )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNLOGRECSERIALIZER__BLDINSTREC ) ;

      const CHAR *fullName = NULL ;
      BSONObj object ;

      // parse log
      rc = dpsRecord2Insert( record, &fullName, object,
                             &timeInfo, NULL, &transInfo ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse record, rc: %d", rc ) ;

      // collection name
      _collectionToBSON( fullName, builder ) ;

      // document key
      _objectKeyToBSON( object, builder ) ;

      // document
      builder.append( FIELD_NAME_DOCUMENT, object ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNLOGRECSERIALIZER__BLDINSTREC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNLOGRECSERIALIZER__BLDUPDTREC, "_rtnLogRecordSerializer::_buildUpdateRecord" )
   INT32 _rtnLogRecordSerializer::_buildUpdateRecord(
                                                const dpsLogRecord &record,
                                                BSONObjBuilder &builder,
                                                dpsRecordTransInfo &transInfo,
                                                dpsRecordTimeInfo &timeInfo )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNLOGRECSERIALIZER__BLDUPDTREC ) ;

      typedef ossPoolList< BSONObj > _dpsUpdateActionObjList ;
      typedef _dpsUpdateActionObjList::iterator _dpsUpdateActionObjListIter ;
      typedef _utilStringMap< _dpsUpdateActionObjList > _dpsUpdateActionMap ;
      typedef _dpsUpdateActionMap::iterator _dpsUpdateActionMapIter ;
      typedef _dpsUpdateActionMap::value_type _dpsUpdateActionMapValue ;
      typedef pair< _dpsUpdateActionMap::iterator, BOOLEAN > _dpsUpdateActionMapInstRes ;

      const CHAR *fullName = NULL ;
      BSONObj oldOID, oldObject, newOID, newObject ;
      UINT32 writeMode = DPS_LOG_WRITE_MODE_INCREMENT ;

      // parse log
      rc = dpsRecord2Update( record, &fullName, oldOID,
                             oldObject, newOID, newObject, NULL, NULL,
                             &timeInfo, &writeMode, NULL, NULL, &transInfo ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse record, rc: %d", rc ) ;

      // collection name
      _collectionToBSON( fullName, builder ) ;

      // document key
      builder.append( FIELD_NAME_DOCUMENT_KEY, oldOID ) ;

      // update action
      if ( DPS_LOG_WRITE_MODE_FULL == writeMode )
      {
         // for full mode, this is the full post-update image,
         // convert to $replace
         BSONObjBuilder subBuilder( builder.subobjStart( FIELD_NAME_UPDATE_ACTION ) ) ;
         subBuilder.append( CMD_ADMIN_PREFIX FIELD_OP_VALUE_REPLACE, newObject ) ;
         subBuilder.doneFast() ;
      }
      else
      {
         // check the update actions, merge the same actions if needed
         BOOLEAN needMerge = FALSE ;
         _dpsUpdateActionMap actionMap ;
         BSONObjIterator iter( newObject ) ;
         while ( iter.more() )
         {
            BSONElement element = iter.next() ;
            const CHAR *actionName = element.fieldName() ;
            PD_CHECK( '$' == actionName[ 0 ], SDB_DPS_CORRUPTED_LOG, error, PDERROR,
                      "Failed to parse update record, invalid action [%s]",
                      actionName ) ;
            PD_CHECK( Object == element.type(), SDB_DPS_CORRUPTED_LOG, error, PDERROR,
                      "Failed to parse update record, invalid action object [%s]",
                      element.toPoolString().c_str() ) ;
            BSONObj actionObject = element.embeddedObject() ;
            _dpsUpdateActionMapIter iter = actionMap.find( actionName ) ;
            if ( iter != actionMap.end() )
            {
               iter->second.push_back( actionObject ) ;
               // has duplicated actions, need merge the same actions
               needMerge = TRUE ;
            }
            else
            {
               _dpsUpdateActionObjList tmp ;
               _dpsUpdateActionMapInstRes res =
                     actionMap.insert( _dpsUpdateActionMapValue( actionName, tmp ) ) ;
               res.first->second.push_back( actionObject ) ;
            }
         }

         BSONObjBuilder subBuilder( builder.subobjStart( FIELD_NAME_UPDATE_ACTION ) ) ;
         if ( needMerge )
         {
            // merge actions
            for ( _dpsUpdateActionMapIter iter = actionMap.begin() ;
                  iter != actionMap.end() ;
                  ++ iter )
            {
               BSONObjBuilder actionBuilder( subBuilder.subobjStart( iter->first._pString ) ) ;
               for ( _dpsUpdateActionObjListIter objIter = iter->second.begin() ;
                     objIter != iter->second.end() ;
                     ++ objIter )
               {
                  actionBuilder.appendElements( *objIter ) ;
               }
               actionBuilder.doneFast() ;
            }
         }
         else
         {
            // no need to merge
            subBuilder.appendElements( newObject ) ;
         }
         subBuilder.doneFast() ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNLOGRECSERIALIZER__BLDUPDTREC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNLOGRECSERIALIZER__BLDDELREC, "_rtnLogRecordSerializer::_buildDeleteRecord" )
   INT32 _rtnLogRecordSerializer::_buildDeleteRecord(
                                                const dpsLogRecord &record,
                                                BSONObjBuilder &builder,
                                                dpsRecordTransInfo &transInfo,
                                                dpsRecordTimeInfo &timeInfo )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNLOGRECSERIALIZER__BLDDELREC ) ;

      const CHAR *fullName = NULL ;
      BSONObj object ;

      // parse log
      rc = dpsRecord2Delete( record, &fullName, object, &timeInfo,
                             NULL, NULL, &transInfo ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse record, rc: %d", rc ) ;

      // collection name
      _collectionToBSON( fullName, builder ) ;

      // document key
      _objectKeyToBSON( object, builder ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNLOGRECSERIALIZER__BLDDELREC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNLOGRECSERIALIZER__BLDPOPREC, "_rtnLogRecordSerializer::_buildPopRecord" )
   INT32 _rtnLogRecordSerializer::_buildPopRecord(
                                                const dpsLogRecord &record,
                                                BSONObjBuilder &builder,
                                                dpsRecordTransInfo &transInfo,
                                                dpsRecordTimeInfo &timeInfo )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNLOGRECSERIALIZER__BLDPOPREC ) ;

      const CHAR *fullName = NULL ;
      INT64 logicalID = 0 ;
      INT8 direction = 1 ;

      // parse log
      rc = dpsRecord2Pop( record, &fullName, logicalID, direction,
                          &timeInfo ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse record, rc: %d", rc ) ;

      // collection name
      _collectionToBSON( fullName, builder ) ;

      {
         // description
         BSONObjBuilder descBuilder( builder.subobjStart( FIELD_NAME_DESP ) ) ;
         descBuilder.append( FIELD_NAME_LOGICAL_ID, logicalID ) ;
         descBuilder.append( FIELD_NAME_DIRECTION, direction ) ;
         descBuilder.doneFast() ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNLOGRECSERIALIZER__BLDPOPREC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNLOGRECSERIALIZER__BLDCRTCSREC, "_rtnLogRecordSerializer::_buildCreateCSRecord" )
   INT32 _rtnLogRecordSerializer::_buildCreateCSRecord(
                                                const dpsLogRecord &record,
                                                BSONObjBuilder &builder,
                                                dpsRecordTransInfo &transInfo,
                                                dpsRecordTimeInfo &timeInfo )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNLOGRECSERIALIZER__BLDCRTCSREC ) ;

      const CHAR *csName = NULL ;
      utilCSUniqueID csUniqueID = UTIL_UNIQUEID_NULL ;
      INT32 pageSize = 0 ;
      INT32 lobPageSize = 0 ;
      INT32 type = 0 ;

      rc = dpsRecord2CSCrt( record, &csName, csUniqueID,
                            pageSize, lobPageSize, type, &timeInfo ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse record, rc: %d", rc ) ;

      // collection space name
      _collectionSpaceToBSON( csName, builder ) ;

      {
         // description
         BSONObjBuilder descBuilder( builder.subobjStart( FIELD_NAME_DESP ) ) ;
         descBuilder.append( FIELD_NAME_UNIQUEID, csUniqueID ) ;
         descBuilder.append( FIELD_NAME_PAGE_SIZE, pageSize ) ;
         descBuilder.append( FIELD_NAME_LOB_PAGE_SIZE, lobPageSize ) ;
         descBuilder.append( FIELD_NAME_TYPE,
                              dmsGetStorageTypeName( (DMS_STORAGE_TYPE)( type ) ) ) ;
         descBuilder.doneFast() ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNLOGRECSERIALIZER__BLDCRTCSREC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNLOGRECSERIALIZER__BLDDELCSREC, "_rtnLogRecordSerializer::_buildDeleteCSRecord" )
   INT32 _rtnLogRecordSerializer::_buildDeleteCSRecord(
                                                const dpsLogRecord &record,
                                                BSONObjBuilder &builder,
                                                dpsRecordTransInfo &transInfo,
                                                dpsRecordTimeInfo &timeInfo )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNLOGRECSERIALIZER__BLDDELCSREC ) ;

      const CHAR *csName = NULL ;
      BSONObj boOptions ;

      rc = dpsRecord2CSDel( record, &csName, &boOptions, &timeInfo ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse record, rc: %d", rc ) ;

      // collection space name
      _collectionSpaceToBSON( csName, builder ) ;

      {
         // description
         BSONObjBuilder descBuilder( builder.subobjStart( FIELD_NAME_DESP ) ) ;
         descBuilder.appendElements( boOptions ) ;
         descBuilder.doneFast() ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNLOGRECSERIALIZER__BLDDELCSREC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNLOGRECSERIALIZER__BLDRENAMECSREC, "_rtnLogRecordSerializer::_buildRenameCSRecord" )
   INT32 _rtnLogRecordSerializer::_buildRenameCSRecord(
                                                const dpsLogRecord &record,
                                                BSONObjBuilder &builder,
                                                dpsRecordTransInfo &transInfo,
                                                dpsRecordTimeInfo &timeInfo )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNLOGRECSERIALIZER__BLDRENAMECSREC ) ;

      const CHAR *csName = NULL ;
      const CHAR *newName = NULL ;

      rc = dpsRecord2CSRename( record, &csName, &newName, &timeInfo ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse record, rc: %d", rc ) ;

      // collection space name
      _collectionSpaceToBSON( csName, builder ) ;

      {
         // description
         BSONObjBuilder descBuilder( builder.subobjStart( FIELD_NAME_DESP ) ) ;
         descBuilder.append( FIELD_NAME_NEW_NAME, newName ) ;
         descBuilder.doneFast() ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNLOGRECSERIALIZER__BLDRENAMECSREC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNLOGRECSERIALIZER__BLDCRTCLREC, "_rtnLogRecordSerializer::_buildCreateCLRecord" )
   INT32 _rtnLogRecordSerializer::_buildCreateCLRecord(
                                                const dpsLogRecord &record,
                                                BSONObjBuilder &builder,
                                                dpsRecordTransInfo &transInfo,
                                                dpsRecordTimeInfo &timeInfo )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNLOGRECSERIALIZER__BLDCRTCLREC ) ;

      const CHAR *fullName = NULL ;
      utilCLUniqueID clUniqueID = UTIL_UNIQUEID_NULL ;
      UINT32 attribute = 0 ;
      UINT8 compType = UTIL_COMPRESSOR_INVALID ;
      BSONObj extOptions, idIdxDef ;
      CHAR tmpAttr[ RTN_LOG_TMP_STR_SIZE + 1 ] = { 0 } ;

      rc = dpsRecord2CLCrt( record, &fullName, clUniqueID,
                            attribute, compType, extOptions, idIdxDef, &timeInfo ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse record, rc: %d", rc ) ;

      mbAttr2String( attribute, tmpAttr, RTN_LOG_TMP_STR_SIZE ) ;

      // collection name
      _collectionToBSON( fullName, builder ) ;

      {
         // description
         BSONObjBuilder descBuilder( builder.subobjStart( FIELD_NAME_DESP ) ) ;
         descBuilder.append( FIELD_NAME_UNIQUEID, (INT64)clUniqueID ) ;
         descBuilder.append( FIELD_NAME_ATTRIBUTE, tmpAttr ) ;
         descBuilder.append( FIELD_NAME_COMPRESSIONTYPE,
                              utilCompressType2String( compType ) ) ;
         if ( !extOptions.isEmpty() )
         {
            descBuilder.append( FIELD_NAME_EXT_OPTIONS, extOptions ) ;
         }
         if ( !idIdxDef.isEmpty() )
         {
            descBuilder.append( FIELD_NAME_ID_INDEX, idIdxDef ) ;
         }
         descBuilder.doneFast() ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNLOGRECSERIALIZER__BLDCRTCLREC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNLOGRECSERIALIZER__BLDDELCLREC, "_rtnLogRecordSerializer::_buildDeleteCLRecord" )
   INT32 _rtnLogRecordSerializer::_buildDeleteCLRecord(
                                                const dpsLogRecord &record,
                                                BSONObjBuilder &builder,
                                                dpsRecordTransInfo &transInfo,
                                                dpsRecordTimeInfo &timeInfo )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNLOGRECSERIALIZER__BLDDELCLREC ) ;

      const CHAR *fullName = NULL ;
      BSONObj boOptions ;

      rc = dpsRecord2CLDel( record, &fullName,
                            &boOptions, &timeInfo ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse record, rc: %d", rc ) ;

      // collection name
      _collectionToBSON( fullName, builder ) ;

      {
         // description
         BSONObjBuilder descBuilder( builder.subobjStart( FIELD_NAME_DESP ) ) ;
         descBuilder.appendElements( boOptions ) ;
         descBuilder.doneFast() ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNLOGRECSERIALIZER__BLDDELCLREC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNLOGRECSERIALIZER__BLDCRTIXREC, "_rtnLogRecordSerializer::_buildCreateIXRecord" )
   INT32 _rtnLogRecordSerializer::_buildCreateIXRecord(
                                                const dpsLogRecord &record,
                                                BSONObjBuilder &builder,
                                                dpsRecordTransInfo &transInfo,
                                                dpsRecordTimeInfo &timeInfo )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNLOGRECSERIALIZER__BLDCRTIXREC ) ;

      const CHAR *fullName = NULL ;
      BSONObj index, option ;

      rc = dpsRecord2IXCrt( record, &fullName, index,
                            option, &timeInfo ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse record, rc: %d", rc ) ;

      // collection name
      _collectionToBSON( fullName, builder ) ;

      {
         // description
         BSONObjBuilder descBuilder( builder.subobjStart( FIELD_NAME_DESP ) ) ;
         descBuilder.appendElements( index ) ;
         descBuilder.doneFast() ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNLOGRECSERIALIZER__BLDCRTIXREC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNLOGRECSERIALIZER__BLDDELIXREC, "_rtnLogRecordSerializer::_buildDeleteIXRecord" )
   INT32 _rtnLogRecordSerializer::_buildDeleteIXRecord(
                                                const dpsLogRecord &record,
                                                BSONObjBuilder &builder,
                                                dpsRecordTransInfo &transInfo,
                                                dpsRecordTimeInfo &timeInfo )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNLOGRECSERIALIZER__BLDDELIXREC ) ;

      const CHAR *fullName = NULL ;
      BSONObj index, option ;

      rc = dpsRecord2IXDel( record, &fullName, index,
                            option, &timeInfo ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse record, rc: %d", rc ) ;

      // collection name
      _collectionToBSON( fullName, builder ) ;

      {
         // description
         BSONObjBuilder descBuilder( builder.subobjStart( FIELD_NAME_DESP ) ) ;
         descBuilder.appendElements( index ) ;
         descBuilder.doneFast() ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNLOGRECSERIALIZER__BLDDELIXREC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNLOGRECSERIALIZER__BLDRENAMECLREC, "_rtnLogRecordSerializer::_buildRenameCLRecord" )
   INT32 _rtnLogRecordSerializer::_buildRenameCLRecord(
                                                const dpsLogRecord &record,
                                                BSONObjBuilder &builder,
                                                dpsRecordTransInfo &transInfo,
                                                dpsRecordTimeInfo &timeInfo )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNLOGRECSERIALIZER__BLDRENAMECLREC ) ;

      const CHAR *csName = NULL ;
      const CHAR *oldShortName = NULL ;
      const CHAR *newShortName = NULL ;
      CHAR fullName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = { 0 } ;
      CHAR newFullName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = { 0 } ;

      rc = dpsRecord2CLRename( record, &csName,
                               &oldShortName, &newShortName, &timeInfo ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse record, rc: %d", rc ) ;

      ossStrncpy( fullName, csName, DMS_COLLECTION_SPACE_NAME_SZ ) ;
      ossStrncat( fullName, ".", 1 ) ;
      ossStrncat( fullName, oldShortName, DMS_COLLECTION_NAME_SZ ) ;

      ossStrncpy( newFullName, csName, DMS_COLLECTION_SPACE_NAME_SZ ) ;
      ossStrncat( newFullName, ".", 1 ) ;
      ossStrncat( newFullName, newShortName, DMS_COLLECTION_NAME_SZ ) ;

      // collection name
      _collectionToBSON( fullName, builder ) ;

      {
         // description
         BSONObjBuilder descBuilder( builder.subobjStart( FIELD_NAME_DESP ) ) ;
         descBuilder.append( FIELD_NAME_NEW_NAME, newFullName ) ;
         descBuilder.doneFast() ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNLOGRECSERIALIZER__BLDRENAMECLREC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNLOGRECSERIALIZER__BLDTRUNCCLREC, "_rtnLogRecordSerializer::_buildTruncateCLRecord" )
   INT32 _rtnLogRecordSerializer::_buildTruncateCLRecord(
                                                const dpsLogRecord &record,
                                                BSONObjBuilder &builder,
                                                dpsRecordTransInfo &transInfo,
                                                dpsRecordTimeInfo &timeInfo )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNLOGRECSERIALIZER__BLDTRUNCCLREC ) ;

      const CHAR *fullName = NULL ;
      BSONObj boOptions ;

      rc = dpsRecord2CLTrunc( record, &fullName, &boOptions,
                              &timeInfo ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse record, rc: %d", rc ) ;

      // collection name
      _collectionToBSON( fullName, builder ) ;

      {
         // description
         BSONObjBuilder descBuilder( builder.subobjStart( FIELD_NAME_DESP ) ) ;
         descBuilder.appendElements( boOptions ) ;
         descBuilder.doneFast() ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNLOGRECSERIALIZER__BLDTRUNCCLREC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNLOGRECSERIALIZER__BLDINVALIDCATAREC, "_rtnLogRecordSerializer::_buildInvalidateCataRecord" )
   INT32 _rtnLogRecordSerializer::_buildInvalidateCataRecord(
                                                const dpsLogRecord &record,
                                                BSONObjBuilder &builder,
                                                dpsRecordTransInfo &transInfo,
                                                dpsRecordTimeInfo &timeInfo )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNLOGRECSERIALIZER__BLDINVALIDCATAREC ) ;

      UINT8 invalidateType = 0 ;
      const CHAR *fullName = NULL ;
      const CHAR *csName = NULL ;
      const CHAR *ixName = NULL ;
      CHAR tmpType[ RTN_LOG_TMP_STR_SIZE + 1 ] = { 0 } ;

      rc = dpsRecord2InvalidCata( record, invalidateType,
                                  &fullName, &ixName, &timeInfo ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse record, rc: %d", rc ) ;

      // Check if only contains name of collection space
      if ( NULL != fullName &&
           NULL == ossStrchr( fullName, '.' ) )
      {
         csName = fullName ;
         fullName = NULL ;

         if ( 0 == ossStrcmp( csName, "SYS" ) )
         {
            csName = NULL ;
         }
      }

      if ( NULL != csName )
      {
         _collectionSpaceToBSON( csName, builder) ;
      }
      else if ( NULL != fullName )
      {
         _collectionToBSON( fullName, builder ) ;
      }

      dpsInvalidateCataTypeToString( invalidateType, tmpType, RTN_LOG_TMP_STR_SIZE ) ;

      {
         BSONObjBuilder descBuilder( builder.subobjStart( FIELD_NAME_DESP ) ) ;
         descBuilder.append( FIELD_NAME_TYPE, tmpType ) ;
         if ( NULL != ixName )
         {
            descBuilder.append( FIELD_NAME_INDEX, ixName ) ;
         }
         descBuilder.doneFast() ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNLOGRECSERIALIZER__BLDINVALIDCATAREC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNLOGRECSERIALIZER__BLDTRANSCOMMITREC, "_rtnLogRecordSerializer::_buildTransCommitRecord" )
   INT32 _rtnLogRecordSerializer::_buildTransCommitRecord(
                                                const dpsLogRecord &record,
                                                BSONObjBuilder &builder,
                                                dpsRecordTransInfo &transInfo,
                                                dpsRecordTimeInfo &timeInfo )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNLOGRECSERIALIZER__BLDTRANSCOMMITREC ) ;

      DPS_TRANS_ID transID = 0 ;
      DPS_LSN_OFFSET preTransLsn = DPS_INVALID_LSN_OFFSET ;
      DPS_LSN_OFFSET firstLsn = DPS_INVALID_LSN_OFFSET ;
      UINT8 commitAttr = 0 ;
      UINT32 nodeNum = 0 ;
      const UINT64 *pNodes = NULL ;

      rc = dpsRecord2TransCommit( record, transID,
                                  preTransLsn, firstLsn, commitAttr,
                                  nodeNum, &pNodes, &timeInfo ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse record, rc: %d", rc ) ;

      {
         BSONObjBuilder transBuilder( builder.subobjStart( FIELD_NAME_TRANS_INFO ) ) ;

         // trans ID
         _transIDToBSON( transID, transBuilder ) ;

         // commit attr
         transBuilder.append( FIELD_NAME_TRANS_COMMIT_ATTR,
                              dpsTSCommitAttr2String( commitAttr ) ) ;

         // commit nodes
         if ( nodeNum > 0 && NULL != pNodes )
         {
            BSONArrayBuilder nodeArrayBuilder( transBuilder.subarrayStart( FIELD_NAME_TRANS_COMMIT_NODES ) ) ;
            for ( UINT32 idx = 0 ; idx < nodeNum ; ++ idx )
            {
               MsgRouteID nodeID ;
               nodeID.value = pNodes[ idx ] ;
               BSONObjBuilder nodeBuilder( nodeArrayBuilder.subobjStart() ) ;
               nodeBuilder.append( FIELD_NAME_GROUPID, nodeID.columns.groupID ) ;
               nodeBuilder.append( FIELD_NAME_NODEID, nodeID.columns.nodeID ) ;
               nodeBuilder.doneFast() ;
            }
            nodeArrayBuilder.doneFast() ;
         }
         transBuilder.doneFast() ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNLOGRECSERIALIZER__BLDTRANSCOMMITREC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNLOGRECSERIALIZER__BLDLOBWRITEREC, "_rtnLogRecordSerializer::_buildLobWriteRecord" )
   INT32 _rtnLogRecordSerializer::_buildLobWriteRecord(
                                                const dpsLogRecord &record,
                                                BSONObjBuilder &builder,
                                                dpsRecordTransInfo &transInfo,
                                                dpsRecordTimeInfo &timeInfo )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNLOGRECSERIALIZER__BLDLOBWRITEREC ) ;

      const CHAR *fullName = NULL ;
      const OID *oid = NULL ;
      UINT32 sequence = 0 ;
      UINT32 offset = 0 ;
      UINT32 length = 0 ;
      UINT32 hash = 0 ;
      const CHAR *lobData = NULL ;
      DMS_LOB_PAGEID page = DMS_LOB_INVALID_PAGEID ;
      UINT32 pageSize = DMS_DEFAULT_LOB_PAGE_SZ ;

      rc = dpsRecord2LobW( record, &fullName, &oid,
                           sequence, offset, length, hash, &lobData, page,
                           &pageSize, &transInfo, &timeInfo ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse record, rc: %d", rc ) ;

      // collection name
      _collectionToBSON( fullName, builder ) ;

      // document key
      builder.append( FIELD_NAME_DOCUMENT_KEY, *oid ) ;

      // LOB description
      rc = _buildLobDescription( lobData, sequence, pageSize, offset, length, builder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build LOB description, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNLOGRECSERIALIZER__BLDLOBWRITEREC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNLOGRECSERIALIZER__BLDLOBRMREC, "_rtnLogRecordSerializer::_buildLobRemoveRecord" )
   INT32 _rtnLogRecordSerializer::_buildLobRemoveRecord(
                                                const dpsLogRecord &record,
                                                BSONObjBuilder &builder,
                                                dpsRecordTransInfo &transInfo,
                                                dpsRecordTimeInfo &timeInfo )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNLOGRECSERIALIZER__BLDLOBRMREC ) ;

      const CHAR *fullName = NULL ;
      const OID *oid = NULL ;
      UINT32 sequence = 0 ;
      UINT32 offset = 0 ;
      UINT32 length = 0 ;
      UINT32 hash = 0 ;
      const CHAR *lobData = NULL ;
      DMS_LOB_PAGEID page = DMS_LOB_INVALID_PAGEID ;
      UINT32 pageSize = DMS_DEFAULT_LOB_PAGE_SZ ;

      rc = dpsRecord2LobRm( record, &fullName, &oid,
                            sequence, offset, length, hash, &lobData, page,
                            &pageSize, &transInfo, &timeInfo ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse record, rc: %d", rc ) ;

      // collection name
      _collectionToBSON( fullName, builder ) ;

      // document key
      builder.append( FIELD_NAME_DOCUMENT_KEY, *oid ) ;

      // LOB description
      // NOTE: remove lob needs no data
      rc = _buildLobDescription( NULL, sequence, pageSize, offset, length, builder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build LOB description, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNLOGRECSERIALIZER__BLDLOBRMREC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNLOGRECSERIALIZER__BLDLOBUPDTREC, "_rtnLogRecordSerializer::_buildLobUpdateRecord" )
   INT32 _rtnLogRecordSerializer::_buildLobUpdateRecord(
                                                const dpsLogRecord &record,
                                                BSONObjBuilder &builder,
                                                dpsRecordTransInfo &transInfo,
                                                dpsRecordTimeInfo &timeInfo )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNLOGRECSERIALIZER__BLDLOBUPDTREC ) ;

      const CHAR *fullName = NULL ;
      const OID *oid = NULL ;
      UINT32 sequence = 0 ;
      UINT32 offset = 0 ;
      UINT32 length = 0 ;
      UINT32 hash = 0 ;
      const CHAR *lobData = NULL ;
      UINT32 oldLen = 0 ;
      const CHAR *oldData = NULL ;
      DMS_LOB_PAGEID page = DMS_LOB_INVALID_PAGEID ;
      UINT32 pageSize = DMS_DEFAULT_LOB_PAGE_SZ ;

      rc = dpsRecord2LobU( record, &fullName, &oid, sequence, offset, length,
                           hash, &lobData, oldLen, &oldData, page, &pageSize,
                           &transInfo, &timeInfo ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse record, rc: %d", rc ) ;

      // collection name
      _collectionToBSON( fullName, builder ) ;

      // document key
      builder.append( FIELD_NAME_DOCUMENT_KEY, *oid ) ;

      // LOB description
      rc = _buildLobDescription( lobData, sequence, pageSize, offset, length, builder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build LOB description, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNLOGRECSERIALIZER__BLDLOBUPDTREC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNLOGRECSERIALIZER__BLDLOBTRUNCREC, "_rtnLogRecordSerializer::_buildLobTruncateRecord" )
   INT32 _rtnLogRecordSerializer::_buildLobTruncateRecord(
                                                const dpsLogRecord &record,
                                                BSONObjBuilder &builder,
                                                dpsRecordTransInfo &transInfo,
                                                dpsRecordTimeInfo &timeInfo )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNLOGRECSERIALIZER__BLDLOBTRUNCREC ) ;

      const CHAR *fullName = NULL ;

      rc = dpsRecord2LobTruncate( record, &fullName, &timeInfo ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse record, rc: %d", rc ) ;

      // collection name
      _collectionToBSON( fullName, builder ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNLOGRECSERIALIZER__BLDLOBTRUNCREC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNLOGRECSERIALIZER__BLDALTERREC, "_rtnLogRecordSerializer::_buildAlterRecord" )
   INT32 _rtnLogRecordSerializer::_buildAlterRecord(
                                                const dpsLogRecord &record,
                                                BSONObjBuilder &builder,
                                                dpsRecordTransInfo &transInfo,
                                                dpsRecordTimeInfo &timeInfo )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNLOGRECSERIALIZER__BLDALTERREC ) ;

      const CHAR *objectName = NULL ;
      RTN_ALTER_OBJECT_TYPE objectType = RTN_ALTER_INVALID_OBJECT ;
      BSONObj alterObject ;

      rc = dpsRecord2Alter( record, &objectName,
                            (INT32 &)objectType, alterObject, &timeInfo ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse record, rc: %d", rc ) ;

      if ( RTN_ALTER_COLLECTION_SPACE == objectType )
      {
         _collectionSpaceToBSON( objectName, builder ) ;
      }
      else if ( RTN_ALTER_COLLECTION == objectType )
      {
         _collectionToBSON( objectName, builder ) ;
      }

      {
         // description
         BSONObjBuilder descBuilder( builder.subobjStart( FIELD_NAME_DESP ) ) ;
         descBuilder.appendElements( alterObject ) ;
         descBuilder.doneFast() ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNLOGRECSERIALIZER__BLDALTERREC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNLOGRECSERIALIZER__BLDADDUIDREC, "_rtnLogRecordSerializer::_buildAddUniqueIDRecord" )
   INT32 _rtnLogRecordSerializer::_buildAddUniqueIDRecord(
                                                const dpsLogRecord &record,
                                                BSONObjBuilder &builder,
                                                dpsRecordTransInfo &transInfo,
                                                dpsRecordTimeInfo &timeInfo )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNLOGRECSERIALIZER__BLDADDUIDREC ) ;

      const CHAR *csName = NULL ;
      utilCSUniqueID csUniqueID = UTIL_UNIQUEID_NULL ;
      BSONObj clInfoObj ;

      rc = dpsRecord2AddUniqueID( record, &csName, csUniqueID,
                                  clInfoObj, &timeInfo ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse record, rc: %d", rc ) ;

      // collection space name
      _collectionSpaceToBSON( csName, builder ) ;

      {
         // description
         BSONObjBuilder descBuilder( builder.subobjStart( FIELD_NAME_DESP ) ) ;
         descBuilder.append( FIELD_NAME_UNIQUEID, csUniqueID ) ;
         descBuilder.appendElements( clInfoObj ) ;
         descBuilder.doneFast() ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNLOGRECSERIALIZER__BLDADDUIDREC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNLOGRECSERIALIZER__BLDRETURNREC, "_rtnLogRecordSerializer::_buildReturnRecord" )
   INT32 _rtnLogRecordSerializer::_buildReturnRecord(
                                                const dpsLogRecord &record,
                                                BSONObjBuilder &builder,
                                                dpsRecordTransInfo &transInfo,
                                                dpsRecordTimeInfo &timeInfo )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNLOGRECSERIALIZER__BLDRETURNREC ) ;

      BSONObj boOptions ;
      dmsReturnOptions options ;

      rc = dpsRecord2Return( record, &( boOptions ), &timeInfo ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse record, rc: %d", rc ) ;

      rc = options.parseOptions( boOptions ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse options, rc: %d", rc ) ;

      if ( UTIL_RECYCLE_CS == options._recycleItem.getType() )
      {
         _collectionSpaceToBSON( options._recycleItem.getOriginName(), builder ) ;
      }
      else if ( UTIL_RECYCLE_CL == options._recycleItem.getType() )
      {
         _collectionToBSON( options._recycleItem.getOriginName(), builder ) ;
      }

      {
         // description
         BSONObjBuilder descBuilder( builder.subobjStart( FIELD_NAME_DESP ) ) ;
         descBuilder.appendElements( boOptions ) ;
         descBuilder.doneFast() ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNLOGRECSERIALIZER__BLDRETURNREC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNLOGRECSERIALIZER__BLDDFLTREC, "_rtnLogRecordSerializer::_buildDefaultRecord" )
   INT32 _rtnLogRecordSerializer::_buildDefaultRecord(
                                                const dpsLogRecord &record,
                                                BSONObjBuilder &builder,
                                                dpsRecordTransInfo &transInfo,
                                                dpsRecordTimeInfo &timeInfo )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNLOGRECSERIALIZER__BLDDFLTREC ) ;

      INT32 tmpRC = SDB_OK ;

      tmpRC = dpsGetTransInfo( record, transInfo ) ;
      if ( SDB_OK != tmpRC )
      {
         PD_LOG( PDWARNING, "Failed to get trans info, rc: %d", tmpRC ) ;
      }

      tmpRC = dpsGetTimeInfo( record, timeInfo ) ;
      if ( SDB_OK != tmpRC )
      {
         PD_LOG( PDWARNING, "Failed to get time info, rc: %d", tmpRC ) ;
      }

      PD_TRACE_EXITRC( SDB__RTNLOGRECSERIALIZER__BLDDFLTREC, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNLOGRECSERIALIZER__BLDLOBMETADATA, "_rtnLogRecordSerializer::_buildLobMetaData" )
   INT32 _rtnLogRecordSerializer::_buildLobMetaData( const dmsLobMeta *meta,
                                                     UINT32 length,
                                                     BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNLOGRECSERIALIZER__BLDLOBMETADATA ) ;

      // build metadata, same format as listLobs()
      BSONObjBuilder metaBuilder( builder.subobjStart( FIELD_NAME_LOB_META_DATA ) ) ;
      metaBuilder.append( FIELD_NAME_LOB_SIZE, meta->_lobLen ) ;
      UINT64 createTimeMS = meta->_createTime ;
      UINT64 modificationTimeMS = 0 != meta->_modificationTime ?
                                    meta->_modificationTime :
                                    createTimeMS ;
      metaBuilder.appendTimestamp( FIELD_NAME_LOB_CREATETIME,
                                    createTimeMS,
                                    createTimeMS % 1000 * 1000 ) ;
      metaBuilder.appendTimestamp( FIELD_NAME_LOB_MODIFICATION_TIME,
                                    modificationTimeMS,
                                    modificationTimeMS % 1000 * 1000 ) ;
      metaBuilder.appendBool( FIELD_NAME_LOB_AVAILABLE, meta->isDone() ) ;
      metaBuilder.appendBool( FIELD_NAME_LOB_HAS_PIECESINFO, meta->hasPiecesInfo() ) ;
      // build pieces info
      if ( meta->hasPiecesInfo() && length >= DMS_LOB_META_LENGTH )
      {
         BSONArray array ;
         _rtnLobPiecesInfo piecesInfo ;

         UINT32 piecesLength = meta->_piecesInfoNum * sizeof( _rtnLobPieces ) ;
         const CHAR *piecesInfoBuf = (const CHAR *)meta + DMS_LOB_META_LENGTH - piecesLength ;

         INT32 rc = piecesInfo.readFrom( piecesInfoBuf, length ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to read pieces info of lob, rc: %d", rc ) ;

         rc = piecesInfo.saveTo( array ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to save pieces info of lob, rc: %d", rc ) ;

         metaBuilder.append( FIELD_NAME_LOB_PIECESINFONUM, meta->_piecesInfoNum ) ;
         metaBuilder.appendArray( FIELD_NAME_LOB_PIECESINFO, array ) ;
      }
      metaBuilder.doneFast() ;

   done:
      PD_TRACE_EXITRC( SDB__RTNLOGRECSERIALIZER__BLDLOBMETADATA, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNLOGRECSERIALIZER__BLDLOBDATA, "_rtnLogRecordSerializer::_buildLobData" )
   INT32 _rtnLogRecordSerializer::_buildLobData( const CHAR *lobData,
                                                 UINT32 pageOffset,
                                                 UINT64 fileOffset,
                                                 UINT32 length,
                                                 BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNLOGRECSERIALIZER__BLDLOBDATA ) ;

      builder.append( FIELD_NAME_PAGE_OFFSET, pageOffset ) ;
      builder.append( FIELD_NAME_FILE_OFFSET, (INT64)fileOffset ) ;
      if ( NULL != lobData )
      {
         builder.append( FIELD_NAME_LOB_LENGTH, length ) ;
         builder.appendBinData( FIELD_NAME_DATA, length, BinDataGeneral, lobData ) ;
      }

      PD_TRACE_EXITRC( SDB__RTNLOGRECSERIALIZER__BLDLOBDATA, rc ) ;

      return rc ;
   }


   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNLOGRECSERIALIZER__BLDLOBDESC, "_rtnLogRecordSerializer::_buildLobDescription" )
   INT32 _rtnLogRecordSerializer::_buildLobDescription( const CHAR *lobData,
                                                        UINT32 sequence,
                                                        UINT32 pageSize,
                                                        UINT32 offset,
                                                        UINT32 length,
                                                        BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNLOGRECSERIALIZER__BLDLOBDESC ) ;

      BSONObjBuilder descBuilder( builder.subobjStart( FIELD_NAME_DESP ) ) ;
      descBuilder.append( FIELD_NAME_LOB_SEQUENCE, sequence ) ;
      descBuilder.append( FIELD_NAME_PAGE_SIZE, pageSize ) ;

      if ( DMS_LOB_META_SEQUENCE == sequence && offset < DMS_LOB_META_LENGTH )
      {
         // metadata sequence
         PD_CHECK( 0 == offset, SDB_DPS_CORRUPTED_LOG, error, PDERROR,
                   "Failed to build metadata for LOB, metadata starts from "
                   "offset [%u] is incomplete", offset ) ;
         PD_CHECK( sizeof( dmsLobMeta ) <= length, SDB_DPS_CORRUPTED_LOG, error, PDERROR,
                   "Failed to build metadata for LOB, metadata ends to "
                   "length [%u] is incomplete", length ) ;

         // build metadata
         if ( NULL != lobData )
         {
            rc = _buildLobMetaData( (const dmsLobMeta *)lobData, length, descBuilder ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build metadata for LOB, rc: %d", rc ) ;
         }

         // build data
         if ( length > DMS_LOB_META_LENGTH )
         {
            rc = _buildLobData( NULL != lobData ? lobData + DMS_LOB_META_LENGTH : NULL,
                                DMS_LOB_META_LENGTH, 0,
                                length - DMS_LOB_META_LENGTH, descBuilder ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build data for LOB, rc: %d", rc ) ;
         }
      }
      else
      {
         // data sequence
         UINT64 fileOffset = (UINT64)sequence * pageSize + offset - DMS_LOB_META_LENGTH ;

         rc = _buildLobData( lobData, offset, fileOffset, length, descBuilder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build data for LOB, rc: %d", rc ) ;
      }

      descBuilder.doneFast() ;

   done:
      PD_TRACE_EXITRC( SDB__RTNLOGRECSERIALIZER__BLDLOBDESC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
