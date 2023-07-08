/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

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
                     const _qgmField &alias,
                     BOOLEAN strict = FALSE,
                     const ossPoolString& arrayIdxName = "" ) ;

      virtual ~_qgmPlSplitBy() ;

   public:
      virtual string toString() const ;

   private:
      virtual INT32 _execute( _pmdEDUCB *eduCB ) ;

      virtual INT32 _fetchNext ( qgmFetchOut &next, _pmdEDUCB *eduCB ) ;

      void _clear() ;

      template<class Builder>
      INT32    _buildNewObj( Builder &b, BSONObjIterator &es,
                             const BSONElement &replace ) ;

      void     _appendReplace( BSONObjBuilder &b,
                               const BSONElement &replace ) ;
      void     _appendReplace( BSONArrayBuilder &b,
                               const BSONElement &replace ) ;

      BOOLEAN  _shouldSkip( const BSONElement &ele ) ;

   private:
      _qgmDbAttr        _splitby ;
      qgmFetchOut       _fetch ;
      BSONObjIterator   _itr ;
      BSONElement       _splitEle ;
      ossPoolString     _fieldName ;
      BOOLEAN           _replaced ;
      INT64             _currentIdx ;        // Note: Keep the type of _currentIdx as INT64, to be
                                             // compatible with mongodb.
      BOOLEAN           _strict ;            // Only return records when the split by field is an
                                             // non-empty array.
      ossPoolString     _arrayIndexName ;    // Name of the field to return the index in the array.
                                             // When it's empty, no index field returned.
   } ;
   typedef class _qgmPlSplitBy qgmPlSplitBy ;
}

#endif // QGMPLSPLITBY_HPP_

