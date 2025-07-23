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

   Source File Name = fileStream.cpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#include "fileStream.hpp"

fileOutStream::fileOutStream() : _force( false ),
                                 _ss( "" )
{
}

fileOutStream::~fileOutStream()
{
}

void fileOutStream::setReplace()
{
   _force = true ;
}

bool fileOutStream::_isReplace( const char *path )
{
   string source ;

   utilGetFileContent( path, source ) ;

   return source.compare( _ss.str() ) != 0 ;
}

int fileOutStream::close( const char *path )
{
   int rc = 0 ;

   if ( _isReplace( path ) || _force )
   {
      utilPutFileContent( path, _ss.str().c_str() ) ;
   }
   else
   {
      rc = -1 ;
   }

   return rc ;
}

