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

   Source File Name = collectionSpaceIdentifier.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_COLLECTION_SPACE_IDENTIFIER_H_
#define VESSEL_COLLECTION_SPACE_IDENTIFIER_H_


#include "vessel/vesselIdDef.h"
#include "utilUniqueID.hpp"

namespace engine
{
namespace vessel
{
   class collectionSpaceIdentifier : public SDBObject
   {
      public:
         collectionSpaceIdentifier(){}
         explicit collectionSpaceIdentifier(UINT32 lid,
                                            utilCSUniqueID uniqueId,
                                            SPACE_ID sid):
                  _lid(lid),
                  _uniqueId(uniqueId),
                  _sid(sid){}
         ~collectionSpaceIdentifier(){}
         collectionSpaceIdentifier(const collectionSpaceIdentifier &o):
         _lid(o._lid),
         _uniqueId(o._uniqueId),
         _sid(o._sid){}

         collectionSpaceIdentifier &operator=(const collectionSpaceIdentifier &o)
         {
            _lid = o._lid;
            _uniqueId = o._uniqueId;
            _sid = o._sid;
            return *this;
         }

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return DMS_INVALID_LOGICCSID != _lid &&
                   INVALID_SPACE_ID != _sid;
         }
         OSS_INLINE UINT32 getLogicalId()const
         {
            return _lid;
         }
         OSS_INLINE UINT32 getUniqueId()const
         {
            return _uniqueId;
         }
         OSS_INLINE SPACE_ID getSpaceId()const
         {
            return _sid;
         }

      private:
         UINT32 _lid = DMS_INVALID_LOGICCSID;
         utilCSUniqueID _uniqueId = UTIL_INVALID_CS_UNIQUE_ID;
         SPACE_ID _sid = INVALID_SPACE_ID;
   };//class collectionSpaceIdentifier
} // namespace vessel

} // namespace engine


#endif//VESSEL_COLLECTION_SPACE_IDENTIFIER_H_
