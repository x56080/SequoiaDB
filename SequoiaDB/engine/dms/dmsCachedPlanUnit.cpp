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

   Source File Name = dmsCachedPlanUnit.cpp

   Descriptive Name = DMS Cached Access Plan Units

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains code logic for
   management of cached access plans of collections.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/10/2017  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#include "dmsCachedPlanUnit.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"

namespace engine
{

   /*
      _dmsCLCachedPlanUnit implement
    */
   _dmsCLCachedPlanUnit::_dmsCLCachedPlanUnit ()
   : _utilSUCacheUnit()
   {
      _paramInvalidCount = 0 ;
      _mainCLInvalidCount = 0 ;
   }

   _dmsCLCachedPlanUnit::_dmsCLCachedPlanUnit ( UINT16 mbID, UINT64 createTime )
   : _utilSUCacheUnit( mbID, createTime )
   {
      _paramInvalidCount = 0 ;
      _mainCLInvalidCount = 0 ;
   }

   _dmsCLCachedPlanUnit::~_dmsCLCachedPlanUnit ()
   {
   }

}

