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

   Source File Name = indexObject.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_INDEX_OBJECT_H_
#define VESSEL_INDEX_OBJECT_H_

#include "vessel/indexProperties.h"
#include "dpsDef.hpp"
#include "vessel/atomicBtreeEntryAddr.h"

#include <memory>
#include <atomic>

namespace engine
{
namespace vessel
{
   class indexObject : public SDBObject
   {
      public:
         indexObject() = default;
         ~indexObject() = default;
         indexObject(const indexObject &) = delete;
         indexObject &operator=(const indexObject &) = delete;

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return INVALID_LOGICAL_INDEX_ID != _logicalID;
         }
         OSS_INLINE BOOLEAN isNormal()const
         {
            return INDEX_STATUS_NORMAL == _status;
         }
         OSS_INLINE BOOLEAN isBuilding()const
         {
            return INDEX_STATUS_BUILDING == _status;
         }
         OSS_INLINE BOOLEAN isTruncating()const
         {
            return INDEX_STATUS_TRUNCATING == _status;
         }
         OSS_INLINE BOOLEAN isRemoving()const
         {
            return INDEX_STATUS_REMOVING == _status;
         }
         OSS_INLINE INDEX_STATUS getStatus()const
         {
            return _status;
         }
         OSS_INLINE void setStatus(INDEX_STATUS status) {_status = status;}
         OSS_INLINE UINT32 getLogicalID()const {return _logicalID;}
         OSS_INLINE const indexProperties &getProperties()const {return _properties;}
         OSS_INLINE const orderingWrapper getOrderingWrapper()const
         {
            return _properties.getPattern().getOrdering();
         }
         OSS_INLINE DPS_LSN_OFFSET getRebornLSN()const {return _rebornLSN;}
         OSS_INLINE void resetRebornLSN(DPS_LSN_OFFSET lsn) {_rebornLSN = lsn;}
         OSS_INLINE BOOLEAN isWritable()const
         {
            return isValid() && (isNormal() || isBuilding());
         }

         OSS_INLINE btreeEntryAddr getBtreeEntryAddr()const
         {
            return _entryAddr.get();
         }
         OSS_INLINE void resetBtreeEntryAddr()
         {
            _entryAddr.reset();
         }
         OSS_INLINE void setBtreeEntryAddr(PAGE_ID pid, UINT32 psn)
         {
            _entryAddr.set(pid, psn);
         }

      public:
         INT32 init(UINT32 indexLid,
                    const indexProperties &properties,
                    INDEX_STATUS status);

         INT32 initFromBson(const bson::BSONObj &obj);

         void reset();

         bson::BSONObj toBson()const;

         BOOLEAN associates(const CHAR *fieldName)const;

      private:
         UINT32 _logicalID = INVALID_LOGICAL_INDEX_ID;
         indexProperties _properties;
         DPS_LSN_OFFSET _rebornLSN = DPS_INVALID_LSN_OFFSET;
         INDEX_STATUS _status = INDEX_STATUS_INVALID;
         atomicBtreeEntryAddr _entryAddr;
   };//class indexObject
} // namespace vessel

} // namespace engine


#endif//VESSEL_INDEX_OBJECT_H_