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

   Source File Name = indexObject.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_INDEX_OBJECT_H_
#define VESSEL_INDEX_OBJECT_H_

#include "vessel/indexProperties.h"
#include "dpsDef.hpp"

#include <memory>

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
         OSS_INLINE const orderingWrapper getOrderingWrapper()const {return _properties.getPattern().getOrdering();}
         OSS_INLINE DPS_LSN_OFFSET getRebornLSN()const {return _rebornLSN;}
         OSS_INLINE void resetRebornLSN(const DPS_LSN_OFFSET &lsn) {_rebornLSN = lsn;}
         OSS_INLINE PAGE_ID getBtreeEntryAddr()const {return _btreeEntryAddr;}
         OSS_INLINE BOOLEAN hasBtreeEntryAddr()const {return INVALID_PAGE_ID != _btreeEntryAddr;}
         OSS_INLINE void resetBtreeEntryAddr(PAGE_ID entry) {_btreeEntryAddr = entry;}

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
         PAGE_ID _btreeEntryAddr = INVALID_PAGE_ID;
   };//class indexObject
} // namespace vessel

} // namespace engine


#endif//VESSEL_INDEX_OBJECT_H_