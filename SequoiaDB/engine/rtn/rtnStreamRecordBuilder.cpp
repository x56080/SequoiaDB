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

   Source File Name = rtnStreamRecordBuilder.cpp

   Descriptive Name = Stream Record Builder

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/01/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#include "rtnStreamRecordBuilder.hpp"
#include "dmsStorageDataCommon.hpp"
#include "dpsOp2Record.hpp"
#include "msgDef.h"
#include "ossUtil.hpp"
#include "pd.hpp"
#include "pmd.hpp"
#include "pdTrace.hpp"
#include "pmdEnv.hpp"
#include "rtnAlterTask.hpp"
#include "rtnTrace.hpp"
#include "utilStreamToken.hpp"
#include "rtnLogRecordSerializer.hpp"

using namespace std ;
using namespace bson ;

namespace engine
{

   #define RTN_LOG_TMP_STR_SIZE ( 128 )

   /*
      _rtnStreamControlRecord implement
    */
   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNSTREAMCONTROLREC_LOADRECORD, "_rtnStreamControlRecord::setControlData" )
   INT32 _rtnStreamControlRecord::setControlData( INT32 errorCode,
                                                  const CHAR *errorDesc )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNSTREAMCONTROLREC_LOADRECORD ) ;

      try
      {
         _controlRC = errorCode ;
         _controlReason.assign( errorDesc ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to set control data, occurred exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNSTREAMCONTROLREC_LOADRECORD, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _rtnStreamChangeRecord implement
    */
   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNSTREAMCHANGEREC_LOADRECORD, "_rtnStreamChangeRecord::loadRecord" )
   INT32 _rtnStreamChangeRecord::loadRecord( const CHAR *logData )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNSTREAMCHANGEREC_LOADRECORD ) ;

      rc = _logRecord.load( logData ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to load log record, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNSTREAMCHANGEREC_LOADRECORD, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _rtnStreamRecordBuilderBase implement
    */
   _rtnStreamRecordBuilderBase::_rtnStreamRecordBuilderBase(
                                                utilStreamRecordType recordType,
                                                utilStreamToken &token,
                                                BSONObjBuilder &builder )
   : _recordType( recordType ),
     _recordTypeName( utilGetStreamRecordTypeName( recordType ) ),
     _tokenRef( token ),
     _builder( builder )
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNSTREAMRECBUILDER_INIT, "_rtnStreamRecordBuilderBase::init" )
   INT32 _rtnStreamRecordBuilderBase::init()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNSTREAMRECBUILDER_INIT ) ;

      _tokenRef.setSource( pmdGetSysInfo()->_nodeID.columns.groupID ) ;

      PD_TRACE_EXITRC( SDB__RTNSTREAMRECBUILDER_INIT, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNSTREAMRECBUILDER_BUILDRECORD, "_rtnStreamRecordBuilderBase::buildRecord" )
   INT32 _rtnStreamRecordBuilderBase::buildRecord( const rtnStreamRecord &recordData,
                                                   BSONObj &result )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNSTREAMRECBUILDER_BUILDRECORD ) ;

      rc = _prepare( recordData ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to prepare record, rc: %d", rc ) ;

      try
      {
         CHAR tokenStr[ MSG_STREAM_TOKEN_STING_SIZE + 1 ] ;
         tokenStr[ 0 ] = '\0' ;

         _builder.reset() ;

         _tokenRef.toString( tokenStr ) ;

         // build token
         _builder.append( FIELD_NAME_TOKEN, tokenStr ) ;
         _builder.append( FIELD_NAME_TYPE, _recordTypeName ) ;

         rc = _buildRecord( recordData, _builder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build record, rc: %d", rc ) ;

         // builder reuse, should not take owned
         result = _builder.done() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build record, occured exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNSTREAMRECBUILDER_BUILDRECORD, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _rtnStreamControlRecordBuilder define
    */
   _rtnStreamControlRecordBuilder::_rtnStreamControlRecordBuilder( BSONObjBuilder &builder )
   : _rtnStreamRecordBuilderBase( UTIL_STREAM_CONTROL_RECORD, _token, builder )
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNSTREAMCTRLRECBUILDER__PREPARE, "_rtnStreamControlRecordBuilder::_prepare" )
   INT32 _rtnStreamControlRecordBuilder::_prepare( const rtnStreamRecord &recordData )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNSTREAMCTRLRECBUILDER__PREPARE ) ;

      const rtnStreamControlRecord &localData =
            static_cast<const rtnStreamControlRecord &>( recordData ) ;

      // set token
      _token.resetDesc() ;
      _token.setLSN( localData.getLSN().offset ) ;
      _token.setCheckCode( localData.getLSN().version ) ;

      if ( localData.isResumeAt() )
      {
         _token.setResumeAtFlag() ;
      }
      if ( localData.isNotResumable() )
      {
         _token.setNotResumableFlag() ;
      }

      PD_TRACE_EXITRC( SDB__RTNSTREAMCTRLRECBUILDER__PREPARE, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNSTREAMCTRLRECBUILDER__BUILDRECORD, "_rtnStreamControlRecordBuilder::_buildRecord" )
   INT32 _rtnStreamControlRecordBuilder::_buildRecord( const rtnStreamRecord &recordData,
                                                       BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNSTREAMCTRLRECBUILDER__BUILDRECORD ) ;

      const rtnStreamControlRecord &localData =
         static_cast<const rtnStreamControlRecord &>( recordData ) ;

      try
      {
         builder.append( FIELD_NAME_CONTROL_TYPE, localData.getControlTypeName() ) ;
         builder.append( FIELD_NAME_CONTROL_RC, localData.getControlRC() ) ;
         builder.append( FIELD_NAME_CONTROL_REASON, localData.getControlReason().c_str() ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build record, occured exception %s",
               e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNSTREAMCTRLRECBUILDER__BUILDRECORD, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _rtnStreamChangeRecordBuilder define
    */
   _rtnStreamChangeRecordBuilder::_rtnStreamChangeRecordBuilder( BSONObjBuilder &builder )
   : _rtnStreamRecordBuilderBase( UTIL_STREAM_CHANGE_RECORD, _token, builder )
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNSTREAMCHANGERECBUILDER__PREPARE, "_rtnStreamChangeRecordBuilder::_prepare" )
   INT32 _rtnStreamChangeRecordBuilder::_prepare( const rtnStreamRecord &recordData )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNSTREAMCHANGERECBUILDER__PREPARE ) ;

      const rtnStreamChangeRecord &localData =
            static_cast<const rtnStreamChangeRecord &>( recordData ) ;
      const dpsLogRecordHeader &header = localData.getRecord().head() ;

      // set token
      _token.resetDesc() ;
      _token.setLSN( header._lsn ) ;
      _token.setCheckCode( header._version ) ;

      PD_TRACE_EXITRC( SDB__RTNSTREAMCHANGERECBUILDER__PREPARE, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNSTREAMCHANGERECBUILDER__BUILDRECORD, "_rtnStreamChangeRecordBuilder::_buildRecord" )
   INT32 _rtnStreamChangeRecordBuilder::_buildRecord( const rtnStreamRecord &recordData,
                                                      BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNSTREAMCHANGERECBUILDER__BUILDRECORD ) ;

      const rtnStreamChangeRecord &localData =
            static_cast<const rtnStreamChangeRecord &>( recordData ) ;

      rc = rtnLogRecordSerializer::buildRecord( localData.getRecord(), builder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build log record, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNSTREAMCHANGERECBUILDER__BUILDRECORD, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
