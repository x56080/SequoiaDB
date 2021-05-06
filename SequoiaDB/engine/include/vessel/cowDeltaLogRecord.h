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

   Source File Name = cowDeltaLogRecord.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_COW_DELTA_LOG_RECORD_H_
#define VESSEL_COW_DELTA_LOG_RECORD_H_

#include "vessel/pageDef.h"
#include "vessel/vesselFileDef.h"

namespace engine
{
namespace vessel
{
   static const UINT16 DELTA_LOG_RECORD_VERSION = 1;

   static const UINT16 INVALID_DELTA_LOG_RECORD_TYPE = 65535;
   static const UINT16 DELTA_LOG_TYPE_IDX_M_SMP_ALLOCATE = 0;
   static const UINT16 DELTA_LOG_RECORD_TYPE_RELEASE = 1;
   
#pragma pack(4)
   class cowDeltaLogRecord
   {
      public:
         OSS_INLINE cowDeltaLogRecord(){}
         OSS_INLINE cowDeltaLogRecord(const cowDeltaLogRecord &o):
         _version(o._version),
         _type(o._type),
         _pid(o._pid),
         _pad0(o._pad0),
         _pad1(o._pad1){}

         OSS_INLINE ~cowDeltaLogRecord(){}
         OSS_INLINE cowDeltaLogRecord &operator=(const cowDeltaLogRecord &o)
         {
            _version = o._version;
            _type = o._type;
            _pid = o._pid;
            _pad0 = o._pad0;
            _pad1 = o._pad1;
            return *this;
         }

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return DELTA_LOG_RECORD_VERSION == _version &&
                   INVALID_DELTA_LOG_RECORD_TYPE != _type &&
                   INVALID_PAGE_ID != _pid;
         }
         OSS_INLINE void reset()
         {
            _version = 0;
            _type = INVALID_DELTA_LOG_RECORD_TYPE;
            _pid = INVALID_PAGE_ID;
            _pad0 = 0;
            _pad1 = 0;
            return;
         }

         OSS_INLINE UINT16 getVersion()const
         {
            return _version;
         }
         OSS_INLINE UINT16 getType()const
         {
            return _type;
         }
         OSS_INLINE UINT32 getPid()const
         {
            return _pid;
         }
         
      public:
         BOOLEAN setAsIdxMSmpAllocate(PAGE_ID pid, UINT32 count);
         BOOLEAN getAsIdxMSmpAllocate(PAGE_ID *pid, UINT32 *count)const;

      private:
         UINT16 _version = 0;
         UINT16 _type = INVALID_DELTA_LOG_RECORD_TYPE;
         UINT32 _pid = INVALID_PAGE_ID;
         UINT32 _pad0 = 0;
         UINT32 _pad1 = 0;
   };//struct cowDeltaLogRecord

   static const UINT32 DELTA_LOG_RECORD_SIZE = 16;
#pragma pack()
}//namespace vessel
}//namespace engine

#endif//VESSEL_COW_DELTA_LOG_RECORD_H_