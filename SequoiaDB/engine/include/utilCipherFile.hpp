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

   Source File Name = utilCipherFile.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/26/2018  ZWB  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTILCIPHERFILE_H_
#define UTILCIPHERFILE_H_

#include "ossFile.hpp"

enum CIPHER_FILE_ROLE
{
   R_ROLE = 0,
   W_ROLE
} ;

class _utilCipherFile
{
public:
   _utilCipherFile() : _isOpen( FALSE ) {}
   ~_utilCipherFile() ;

   INT32       init( string &filePath, UINT32 role ) ;
   INT32       read( CHAR **fileContent, INT64 &contentLen ) ;
   INT32       write( const string &fileContent ) ;
   const CHAR* getFilePath(){ return _filePath.c_str() ; }

private:
   BOOLEAN _isOpen ;
   string  _filePath ;
   engine::ossFile _file ;
} ;
typedef _utilCipherFile utilCipherFile ;

INT32  utilBuildDefaultCipherFilePath( string &cipherFilePath ) ;
string utilGetUserShortNameFromUserFullName( const string &userFullName,
                                             string *clusterName = NULL ) ;

#endif
