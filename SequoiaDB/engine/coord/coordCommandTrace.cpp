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

   Source File Name = coordCommandTrace.cpp

   Descriptive Name = Coord Commands

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   user command processing on coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/
#include "coordCommandTrace.hpp"
#include "rtnCommand.hpp"
#include "msgMessage.hpp"
#include "pmd.hpp"
#include "rtnCB.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"

using namespace bson ;

namespace engine
{

   /*
      _coordCMDTraceStart implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDTraceStart,
                                      CMD_NAME_TRACE_START,
                                      TRUE ) ;
   _coordCMDTraceStart::_coordCMDTraceStart()
   {
   }

   _coordCMDTraceStart::~_coordCMDTraceStart()
   {
   }

   INT32 _coordCMDTraceStart::execute( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       INT64 &contextID,
                                       rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      contextID = -1 ;

      INT32 flag = 0 ;
      const CHAR *pCMDName = NULL ;
      SINT64 numToSkip = 0 ;
      SINT64 numToReturn = -1 ;
      const CHAR *pQuery = NULL ;
      const CHAR *pFieldSelector = NULL ;
      const CHAR *pOrderBy = NULL ;
      const CHAR *pHint = NULL ;
      _rtnTraceStart tracestart ;

      rc = msgExtractQuery( (const CHAR*)pMsg, &flag, &pCMDName, &numToSkip,
                            &numToReturn, &pQuery, &pFieldSelector,
                            &pOrderBy, &pHint );
      PD_RC_CHECK ( rc, PDERROR,
                    "Failed to extract query, rc = %d", rc ) ;
      rc = tracestart.init ( flag, numToSkip, numToReturn, pQuery,
                             pFieldSelector, pOrderBy, pHint ) ;
      PD_RC_CHECK ( rc, PDERROR,
                    "Failed to init tracestart, rc = %d", rc ) ;
      rc = tracestart.doit ( cb, NULL, NULL, NULL, 0, NULL ) ;
      PD_RC_CHECK ( rc, PDERROR,
                    "Failed to run tracestart, rc = %d", rc ) ;
   done:
      return rc;
   error:
      goto done;
   }

   /*
      _coordCMDTraceResume implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDTraceResume,
                                      CMD_NAME_TRACE_RESUME,
                                      TRUE ) ;
   _coordCMDTraceResume::_coordCMDTraceResume()
   {
   }

   _coordCMDTraceResume::~_coordCMDTraceResume()
   {
   }

   INT32 _coordCMDTraceResume::execute( MsgHeader *pMsg,
                                        pmdEDUCB *cb,
                                        INT64 &contextID,
                                        rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK;
      contextID = -1 ;

      INT32 flag = 0 ;
      const CHAR *pCMDName = NULL ;
      SINT64 numToSkip = 0 ;
      SINT64 numToReturn = -1 ;
      const CHAR *pQuery = NULL ;
      const CHAR *pFieldSelector = NULL ;
      const CHAR *pOrderBy = NULL ;
      const CHAR *pHint = NULL ;
      _rtnTraceResume traceResume ;

      rc = msgExtractQuery( (const CHAR*)pMsg, &flag, &pCMDName, &numToSkip,
                            &numToReturn, &pQuery, &pFieldSelector,
                            &pOrderBy, &pHint );
      PD_RC_CHECK ( rc, PDERROR,
                    "Failed to extract query, rc = %d", rc ) ;
      rc = traceResume.init ( flag, numToSkip, numToReturn, pQuery,
                             pFieldSelector, pOrderBy, pHint ) ;
      PD_RC_CHECK ( rc, PDERROR,
                    "Failed to init tracestart, rc = %d", rc ) ;
      rc = traceResume.doit ( cb, NULL, NULL, NULL, 0, NULL ) ;
      PD_RC_CHECK ( rc, PDERROR,
                    "Failed to run tracestart, rc = %d", rc ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _coordCMDTraceStop implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDTraceStop,
                                      CMD_NAME_TRACE_STOP,
                                      TRUE ) ;
   _coordCMDTraceStop::_coordCMDTraceStop()
   {
   }

   _coordCMDTraceStop::~_coordCMDTraceStop()
   {
   }

   INT32 _coordCMDTraceStop::execute( MsgHeader *pMsg,
                                      pmdEDUCB *cb,
                                      INT64 &contextID,
                                      rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK;
      contextID = -1 ;

      INT32 flag = 0 ;
      const CHAR *pCMDName = NULL ;
      SINT64 numToSkip = 0 ;
      SINT64 numToReturn = -1 ;
      const CHAR *pQuery = NULL ;
      const CHAR *pFieldSelector = NULL ;
      const CHAR *pOrderBy = NULL ;
      const CHAR *pHint = NULL ;
      _rtnTraceStop tracestop ;

      rc = msgExtractQuery( (const CHAR*)pMsg, &flag, &pCMDName, &numToSkip,
                            &numToReturn, &pQuery, &pFieldSelector,
                            &pOrderBy, &pHint );
      PD_RC_CHECK ( rc, PDERROR,
                    "Failed to extract query, rc = %d", rc ) ;
      rc = tracestop.init ( flag, numToSkip, numToReturn, pQuery,
                            pFieldSelector, pOrderBy, pHint ) ;
      PD_RC_CHECK ( rc, PDERROR,
                    "Failed to init tracestop, rc = %d", rc ) ;
      rc = tracestop.doit ( cb, NULL, NULL, NULL, 0, NULL ) ;
      PD_RC_CHECK ( rc, PDERROR,
                    "Failed to run tracestop, rc = %d", rc ) ;
   done:
      return rc;
   error:
      goto done;
   }

   /*
      _coordCMDTraceStatus implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDTraceStatus,
                                      CMD_NAME_TRACE_STATUS,
                                      TRUE ) ;
   _coordCMDTraceStatus::_coordCMDTraceStatus()
   {
   }

   _coordCMDTraceStatus::~_coordCMDTraceStatus()
   {
   }

   INT32 _coordCMDTraceStatus::execute( MsgHeader *pMsg,
                                        pmdEDUCB *cb,
                                        INT64 &contextID,
                                        rtnContextBuf *buf )
   {
      INT32 rc                         = SDB_OK;
      pmdKRCB *pKrcb                   = pmdGetKRCB();
      SDB_RTNCB *pRtncb                = pKrcb->getRTNCB();
      contextID = -1 ;

      INT32 flag = 0 ;
      const CHAR *pCMDName = NULL ;
      SINT64 numToSkip = 0 ;
      SINT64 numToReturn = -1 ;
      const CHAR *pQuery = NULL ;
      const CHAR *pFieldSelector = NULL ;
      const CHAR *pOrderBy = NULL ;
      const CHAR *pHint = NULL ;
      _rtnTraceStatus tracestatus ;

      rc = msgExtractQuery( (const CHAR*)pMsg, &flag, &pCMDName, &numToSkip,
                            &numToReturn, &pQuery, &pFieldSelector,
                            &pOrderBy, &pHint );
      PD_RC_CHECK ( rc, PDERROR,
                    "Failed to extract query, rc = %d", rc ) ;
      rc = tracestatus.init ( flag, numToSkip, numToReturn, pQuery,
                              pFieldSelector, pOrderBy, pHint ) ;
      PD_RC_CHECK ( rc, PDERROR,
                    "Failed to init tracestop, rc = %d", rc ) ;
      rc = tracestatus.doit ( cb, NULL, pRtncb, NULL, 0, &contextID ) ;
      PD_RC_CHECK ( rc, PDERROR,
                    "Failed to run tracestop, rc = %d", rc ) ;

   done:
      return rc ;
   error:
      if ( contextID >= 0 )
      {
         pRtncb->contextDelete( contextID, cb ) ;
         contextID = -1 ;
      }
      goto done ;
   }

}

