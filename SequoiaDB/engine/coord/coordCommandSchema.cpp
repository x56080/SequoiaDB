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

   Source File Name = coordCommandSchema.cpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#include "coordCommandSchema.hpp"

namespace engine
{

   /*
      _coordCMDCreateSchema implement
    */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDCreateSchema,
                                      CMD_NAME_CREATE_SCHEMA,
                                      FALSE )
   _coordCMDCreateSchema::_coordCMDCreateSchema()
   {
   }

   _coordCMDCreateSchema::~_coordCMDCreateSchema()
   {
   }

   INT32 _coordCMDCreateSchema::execute( MsgHeader *pMsg,
                                         pmdEDUCB *cb,
                                         INT64 &contextID,
                                         rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;

      contextID = -1 ;

      rc = executeOnCataGroup( pMsg, cb, TRUE, NULL, NULL, buf ) ;
      PD_RC_CHECK( rc, PDERROR, "Execute on catalog failed in command [%s], "
                   "rc: %d", getName(), rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _coordCMDDropSchema implement
    */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDDropSchema,
                                      CMD_NAME_DROP_SCHEMA,
                                      FALSE )
   _coordCMDDropSchema::_coordCMDDropSchema()
   {
   }

   _coordCMDDropSchema::~_coordCMDDropSchema()
   {
   }

   INT32 _coordCMDDropSchema::execute( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       INT64 &contextID,
                                       rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;

      contextID = -1 ;

      rc = executeOnCataGroup( pMsg, cb, TRUE, NULL, NULL, buf ) ;
      PD_RC_CHECK( rc, PDERROR, "Execute on catalog failed in command [%s], "
                   "rc: %d", getName(), rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _coordCMDCreateSchema implement
    */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDAlterSchema,
                                      CMD_NAME_ALTER_SCHEMA,
                                      FALSE )
   _coordCMDAlterSchema::_coordCMDAlterSchema()
   : _coordDataCMD3Phase(),
     _hasCollection( FALSE )
   {
   }

   _coordCMDAlterSchema::~_coordCMDAlterSchema()
   {
   }

   INT32 _coordCMDAlterSchema::_generateDataMsg( MsgHeader *pMsg,
                                                 pmdEDUCB *cb,
                                                 coordCMDArguments *pArgs,
                                                 const vector<BSONObj> &cataObjs,
                                                 CHAR **ppMsgBuf,
                                                 INT32 *pBufSize )
   {
      INT32 rc = SDB_OK ;

      BOOLEAN hasRewrite = FALSE ;

      while ( TRUE )
      {
         if ( cataObjs.empty() )
         {
            break ;
         }

         try
         {
            CHAR *pBuf = NULL ;
            INT32 bufSize = 0 ;

            const CHAR *collectionName = NULL ;
            BSONObjBuilder commandBuilder ;
            BSONElement schemaEle, actionEle, shardkingKeyEle ;
            BSONObj boCommand, newQuery ;

            BSONObj boReply = cataObjs[ 0 ] ;

            BSONElement ele = boReply.getField( FIELD_NAME_ALTER_COMMAND ) ;
            if ( ele.eoo() )
            {
               break ;
            }
            PD_CHECK( Object == ele.type(), SDB_INVALIDARG, error, PDERROR,
                      "Failed to get field [%s] from reply, it is not an object",
                      FIELD_NAME_ALTER_COMMAND ) ;
            boCommand = ele.embeddedObject() ;

            ele = boCommand.getField( FIELD_NAME_NAME ) ;
            PD_CHECK( String == ele.type(), SDB_INVALIDARG, error, PDERROR,
                      "Failed to get field [%s] from reply, it is not an string",
                      FIELD_NAME_NAME ) ;
            collectionName = ele.valuestrsafe() ;

            commandBuilder.appendElements( boCommand ) ;
            schemaEle = boReply.getField( FIELD_NAME_SCHEMA ) ;
            actionEle = boReply.getField( FIELD_NAME_SCHEMA_ACTION ) ;
            shardkingKeyEle = boReply.getField( FIELD_NAME_SHARDINGKEY ) ;
            if ( !schemaEle.eoo() || !actionEle.eoo() || !shardkingKeyEle.eoo() )
            {
               BSONObjBuilder infoBuilder(
                     commandBuilder.subobjStart( FIELD_NAME_ALTER_INFO ) ) ;
               if ( !schemaEle.eoo() )
               {
                  infoBuilder.append( schemaEle ) ;
               }
               if ( !actionEle.eoo() )
               {
                  infoBuilder.append( actionEle ) ;
               }
               if ( !shardkingKeyEle.eoo() )
               {
                  infoBuilder.append( shardkingKeyEle ) ;
               }
               infoBuilder.doneFast() ;
            }
            newQuery = commandBuilder.obj() ;

            PD_LOG( PDDEBUG, "Got new alter collection command [%s]",
                    newQuery.toPoolString().c_str() ) ;

            rc = msgBuildQueryMsg( &pBuf, &bufSize,
                                   CMD_ADMIN_PREFIX CMD_NAME_ALTER_COLLECTION,
                                   0, 0, 0, -1, &newQuery, NULL, NULL, NULL, cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Build data message failed on command[%s], "
                         "rc: %d", getName(), rc ) ;

            *ppMsgBuf = (CHAR *)pBuf ;
            *pBufSize = bufSize ;

            pArgs->_targetName.assign( collectionName ) ;
            _hasCollection = TRUE ;
            hasRewrite = TRUE ;
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to generate data message, occur exception %s",
                    e.what() ) ;
            rc = ossException2RC( &e ) ;
            goto error ;
         }
         break ;
      }

      if ( !hasRewrite )
      {
         rc = _coordDataCMD3Phase::_generateDataMsg( pMsg, cb, pArgs,
                                                     cataObjs, ppMsgBuf,
                                                     pBufSize ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to generate alter schema message, "
                      "rc: %d", rc ) ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

}
