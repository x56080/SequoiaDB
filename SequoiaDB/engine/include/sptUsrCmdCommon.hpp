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

   Source File Name = sptUsrCmdCommon.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/24/2017  WJM  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SPT_USRCMD_COMMON_HPP_
#define SPT_USRCMD_COMMON_HPP_
#include "ossTypes.hpp"
#include <string>

using namespace std ;
namespace engine
{
   class _sptUsrCmdCommon: public SDBObject
   {
   public:
      _sptUsrCmdCommon() ;
      virtual ~_sptUsrCmdCommon() ;

      INT32 exec( const std::string& command, const std::string& env,
                  const INT32& timeout, const INT32& useShell,
                  std::string &err, std::string &retStr ) ;

      INT32 start( const std::string& command, const std::string& env,
                   const INT32& useShell, const INT32& timeout,
                   std::string &err, INT32 &pid, std::string &retStr ) ;

      INT32 getLastRet( std::string &err, UINT32 &lastRet ) ;

      INT32 getLastOut( std::string &err, string &lastOut ) ;

      INT32 getCommand( std::string &err, string &command ) ;

   private:
      UINT32         _retCode ;
      string         _strOut ;
      string         _command ;
   } ;
}
#endif
