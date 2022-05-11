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

   Source File Name = lobMetaDataFile.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_LOB_META_DATA_FILE_H_
#define VESSEL_LOB_META_DATA_FILE_H_

#include "vessel/baseMetaDataFile.h"

namespace engine
{
namespace vessel
{
   class lobMetaDataFile : public baseMetaDataFile
   {
      public:
         lobMetaDataFile() = default;
         virtual ~lobMetaDataFile() = default;

      public:
         static constexpr UINT32 PAGE_SIZE = 65536;
         static constexpr UINT32 PAGE_COUNT_PER_SEG = 512; /// 32MB per segment
         static constexpr UINT64 MAX_FILE_SIZE = (UINT64)512 << 30; /// 512GB
         static constexpr UINT32 MAX_PAGE_COUNT = MAX_FILE_SIZE / PAGE_SIZE;
         static constexpr UINT32 MAX_SEG_COUNT = MAX_PAGE_COUNT / PAGE_COUNT_PER_SEG;
         static constexpr UINT32 SME_SIZE = MAX_PAGE_COUNT / 8;

      private:
         virtual UINT32 _getReservedAreaSize()const override
         {
            return SME_SIZE;
         }
   };//class lobMetaDataFile
} // namespace vessel

} // namespace engine


#endif//VESSEL_LOB_META_DATA_FILE_H_