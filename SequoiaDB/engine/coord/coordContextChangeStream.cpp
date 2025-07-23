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

   Source File Name = coordContextChangeStream.cpp

   Descriptive Name = RunTime Change Stream Context Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Runtime
   Context.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/05/2023  HYQ Initial Draft

   Last Changed =

*******************************************************************************/
#include "coordContextChangeStream.hpp"
#include "coordTrace.h"
#include "pdTrace.hpp"
#include "msgMessage.hpp"
#include "coordCommandBase.hpp"


namespace engine
{
   #define COORD_WATCH_RETRY_TIMES    5
   #define COORD_WATCH_RETRY_TIME     1000

   /*
      _coordCMDWatchInternal define
   */
   class _coordCMDWatchInternal : public _coordCommandBase
   {

   public:
      _coordCMDWatchInternal() ;
      virtual ~_coordCMDWatchInternal() ;

   public:
      INT32 execute( MsgHeader *pMsg,
                     pmdEDUCB *cb,
                     INT64 &contextID,
                     rtnContextBuf *buf ) ;

      INT32 executeWatch( MsgHeader *pMsg,
                          pmdEDUCB *cb,
                          const CoordGroupList &groupLst,
                          rtnContextCoord::sharePtr *ppContext ) ;
   } ;

   typedef class _coordCMDWatchInternal coordCMDWatchInternal ;

   _coordCMDWatchInternal::_coordCMDWatchInternal()
   :_coordCommandBase()
   {
   }

   _coordCMDWatchInternal::~_coordCMDWatchInternal()
   {
   }

   INT32 _coordCMDWatchInternal::execute( MsgHeader *pMsg,
                                          pmdEDUCB *cb,
                                          INT64 &contextID,
                                          rtnContextBuf *buf )
   {
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_COORDCMDWATCHINTERNAL_EXECUTEWATCH, "_coordCMDWatchInternal::executeWatch" )
   INT32 _coordCMDWatchInternal::executeWatch( MsgHeader *pMsg,
                                               pmdEDUCB *cb,
                                               const CoordGroupList &groupLst,
                                               rtnContextCoord::sharePtr *ppContext )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_COORDCMDWATCHINTERNAL_EXECUTEWATCH ) ;

      // the group may be cata group or data group
      rc = executeOnDataGroup( pMsg, cb, groupLst, FALSE, NULL, NULL, ppContext, NULL ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Execute on group failed, rc: %d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_COORDCMDWATCHINTERNAL_EXECUTEWATCH, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _rtnContextChangeStreamFetcher implement
    */
   RTN_CTX_AUTO_REGISTER( _rtnCoordContextChangeStream,
                          RTN_CONTEXT_COORD_CHANGE_STREAM,
                          "COORDCHANGESTREAM" )
   _rtnCoordContextChangeStream::_rtnCoordContextChangeStream( SINT64 contextID, UINT64 eduID )
   : _rtnContextBase( contextID, eduID ),
     _rtnSubContextHolder()
   {
      _tokenStr[0] = '\0' ;
   }

   //PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOORDCTXCHANGESTREAM_OPEN, "_rtnCoordContextChangeStream::open" )
   INT32 _rtnCoordContextChangeStream::open( MsgHeader *pMsg,
                                             coordResource *resource,
                                             pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNCOORDCTXCHANGESTREAM_OPEN ) ;

      rc = _parseArguments( pMsg, resource ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse arguments, rc: %d", rc ) ;

      rc = _executeWatch( pMsg, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to execute watch, rc: %d", rc ) ;

      _isOpened = TRUE ;
      _hitEnd = FALSE ;

   done:
      PD_TRACE_EXIT( SDB_RTNCOORDCTXCHANGESTREAM_OPEN ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB_RTNCOORDCTXCHANGESTREAM__PARSEARGUMENTS, "_rtnCoordContextChangeStream::_parseArguments" )
   INT32 _rtnCoordContextChangeStream::_parseArguments( MsgHeader *pMsg, coordResource *resource )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNCOORDCTXCHANGESTREAM__PARSEARGUMENTS ) ;

      BSONElement ele ;
      ossPoolVector< const CHAR* > groups ;
      BSONObj queryObj ;
      const CHAR *pQuery = NULL ;

      rc = msgExtractQuery( (const CHAR*)pMsg, NULL, NULL, NULL,
                            NULL, &pQuery, NULL, NULL, NULL ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse message, rc: %d", rc ) ;

      try
      {
         queryObj = BSONObj( pQuery ) ;
         // copy options
         _options = queryObj.copy() ;

         // get token
         ele = queryObj.getField( FIELD_NAME_TOKEN ) ;
         PD_LOG_MSG_CHECK( String == ele.type(),
                           SDB_INVALIDARG, error, PDERROR,
                           "Failed to get field [%s], it is not a string",
                           FIELD_NAME_TOKEN ) ;
         // check string size
         // NOTE: string size in BSONElment includes the trailing terminator
         PD_LOG_MSG_CHECK( ( ( 1 == ele.valuestrsize() ) ||
                           ( MSG_STREAM_TOKEN_STING_SIZE + 1 == ele.valuestrsize() ) ),
                           SDB_INVALIDARG, error, PDERROR,
                           "Failed to get field [%s], "
                           "invalid string size [%d], "
                           "expected empty or [%d]",
                           FIELD_NAME_TOKEN, ele.valuestrsize() - 1,
                           MSG_STREAM_TOKEN_STING_SIZE ) ;
         ossStrncpy( _tokenStr, ele.valuestr(), MSG_STREAM_TOKEN_STING_SIZE ) ;

         // get groups
         ele = queryObj.getField( FIELD_NAME_GROUPS ) ;
         PD_LOG_MSG_CHECK( Array == ele.type() || EOO == ele.type(), SDB_INVALIDARG, error, PDERROR,
                           "Failed to get field [%s], it is not an array",
                           FIELD_NAME_GROUPS ) ;

         if ( EOO != ele.type() )
         {
            BSONObjIterator iter( ele.embeddedObject() ) ;
            while ( iter.more() )
            {
               BSONElement groupElement = iter.next() ;
               PD_CHECK( String == groupElement.type(), SDB_INVALIDARG, error,
                        PDERROR, "Failed to get field [%s]: invalid type",
                        FIELD_NAME_GROUPS ) ;
               groups.push_back( groupElement.valuestr() ) ;
            }
         }

         // only support one group
         PD_LOG_MSG_CHECK( groups.size() == 1 && EOO != ele.type(),
                           SDB_OPTION_NOT_SUPPORT, error, PDERROR,
                           "Only support one group in watch command" ) ;

         for ( ossPoolVector< const CHAR* >::const_iterator it = groups.begin() ;
               it != groups.end() ; ++it )
         {
            const CHAR *groupName = *it ;
            UINT32 groupID = INVALID_GROUPID ;
            rc = resource->groupName2ID( groupName, groupID ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get group id, rc: %d", rc ) ;

            // only support data group and catalog group
            if ( CATALOG_GROUPID == groupID ||
                 ( DATA_GROUP_ID_BEGIN <= groupID && DATA_GROUP_ID_END >= groupID ) )
            {
               _groupList[ groupID ] = groupID ;
            }
            else
            {
               PD_LOG_MSG_CHECK( FALSE, SDB_INVALIDARG, error, PDERROR,
                                 "Could not watch group [%s]", groupName ) ;
            }
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "unexpected error happened:%s", e.what() ) ;
         goto error ;
      }

      // save the resource
      _pResource = resource ;

   done:
      PD_TRACE_EXITRC( SDB_RTNCOORDCTXCHANGESTREAM__PARSEARGUMENTS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOORDCTXCHANGESTREAM__PREPAREDATA, "_rtnCoordContextChangeStream::_prepareData" )
   INT32 _rtnCoordContextChangeStream::_prepareData( pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNCOORDCTXCHANGESTREAM__PREPAREDATA ) ;

      rtnContextBuf buffObj ;
      rtnContext *ctx = NULL ;

   retry:

      if ( _hitEnd )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      buffObj.release() ;

      ctx = _getSubContext() ;
      if ( NULL == ctx )
      {
         rc = _retryWatch( cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to retry watch, rc: %d", rc ) ;

         ctx = _getSubContext() ;
         if ( NULL == ctx )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Failed to get sub context, rc: %d", rc ) ;
            goto error ;
         }
      }

      rc = ctx->getMore( -1, buffObj, cb ) ;
      if ( SDB_DMS_EOC == rc )
      {
         goto done ;
      }
      else if ( SDB_OK == rc )
      {
         while ( !buffObj.eof() )
         {
            BSONObj obj ;
            rc = buffObj.nextObj( obj ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get obj from obj buf, rc: %d", rc ) ;

            if ( buffObj.eof() )
            {
               // save last token and last control rc
               rc = _parseTokenAndControlRC( obj ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to parse token and control rc, rc: %d", rc ) ;

               if ( _controlRC == SDB_DATABASE_DOWN || _controlRC == SDB_APP_FORCED )
               {

                  _deleteSubContext () ;
                  // if no data, retry to get data
                  if ( isEmpty () )
                  {
                     goto retry ;
                  }
                  // if has data, return the data first
                  else
                  {
                     goto done ;
                  }
               }
               else if ( SDB_OK != _controlRC )
               {
                  _hitEnd = TRUE ;
               }
            }

            rc = append( obj ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to append obj to result, rc: %d", rc ) ;
         }
      }
      // getMore error, we need to retry watch
      else
      {
         rc = _retryWatch( cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to retry watch, rc: %d", rc ) ;
         goto retry ;
      }

   done:
      PD_TRACE_EXIT( SDB_RTNCOORDCTXCHANGESTREAM__PREPAREDATA ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOORDCTXCHANGESTREAM__EXECUTEWATCH, "_rtnCoordContextChangeStream::_executeWatch" )
   INT32 _rtnCoordContextChangeStream::_executeWatch( MsgHeader *pMsg, pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNCOORDCTXCHANGESTREAM__EXECUTEWATCH ) ;

      coordCMDWatchInternal watchInternalCmd ;
      rtnContextCoord::sharePtr pContext ;

      // init watch internal command
      rc = watchInternalCmd.init( _pResource, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init watch internal command, rc: %d", rc ) ;

      // do the watch internal command
      rc = watchInternalCmd.executeWatch( pMsg, cb, _groupList, &pContext ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to execute watch internal command, rc: %d", rc ) ;

      // save the new context
      _setSubContext( pContext, cb ) ;

   done:
      PD_TRACE_EXITRC( SDB_RTNCOORDCTXCHANGESTREAM__EXECUTEWATCH, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOORDCTXCHANGESTREAM__RETRYWATCH, "_rtnCoordContextChangeStream::_retryWatch" )
   INT32 _rtnCoordContextChangeStream::_retryWatch( pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNCOORDCTXCHANGESTREAM__RETRYWATCH ) ;

      CHAR *msg = NULL ;
      INT32 buffSize = 0 ;
      UINT32 retryTimes = 0 ;

      PD_LOG( PDDEBUG, "Retry watch, token: %s", _tokenStr ) ;

      // delete the old context
      _deleteSubContext () ;

      rc = msgBuildWatchMsg( &msg, &buffSize, _options, _tokenStr, 0, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build watch message, rc: %d", rc ) ;

   retry:
      PD_CHECK( !cb->isInterrupted(), cb->getInterruptRC(), error, PDWARNING,
                "Failed to retry watch, session is interrupted" ) ;
      PD_CHECK( !PMD_IS_DB_DOWN(), SDB_DATABASE_DOWN, error, PDWARNING,
                "Failed to retry watch, database is down" ) ;

      rc = _executeWatch( (MsgHeader*)msg, cb ) ;
      if ( SDB_OK != rc )
      {
         if ( retryTimes < COORD_WATCH_RETRY_TIMES )
         {
            ossSleep( COORD_WATCH_RETRY_TIME ) ;
            retryTimes++ ;
            goto retry ;
         }
         else
         {
            PD_LOG( PDERROR, "Failed to execute watch internal command, rc: %d", rc ) ;
            goto error ;
         }
      }


   done:
      if ( NULL != msg )
      {
         msgReleaseBuffer( msg, cb ) ;
      }
      PD_TRACE_EXIT( SDB_RTNCOORDCTXCHANGESTREAM__RETRYWATCH ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOORDCTXCHANGESTREAM__PARSETOKENANDCONTROLRC, "_rtnCoordContextChangeStream::_parseTokenAndControlRC" )
   INT32 _rtnCoordContextChangeStream::_parseTokenAndControlRC( const BSONObj& obj )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNCOORDCTXCHANGESTREAM__PARSETOKENANDCONTROLRC ) ;

      // first reset the token and control rc
      _tokenStr[0] = '\0' ;
      _controlRC = SDB_OK ;

      try
      {
         BSONObjIterator it( obj ) ;
         while ( it.more() )
         {
            BSONElement ele = it.next() ;
            const CHAR *fieldName = ele.fieldName() ;
            if ( 0 == ossStrcmp( fieldName, FIELD_NAME_TOKEN ) )
            {
               SDB_ASSERT( String == ele.type(), "Field Token must be String" ) ;
               SDB_ASSERT( MSG_STREAM_TOKEN_STING_SIZE + 1 == ele.valuestrsize(),
                           "Invalid token string size" ) ;

               ossStrncpy( _tokenStr, ele.valuestr(), MSG_STREAM_TOKEN_STING_SIZE ) ;
            }
            else if ( 0 == ossStrcmp( fieldName, FIELD_NAME_CONTROL_RC ) )
            {
               SDB_ASSERT( NumberInt == ele.type(), "Field ControlRC must be NumberInt" ) ;
               _controlRC = ele.numberInt() ;
            }
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse token and control rc" ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXIT( SDB_RTNCOORDCTXCHANGESTREAM__PARSETOKENANDCONTROLRC ) ;
      return rc ;

   error:
      goto done ;
   }
}