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

   Source File Name = collectionProperties.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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