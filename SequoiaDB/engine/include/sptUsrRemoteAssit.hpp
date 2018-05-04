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

   Source File Name = sptUsrRemoteAssit.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          18/07/2016  WJM Initial Draft

   Last Changed =

*******************************************************************************/


#ifndef SPT_USRREMOTE_ASSIT_HPP__
#define SPT_USRREMOTE_ASSIT_HPP__

#include "msg.h"
#include "oss.hpp"
#include "sptRemote.hpp"
#include <string>
using std::string ;
namespace engine
{
   /*
      sptUsrRemoteAssit define
   */
   class _sptUsrRemoteAssit : public SDBObject
   {
      public:
         _sptUsrRemoteAssit() ;

         ~_sptUsrRemoteAssit() ;

      public:
         INT32       connect( const CHAR *pHostName,
                              const CHAR *pServiceName ) ;

         INT32       disconnect() ;

         INT32       runCommand( string command,
                                 const CHAR* arg1,
                                 CHAR **ppRetBuffer,
                                 INT32 &retCode,
                                 BOOLEAN needRecv = TRUE ) ;
         inline ossValuePtr getHandle()
         {
            return _handle ;
         } ;

      private:
         ossValuePtr _handle ;
         sptRemote   _remote ;
   } ;
   typedef _sptUsrRemoteAssit sptUsrRemoteAssit ;
}
#endif