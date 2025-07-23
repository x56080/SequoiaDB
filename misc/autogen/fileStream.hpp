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

   Source File Name = fileStream.hpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef FILE_STREAM_HPP
#define FILE_STREAM_HPP

#include "util.h"
#include <string>
#include <iomanip>
#include <sstream>
using namespace std;

class fileOutStream
{
public:
   fileOutStream() ;
   ~fileOutStream() ;

   template <typename T>
   ostringstream& operator << (T a)
   {
      _ss << a ;
      return _ss ;
   }

   void setReplace() ;
   int close( const char *path ) ;

private:
   bool _isReplace( const char *path ) ;

private:
   bool           _force ;
   ostringstream  _ss ;
} ;

#endif