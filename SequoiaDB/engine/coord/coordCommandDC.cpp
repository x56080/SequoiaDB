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

   Source File Name = coordCommandDC.cpp

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
#include "coordCommandDC.hpp"
#include "msgMessage.hpp"
#include "coordUtil.hpp"
#include "catDef.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"
#include "coordCMDEventHandler.hpp"

using namespace bson ;

namespace engine
{

   /*
      _coordAlterDC implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordAlterDC,
                                      CMD_NAME_ALTER_DC,
                                      FALSE ) ;
   _coordAlterDC::_coordAlterDC()
   {
   }

   _coordAlterDC::~_coordAlterDC()
   {
   }

   INT32 _coordAlterDC::execute( MsgHeader *pMsg,
                                 pmdEDUCB *cb,
                                 INT64 &contextID,
                                 rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      CoordGroupList datagroups ;
      CoordGroupList allgroups ;
      vector< BSONObj > replyObjs ;
      const CHAR *pAction = NULL ;

      // fill default-reply
      contextID                        = -1 ;

      MsgOpQuery *pAttachMsg           = (MsgOpQuery *)pMsg ;
      pAttachMsg->header.opCode        = MSG_CAT_ALTER_IMAGE_REQ ;

      // extrace query msg
      {
         const CHAR *pQuery = NULL ;
         rc = msgExtractQuery( (const CHAR*)pMsg, NULL, NULL, NULL, NULL,
                               &pQuery, NULL, NULL, NULL ) ;
         PD_RC_CHECK( rc, PDERROR, "Extract command[%s] msg failed, rc: %d",
                      getName(), rc ) ;
         try
         {
            BSONObj objQuery( pQuery ) ;
            BSONElement eleAction = objQuery.getField( FIELD_NAME_ACTION ) ;
            if ( String != eleAction.type() )
            {
               PD_LOG( PDERROR, "The field[%s] is not valid in command[%s]'s "
                       "param[%s]", FIELD_NAME_ACTION, getName(),
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
                    getName(), e.what() ) ;
            goto error ;
         }
      }

      // 1. execute on catalog
      rc = executeOnCataGroup( pMsg, cb, &datagroups, &replyObjs, TRUE, NULL, buf ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to execute %s:%s on catalog node, rc: %d",
                 getName(), pAction, rc ) ;
         goto error ;
      }

      // 2. execute on the special groups or special nodes, ignore error
      pAttachMsg->header.opCode        = MSG_BS_QUERY_REQ ;

      // These commands will return all groups, no need to update group again
      if ( 0 == ossStrcasecmp( CMD_VALUE_NAME_SET_ACTIVE_LOCATION, pAction ) ||
           0 == ossStrcasecmp( CMD_VALUE_NAME_SET_LOCATION, pAction ) ||
           0 == ossStrcasecmp( CMD_VALUE_NAME_START_MAINTENANCE_MODE, pAction ) ||
           0 == ossStrcasecmp( CMD_VALUE_NAME_STOP_MAINTENANCE_MODE, pAction ) ||
           0 == ossStrcasecmp( CMD_VALUE_NAME_START_CRITICAL_MODE, pAction ) ||
           0 == ossStrcasecmp( CMD_VALUE_NAME_STOP_CRITICAL_MODE, pAction ) )
      {
         coordNodeCMDHelper helper ;
         BSONObj objResult ;

         /// notify
         helper.notify2NodesByGroups( _pResource, datagroups, cb ) ;

         /// build result
         rc = coordBuildBatchLocationResultObj( datagroups, replyObjs, objResult ) ;
         if ( rc )
         {
            PD_LOG_MSG( PDERROR, "Failed to build result information, rc: %d", rc ) ;
            goto error ;
         }

         *buf = _rtnContextBuf( objResult ) ;
      }
      else
      {
         // update all groups
         rc = _pResource->updateGroupList( allgroups, cb, NULL, FALSE, TRUE, TRUE ) ;
         if ( rc )
         {
            PD_LOG( PDWARNING, "Failed to update all group list, rc: %d", rc ) ;
            rc = SDB_OK ;
         }

         if ( 0 == ossStrcasecmp( CMD_VALUE_NAME_ENABLE_READONLY, pAction ) ||
              0 == ossStrcasecmp( CMD_VALUE_NAME_DISABLE_READONLY, pAction ) ||
              0 == ossStrcasecmp( CMD_VALUE_NAME_ACTIVATE, pAction ) ||
              0 == ossStrcasecmp( CMD_VALUE_NAME_DEACTIVATE, pAction ) )
         {
            _executeByNodes( pMsg, cb, allgroups, pAction, buf ) ;
         }
         else
         {
            _executeByGroups( pMsg, cb, allgroups, pAction, buf ) ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordAlterDC::_executeByGroups( MsgHeader *pMsg,
                                          pmdEDUCB *cb,
                                          CoordGroupList &groupLst,
                                          const CHAR *pAction,
                                          rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;

      rc = executeOnDataGroup( pMsg, cb, groupLst,
                               TRUE, NULL, NULL, NULL, buf ) ;
      if ( rc )
      {
         PD_LOG( PDWARNING, "Failed to execute %s:%s on data groups, "
                 "rc: %d", getName(), pAction, rc ) ;
      }

      return rc ;
   }

   INT32 _coordAlterDC::_executeByNodes( MsgHeader *pMsg,
                                         pmdEDUCB *cb,
                                         CoordGroupList &groupLst,
                                         const CHAR *pAction,
                                         rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      SET_ROUTEID nodes ;
      ROUTE_RC_MAP faileds ;
      SET_ROUTEID sucNodes ;

      rc = coordGetGroupNodes( _pResource, cb, BSONObj(), NODE_SEL_ALL,
                               groupLst, nodes, NULL, FALSE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Get group nodes failed, rc: %d", rc ) ;
         goto error ;
      }

      rc = executeOnNodes( pMsg, cb, nodes, faileds, &sucNodes, NULL, NULL ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to execute %s:%s on data nodes, "
                 "rc: %d", getName(), pAction, rc ) ;
         goto error ;
      }

   done:
      if ( ( rc || faileds.size() > 0 ) && buf )
      {
         *buf = _rtnContextBuf( coordBuildErrorObj( _pResource, rc,
                                                    cb, &faileds,
                                                    sucNodes.size() ) ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   /*
      _coordGetDCInfo implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordGetDCInfo,
                                      CMD_NAME_GET_DCINFO,
                                      TRUE ) ;
   _coordGetDCInfo::_coordGetDCInfo()
   {
   }

   _coordGetDCInfo::~_coordGetDCInfo()
   {
   }

   INT32 _coordGetDCInfo::_preProcess( rtnQueryOptions &queryOpt,
                                       string &clName,
                                       BSONObj &outSelector )
   {
      clName = CAT_SYSDCBASE_COLLECTION_NAME ;
      queryOpt.setQuery( BSON( FIELD_NAME_TYPE << CAT_BASE_TYPE_GLOBAL_STR ) ) ;
      return SDB_OK ;
   }

}

