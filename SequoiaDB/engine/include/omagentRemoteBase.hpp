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

   Source File Name = omagentRemoteBase.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/03/2016  WJM  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OMAGENT_REMOTE_BASE_HPP__
#define OMAGENT_REMOTE_BASE_HPP__

#include "omagentCmdBase.hpp"
#include <string>

using namespace bson ;
using namespace std ;

namespace engine
{

   /*
      _remoteExec define
   */
   class _remoteExec : public _omaCommand
   {
      public:
         _remoteExec() ;

         virtual ~_remoteExec() ;

         virtual INT32 init( const CHAR * pInfomation ) ;

         virtual const CHAR * name() = 0 ;

         virtual BOOLEAN needCheckBusiness() const { return FALSE ; }

      protected:
         BSONObj _optionObj ;
         BSONObj _matchObj ;
         BSONObj _valueObj ;

   } ;

   /*
      _remoteOmaConfigs define
   */
   class _remoteOmaConfigs : public _remoteExec
   {
      public:
         _remoteOmaConfigs() ;

         virtual ~_remoteOmaConfigs() ;

         virtual const CHAR * name() = 0 ;

      protected:
         INT32  _confObj2Str( const bson::BSONObj &conf, string &str,
                              string &errMsg, const CHAR* pExcept = NULL ) ;

         INT32 _getNodeConfigFile( const string &svcname, string &filePath ) ;

         INT32 _getNodeConfInfo( const string & confFile, bson::BSONObj &conf,
                             string &errMsg, BOOLEAN allowNotExist = FALSE  ) ;
   } ;
}
#endif
