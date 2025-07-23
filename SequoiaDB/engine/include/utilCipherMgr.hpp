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
#include <map>
#include <vector>

class _utilCipherMgr : public SDBObject
{
public:
   _utilCipherMgr() {}
   ~_utilCipherMgr() {}

   INT32   init( utilCipherFile *file ) ;
   INT32   addUser( const string &userFullName, const string &token,
                    const string &passwd ) ;
   INT32   removeUser( const string &userFullName, INT32 &retCode ) ;
   INT32   getPasswd( const string &filePath,
                      const string &userFullName,
                      const string &token,
                      string &passwd ) ;

private:
   INT32   _parseLine( const string &line,
                       string &userFullName,
                       string &cipherText ) ;
   INT32   _findCipherText( const string &userFullName,
                            string &cipherText ) ;
   BOOLEAN _isValidHex( const CHAR *hexString ) ;

private:
   utilCipherFile *_cipherfile ;
   std::map<std::string, std::string> _usersCipher ;
} ;
typedef _utilCipherMgr utilCipherMgr ;

#endif // UTIL_CIPHER_HPP_