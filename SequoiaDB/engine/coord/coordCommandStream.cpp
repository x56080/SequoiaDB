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

   Source File Name = coordCommandStream.cpp

   Descriptive Name = Coord Stream Commands

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/01/2023  HGM Initial Draft
   Last Changed =

*******************************************************************************/

#include "coordCommandStream.hpp"
#include "ossErr.h"
#include "pdTrace.hpp"
#include "coordTrace.hpp"
#include "rtnCB.hpp"
#include "coordContextChangeStream.hpp"

namespace engine
{

   /*
      _coordCMDWatch implement
    */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDWatch,
                                      CMD_NAME_WATCH,
                                      TRUE ) ;
   _coordCMDWatch::_coordCMDWatch()
   : _coordCommandBase()
   {
   }

   _coordCMDWatch::~_coordCMDWatch()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_COORDCMDWATCH_EXECUTE, "_coordCMDWatch::execute" )
   INT32 _coordCMDWatch::execute( MsgHeader *pMsg,
                                  pmdEDUCB *cb,
                                  INT64 &contextID,
                                  rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_COORDCMDWATCH_EXECUTE ) ;

      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      rtnCoordContextChangeStream::sharePtr pContext ;
      contextID = -1 ;

      rc = _checkPrivileges( pMsg, cb );
      PD_RC_CHECK( rc, PDERROR, "Failed to check privileges, rc: %d", rc ) ;

      rc = rtnCB->contextNew( RTN_CONTEXT_COORD_CHANGE_STREAM, pContext, contextID, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create coord change stream context, rc: %d", rc ) ;

      rc = pContext->open( pMsg, _pResource, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open coord change stream context, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_COORDCMDWATCH_EXECUTE, rc ) ;
      return rc ;

   error:
      if ( -1 != contextID )
      {
         rtnCB->contextDelete( contextID, cb ) ;
         contextID = -1 ;
      }
      goto done ;
   }

   INT32 checkPrivilegesByWatchOptions( pmdEDUCB *cb, const BSONObj &options )
   {
      INT32 rc = SDB_OK;
      BSONElement element;
      authActionSet actions;
      actions.addAction( ACTION_TYPE_changeStream );
      actions.addAction( ACTION_TYPE_find );
      BOOLEAN csSpecified = FALSE;
      BOOLEAN clSpecified = FALSE;
      for ( BSONObjIterator it( options ); it.more(); )
      {
         element = it.next();
         const CHAR *fieldName = element.fieldName();

         if ( 0 == ossStrcmp( fieldName, FIELD_NAME_COLLECTION_SPACES ) )
         {
            // get collection spaces
            if ( Array == element.type() )
            {
               BSONObjIterator iter( element.embeddedObject() );
               while ( iter.more() )
               {
                  BSONElement nameElement = iter.next();
                  PD_LOG_MSG_CHECK( String == nameElement.type(), SDB_INVALIDARG, error, PDERROR,
                                    "Failed to get string element from field [%s]",
                                    FIELD_NAME_COLLECTION_SPACES );
                  const CHAR *name = nameElement.valuestr();
                  rc = dmsCheckCSName( name, TRUE );
                  if ( SDB_OK != rc )
                  {
                     PD_LOG_MSG( PDERROR,
                                 "Failed to check collection space "
                                 "name [%s], rc: %d",
                                 name, rc );
                     goto error;
                  }
                  csSpecified = TRUE;
                  boost::shared_ptr< authResource > res = authResource::forCS( name );
                  rc = cb->getSession()->checkPrivilegesForActionsOnResource( res, actions );
                  PD_RC_CHECK( rc, PDERROR, "Failed to check privileges, rc: %d", rc );
               }
            }
            else if ( String == element.type() )
            {
               const CHAR *name = element.valuestr();
               rc = dmsCheckCSName( name, TRUE );
               if ( SDB_OK != rc )
               {
                  PD_LOG_MSG( PDERROR,
                              "Failed to check collection space "
                              "name [%s], rc: %d",
                              name, rc );
                  goto error;
               }
               csSpecified = TRUE;
               boost::shared_ptr< authResource > res = authResource::forCS( name );
               rc = cb->getSession()->checkPrivilegesForActionsOnResource( res, actions );
               PD_RC_CHECK( rc, PDERROR, "Failed to check privileges, rc: %d", rc );
            }
            else if ( EOO != element.type() )
            {
               PD_LOG_MSG_CHECK( FALSE, SDB_INVALIDARG, error, PDERROR,
                                 "Failed to get field [%s], "
                                 "it is not an array or a string",
                                 FIELD_NAME_COLLECTION_SPACES );
            }
         }
         else if ( 0 == ossStrcmp( fieldName, FIELD_NAME_COLLECTIONS ) )
         {
            if ( Array == element.type() )
            {
               BSONObjIterator iter( element.embeddedObject() );
               while ( iter.more() )
               {
                  BSONElement nameElement = iter.next();
                  PD_LOG_MSG_CHECK( String == nameElement.type(), SDB_INVALIDARG, error, PDERROR,
                                    "Failed to get string element from field [%s]",
                                    FIELD_NAME_COLLECTIONS );
                  const CHAR *name = nameElement.valuestr();
                  rc = dmsCheckFullCLName( name, TRUE );
                  if ( SDB_OK != rc )
                  {
                     PD_LOG_MSG( PDERROR,
                                 "Failed to check collection "
                                 "name [%s], rc: %d",
                                 name, rc );
                     goto error;
                  }
                  clSpecified = TRUE;
                  rc = cb->getSession()->checkPrivilegesForActionsOnExact( name, actions );
                  PD_RC_CHECK( rc, PDERROR, "Failed to check privileges, rc: %d", rc );
               }
            }
            else if ( String == element.type() )
            {
               const CHAR *name = element.valuestr();
               rc = dmsCheckFullCLName( name, TRUE );
               if ( SDB_OK != rc )
               {
                  PD_LOG_MSG( PDERROR,
                              "Failed to check collection "
                              "name [%s], rc: %d",
                              name, rc );
                  goto error;
               }
               clSpecified = TRUE;
               rc = cb->getSession()->checkPrivilegesForActionsOnExact( name, actions );
               PD_RC_CHECK( rc, PDERROR, "Failed to check privileges, rc: %d", rc );
            }
            else if ( EOO != element.type() )
            {
               PD_LOG_MSG_CHECK( FALSE, SDB_INVALIDARG, error, PDERROR,
                                 "Failed to get field [%s], "
                                 "it is not an array or a string",
                                 FIELD_NAME_COLLECTIONS );
            }
         }
      }
      if ( !csSpecified && !clSpecified )
      {
         rc = cb->getSession()->checkPrivilegesForActionsOnResource( authResource::forNonSystem(),
                                                                     actions );
         PD_RC_CHECK( rc, PDERROR, "Failed to check privileges, rc: %d", rc );
      }

   done:
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_COORDCMDWATCH_CHECKPRIVILEGES, "_coordCMDWatch::_checkPrivileges" )
   INT32 _coordCMDWatch::_checkPrivileges( MsgHeader *pMsg, pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK;

      if ( !cb->getSession()->privilegeCheckEnabled() )
      {
         goto done;
      }

      {
         const CHAR *pQuery = NULL;
         rc = msgExtractQuery( (const CHAR *)pMsg, NULL, NULL, NULL, NULL, &pQuery, NULL, NULL,
                               NULL );
         PD_RC_CHECK( rc, PDERROR, "Failed to extract query, rc: %d", rc );
         BSONObj options( pQuery );
         rc = checkPrivilegesByWatchOptions( cb, options );
         PD_RC_CHECK( rc, PDERROR, "Failed to check privileges, rc: %d", rc );
      }
   done:
      return rc;
   error:
      goto done;
   }

   
} // namespace engine