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

   Source File Name = dmsIndexSortingBuilder.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          8/6/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMS_INDEX_SORTING_BUILDER_HPP_
#define DMS_INDEX_SORTING_BUILDER_HPP_

#include "dmsIndexBuilder.hpp"
#include "dmsExtDataHandler.hpp"
#include "utilString.hpp"
#include "../bson/ordering.h"
#include "../bson/bsonobj.h"

using namespace bson ;

namespace engine
{

   class _dmsIxmKeySorter ;

   class _dmsIndexSortingBuilder: public _dmsIndexBuilder
   {
   public:
      _dmsIndexSortingBuilder( _dmsStorageUnit* su,
                               _dmsMBContext* mbContext,
                               _pmdEDUCB* eduCB,
                               dmsExtentID indexExtentID,
                               dmsExtentID indexLogicID,
                               INT32 sortBufferSize,
                               dmsIndexBuildGuardPtr &guardPtr,
                               dmsDupKeyProcessor *dkProcessor,
                               dmsIdxTaskStatus* pIdxStatus = NULL ) ;
      ~_dmsIndexSortingBuilder() ;

   private:
      INT32 _init() ;
      INT32 _fillSorter( rtnTBScanner &scanner,
                         dmsRecordID &minRID,
                         dmsRecordID &maxRID,
                         dmsRecordID &moveRID,
                         ossScopedRWLock &lock ) ;
      INT32 _insertKeys( const dmsRecordID &minRID,
                         const dmsRecordID &maxRID,
                         const Ordering& ordering,
                         ossScopedRWLock &lock ) ;
      INT32 _build() ;

   private:
      BOOLEAN           _needBuildByRange ;
      _dmsIxmKeySorter* _sorter ;
      INT64             _bufSize ;
      INT64             _bufExtSize ;
      BOOLEAN           _eoc ;
   } ;
   typedef class _dmsIndexSortingBuilder dmsIndexSortingBuilder ;

}

#endif /* DMS_INDEX_SORTING_BUILDER_HPP_ */
