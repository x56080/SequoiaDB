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

   Source File Name = qgmSelectorExprNode.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef QGM_SELECTOREXPRNODE_HPP_
#define QGM_SELECTOREXPRNODE_HPP_

#include "oss.hpp"
#include "core.hpp"
#include <boost/noncopyable.hpp>
#include <sstream>
#include "../bson/bson.hpp"

namespace engine
{
   class _qgmValueTuple ;

   class _qgmSelectorExprNode : public boost::noncopyable
   {
   public:
      _qgmSelectorExprNode() ;
      /// guarantee ele be valid
      ~_qgmSelectorExprNode() ;

   public:
      OSS_INLINE void setChildren( _qgmSelectorExprNode *l,
                                   _qgmSelectorExprNode *r )
      {
         _left = l ;
         _right = r ;
         return ;
      }

      OSS_INLINE void setLeft( _qgmSelectorExprNode *l )
      {
         _left = l ;
         return ;
      }

      OSS_INLINE void setRight( _qgmSelectorExprNode *r )
      {
         _right = r ;
         return ;
      }

      OSS_INLINE void setType( INT32 type )
      {
         _type = type ;
         return ;
      }

      OSS_INLINE void setValue( INT64 v )
      {
         *(( INT64 *)_data) = v ;
         _isDouble = FALSE ;
         return ;
      }

      OSS_INLINE void setValue( FLOAT64 v )
      {
         *(( FLOAT64 *)_data) = v ;
         _isDouble = TRUE ;
         return ;
      }

      INT32 getValue( const bson::BSONElement &e,
                      _qgmValueTuple *v ) const ;

      void toString( std::stringstream &ss )const ;

   private:
      INT32 _calcValue( const _qgmValueTuple &lv,
                        const _qgmValueTuple &rv,
                        INT32 type,
                        _qgmValueTuple &v ) const ;

   private:
      INT32 _type ;
      BOOLEAN _isDouble ;
      CHAR _data[8] ;
      _qgmSelectorExprNode *_left ;
      _qgmSelectorExprNode *_right ;
   } ;
}

#endif

