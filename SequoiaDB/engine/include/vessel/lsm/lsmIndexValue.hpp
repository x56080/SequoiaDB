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
#include "vessel/lsm/lsmIdxKey.hpp"

namespace engine
{
namespace vessel
{
#pragma pack(4)
   class lsmIndexValue : public SDBObject
   {
      public:
         lsmIndexValue(){}
         ~lsmIndexValue(){}

         lsmIndexValue(const lsmIndexValue &o):
         _type(o._type),
         _flags(o._flags),
         _rbsOffsetCL(o._rbsOffsetCL),
         _rbsOffsetLid(o._rbsOffsetLid){}

         lsmIndexValue &operator=(const lsmIndexValue &o)
         {
            _type = o._type;
            _flags = o._flags;
            _rbsOffsetCL = o._rbsOffsetCL;
            _rbsOffsetLid = o._rbsOffsetLid;
            return *this;
         }

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return  LSM_VALUE_TYPE_INVALID != _type &&
                    (LSM_VALUE_TYPE_INSERT == _type ||
                     LSM_VALUE_TYPE_DELETE == _type ||
                     LSM_VALUE_TYPE_OLD_VER_INSERT == _type);
         }
         OSS_INLINE BOOLEAN isDeleted()const
         {
            SDB_ASSERT(isValid(), "must be valid");
            return LSM_VALUE_TYPE_DELETE == _type;
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
         OSS_INLINE void reset(UINT8 type = LSM_VALUE_TYPE_INVALID,
                               UINT8 flags = 0,
                               UINT16 rbsOffsetCL = DMS_INVALID_CLID,
                               INT16 rbsOffsetLid = -1)
         {
            _type = type;
            _flags = flags;
            _rbsOffsetCL = rbsOffsetCL;
            _rbsOffsetLid = rbsOffsetLid;
            return;
         }

      private:
         UINT8 _type = LSM_VALUE_TYPE_INVALID;
         UINT8 _flags = 0;
         UINT16 _rbsOffsetCL = DMS_INVALID_CLID;
         INT64 _rbsOffsetLid = -1;
   };//class lsmIndexValue

   constexpr UINT32 LSM_VALUE_SIZE = sizeof(lsmIndexValue);
#pragma pack()
}//namespace vessel
}//namespace engine

#endif//LSM_INDEX_VALUE_HPP_