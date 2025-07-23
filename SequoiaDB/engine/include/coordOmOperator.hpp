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

   Source File Name = coordOmOperator.hpp

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
#ifndef COORD_OM_OPERATOR_HPP__
#define COORD_OM_OPERATOR_HPP__

#include "coordCommandBase.hpp"

namespace engine
{

   /*
      _coordOmOperatorBase define
   */
   class _coordOmOperatorBase : public _coordCommandBase
   {
      public:
         _coordOmOperatorBase() ;
         virtual ~_coordOmOperatorBase() ;

      private:
         virtual INT32        execute( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       INT64 &contextID,
                                       rtnContextBuf *buf ) ;

      public:
         INT32         executeOnOm ( MsgHeader *pMsg,
                                     pmdEDUCB *cb,
                                     BOOLEAN onPrimary,
                                     SET_RC *pIgnoreRC = NULL,
                                     rtnContextCoord::sharePtr *ppContext = NULL,
                                     rtnContextBuf *buf = NULL ) ;

         INT32         executeOnOm ( MsgHeader *pMsg,
                                     pmdEDUCB *cb,
                                     vector<BSONObj> *pReplyObjs,
                                     BOOLEAN onPrimary = TRUE,
                                     SET_RC *pIgnoreRC = NULL,
                                     rtnContextBuf *buf = NULL ) ;

         INT32          queryOnOm( MsgHeader *pMsg,
                                   INT32 requestType,
                                   pmdEDUCB *cb,
                                   INT64 &contextID,
                                   rtnContextBuf *buf ) ;

         INT32          queryOnOm( const rtnQueryOptions &options,
                                   pmdEDUCB *cb,
                                   SINT64 &contextID,
                                   rtnContextBuf *buf ) ;

         INT32          queryOnOmAndPushToVec( const rtnQueryOptions &options,
                                               pmdEDUCB *cb,
                                               vector<BSONObj> &objs,
                                               rtnContextBuf *buf ) ;
   } ;
   typedef _coordOmOperatorBase coordOmOperatorBase ;

}

#endif //COORD_OM_OPERATOR_HPP__

