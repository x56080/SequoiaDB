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

   Source File Name = utilCipherMgr.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/26/2018  ZWB  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTILCIPHERMGR_H_
#define UTILCIPHERMGR_H_

#include "utilCipherFile.hpp"
#include "ossTypes.hpp"
#include <string>
#include <map>
#include <vector>

namespace engine
{

   class _utilCipherMgr : public SDBObject
   {
   public:
      static const INT32  BYTES_PER_TIME = 8 ;
      static const INT32  KEY_BYTE_LENGTH = 8 ;
      static const INT32  RANDOM_ARRAY_BYTE_LENGTH = 16 ;
      static const INT32  UINT8_MAX_NUMBER = 65536 ;
      static const UINT32 INSERTABLE_MAX_LENGTH = 234 ;

      _utilCipherMgr() {}
      ~_utilCipherMgr() {}

      INT32 init( utilCipherAbstractFile *file ) ;
      INT32 addUser( const std::string &user, const std::string &token,
                     const std::string &passwd ) ;
      INT32 removeUser( const std::string &user ) ;
      INT32 getPasswd( const std::string &userInfo, const std::string &token,
                       std::string &passwd ) ;
      void  getConnectionUserName( const std::string &userInfo,
                                   std::string &connectionUserName ) ;

   private:

      INT32  _parseLine( std::string line, std::string& usr, std::string& cipherText ) ;
      INT32  _write( const std::string& fileContent ) ;
      void   _extractUserInfo( const std::string &userInfo, std::string &userName,
                               std::string &fullName ) ;
      INT32  _findCipherText( const std::string &userName, const std::string &fullName,
                              std::string &cipherText ) ;

   private:
      utilCipherAbstractFile *_cipherfile ;
      std::map<std::string, std::string> _usersCipher ;
   } ;
   typedef _utilCipherMgr utilCipherMgr ;

}

#endif // UTIL_CIPHER_HPP_