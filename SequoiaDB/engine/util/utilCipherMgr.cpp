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

   Source File Name = utilCipherMgr.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/26/2018  ZWB  Initial Draft

   Last Changed =

*******************************************************************************/
#include "ossErr.h"
#include "ossUtil.hpp"
#include "utilCipher.hpp"
#include "utilCipherMgr.hpp"
#include "msgDef.hpp"
#include "openssl/des.h"
#include "openssl/sha.h"
#include <iostream>
#include <sstream>

namespace engine
{

   INT32 _utilCipherMgr::_parseLine( string line, string &usr, string &cipherText )
   {
      INT32 rc = SDB_OK ;

      string::size_type offset = line.find( ":" ) ;
      if ( string::npos == offset || line.length() -1 == offset || 0 == offset )
      {
         PD_LOG ( PDERROR, "line [%s] is in wrong foramt. ", line.c_str() ) ;
         goto error ;
      }

      usr = line.substr( 0, offset ) ;
      cipherText = line.substr( offset + 1, line.length() - offset - 1 ) ;

   done:
      return rc ;
   error:
      rc = SDB_INVALIDARG ;
      goto done ;
   }

   INT32 _utilCipherMgr::_write( const string &fileContent )
   {
      return _cipherfile->writeToFile( fileContent ) ;
   }

   void _utilCipherMgr::_extractUserInfo( const string &userInfo, string &userName,
                                          string &fullName )
   {
      string::size_type atPos = userInfo.find( "@" ) ;

      if ( string::npos != atPos )
      {
         userName = userInfo.substr( 0, atPos ) ;
      }
      else
      {
         userName = userInfo ;
      }
      fullName = userInfo ;
   }

   INT32 _utilCipherMgr::_findCipherText( const string &userName, const string &fullName,
                                          string &cipherText )
   {
      INT32                           rc = SDB_OK ;

      INT32                           foundFullNameCount = 0 ;
      INT32                           foundHalfNameCount = 0 ;
      map<string, string>::iterator   itor ;
      map<string, string>::iterator   found ;

      for ( itor = _usersCipher.begin(); itor != _usersCipher.end(); itor++ )
      {
         string lineUserName = itor->first ;
         string::size_type atPos = lineUserName.find( '@' ) ;

         if ( string::npos != atPos )
         {
            lineUserName = lineUserName.substr( 0, atPos ) ;
         }

         if ( itor->first == fullName )
         {
            foundFullNameCount++ ;
            found = itor ;
            break ;
         }
         else if ( lineUserName == userName )
         {
            foundHalfNameCount++ ;
            found = itor ;
         }
      }

      if ( 1 == foundFullNameCount || 1 == foundHalfNameCount )
      {
         cipherText = found->second ;
      }
      else if ( 1 < foundHalfNameCount )
      {
         PD_LOG ( PDERROR, "ambiguous user name, try providing cluster name." ) ;
         std::cerr << "ambiguous user name, try providing cluster name." << std::endl ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else
      {
         PD_LOG ( PDWARNING, "no corresponding user information." ) ;
         std::cerr << "no corresponding user information." << std::endl ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilCipherMgr::init( utilCipherAbstractFile *file )
   {
      INT32  rc = SDB_OK ;

      string user ;
      string cipherText ;
      INT64  contentLen = 0 ;
      INT64  begin = 0 ;
      INT64  lineLen = 0 ;
      const CHAR*  fileContent = NULL ;

      _cipherfile = file ;
      rc = _cipherfile->readFromFile( &fileContent, &contentLen );
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      while( begin < contentLen )
      {
         const CHAR* find = ossStrstr( fileContent + begin, OSS_NEWLINE ) ;
         if ( NULL != find )
         {
            lineLen = find - ( fileContent + begin ) ;
            string line( fileContent + begin, lineLen ) ;
            begin += lineLen + ossStrlen( OSS_NEWLINE ) ;

            if ( SDB_OK == _parseLine( line, user, cipherText ) )
            {
               _usersCipher[user] = cipherText ;
            }
         }
         else
         {
            PD_LOG ( PDWARNING, "cipherfile bad format") ;
            break ;
         }  
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilCipherMgr::addUser( const string &user, const string &token,
                                  const string &passwd )
   {
      INT32 rc = SDB_OK ;

      string        fileContent ;
      ostringstream errorMsgBuider ;
      map<string,string>::iterator it ;
      CHAR          cipherText[SDB_MAX_PASSWORD_LENGTH * 2 + 1] = { '\0' } ;

      if ( SDB_MAX_USERNAME_LENGTH < user.size() )
      {
         rc = SDB_INVALIDARG ;
         errorMsgBuider << "user maximum length is " <<
                           SDB_MAX_USERNAME_LENGTH ;
         PD_LOG ( PDERROR, errorMsgBuider.str().c_str() ) ;
         std::cerr << errorMsgBuider.str() << std::endl ;
         goto error ;
      }

      if ( SDB_MAX_PASSWORD_LENGTH < passwd.size() )
      {
         rc = SDB_INVALIDARG ;
         errorMsgBuider << "password maximum length is " <<
                           SDB_MAX_PASSWORD_LENGTH ;
         PD_LOG ( PDERROR, errorMsgBuider.str().c_str() ) ;
         std::cerr << errorMsgBuider.str() << std::endl ;
         goto error ;
      }

      rc = utilCipherEncrypt( passwd.c_str(), token.c_str(), cipherText ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "encrypt user %s passwd failed.",
                  user.c_str() ) ;
         goto error ;
      }

      _usersCipher[user] = cipherText ;

      it = _usersCipher.begin() ;

      for ( ; it != _usersCipher.end(); it++ )
      {
         fileContent += ( it->first + ":" + it->second + OSS_NEWLINE ) ;
      }

      rc = _write( fileContent ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilCipherMgr::removeUser( const string &user )
   {
      INT32 rc = SDB_OK ;

      map<string, string>::iterator it = _usersCipher.find( user ) ;
      if ( it != _usersCipher.end() )
      {
         string fileContent ;

         _usersCipher.erase( it ) ;

         for( it = _usersCipher.begin(); it != _usersCipher.end(); it++ )
         {
            fileContent += ( it->first + ":" + it->second + OSS_NEWLINE ) ;
         }

         rc = _write( fileContent ) ;
      }

      return rc ;
   }

   INT32 _utilCipherMgr::getPasswd( const string &userInfo, const string &token,
                                    string &passwd )
   {
      INT32 rc = SDB_OK ;

      string userName ;
      string fullName ;
      string cipherText ;
      CHAR   clearText[SDB_MAX_PASSWORD_LENGTH + 1] = { '\0' } ;

      _extractUserInfo( userInfo, userName, fullName ) ;

      rc = _findCipherText( userName, fullName, cipherText ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      rc = utilCipherDecrypt( cipherText.c_str(), token.c_str(), clearText ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "decrypt user %s passwd failed.",
                  fullName.c_str() ) ;
         goto error ;
      }
      passwd = clearText ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void _utilCipherMgr::getConnectionUserName( const std::string &userInfo,
                                               std::string &connectionUserName )
   {
      string fullName ;

      _extractUserInfo( userInfo, connectionUserName, fullName ) ;
   }

}
