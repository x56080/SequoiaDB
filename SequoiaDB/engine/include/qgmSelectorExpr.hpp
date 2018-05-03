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

   Source File Name = qgmSelectorExpr.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef QGM_SELECTOREXPR_HPP_
#define QGM_SELECTOREXPR_HPP_

#include "qgmSelectorExprNode.hpp"
#include <boost/shared_ptr.hpp>

namespace engine
{
   class _qgmSelectorExpr : public SDBObject
   {
   public:
      _qgmSelectorExpr() ;
      ~_qgmSelectorExpr() ;
      _qgmSelectorExpr( const _qgmSelectorExpr &e )
      :_exprRoot( e._exprRoot )
      {

      }

      _qgmSelectorExpr &operator=(const _qgmSelectorExpr &e)
      {
         _exprRoot = e._exprRoot ;
         return *this ;
      }

   public:
      BOOLEAN isEmpty() const
      {
         return NULL == _exprRoot.get() ;
      }

      void set( _qgmSelectorExprNode *root )
      {
         _exprRoot.reset( root ) ;
         return ;
      }

      std::string toString() const ;

      INT32 getValue( const bson::BSONElement &e,
                      _qgmValueTuple &v ) const ;

   private:
      boost::shared_ptr<_qgmSelectorExprNode> _exprRoot ;
   } ;
}

#endif

