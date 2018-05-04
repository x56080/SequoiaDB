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

   Source File Name = qgmPlFilter.hpp

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

#ifndef QGMPLFILTER_HPP_
#define QGMPLFILTER_HPP_

#include "qgmPlan.hpp"
#include "qgmMatcher.hpp"
#include "qgmSelector.hpp"

namespace engine
{
   struct _qgmConditionNode ;

   class _qgmPlFilter : public _qgmPlan
   {
   public:
      /// not the same to qgmPlScan, selector and condition
      /// should include alias.
      /// eg: element in selector is T.a:alias
      _qgmPlFilter( const qgmOPFieldVec &selector,
                    _qgmConditionNode *condition,
                    INT64 numSkip,
                    INT64 numReturn,
                    const qgmField &alias ) ;

      virtual ~_qgmPlFilter() ;

   public:
      virtual string toString() const ;

   private:
      virtual INT32 _execute( _pmdEDUCB *eduCB ) ;

      virtual INT32 _fetchNext ( qgmFetchOut &next ) ;

   protected:
      qgmSelector _selector ;
      INT64 _return ;
      INT64 _skip ;
      INT64 _currentSkip ;
      INT64 _currentReturn ;

   private:
      qgmMatcher _matcher ;
      _qgmConditionNode *_condition ;
      BOOLEAN _hasSelector ;
   } ;

   typedef class _qgmPlFilter qgmPlFilter ;
}

#endif

