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

   Source File Name = baseMetaDataFile.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_BASE_META_DATA_FILE_H_
#define VESSEL_BASE_META_DATA_FILE_H_

#include "vessel/storageFile.h"
#include "vessel/bitmapScanner.h"
#include "ossMemPool.hpp"

#include <mutex>//c++11

namespace engine
{
namespace vessel
{
   class baseMetaDataFile : public storageFile
   {
      public:
         baseMetaDataFile() = default;
         virtual ~baseMetaDataFile() = default;

      public:
         INT32 reservePid(PAGE_ID &pid, mmapPagePointer *ptr=nullptr);
         void freePid(PAGE_ID pid);
         void freePids(UINT32 size, const PAGE_ID *pids);
         void freePids(const ossPoolSet<PAGE_ID> &set);

      private:
         virtual void _close() override;
         virtual INT32 _open(BOOLEAN isCreating) override;

      private:
         void _loadSme();
         INT32 _initSme();

      private:
         std::mutex _mutex;
         bitmapScanner _scanner;
   };//class baseMetaDataFile
} // namespace vessel

} // namespace engine


#endif//VESSEL_BASE_META_DATA_FILE_H_
