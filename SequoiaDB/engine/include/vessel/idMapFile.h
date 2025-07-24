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

   Source File Name = idMapFile.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_ID_MAP_FILE_H_
#define VESSEL_ID_MAP_FILE_H_

#include "vessel/storageFile.h"
#include "vessel/logicalPageSpaceCheckpoint.h"

namespace engine
{
namespace vessel
{
   constexpr UINT32 ID_MAP_FILE_HEAD_VERSION = 1;

#pragma pack(4)
   struct idMapFileHead
   {
      OSS_INLINE BOOLEAN isValid()const
      {
         return ID_MAP_FILE_HEAD_VERSION == version;
      }
      
      UINT32 version = 0;
      UINT32 flags = 0;
      /// mutable fields
      UINT32 totalPageCount = 0;
      ///checkpoint
      LPS_CHECKPOINT checkpoint;

   };//struct idMapFileHead

#pragma pack()

   class idMapFile : public storageFile
   {
      public:
         idMapFile(){}
         virtual ~idMapFile(){}

      public:
         INT32 getIdMapFileHead(idMapFileHead &h)const;
         UINT32 getTotalPageCount()const {return _pageCount;}

      private:
         virtual void _close() override;
         virtual INT32 _open(BOOLEAN isCreating) override;
         virtual void _onHeaderUpdated(const slice &hs) override;
      private:
         UINT32 _pageCount = 0;
   }; // class idMapFile
} // namespace vessel
} // namespace engine

#endif // VESSEL_ID_MAP_FILE_H_