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

   Source File Name = baseMetaDataFile.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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
