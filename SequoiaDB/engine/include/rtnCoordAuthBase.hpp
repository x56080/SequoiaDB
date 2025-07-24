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

   Source File Name = rtnCoordAuthBase.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/12/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTNCOORDAUTHBASE_HPP_
#define RTNCOORDAUTHBASE_HPP_

#include "rtnCoordOperator.hpp"

namespace engine
{
   class rtnCoordAuthBase : public rtnCoordOperator
   {
   protected:
      INT32 forward( MsgHeader *pMsg,
                     pmdEDUCB *cb,
                     INT32 msgType,
                     BOOLEAN sWhenNoPrimary,
                     INT64 &contextID,
                     const CHAR **ppUserName = NULL,
                     const CHAR **ppPass = NULL,
                     BSONObj *pOptions = NULL ) ;
					 
      void  updateSessionByOptions( const BSONObj &options ) ;
	  
   private:

      virtual void   _onSucReply( const MsgOpReply *pReply ) ;

   } ;
}

#endif

