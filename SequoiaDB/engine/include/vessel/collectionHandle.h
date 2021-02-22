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

   Source File Name = collectionHandle.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_COLLECTION_HANDLE_H_
#define VESSEL_COLLECTION_HANDLE_H_

#include "vessel/vesselDef.h"

namespace engine
{
namespace vessel
{
   class collectionHandle : public SDBObject
   {
      public:
         OSS_INLINE collectionHandle():
         _cslid(DMS_INVALID_LOGICCSID),
         _cllid(DMS_INVALID_LOGICCLID),
         _sid(INVALID_SPACE_ID),
         _mbid(INVALID_CL_MB_ID)
         {}

         OSS_INLINE ~collectionHandle()
         {}

      public:
         OSS_INLINE BOOLEAN valid()const
         {
            return DMS_INVALID_LOGICCSID != _cslid;
         }

         OSS_INLINE UINT32 getLogicalCSId()const
         {
            return _cslid;
         }

         OSS_INLINE UINT32 getLogicalCLId()const
         {
            return _cllid;
         }

         OSS_INLINE SPACE_ID getSpaceID()const
         {
            return _sid;
         }

         OSS_INLINE CL_MB_ID getMBID()const
         {
            return _mbid;
         }

         OSS_INLINE void setLogicalID(UINT32 csid,
                                      UINT32 clid)
         {
            if (DMS_INVALID_LOGICCSID != csid &&
                DMS_INVALID_LOGICCLID != clid)
            {
               _cslid = csid;
               _cllid = clid;
            }
            return;
         }

         OSS_INLINE void setSlotID(SPACE_ID sid, CL_MB_ID mbid)
         {
            if (INVALID_SPACE_ID != sid &&
                INVALID_CL_MB_ID != mbid)
            {
               _sid = sid;
               _mbid = mbid;
            }
            return;
         }

         OSS_INLINE void reset()
         {
            _cslid = DMS_INVALID_LOGICCSID;
            _cllid = DMS_INVALID_LOGICCLID;
            _sid = INVALID_SPACE_ID;
            _mbid = INVALID_CL_MB_ID;
            return;
         }
      private:
         UINT32 _cslid;
         UINT32 _cllid;
         SPACE_ID _sid;
         CL_MB_ID _mbid;
   };//class collectionHandle
}//namespace vessel
}//namespace engine

#endif//VESSEL_COLLECTION_HANDLE_H_