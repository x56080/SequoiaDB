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

   Source File Name = qgmPlSort.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef QGMPLSORT_HPP_
#define QGMPLSORT_HPP_

#include "qgmPlan.hpp"

namespace engine
{
   class _SDB_RTNCB ;

   class _qgmPlSort : public _qgmPlan
   {
   public:
      _qgmPlSort( const qgmOPFieldVec &order ) ;
      virtual ~_qgmPlSort() ;

   public:
      virtual void close() ;
      virtual string toString() const ;

   private:
      virtual INT32 _execute( _pmdEDUCB *eduCB ) ;

      virtual INT32 _fetchNext ( qgmFetchOut &next ) ;

   private:
      SINT64 _contextID ;
      SINT64 _contextSort ;
      BSONObj _orderBy ;
      _SDB_RTNCB *_rtnCB ;
   } ;
}

#endif

