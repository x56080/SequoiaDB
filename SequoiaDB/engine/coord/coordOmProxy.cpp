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

   Source File Name = coordOmProxy.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          13/03/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#include "coordOmProxy.hpp"
#include "coordOmOperator.hpp"
#include "pd.hpp"
#include "coordTrace.hpp"

using namespace bson ;

namespace engine
{

   /*
      _coordOmProxy implement
   */
   _coordOmProxy::_coordOmProxy()
   {
      _pResource = NULL ;
      _oprTimeout = -1 ;
   }

   _coordOmProxy::~_coordOmProxy()
   {
   }

   INT32 _coordOmProxy::init( _coordResource *pResource )
   {
      _pResource = pResource ;

      return SDB_OK ;
   }

   INT32 _coordOmProxy::queryOnOm( const rtnQueryOptions &options,
                                   pmdEDUCB *cb,
                                   SINT64 &contextID,
                                   rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      coordOmOperatorBase omOpr ;

      rc = omOpr.init( _pResource, cb, _oprTimeout ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Init om operator failed, rc: %d", rc ) ;
         goto error ;
      }

      rc = omOpr.queryOnOm( options, cb, contextID, buf ) ;
      if ( rc )
      {
         goto error ;         
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordOmProxy::queryOnOm( MsgHeader *pMsg,
                                   INT32 requestType,
                                   pmdEDUCB *cb,
                                   INT64 &contextID,
                                   rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      coordOmOperatorBase omOpr ;

      rc = omOpr.init( _pResource, cb, _oprTimeout ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Init om operator failed, rc: %d", rc ) ;
         goto error ;
      }

      rc = omOpr.queryOnOm( pMsg, requestType, cb, contextID, buf ) ;
      if ( rc )
      {
         goto error ;         
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordOmProxy::queryOnOmAndPushToVec( const rtnQueryOptions &options,
                                               pmdEDUCB *cb,
                                               vector< BSONObj > &objs,
                                               rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      coordOmOperatorBase omOpr ;

      rc = omOpr.init( _pResource, cb, _oprTimeout ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Init om operator failed, rc: %d", rc ) ;
         goto error ;
      }

      rc = omOpr.queryOnOmAndPushToVec( options, cb, objs, buf ) ;
      if ( rc )
      {
         goto error ;         
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _coordOmProxy::setOprTimeout( INT64 timeout )
   {
      _oprTimeout = timeout ;
   }

}


