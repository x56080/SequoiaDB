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

   Source File Name = sdbIOmProxy.hpp

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
#ifndef SDB_I_OMPROXY_HPP__
#define SDB_I_OMPROXY_HPP__

#include "rtnContextBuff.hpp"
#include "pmdEDU.hpp"
#include "rtnQueryOptions.hpp"
#include <vector>
#include "../bson/bson.h"

using namespace bson ;

namespace engine
{

   /*
      _IOmProxy define
   */
   class _IOmProxy
   {
      public:
         _IOmProxy() {}
         virtual ~_IOmProxy() {}

      public:
         virtual INT32 queryOnOm( MsgHeader *pMsg,
                                  INT32 requestType,
                                  pmdEDUCB *cb,
                                  INT64 &contextID,
                                  rtnContextBuf *buf ) = 0 ;

         virtual INT32 queryOnOm( const rtnQueryOptions &options,
                                  pmdEDUCB *cb,
                                  SINT64 &contextID,
                                  rtnContextBuf *buf ) = 0 ;

         virtual INT32 queryOnOmAndPushToVec( const rtnQueryOptions &options,
                                              pmdEDUCB *cb,
                                              vector<BSONObj> &objs,
                                              rtnContextBuf *buf ) = 0 ;

         virtual void  setOprTimeout( INT64 timeout ) = 0 ;

   } ;
   typedef _IOmProxy IOmProxy ;

}

#endif //SDB_I_OMPROXY_HPP__

