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

   Source File Name = dmsIndexOnlineBuilder.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          8/6/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMS_INDEX_ONLINE_BUILDER_HPP_
#define DMS_INDEX_ONLINE_BUILDER_HPP_

#include "dmsIndexBuilder.hpp"
#include "dmsExtDataHandler.hpp"
#include "utilString.hpp"
#include "../bson/ordering.h"
#include "../bson/bsonobj.h"

using namespace bson ;

namespace engine
{
   class _dmsIndexOnlineBuilder: public _dmsIndexBuilder
   {
   public:
      _dmsIndexOnlineBuilder( _dmsStorageUnit* su,
                              _dmsMBContext* mbContext,
                              _pmdEDUCB* eduCB,
                              dmsExtentID indexExtentID,
                              dmsExtentID indexLogicID,
                              dmsIndexBuildGuardPtr &guardPtr,
                              dmsDupKeyProcessor *dkProcessor,
                              dmsIdxTaskStatus* pIdxStatus = NULL ) ;
      ~_dmsIndexOnlineBuilder() ;

   private:
      INT32 _build() ;
   } ;
   typedef class _dmsIndexOnlineBuilder dmsIndexOnlineBuilder ;
}

#endif /* DMS_INDEX_ONLINE_BUILDER_HPP_ */

