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

   Source File Name = qgmPlAggregation.hpp

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

#ifndef QGMPLAGGREGATION_HPP_
#define QGMPLAGGREGATION_HPP_

#include "qgmPlan.hpp"
#include "qgmOptiAggregation.hpp"
#include "rtnSQLFunc.hpp"
#include "qgmSelector.hpp"

namespace engine
{
   class _qgmPlAggregation : public _qgmPlan
   {
   public:
      _qgmPlAggregation( const qgmAggrSelectorVec &selector,
                         const qgmOPFieldVec &groupby,
                         const qgmField &alias,
                         _qgmPtrTable *table ) ;

      virtual ~_qgmPlAggregation() ;

   public:
      virtual string toString() const ;

   private:
      virtual INT32 _execute( _pmdEDUCB *eduCB ) ;

      virtual INT32 _fetchNext( qgmFetchOut &next ) ;

   private:
      INT32 _push( const qgmFetchOut &next ) ;

      INT32 _result( qgmFetchOut &result ) ;

      INT32 _select( const qgmFetchOut &next,
                     const vector<qgmOpField> &fields,
                     RTN_FUNC_PARAMS &param ) ;

   private:
      std::vector<_rtnSQLFunc *> _func ;
      _qgmSelector _groupby ;
      BSONObj _groupbyKey ;
      BSONObj _preObj ;
      BOOLEAN _eoc ;
      BOOLEAN _pushedAtThisTime ;
      BOOLEAN _pushedAtAnyTime ;
      BOOLEAN _isAggr;
      BOOLEAN _isStat ;
   } ;

   typedef class _qgmPlAggregation qgmPlAggregation ;
}

#endif

