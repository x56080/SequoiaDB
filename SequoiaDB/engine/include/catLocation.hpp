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

   Source File Name = catLocation.hpp

   Descriptive Name = N/A

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          02/20/2023  LCX Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef CAT_LOCATION_HPP__
#define CAT_LOCATION_HPP__

#include "msgCatalogDef.h"
#include "clsDef.hpp"

namespace engine
{

   INT32 catGetLocationInfo( const BSONObj &groupObj,
                             CLS_LOC_INFO_MAP *pLocationInfo ) ;

   INT32 catGetLocationID( const BSONObj &groupObj,
                           const CHAR *pLocation,
                           UINT32 &locationID ) ;

   INT32 catCheckAndGetActiveLocation( const BSONObj &groupObj,
                                       const UINT32 groupID,
                                       const ossPoolString &newActLoc,
                                       ossPoolString &oldActLoc ) ;
}

#endif // CAT_LOCATION_HPP__