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

   Source File Name = fmpJSVM.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/19/2013  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef FMPJSVM_HPP_
#define FMPJSVM_HPP_

#include "fmpVM.hpp"
#include <string>

namespace engine
{
   class _sptContainer ;
   class _sptScope ;
   class _sptSPVal ;
}
namespace sdbclient
{
   class _sdbCursor ;
}

class _fmpJSVM : public _fmpVM
{
public:
   _fmpJSVM() ;
   virtual ~_fmpJSVM() ;

public:
   virtual INT32 eval( const BSONObj &func,
                       BSONObj &res ) ;

   virtual INT32 fetch( BSONObj &res ) ;

   virtual INT32 initGlobalDB( const CHAR *pHostName,
                               const CHAR *pSvcname,
                               const CHAR *pUser,
                               const CHAR *pPasswd,
                               BSONObj &res ) ;

  // virtual INT32 compile( const BSONElement &func, const CHAR *name ) ;

private:
   INT32 _transCode2Str( const BSONElement &ele, std::string &str ) ;

   INT32 _getValType( const engine::_sptSPVal *pVal ) const ;

private:
   engine::_sptContainer   *_engine ;
   engine::_sptScope       *_scope ;
   std::string             _cmd ;
   sdbclient::_sdbCursor*  _cursor ;
} ;

#endif

