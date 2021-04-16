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

#include "vessel/vesselIdDef.h"
#include "dms.hpp"

namespace engine
{
namespace vessel
{
   class collectionHandle : public SDBObject
   {
      public:
         OSS_INLINE collectionHandle():
                    _csLogicalID(DMS_INVALID_LOGICCSID),
                    _clLogicalID(DMS_INVALID_LOGICCLID),
                    _sid(INVALID_SPACE_ID),
                    _mbid(INVALID_CL_MB_ID){}

         OSS_INLINE collectionHandle(UINT32 cslid, UINT32 cllid,
                                      SPACE_ID sid, CL_MB_ID mbid):
                    _csLogicalID(cslid),
                    _clLogicalID(cllid),
                    _sid(sid),
                    _mbid(mbid){}

         OSS_INLINE collectionHandle(const collectionHandle &o):
                    _csLogicalID(o._csLogicalID),
                    _clLogicalID(o._clLogicalID),
                    _sid(o._sid),
                    _mbid(o._mbid){}

         OSS_INLINE~collectionHandle()
         {}

         collectionHandle &operator=(const collectionHandle &o)
         {
            _csLogicalID = o._csLogicalID;
            _clLogicalID = o._clLogicalID;
            _sid = o._sid;
            _mbid = o._mbid;
            return *this;
         }

      public:
         OSS_INLINE UINT32 getCSLId()const
         {
            return _csLogicalID;
         }
         OSS_INLINE UINT32 getCLLId()const
         {
            return _clLogicalID;
         }
         OSS_INLINE SPACE_ID getSpaceID()const
         {
            return _sid;
         }
         OSS_INLINE CL_MB_ID getMbId()const
         {
            return _mbid;
         }
         OSS_INLINE BOOLEAN isValid()const
         {
            return DMS_INVALID_LOGICCSID != _csLogicalID &&
                   DMS_INVALID_LOGICCLID != _clLogicalID &&
                   INVALID_SPACE_ID != _sid &&
                   INVALID_CL_MB_ID != _mbid;
         }

         OSS_INLINE void reset()
         {
            _csLogicalID = DMS_INVALID_LOGICCSID;
            _clLogicalID = DMS_INVALID_LOGICCLID;
            _sid = INVALID_SPACE_ID;
            _mbid = INVALID_CL_MB_ID;
            return;
         }
          
      private:
         UINT32 _csLogicalID;
         UINT32 _clLogicalID;
         SPACE_ID _sid;
         CL_MB_ID _mbid;
   };//class collectionHandle
}//namespace vessel
}//namespace engine

#endif//VESSEL_COLLECTION_HANDLE_H_