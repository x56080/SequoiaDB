/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = qgmPlNLJoin.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains declare for QGM operators

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/09/2013  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef QGMPLNLJOIN_HPP_
#define QGMPLNLJOIN_HPP_

#include "qgmPlJoin.hpp"

namespace engine
{
   class _qgmPlNLJoin : public _qgmPlJoin
   {
   public:
      _qgmPlNLJoin( INT32 type ) ;
      virtual ~_qgmPlNLJoin() ;

   private:
      virtual INT32 _execute( _pmdEDUCB *eduCB ) ;

      virtual INT32 _fetchNext ( qgmFetchOut &next ) ;

     INT32 _modifyInnerCondition( BSONObj &obj ) ;

   private:
      INT32 _init() ;

   private:
      BOOLEAN _makeOuterInner ;
      BOOLEAN _innerEnd ;
      BOOLEAN _notMatched ;
      qgmFetchOut _outerF ;
      qgmFetchOut *_innerF ;
   } ;

   typedef class _qgmPlNLJoin qgmPlNLJoin ;
}

#endif

