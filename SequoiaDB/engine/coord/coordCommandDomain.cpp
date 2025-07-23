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

   Source File Name = coordCommandDomain.cpp

   Descriptive Name = Coord Commands for Data Management

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   user command processing on coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/27/2017  XJH Init
   Last Changed =

*******************************************************************************/
#include "coordCommandDomain.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"
#include "rtn.hpp"
#include "coordUtil.hpp"

using namespace bson;

namespace engine
{

   /*
      _coordCMDCreateDomain implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDCreateDomain,
                                      CMD_NAME_CREATE_DOMAIN,
                                      FALSE ) ;
   _coordCMDCreateDomain::_coordCMDCreateDomain()
   {
   }

   _coordCMDCreateDomain::~_coordCMDCreateDomain()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_CREATEDOMAIN_EXE, "_coordCMDCreateDomain::execute" )
   INT32 _coordCMDCreateDomain::execute( MsgHeader *pMsg,
                                         pmdEDUCB *cb,
                                         INT64 &contextID,
                                         rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_CREATEDOMAIN_EXE ) ;

      contextID = -1 ;

      MsgOpQuery *forward  = (MsgOpQuery *)pMsg;
      forward->header.opCode = MSG_CAT_CREATE_DOMAIN_REQ ;

      _printDebug ( (const CHAR*)pMsg, getName() ) ;

      rc = executeOnCataGroup ( pMsg, cb, TRUE, NULL, NULL, buf ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Execute on catalog failed in command[%s], "
                  "rc: %d", getName(), rc ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( COORD_CREATEDOMAIN_EXE, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   /*
      _coordCMDDropDomain implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDDropDomain,
                                      CMD_NAME_DROP_DOMAIN,
                                      FALSE ) ;
   _coordCMDDropDomain::_coordCMDDropDomain()
   {
   }

   _coordCMDDropDomain::~_coordCMDDropDomain()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DROPDOMAIN_EXE, "_coordCMDDropDomain::execute" )
   INT32 _coordCMDDropDomain::execute( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       INT64 &contextID,
                                       rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_DROPDOMAIN_EXE ) ;

      contextID = -1 ;

      MsgOpQuery *forward  = (MsgOpQuery *)pMsg ;
      forward->header.opCode = MSG_CAT_DROP_DOMAIN_REQ ;

      _printDebug ( (const CHAR*)pMsg, getName() ) ;

      rc = executeOnCataGroup ( pMsg, cb, TRUE, NULL, NULL, buf ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Execute on catalog failed in command[%s], "
                  "rc: %d", getName(), rc ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( COORD_DROPDOMAIN_EXE, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   /*
      _coordCMDAlterDomain implement
    */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDAlterDomain,
                                      CMD_NAME_ALTER_DOMAIN,
                                      FALSE ) ;
   _coordCMDAlterDomain::_coordCMDAlterDomain ()
   : _coordDataCMDAlter()
   {
   }

   _coordCMDAlterDomain::~_coordCMDAlterDomain ()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_ALTERDOMAIN_DO_ON_CATA, "_coordCMDAlterDomain::_doOnDataGroup" )
   INT32 _coordCMDAlterDomain::_doOnDataGroup ( MsgHeader *pMsg,
                                                pmdEDUCB *cb,
                                                rtnContextCoord::sharePtr *ppContext,
                                                coordCMDArguments *pArgs,
                                                const CoordGroupList &groupLst,
                                                const vector<BSONObj> &cataObjs,
                                                CoordGroupList &sucGroupLst )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_ALTERDOMAIN_DO_ON_CATA ) ;

      if ( RTN_ALTER_DOMAIN_SET_ACTIVE_LOCATION == _arguments.getTaskRunner()->getActionType() ||
           RTN_ALTER_DOMAIN_SET_LOCATION == _arguments.getTaskRunner()->getActionType() )
      {
         coordNodeCMDHelper helper ;
         BOOLEAN hasFailedGroup = FALSE ;
         CoordGroupList allGroups = groupLst ;

         // Remove failed groups in allgropus
         rc = coordRemoveFailedGroup( allGroups, hasFailedGroup, cataObjs ) ;
         if ( rc )
         {
            PD_LOG( PDWARNING, "Failed to remove failed group list, rc: %d", rc ) ;
         }
         rc = hasFailedGroup ? SDB_COORD_NOT_ALL_DONE : SDB_OK ;

         helper.notify2NodesByGroups( _pResource, allGroups, cb ) ;
      }
      else
      {
         rc = _coordDataCMDAlter::_doOnDataGroup( pMsg, cb, ppContext, pArgs, groupLst,
                                                  cataObjs, sucGroupLst ) ;
      }

      PD_TRACE_EXITRC ( COORD_ALTERDOMAIN_DO_ON_CATA, rc ) ;
      return rc ;
   }

}
