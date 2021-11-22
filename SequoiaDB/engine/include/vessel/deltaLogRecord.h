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

   Source File Name = deltaLogRecord.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_DELTA_LOG_RECORD_H_
#define VESSEL_DELTA_LOG_RECORD_H_

#include "vessel/pageDef.h"
#include "vessel/vesselFileDef.h"

namespace engine
{
namespace vessel
{
   constexpr UINT8 DELTA_LOG_RECORD_VERSION = 1;

   typedef UINT8 DELTA_LOG_RECORD_TYPE;
   constexpr DELTA_LOG_RECORD_TYPE  INVALID_DELTA_LOG_RECORD_TYPE = 0;

   constexpr DELTA_LOG_RECORD_TYPE DELTA_LOG_TYPE_DUMMY = 1;
   constexpr DELTA_LOG_RECORD_TYPE DELTA_LOG_TYPE_CHECKPOINT = 2;

   constexpr DELTA_LOG_RECORD_TYPE DELTA_LOG_TYPE_MAPPING = 10;
   constexpr DELTA_LOG_RECORD_TYPE DELTA_LOG_TYPE_REMAPPING = 11;
   constexpr DELTA_LOG_RECORD_TYPE DELTA_LOG_TYPE_UNMAPPING = 12;
   //constexpr DELTA_LOG_RECORD_TYPE DELTA_LOG_TYPE_RELEASING = 13;

   OSS_INLINE BOOLEAN isOperationalDeltaLogRecord(DELTA_LOG_RECORD_TYPE type)
   {
      return type >= DELTA_LOG_TYPE_MAPPING;
   }

   constexpr UINT32 MAX_DELTA_LOG_RECORD_SIZE = 512;

   ///DELTA_LOG_TYPE_REMAPPING flags
   constexpr UINT8 DELTA_LOG_TYPE_REMAPPING_FLAG_RELEASE_PID = 0x01;

   ///DELTA_LOG_TYPE_UNMAPPING flags
   constexpr UINT8 DELTA_LOG_TYPE_UNMAPPING_FLAG_RELEASE_PID = 0x01;

#pragma pack(4)
   class deltaLogRecordHead
   {
      public:
         deltaLogRecordHead(){}
         ~deltaLogRecordHead(){}

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return DELTA_LOG_RECORD_VERSION == _version &&
                   INVALID_DELTA_LOG_RECORD_TYPE != _type &&
                   sizeof(deltaLogRecordHead) <= _size &&
                   _size <= MAX_DELTA_LOG_RECORD_SIZE;
         }
      public:
         UINT8 _version = 0;
         UINT8 _type = 0;
         UINT16 _size = 0;
   };//class deltaLogRecordHead
#pragma pack()

   constexpr UINT32 DELTA_LOG_RECORD_HEAD_SIZE = sizeof(deltaLogRecordHead);

   /// in-mem object
   class deltaLogRecord : public SDBObject
   {
      public:
         deltaLogRecord();
         ~deltaLogRecord();
         deltaLogRecord(const deltaLogRecord &o):
         _head(o._head){}
         deltaLogRecord &operator=(const deltaLogRecord &o)
         {
            _head = o._head;
            return *this;
         }

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return NULL != _head &&
                   _head->isValid();
         }
         OSS_INLINE const deltaLogRecordHead *getLogHead()const
         {
            return _head;
         }

         OSS_INLINE UINT32 getLogSize()const
         {
            return isValid() ? _head->_size : 0;
         }

         OSS_INLINE DELTA_LOG_RECORD_TYPE getLogType()const
         {
            return isValid() ?  _head->_type :
                                INVALID_DELTA_LOG_RECORD_TYPE;
         }
         
         void reset(const void *ptr = NULL);

         ///offset 0 means skip record head and read.
         const void *getRecordBodyPtr(UINT32 offset, UINT32 len)const;

         ///fixed length object only.
         template <typename T>
         const T *getRecordBodyPtr(UINT32 offset)const
         {
            return (const T*)(getRecordBodyPtr(offset, sizeof(T)));
         }

      private:
         const deltaLogRecordHead *_head = NULL;
   };//class deltaLogRecord
}//namespace vessel
}//namespace engine

#endif//VESSEL_DELTA_LOG_RECORD_H_