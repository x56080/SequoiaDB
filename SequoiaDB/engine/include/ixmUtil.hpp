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

   Source File Name = ixmUtil.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains common function of ixm
   component.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who      Description
   ====== =========== ======== ==============================================
          2019/09/20  Ting YU  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IXMUTIL_HPP_
#define IXMUTIL_HPP_

#include "ossTypes.hpp"
#include "../bson/bson.h"

using namespace bson ;

namespace engine
{
   BOOLEAN        ixmIsTextIndex( const BSONObj& indexDef ) ;

   INT32          ixmGetIndexType( const BSONObj& indexDef,
                                   UINT16 &type ) ;

   ossPoolString  ixmGetIndexTypeDesp( UINT16 type ) ;

   INT32          ixmBuildExtDataName( UINT64 clUniqID,
                                       const CHAR *idxName,
                                       CHAR *extName, UINT32 buffSize ) ;

   BOOLEAN        ixmIsSameDef( const BSONObj &defObj1,
                                const BSONObj &defObj2,
                                BOOLEAN strict = FALSE ) ;

}

#endif

