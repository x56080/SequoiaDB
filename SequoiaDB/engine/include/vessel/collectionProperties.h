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

namespace engine
{
namespace vessel
{
   struct collectionProperties : public SDBObject
   {
      OSS_INLINE void reset()
      {
         csid.reset();
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
         id.reset(csid, clid);
         return id;
      }

      collectionSpaceId csid; ///TODO: move to cs properties
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