/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = omagentRemoteUsrCmd.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/03/2016  WJM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OMAGENT_REMOTE_USRCMD_HPP_
#define OMAGENT_REMOTE_USRCMD_HPP_

#include "omagentCmdBase.hpp"
#include "omagentRemoteBase.hpp"

using namespace bson ;
using namespace std ;

namespace engine
{
   /*
      _remoteCmdRun define
   */
   class _remoteCmdRun : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteCmdRun() ;

         ~_remoteCmdRun() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteCmdStart define
   */
   class _remoteCmdStart : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteCmdStart() ;

         ~_remoteCmdStart() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteCmdRunJS define
   */
   class _remoteCmdRunJS : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteCmdRunJS() ;

         ~_remoteCmdRunJS() ;

         INT32 init ( const CHAR *pInfomation ) ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;

         INT32 final( BSONObj &rval, BSONObj &retObj ) ;

      protected:
         string _code ;
         _sptScope* _jsScope ;
         BOOLEAN _isRelease ;
   } ;
}
#endif