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

   Source File Name = deltaPageList.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_DELTA_PAGE_LIST_H_
#define VESSEL_DELTA_PAGE_LIST_H_

#include "vessel/pageIdentifier.h"
#include "vessel/vesselIdDef.h"
#include "ossMemPool.hpp"
#include <tuple> //c++11

namespace engine
{
namespace vessel
{
   ///<lpid, pid, psv>
   typedef std::tuple<PAGE_ID, PAGE_ID, PAGE_SNAPSHOT_VERION> DELTA_PAGE_TUPLE;
   typedef ossPoolVector<DELTA_PAGE_TUPLE> DELTA_PAGE_LIST;
   struct DELTA_PAGE_LIST_CMP
   {
      BOOLEAN operator()(const DELTA_PAGE_TUPLE &l,
                         const DELTA_PAGE_TUPLE &r)const
      {
         return std::get<0>(l) < std::get<0>(r);
      }
   };
} // namespace vessel

} // namespace engine


#endif//VESSEL_DELTA_PAGE_LIST_H_