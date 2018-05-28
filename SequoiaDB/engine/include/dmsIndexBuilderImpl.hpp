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

   Source File Name = dmsIndexBuilderImpl.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          8/6/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMS_INDEX_BUILDER_IMPL_HPP_
#define DMS_INDEX_BUILDER_IMPL_HPP_

#include "dmsIndexBuilder.hpp"
#include "../bson/ordering.h"
#include "../bson/bsonobj.h"

using namespace bson ;

namespace engine
{
   class _dmsIndexOnlineBuilder: public _dmsIndexBuilder
   {
   public:
      _dmsIndexOnlineBuilder( _dmsStorageIndex* indexSU,
                              _dmsStorageData* dataSU,
                              _dmsMBContext* mbContext,
                              _pmdEDUCB* eduCB,
                              dmsExtentID indexExtentID ) ;
      ~_dmsIndexOnlineBuilder() ;

   private:
      INT32 _build() ;
   } ;
   typedef class _dmsIndexOnlineBuilder dmsIndexOnlineBuilder ;

   class _dmsIxmKeySorter ;

   class _dmsIndexSortingBuilder: public _dmsIndexBuilder
   {
   public:
      _dmsIndexSortingBuilder( _dmsStorageIndex* indexSU,
                               _dmsStorageData* dataSU,
                               _dmsMBContext* mbContext,
                               _pmdEDUCB* eduCB,
                               dmsExtentID indexExtentID,
                               INT32 sortBufferSize ) ;
      ~_dmsIndexSortingBuilder() ;

   private:
      INT32 _init() ;
      INT32 _fillSorter() ;
      INT32 _insertKeys( const Ordering& ordering ) ;
      INT32 _build() ;

   private:
      _dmsIxmKeySorter* _sorter ;
      INT64             _bufSize ;
      INT64             _bufExtSize ;
      BOOLEAN           _eoc ;
   } ;
   typedef class _dmsIndexSortingBuilder dmsIndexSortingBuilder ;
}

#endif /* DMS_INDEX_BUILDER_IMPL_HPP_ */

