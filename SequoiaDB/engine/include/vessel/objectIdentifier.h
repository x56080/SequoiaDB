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

   Source File Name = objectIdentifier.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_OBJECT_IDENTIFIER_H_
#define VESSEL_OBJECT_IDENTIFIER_H_

#include "vessel/vesselIdDef.h"
#include "dms.hpp"
#include "utilUniqueID.hpp"
#include "vessel/indexDef.h"

namespace engine
{
namespace vessel
{
#pragma pack(4)
   class collectionSpaceId : public SDBObject
   {
      public:
         collectionSpaceId() = default;
         ~collectionSpaceId() = default;
         explicit collectionSpaceId(UINT32 lid, UINT32 uniqueId, SPACE_ID sid):
         _lid(lid), _uniqueId(uniqueId), _sid(sid), _pad(0){}
         OSS_INLINE BOOLEAN operator==(const collectionSpaceId &o)const
         {
            return _lid == o._lid &&
                   _uniqueId == o._uniqueId &&
                   _sid == o._sid;
         }
         OSS_INLINE BOOLEAN operator!=(const collectionSpaceId &o)const
         {
            return !(o == *this);
         }

      public:
         OSS_INLINE UINT32 getLid()const {return _lid;}
         OSS_INLINE UINT32 getUniqueId()const {return _uniqueId;}
         OSS_INLINE UINT16 getSpaceId()const {return _sid;}
         OSS_INLINE BOOLEAN isValid()const
         {
            return DMS_INVALID_LOGICCSID != _lid &&
                   INVALID_SPACE_ID != _sid &&
                   0 == _pad;   
         }
         OSS_INLINE void reset()
         {
            _lid = DMS_INVALID_LOGICCSID;
            _uniqueId = UTIL_UNIQUEID_NULL;
            _sid = INVALID_SPACE_ID;
            _pad = 0;
         }
      private:
         UINT32 _lid = DMS_INVALID_LOGICCSID;
         UINT32 _uniqueId = UTIL_UNIQUEID_NULL;
         UINT16 _sid = INVALID_SPACE_ID;
         UINT16 _pad = 0;
   };//class collectionSpaceId

   class collectionId : public SDBObject
   {
      public:
         collectionId() = default;
         ~collectionId() = default;
         explicit collectionId(UINT32 lid, UINT32 innerId, CL_MB_ID mbId):
         _lid(lid), _innerId(innerId), _mbId(mbId), _pad(0){}
         OSS_INLINE BOOLEAN operator==(const collectionId &o)const
         {
            return _lid == o._lid &&
                   _innerId == o._innerId &&
                   _mbId == o._mbId;
         }

      public:
         OSS_INLINE UINT32 getLid()const {return _lid;}
         OSS_INLINE UINT32 getInnerId()const {return _innerId;}
         OSS_INLINE UINT16 getMbId()const {return _mbId;}
         OSS_INLINE BOOLEAN isValid()const
         {
            return DMS_INVALID_LOGICCLID !=_lid &&
                   INVALID_CL_MB_ID != _mbId &&
                   0 == _pad;   
         }
      private:
         UINT32 _lid = DMS_INVALID_LOGICCLID;
         UINT32 _innerId = UTIL_UNIQUEID_NULL;
         UINT16 _mbId = INVALID_CL_MB_ID;
         UINT16 _pad = 0;
   };//class collectionSpaceId

   class globalCollectionId : public SDBObject
   {
      public:
         OSS_INLINE UINT16 getSpaceId()const {return _sid;}
         OSS_INLINE UINT16 getMbId()const {return _mbId;}
         OSS_INLINE UINT32 getCSLid()const {return _csLid;}
         OSS_INLINE UINT32 getCLLid()const {return _clLid;}
         OSS_INLINE const utilCLUniqueID &getUniqueId()const {return _uniqueId;}

         OSS_INLINE BOOLEAN isValid()const
         {
            return DMS_INVALID_LOGICCSID != _csLid &&
                   DMS_INVALID_LOGICCLID != _clLid &&
                   INVALID_SPACE_ID != _sid &&
                   INVALID_CL_MB_ID != _mbId;
         }

         OSS_INLINE BOOLEAN operator==(const globalCollectionId &o)const
         {
            return _csLid == o._csLid &&
                   _clLid == o._clLid &&
                   _uniqueId == o._uniqueId &&
                   _sid == o._sid &&
                   _mbId == o._mbId;
         }

         OSS_INLINE void reset(UINT32 csLid, UINT32 clLid,
                               const utilCLUniqueID &uniqueId,
                               UINT16 sid, UINT16 mbId)
         {
            _csLid = csLid;
            _clLid = clLid;
            _uniqueId = uniqueId;
            _sid = sid;
            _mbId = mbId;
            return;
         }

         OSS_INLINE void reset(const collectionSpaceId &cs,
                               const collectionId &cl)
         {
            _csLid = cs.getLid();
            _clLid = cl.getLid();
            _uniqueId = utilBuildCLUniqueID(cs.getUniqueId(), cl.getInnerId());
            _sid = cs.getSpaceId();
            _mbId = cl.getMbId();
            return;
         }

         OSS_INLINE void reset()
         {
            _csLid = DMS_INVALID_LOGICCSID;
            _clLid = DMS_INVALID_LOGICCLID;
            _uniqueId = UTIL_UNIQUEID_NULL;
            _sid = INVALID_SPACE_ID;
            _mbId = INVALID_CL_MB_ID;
            return;
         }

         OSS_INLINE collectionSpaceId getCSIdentifier()const
         {
            return collectionSpaceId(_csLid, utilGetCSUniqueID(_uniqueId), _sid);
         }
         OSS_INLINE collectionId getCLIdentifier()const
         {
            return collectionId(_clLid, utilGetCLInnerID(_uniqueId), _mbId);
         }

      private:
         UINT32 _csLid = DMS_INVALID_LOGICCSID;
         UINT32 _clLid = DMS_INVALID_LOGICCLID;
         utilCLUniqueID _uniqueId = UTIL_UNIQUEID_NULL;
         UINT16 _sid = INVALID_SPACE_ID;
         UINT16 _mbId = INVALID_CL_MB_ID;
   };//class globalCollectionId

   class indexIdentifier : public SDBObject
   {
      public:
         indexIdentifier() = default;
         explicit indexIdentifier(INT32 slot, UINT32 lid, utilIdxInnerID innerID):
         _indexSlot(slot), _indexLid(lid), _indexInnerID(innerID){}
         ~indexIdentifier() = default;

         BOOLEAN operator==(const indexIdentifier &o)const
         {
            return _indexSlot == o._indexSlot &&
                   _indexLid == o._indexLid;
         }

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return isValidIndexSlot(_indexSlot) &&
                   INVALID_LOGICAL_INDEX_ID != _indexLid;
         }
         OSS_INLINE INT32 getIndexSlot()const {return _indexSlot;}
         OSS_INLINE UINT32 getLogicalIndexId()const {return _indexLid;}
         OSS_INLINE void reset(INT32 slot=-1,
                               UINT32 lid=INVALID_LOGICAL_INDEX_ID,
                               utilIdxInnerID innerID=UTIL_UNIQUEID_NULL)
         {
            _indexSlot = slot;
            _indexLid = lid;
            _indexInnerID = innerID;
            return;
         }

      private:
         INT32 _indexSlot = -1;
         UINT32 _indexLid = INVALID_LOGICAL_INDEX_ID;
         utilIdxInnerID _indexInnerID = UTIL_UNIQUEID_NULL;
   };//class indexIdentifier

#pragma pack()
} // namespace vessel

} // namespace engine


#endif//VESSEL_OBJECT_IDENTIFIER_H_
