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

   Source File Name = qgmSelector.hpp

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

#ifndef QGMSELECTOR_HPP_
#define QGMSELECTOR_HPP_

#include "qgmDef.hpp"

namespace engine
{
   class _qgmSelector : public SDBObject
   {
   public:
      _qgmSelector() ;
      virtual ~_qgmSelector() ;

   public:
      OSS_INLINE BOOLEAN empty()const{ return _selector.empty() ;}

      OSS_INLINE BOOLEAN needSelect()const{return _needSelect ;}

      INT32 load( const qgmOPFieldVec &op ) ;

      INT32 select( const BSONObj &src, BSONObj &out ) const;

      INT32 select( const qgmFetchOut &src, BSONObj &out ) const ;

      BSONObj selector() const;

      string toString() const ;

   private:
      INT32 _createValueWithExpr( const BSONElement &e,
                                  const CHAR *fieldName,
                                  const _qgmSelectorExpr &expr,
                                  BSONObjBuilder &builder ) const ;

   private:
      qgmOPFieldVec _selector ;
      BOOLEAN _needSelect ;
   } ;

   typedef class _qgmSelector qgmSelector ;
}

#endif

