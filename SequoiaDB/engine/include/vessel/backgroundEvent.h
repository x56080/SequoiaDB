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

   Source File Name = backgroundEvent.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_BACKGROUND_EVENT_H_
#define VESSEL_BACKGROUND_EVENT_H_

#include "vessel/autoEventList.hpp"
#include "ossEvent.hpp"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
   typedef INT16 BG_EVENT_TYPE_WORD;
   struct BACKGROUND_EVENT_TYPE
   {
      static constexpr BG_EVENT_TYPE_WORD INVALID = 0;
      static constexpr BG_EVENT_TYPE_WORD QUIT = 1;
      static constexpr BG_EVENT_TYPE_WORD _USR_DEFINE = 1024;

      static constexpr BG_EVENT_TYPE_WORD DATA_BUF_TASK = 1024;
      static constexpr BG_EVENT_TYPE_WORD FLUSH_LITE_BUF_POOL = 1025;
      static constexpr BG_EVENT_TYPE_WORD LOB_BUF_TASK = 1026;
      static constexpr BG_EVENT_TYPE_WORD FLUSH_LOB_BUF = 1027;
      
      static constexpr BG_EVENT_TYPE_WORD HIT_ENTRY_TRANSFER = 1028;
   };//struct BACKGROUND_TYPE

   typedef UINT16 BG_EVENT_FLAG_WORD;
   struct BACKGROUND_EVENT_FLAG
   {
      static constexpr BG_EVENT_FLAG_WORD PTR_DATA = 0x01;
   };//struct BACKGROUND_FLAG

#pragma pack(4)
   class backgroundEvent : public SDBObject
   {
      public:
         backgroundEvent(){}
         ~backgroundEvent(){}

         backgroundEvent(const backgroundEvent &o):
         _type(o._type),
         _flags(o._flags),
         _rc(o._rc),
         _responser(o._responser)
         {
            for (UINT32 i = 0; i < _DATA_WORD_COUNT; ++i)
            {
               _data[i] = o._data[i];
            }
         }

         backgroundEvent &operator=(const backgroundEvent &o)
         {
            _type = o._type;
            _flags = o._flags;
            for (UINT32 i = 0; i < _DATA_WORD_COUNT; ++i)
            {
               _data[i] = o._data[i];
            }
            _rc = o._rc;
            _responser = o._responser;
            return *this;
         }

      public:
         static BOOLEAN isRequest(BG_EVENT_TYPE_WORD type)
         {
            return BACKGROUND_EVENT_TYPE::INVALID < type;
         }
         static BOOLEAN isSystemRequest(BG_EVENT_TYPE_WORD type)
         {
            return BACKGROUND_EVENT_TYPE::INVALID < type &&
                   type < BACKGROUND_EVENT_TYPE::_USR_DEFINE;
         }
         static BOOLEAN isUserRequest(BG_EVENT_TYPE_WORD type)
         {
            return BACKGROUND_EVENT_TYPE::_USR_DEFINE <= type;
         }
         static BOOLEAN isResponse(BG_EVENT_TYPE_WORD type)
         {
            return type < BACKGROUND_EVENT_TYPE::INVALID;
         }
         static BG_EVENT_TYPE_WORD makeResponse(BG_EVENT_TYPE_WORD type)
         {
            SDB_ASSERT(isRequest(type), "must be request");
            return 0 - type;
         }
         static BG_EVENT_TYPE_WORD getRequest(BG_EVENT_TYPE_WORD type)
         {
            SDB_ASSERT(isResponse(type), "must be request");
            return 0 - type;
         }

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return BACKGROUND_EVENT_TYPE::INVALID != _type;
         }
         OSS_INLINE BOOLEAN isSystemRequest()const
         {
            return isSystemRequest(_type);
         }
         OSS_INLINE BOOLEAN isQuitEvent()const
         {
            return BACKGROUND_EVENT_TYPE::QUIT == _type;
         }
         OSS_INLINE BOOLEAN isRequest()const
         {
            return isRequest(_type);
         }
         OSS_INLINE BOOLEAN isUserRequest()const
         {
            return isUserRequest(_type);
         }
         OSS_INLINE BOOLEAN isResponse()const
         {
            return isResponse(_type);
         }
         OSS_INLINE BOOLEAN isResponseOf(BG_EVENT_TYPE_WORD type)const
         {
            SDB_ASSERT(isRequest(type), "must be request");
            return _type == makeResponse(type);
         }

         void reset()
         {
            _type = BACKGROUND_EVENT_TYPE::INVALID;
            _flags = 0;
            for (UINT32 i = 0; i < _DATA_WORD_COUNT; ++i)
            {
               _data[i] = 0;
            }
            _rc = 0;
            _responser = nullptr;
         }
         
         void initAsRequest(BG_EVENT_TYPE_WORD type)
         {
            reset();
            SDB_ASSERT(isRequest(type), "invalid type");
            _type = type;
         }

         void initAsResponse(BG_EVENT_TYPE_WORD type)
         {
            reset();
            SDB_ASSERT(isRequest(type), "invalid type");
            _type = makeResponse(type);
         }

         void setType(BG_EVENT_TYPE_WORD type)
         {
            _type = type;
         }

         BG_EVENT_TYPE_WORD getType()const
         {
            return _type;
         }

         BOOLEAN isPtrData()const
         {
            return 0 != OSS_BIT_TEST(_flags, BACKGROUND_EVENT_FLAG::PTR_DATA);
         }

         template<class T>
         T &getShortData()
         {
            static_assert(sizeof(T) <= sizeof(_data), "out of size");
            SDB_ASSERT(0 == OSS_BIT_TEST(_flags, BACKGROUND_EVENT_FLAG::PTR_DATA),
                       "it is ptr data");
            return *((T*)(_data));
         }

         template<class T>
         const T &getShortData()const
         {
            static_assert(sizeof(T) <= sizeof(_data), "out of size");
            SDB_ASSERT(0 == OSS_BIT_TEST(_flags, BACKGROUND_EVENT_FLAG::PTR_DATA),
                       "it is ptr data");
            return *((const T*)(_data));
         }

         template<class T>
         void copyShortData(const T *data)
         {
            static_assert(sizeof(T) <= sizeof(_data), "out of size");
            SDB_ASSERT(nullptr != data, "can not be invalid");
            ossMemcpy(_data, data, sizeof(T));
            OSS_BIT_CLEAR(_flags, BACKGROUND_EVENT_FLAG::PTR_DATA);
         }

         void setShortData(UINT64 data)
         {
            _data[0] = data;
            OSS_BIT_CLEAR(_flags, BACKGROUND_EVENT_FLAG::PTR_DATA);
         }

         UINT64 getData()const
         {
            return _data[0];
         }

         template <class T>
         T *getPtrData()
         {
            SDB_ASSERT(0 != OSS_BIT_TEST(_flags, BACKGROUND_EVENT_FLAG::PTR_DATA),
                       "it is not ptr data");
            return (T*)(_data[0]);
         }

         void setPtrData(void *ptr)
         {
            SDB_ASSERT(isValid(), "can not be invalid");
            SDB_ASSERT(nullptr != ptr, "can not be invalid");
            _data[0] = (UINT64)ptr;
            OSS_BIT_SET(_flags, BACKGROUND_EVENT_FLAG::PTR_DATA);
         }

         backgroundEvent createSimpleResponse(INT32 rc=SDB_OK)const
         {
            SDB_ASSERT(isRequest(), "must be request");
            backgroundEvent response;
            response._type = 0 - _type;
            if (isPtrData())
            {
               OSS_BIT_SET(response._flags, BACKGROUND_EVENT_FLAG::PTR_DATA);
            }
            for (UINT32 i = 0; i < _DATA_WORD_COUNT; ++i)
            {
               response._data[i] = _data[i];
            }
            response._rc = rc;
            return response;
         }

         INT32 getRC()const
         {
            return _rc;
         }

         void setRC(INT32 rc)
         {
            _rc = rc;
         }

         BOOLEAN hasResponser()const
         {
            return nullptr != _responser;
         }

         void setResponser(autoEventList<backgroundEvent> *responser)
         {
            _responser = responser;
         }

         autoEventList<backgroundEvent> *getResponser() const
         {
            return _responser;
         }
      public:
         static backgroundEvent createQuitEvent()
         {
            backgroundEvent e;
            e._type = BACKGROUND_EVENT_TYPE::QUIT;
            return e;
         }

      private:
         static constexpr UINT32 _DATA_WORD_COUNT = 2;

      private:
         BG_EVENT_TYPE_WORD _type = BACKGROUND_EVENT_TYPE::INVALID;
         BG_EVENT_FLAG_WORD _flags = 0;
         UINT64 _data[_DATA_WORD_COUNT] = {};         
         INT32 _rc = 0;
         mutable autoEventList<backgroundEvent> *_responser = nullptr; 
   };//class backgroundEvent
#pragma pack()
}//namespace vessel
}//namespace engine

#endif//VESSEL_BACKGROUND_EVENT_H_