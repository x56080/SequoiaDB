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

   
*******************************************************************************/
#ifndef   SPT_PARSE_MANDOC_HPP__
#define   SPT_PARSE_MANDOC_HPP__
#include <core.hpp>

class _sptParseMandoc
{
public:
   static _sptParseMandoc& getInstance() ;
   INT32 parse( const CHAR* filename ) ;
private:
   _sptParseMandoc() {} ;
   ~_sptParseMandoc() {} ;
   _sptParseMandoc( const _sptParseMandoc& ) ;
   _sptParseMandoc& operator=( const _sptParseMandoc& ) ;
} ;
typedef _sptParseMandoc sptParseMandoc ;

#endif // SPT_PARSE_MANDOC_HPP__