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

   Source File Name = sptSPInfo.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/07/2017  WJM  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SPT_PRIVATE_DATA_HPP_
#define SPT_PRIVATE_DATA_HPP_
#include "sptScope.hpp"
#include "oss.hpp"
#include <iostream>

namespace engine
{
   class _sptPrivateData:public SDBObject
   {
   public:
      _sptPrivateData() ;

      _sptPrivateData( sptScope *pScope ) ;

      virtual ~_sptPrivateData() ;

      sptScope *getScope() ;

      const CHAR* getErrFileName() ;

      UINT32 getErrLineno() ;

      void SetErrInfo( const std::string &filename, const UINT32 lineno ) ;

      BOOLEAN isSetErrInfo() ;

      void clearErrInfo() ;
   private:
      // _scope can't be modified after init
      sptScope *_scope ;
      std::string _errFileName ;
      UINT32 _errLineno ;
      BOOLEAN _isSetErrInfo ;
   } ;

   typedef class _sptPrivateData sptPrivateData ;
}
#endif
