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

   Source File Name = storageUnitManifest.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_STORAGE_UNIT_MANIFEST_H_
#define VESSEL_STORAGE_UNIT_MANIFEST_H_

#include "vessel/storageFileDef.h"
#include "vessel/objectIdentifier.h"

namespace engine
{
namespace vessel
{
#pragma pack(4)
   struct storageUnitManifest : public SDBObject
   {
      OSS_INLINE BOOLEAN isValid()const
      {
         return id.isValid() &&
                dataArgs.isValid() &&
                idxArgs.isValid() &&
                lobArgs.isValid();
      }

      void reset()
      {
         id.reset();
         flags = 0;
         secretValue = 0;
         dataArgs.reset();
         idxArgs.reset();
         lobArgs.reset();
         return;
      }
      
      collectionSpaceId id;
      UINT32 flags;
      UINT32 secretValue = 0;

      storageCoreArgs dataArgs;
      storageCoreArgs idxArgs;
      storageCoreArgs lobArgs;
   };//struct storageUnitManifest

   struct storageFileManifest : public SDBObject
   {
      OSS_INLINE BOOLEAN isValid()const
      {
         return INVALID_SPACE_ID != sid &&
                INVALID_SPACE_TYPE != stype &&
                INVALID_FILE_TYPE != ftype &&
                args.isValid();
      }

      void reset()
      {
         sid = INVALID_SPACE_ID;
         stype = INVALID_SPACE_TYPE;
         ftype = INVALID_FILE_TYPE;
         secretValue = 0;
         args.reset();
      }

      SPACE_ID sid = INVALID_SPACE_ID;
      SPACE_TYPE stype = INVALID_SPACE_TYPE;
      SPACE_TYPE ftype = INVALID_FILE_TYPE;
      UINT32 secretValue = 0;
      storageCoreArgs args;
   };//struct storageFileManifest

#pragma pack()
} // namespace vessel

} // namespace engine


#endif//VESSEL_STORAGE_UNIT_MANIFEST_H_