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

   Source File Name = collectionProperties.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_COLLECTION_PROPERTIES_H_
#define VESSEL_COLLECTION_PROPERTIES_H_

#include "vessel/objectIdentifier.h"
#include "vessel/objectBaseDef.h"
#include "utilCompression.hpp"
#include "dmsStripingId.hpp"
#include "vessel/csProperties.h"

namespace engine
{
namespace vessel
{
   struct collectionProperties : public SDBObject
   {
      OSS_INLINE void reset()
      {
         csproperties = nullptr;
         clid.reset();
         name.clear();
         type = CL_TYPE_INVALID;
         compressor = UTIL_COMPRESSOR_INVALID;
         _minFreePct = 0;
         stripingRange.reset();
         return;
      }

      OSS_INLINE FLOAT32 getMinFreePct()const
      {
         return static_cast<FLOAT32>(_minFreePct) / 100;
      }

      OSS_INLINE globalCollectionId getGlobalId()const
      {
         globalCollectionId id;
         if (nullptr != csproperties)
         {
            id.reset(csproperties->csid, clid);
         }
         return id;
      }

      OSS_INLINE globalLogicalClId getGlobalLogicalId()const
      {
         return (nullptr != csproperties) ?
                globalLogicalClId(csproperties->csid.getLid(),
                                  clid.getLid()) :
                globalLogicalClId();
      }

      ossPoolString getFullName()const
      {
         SDB_ASSERT(nullptr != csproperties, "can not be invalid");
         ossPoolString fullName;
         fullName.reserve(csproperties->name.size() +
                          name.size() + 2);
         fullName.append(csproperties->name.c_str()).append(".").append(name.c_str());
         return std::move(fullName);
      }


      const csProperties *csproperties = nullptr;
      collectionId clid;
      std::string name;
      CL_TYPE type = CL_TYPE_INVALID;
      UTIL_COMPRESSOR_TYPE compressor = UTIL_COMPRESSOR_INVALID;
      UINT8 _minFreePct = 0;
      dmsStripingRange stripingRange;
   };//struct collectionProperties

} // namespace vessel

} // namespace engine


#endif//VESSEL_COLLECTION_PROPERTIES_H_