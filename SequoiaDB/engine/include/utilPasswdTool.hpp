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

   Source File Name = utilPasswdTool.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/26/2018  ZWB  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTILPASSWDTOOL_H_
#define UTILPASSWDTOOL_H_

#include "utilCipherMgr.hpp"

class _utilPasswordTool : public SDBObject
{
public:
   _utilPasswordTool() {}
   ~_utilPasswordTool() {}
   static BOOLEAN interactivePasswdInput( string &passwd ) ;
   INT32  getPasswdByCipherFile( const string &userFullName,
                                 const string &token,
                                 string &filePath,
                                 string &password ) ;

private:
   utilCipherMgr    _cipherMgr ;
   utilCipherFile   _cipherfile ;
} ;
typedef _utilPasswordTool utilPasswordTool ;

#endif