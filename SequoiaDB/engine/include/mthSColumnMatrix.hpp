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

   Source File Name = mthSColumnMatrix.hpp

   Descriptive Name = mth selector column matrix

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/01/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef MTH_SCOLUMNMATRIX_HPP_
#define MTH_SCOLUMNMATRIX_HPP_

#include "mthSColumn.hpp"
#include "mthNodePool.hpp"

namespace engine
{
   class _mthSColumnMatrix : public _mthSColumn 
   {
   public:
      _mthSColumnMatrix() ;
      ~_mthSColumnMatrix() ;

   public:
      OSS_INLINE const bson::BSONObj &getPattern() const
      {
         return _pattern ;
      }

      OSS_INLINE BOOLEAN empty() const
      {
         return _pattern.isEmpty() ;
      }

   public:
      virtual void clear() ;

   public:
      INT32 load( const bson::BSONObj &obj, BOOLEAN strictDataMode = FALSE,
                  IXM_FIELD_NAME_SET *pSelectSet = NULL ) ;

      INT32 select( const bson::BSONObj &obj,
                    bson::BSONObj &selected ) ;

   private:
      /// build matirx
      INT32 _load( const bson::BSONElement &e, BOOLEAN strictDataMode ) ;

      INT32 _loadObj( _mthSColumn *column,
                      const bson::BSONObj &obj,
                      UINT32 &actionNum,
                      BOOLEAN strictDataMode ) ;

      INT32 _loadDefaultValue( const bson::BSONElement &e ) ;

      /// build column
      INT32 _getColumn( const CHAR *name, _mthSColumn *&column ) ;

      INT32 _getColumn( const CHAR *name,
                        _mthSColumn *father,
                        _mthSColumn *&column) ;

      INT32 _allocateAction( _mthSAction *&action ) ;

      INT32 _addMiddleAction( _mthSColumn *column,
                              INT32 numberic ) ;
   private:
      bson::BSONObj _pattern ;

      _mthNodePool<mthSColumn> _columnPool ;
      _mthNodePool<mthSAction> _actionPool ;
   } ;
   typedef class _mthSColumnMatrix mthSColumnMatrix ;
}

#endif

