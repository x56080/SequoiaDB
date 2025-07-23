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

   Source File Name = sptSPInfo.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/07/2017  WJM  Initial Draft

   Last Changed =

*******************************************************************************/
#include "sptPrivateData.hpp"

using namespace std ;
namespace engine
{
   _sptPrivateData::_sptPrivateData():
      _scope( NULL ), _errLineno( 0 ), _isSetErrInfo( FALSE )
   {
   }

   _sptPrivateData::_sptPrivateData( sptScope *pScope ):
      _scope( pScope ), _errLineno( 0 ), _isSetErrInfo( FALSE )
   {
   }

   _sptPrivateData::~_sptPrivateData()
   {
      _scope = NULL ;
   }

   sptScope* _sptPrivateData::getScope()
   {
      return _scope ;
   }

   const CHAR* _sptPrivateData::getErrFileName()
   {
      return _errFileName.c_str() ;
   }

   UINT32 _sptPrivateData::getErrLineno()
   {
      return _errLineno ;
   }

   void _sptPrivateData::SetErrInfo( const std::string &filename,
                                     const UINT32 lineno )
   {
      _errFileName = filename ;
      _errLineno = lineno ;
      _isSetErrInfo = TRUE ;
   }

   BOOLEAN _sptPrivateData::isSetErrInfo()
   {
      return _isSetErrInfo ;
   }

   void _sptPrivateData::clearErrInfo()
   {
      _errFileName.clear() ;
      _errLineno = 0 ;
      _isSetErrInfo = FALSE ;
   }

}
