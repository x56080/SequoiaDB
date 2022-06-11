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

   Source File Name = lsmIndexValue.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          02/12/2021  WY  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef LSM_INDEX_VALUE_HPP_
#define LSM_INDEX_VALUE_HPP_

#include "dms.hpp"
#include "vessel/lsm/lsmIndexKey.h"
#include "dpsTransID.hpp"

namespace engine
{
namespace vessel
{
   constexpr UINT8 LSM_IDX_VALUE_TYPE_INVALID = 0xFF;
   constexpr UINT8 LSM_IDX_VALUE_TYPE_INSERT = 0x0;
   constexpr UINT8 LSM_IDX_VALUE_TYPE_DELETE = 0x01;
   constexpr UINT8 LSM_IDX_VALUE_TYPE_OLD_VER_INSERT = 0x02;

#pragma pack(4)
   class lsmIndexValue : public SDBObject
   {
      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return LSM_IDX_VALUE_TYPE_INVALID != _type;
         }
         OSS_INLINE BOOLEAN isDeleted()const
         {
            return LSM_IDX_VALUE_TYPE_DELETE == _type;
         }
         OSS_INLINE UINT8 getFlags()const
         {
            return _flags;
         }
         OSS_INLINE UINT16 getRbsOffsetCL()const
         {
            return _rbsOffsetCL;
         }
         OSS_INLINE INT16 getRbsOffsetLid()const
         {
            return _rbsOffsetLid;
         }
         OSS_INLINE rocksdb::Slice getSlice()const
         {
            return rocksdb::Slice((const CHAR *)this, sizeof(lsmIndexValue));
         }
         OSS_INLINE const DPS_TRANS_ID &getTransID()const {return _transID;}

         OSS_INLINE void reset(UINT8 type = LSM_IDX_VALUE_TYPE_INVALID,
                               UINT8 flags = 0,
                               UINT16 rbsOffsetCL = DMS_INVALID_CLID,
                               INT16 rbsOffsetLid = -1,
                               const DPS_TRANS_ID &transID=DPS_TRANS_ID())
         {
            _version = 0;
            _type = type;
            _flags = flags;
            _rbsOffsetCL = rbsOffsetCL;
            _rbsOffsetLid = rbsOffsetLid;
            _transID = transID;
            _flags = 0;
            return;
         }

      private:
         UINT8 _version = 0;
         UINT8 _type = LSM_IDX_VALUE_TYPE_INVALID;
         UINT16 _rbsOffsetCL = DMS_INVALID_CLID;
         INT64 _rbsOffsetLid = -1;
         DPS_TRANS_ID _transID;
         UINT16 _flags = 0;
   };//class lsmIndexValue

   constexpr UINT32 LSM_VALUE_SIZE = sizeof(lsmIndexValue);
   static_assert(24 == LSM_VALUE_SIZE, "invalid size");
#pragma pack()
}//namespace vessel
}//namespace engine

#endif//LSM_INDEX_VALUE_HPP_