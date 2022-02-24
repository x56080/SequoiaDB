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

   Source File Name = deltaPageList.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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