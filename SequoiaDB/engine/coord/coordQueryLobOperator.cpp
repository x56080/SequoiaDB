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

   Source File Name = coordQueryLobOperator.cpp

   Descriptive Name = Coord Query Lob

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   query on coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08-14-2019  Linyoubin Init
   Last Changed =

*******************************************************************************/
#include "coordQueryLobOperator.hpp"
#include "ossUtil.hpp"

using namespace bson;

namespace engine
{
   /*
      _coordQueryLobOperator implement
   */
   _coordQueryLobOperator::_coordQueryLobOperator()
                          : _coordQueryOperator( TRUE )
   {
   }

   _coordQueryLobOperator::~_coordQueryLobOperator()
   {
   }

   INT32 _coordQueryLobOperator::doOpOnCL( coordCataSel &cataSel,
                                           const BSONObj &objMatch,
                                           coordSendMsgIn &inMsg,
                                           coordSendOptions &options,
                                           pmdEDUCB *cb,
                                           coordProcessResult &result )
   {
      INT32 rc = SDB_OK ;

      if ( FALSE == options._useSpecialGrp )
      {
         options._groupLst.clear() ;

         rc = cataSel.getLobGroupLst( cb, result._sucGroupLst,
                                      options._groupLst, &objMatch ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Get the groups by catalog info failed, "
                    "matcher: %s, rc: %d", objMatch.toString().c_str(), rc ) ;
            goto error ;
         }
      }
      else
      {
         PD_LOG( PDDEBUG, "Using specified group" ) ;
      }

      // construct msg
      if ( cataSel.getCataPtr()->isMainCL() )
      {
         rc = _prepareMainCLOp( cataSel, inMsg, options, cb, result ) ;
      }
      else
      {
         rc = _prepareCLOp( cataSel, inMsg, options, cb, result ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Prepare collection operation failed, "
                   "rc: %d", rc ) ;

      // do
      rc = doOnGroups( inMsg, options, cb, result ) ;

      if ( cataSel.getCataPtr()->isMainCL() )
      {
         _doneMainCLOp( cataSel, inMsg, options, cb, result ) ;
      }
      else
      {
         _doneCLOp( cataSel, inMsg, options, cb, result ) ;
      }
      _cataPtr = cataSel.getCataPtr() ;

      PD_RC_CHECK( rc, PDERROR, "Do command[%d] on groups failed, rc: %d",
                   inMsg.opCode(), rc ) ;

      if ( result.nokSize() > 0 )
      {
         goto done ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
}

