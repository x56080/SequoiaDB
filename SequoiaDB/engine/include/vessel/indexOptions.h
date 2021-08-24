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

   Source File Name = indexOptions.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_INDEX_OPTIONS_H_
#define VESSEL_INDEX_OPTIONS_H_

#include "core.hpp"
#include "oss.hpp"

namespace engine
{
namespace vessel
{
   class buildIndexOptions : public SDBObject
   {
      public:
         buildIndexOptions(){}
         ~buildIndexOptions(){}
         buildIndexOptions(const buildIndexOptions &) = delete;
         buildIndexOptions &operator=(const buildIndexOptions &o)
         {
            sortBufferSize = o.sortBufferSize;
            blockDML = o.blockDML;
            return *this;
         }

      public:
         BOOLEAN isSortingDisabled()const
         {
            return 0 == sortBufferSize;
         }
      public:
         UINT32 sortBufferSize = 64;//MB
         BOOLEAN blockDML = FALSE;
   };//class buildIndexOptions

   class createIndexOptions : public SDBObject
   {
      public:
         createIndexOptions(){}
         ~createIndexOptions(){}
         createIndexOptions(const createIndexOptions &) = delete;
         createIndexOptions &operator=(const createIndexOptions &o)
         {
            build = o.build;
            return *this;
         }
      public:
         buildIndexOptions build;
   };//class createIndexOptions
}//namespace vessel
}//namespace engine

#endif//VESSEL_INDEX_OPTIONS_H_