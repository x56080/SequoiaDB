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

   Source File Name = rtnCoordDCCommands.cpp

   Descriptive Name = Runtime Coord Common

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          02/11/15    XJH Init
   Last Changed =

*******************************************************************************/
#include "rtnCoordDCCommands.hpp"
#include "msgMessage.hpp"
#include "pmdCB.hpp"

using namespace bson ;

namespace engine
{

   /*
      rtnCoordAlterDC implement
   */
   INT32 rtnCoordAlterDC::execute( MsgHeader *pMsg,
                                   pmdEDUCB *cb,
                                   INT64 &contextID,
                                   rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      CoordGroupList datagroups ;
      CoordGroupList allgroups ;
      const CHAR *pAction = NULL ;

      // fill default-reply
      contextID                        = -1 ;

      MsgOpQuery *pAttachMsg           = (MsgOpQuery *)pMsg ;
      pAttachMsg->header.opCode        = MSG_CAT_ALTER_IMAGE_REQ ;

      // extrace query msg
      {
         CHAR *pQuery = NULL ;
         rc = msgExtractQuery( (CHAR*)pMsg, NULL, NULL, NULL, NULL,
                               &pQuery, NULL, NULL, NULL ) ;
         PD_RC_CHECK( rc, PDERROR, "Extract command[%s] msg failed, rc: %d",
                      COORD_CMD_ALTER_DC, rc ) ;
         try
         {
            BSONObj objQuery( pQuery ) ;
            BSONElement eleAction = objQuery.getField( FIELD_NAME_ACTION ) ;
            if ( String != eleAction.type() )
            {
               PD_LOG( PDERROR, "The field[%s] is not valid in command[%s]'s "
                       "param[%s]", FIELD_NAME_ACTION, COORD_CMD_ALTER_DC,
                       objQuery.toString().c_str() ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            pAction = eleAction.valuestr() ;
         }
         catch( std::exception &e )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Parse command[%s]'s param occur exception: %s",
                    COORD_CMD_ALTER_DC, e.what() ) ;
            goto error ;
         }
      }

      // 1. execute on catalog
      rc = executeOnCataGroup( pMsg, cb, &datagroups ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to execute %s:%s on catalog node, rc: %d",
                 COORD_CMD_ALTER_DC, pAction, rc ) ;
         goto error ;
      }

      // update all groups
      rc = rtnCoordGetAllGroupList( cb, allgroups, NULL, FALSE, TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDWARNING, "Failed to update all group list, rc: %d", rc ) ;
         rc = SDB_OK ;
      }

      // 2. execute on the special groups or special nodes, ignore error
      pAttachMsg->header.opCode        = MSG_BS_QUERY_REQ ;
      if ( 0 == ossStrcasecmp( CMD_VALUE_NAME_ENABLE_READONLY, pAction ) ||
           0 == ossStrcasecmp( CMD_VALUE_NAME_DISABLE_READONLY, pAction ) ||
           0 == ossStrcasecmp( CMD_VALUE_NAME_ACTIVATE, pAction ) ||
           0 == ossStrcasecmp( CMD_VALUE_NAME_DEACTIVATE, pAction ) )
      {
         _executeByNodes( pMsg, cb, allgroups, pAction ) ;
      }
      else
      {
         _executeByGroups( pMsg, cb, allgroups, pAction ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 rtnCoordAlterDC::_executeByGroups( MsgHeader *pMsg,
                                            pmdEDUCB *cb,
                                            CoordGroupList &groupLst,
                                            const CHAR *pAction )
   {
      INT32 rc = SDB_OK ;

      rc = executeOnDataGroup( pMsg, cb, groupLst,
                               TRUE, NULL, NULL, NULL ) ;
      if ( rc )
      {
         PD_LOG( PDWARNING, "Failed to execute %s:%s on data groups, "
                 "rc: %d", COORD_CMD_ALTER_DC, pAction, rc ) ;
      }

      return rc ;
   }

   INT32 rtnCoordAlterDC::_executeByNodes( MsgHeader *pMsg,
                                           pmdEDUCB *cb,
                                           CoordGroupList &groupLst,
                                           const CHAR *pAction )
   {
      INT32 rc = SDB_OK ;
      ROUTE_SET nodes ;
      ROUTE_RC_MAP faileds ;
      MsgRouteID routeID ;
      ROUTE_RC_MAP::iterator it ;
      BSONObjBuilder errBuild ;
      rtnCoordCtrlParam ctrlParam ;
      BSONArrayBuilder arrayBuild( errBuild.subarrayStart(
                                   FIELD_NAME_ERROR_NODES ) ) ;
      BSONObj errorInfo ;

      rc = rtnCoordGetGroupNodes( cb, BSONObj(), NODE_SEL_ALL, groupLst,
                                  nodes, NULL ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Get group nodes failed, rc: %d", rc ) ;
         goto error ;
      }

      rc = executeOnNodes( pMsg, cb, nodes, faileds, ctrlParam, NULL, NULL, NULL ) ;
      it = faileds.begin() ;
      while( it != faileds.end() )
      {
         routeID.value = it->first ;
         BSONObj errObj = BSON( FIELD_NAME_NODEID <<
                                (INT32)routeID.columns.nodeID <<
                                FIELD_NAME_RCFLAG << it->second ) ;
         arrayBuild.append( errObj ) ;
         ++it ;
      }
      arrayBuild.done() ;
      errorInfo = errBuild.obj() ;

      if ( rc || faileds.size() > 0 )
      {
         PD_LOG( PDERROR, "Failed to execute %s:%s on data nodes, "
                 "rc: %d, error: %s", COORD_CMD_ALTER_DC, pAction, rc,
                 errorInfo.toString().c_str() ) ;

         if ( SDB_OK == rc && faileds.size() > 0 )
         {
            rc = faileds.begin()->second ;
         }
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      rtnCoordGetDCInfo implement
   */
   INT32 rtnCoordGetDCInfo::_preProcess( rtnQueryOptions &queryOpt,
                                         string &clName )
   {
      clName = CAT_SYSDCBASE_COLLECTION_NAME ;
      queryOpt._query = BSON( FIELD_NAME_TYPE << CAT_BASE_TYPE_GLOBAL_STR ) ;
      return SDB_OK ;
   }

}

