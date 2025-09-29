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

   Source File Name = sdbPasswd.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who      Description
   ====== =========== ======== ==============================================
          05/19/2022  Ting YU  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SDB_SECURE_TOOL_HPP_
#define SDB_SECURE_TOOL_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossMemPool.hpp"

using namespace std ;

class _sdbSecureTool
{
public:
   _sdbSecureTool() ;
   ~_sdbSecureTool() {}

   INT32 init( INT32 argc, CHAR* argv[] ) ;
   INT32 process() ;

private:
   INT32 _processData() ;
   INT32 _processFiles() ;
   INT32 _processOneFile( const ossPoolString& sourceFullName,
                          const ossPoolString& outputFullOrPath ) ;
   INT32 _checkSourceFilePath( ossPoolVector<ossPoolString>& sourceFiles ) ;
   INT32 _checkOutputFilePath( ossPoolString& outputFileOrPath ) ;
   INT32 _buildOutputFullFile( const ossPoolString& sourceFullName,
                               const ossPoolString& outputPath,
                               ossPoolString& outputFullFile ) ;
   void _decryptContent( const CHAR* str,
                         INT32 len,
                         ossPoolString& output ) ;

private:
   ossPoolString _decryptData ;
   BOOLEAN       _hasDecrypt ;

   ossPoolString _sourceFileOrPath ;
   BOOLEAN       _sourceIsPath ; // _sourceFileOrPath is path or file

   ossPoolString _outputFileOrPath ;
   BOOLEAN       _hasOutputPath ;
   BOOLEAN       _outputIsPath ; // _outputFileOrPath is path or file
} ;
typedef _sdbSecureTool sdbSecureTool ;

#endif
