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

   Source File Name = omSdbAdaptor.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/14/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OM_SDB_PROXY_HPP__
#define OM_SDB_PROXY_HPP__

#include "pmdEDU.hpp"
#include <string>
#include <vector>
#include "../bson/bson.h"

using namespace std ;
using namespace bson ;

namespace engine
{
   /*
      _omSdbNodeInfo define
   */
   struct _omSdbNodeInfo
   {
      string            _host ;
      string            _svcname ;
      string            _userName ;
      string            _password ;
   } ;
   typedef _omSdbNodeInfo omSdbNodeInfo ;
   typedef vector< omSdbNodeInfo >        VEC_SDBNODES ;

   /*
      _omBizInfo define
   */
   struct _omBizInfo
   {
      string            _clsName ;
      string            _bizName ;
      string            _userName ;
      string            _password ;
      VEC_SDBNODES      _vecNodes ;
   } ;
   typedef _omBizInfo omBizInfo ;

   /*
      _omSdbAdaptor define
   */
   class _omSdbAdaptor : public SDBObject
   {
      public:
         _omSdbAdaptor() ;
         ~_omSdbAdaptor() ;

         INT32    notifyStrategyChanged( const string &clsName,
                                         const string &bizName,
                                         pmdEDUCB *cb ) ;

      protected:

         INT32    getBizNodeInfo( omBizInfo &bizInfo,
                                  pmdEDUCB *cb ) ;

         INT32    notifyStrategyChange2Node( const omSdbNodeInfo *nodeInfo,
                                             const string &userName,
                                             const string &password,
                                             pmdEDUCB *cb ) ;

      private:

         INT32    _parseCoordNodeInfo( const BSONObj &obj,
                                       omSdbNodeInfo &nodeInfo ) ;

         INT32    _fillAuthInfo( omBizInfo &bizInfo,
                                 pmdEDUCB *cb ) ;

         INT32    _parseAuthInfo( const BSONObj &obj,
                                  string &userName,
                                  string &password ) ;

   } ;
   typedef _omSdbAdaptor omSdbAdaptor ;

}

#endif //OM_SDB_PROXY_HPP__
