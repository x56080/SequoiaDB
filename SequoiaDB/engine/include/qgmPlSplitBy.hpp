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

   Source File Name = qgmPlSplitBy.hpp

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

#ifndef QGMPLSPLITBY_HPP_
#define QGMPLSPLITBY_HPP_

#include "qgmPlan.hpp"
#include "msgDef.h"

using namespace bson ;

namespace engine
{
   /*
      _qgmPlSplitBy define
   */
   class _qgmPlSplitBy : public _qgmPlan
   {
   public:
      _qgmPlSplitBy( const _qgmDbAttr &split,
                     const _qgmField &alias ) ;
      virtual ~_qgmPlSplitBy() ;

   public:
      virtual string toString() const ;

   private:
      virtual INT32 _execute( _pmdEDUCB *eduCB ) ;

      virtual INT32 _fetchNext ( qgmFetchOut &next ) ;

      void _clear() ;

      template<class Builder>
      INT32    _buildNewObj( Builder &b, BSONObjIterator &es,
                             const BSONElement &replace ) ;

      void     _appendReplace( BSONObjBuilder &b,
                               const BSONElement &replace ) ;
      void     _appendReplace( BSONArrayBuilder &b,
                               const BSONElement &replace ) ;

   private:
      _qgmDbAttr        _splitby ;
      qgmFetchOut       _fetch ;
      BSONObjIterator   _itr ;
      BSONElement       _splitEle ;
      std::string       _fieldName ;
      BOOLEAN           _replaced ;

   } ;
   typedef class _qgmPlSplitBy qgmPlSplitBy ;
}

#endif // QGMPLSPLITBY_HPP_

