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

#include "vessel/vesselIdDef.h"
#include "vessel/vesselFileDef.h"
#include "vessel/autoEventList.hpp"
#include "ossEvent.hpp"

namespace engine
{
namespace vessel
{
   static const UINT32 BG_EVENT_MSG_BUFFER_SIZE = 16;

   class backgroundEvent : public SDBObject
   {
      public:
         backgroundEvent(){}
         ~backgroundEvent(){}
         backgroundEvent(const backgroundEvent &o):
         _type(o._type),
         _responseEvent(o._responseEvent),
         _responseList(o._responseList)
         {
            ossMemcpy(_msg, o._msg, BG_EVENT_MSG_BUFFER_SIZE);
         }
         backgroundEvent &operator=(const backgroundEvent &o)
         {
            _type = o._type;
            ossMemcpy(_msg, o._msg, BG_EVENT_MSG_BUFFER_SIZE);
            _responseEvent = o._responseEvent;
            _responseList = o._responseList;
            return *this;
         }

      public:
         enum EVENT_TYPE
         {
            EVENT_TYPE_INVALID = 0,
            EVENT_TYPE_QUIT = 1,
            EVENT_TYPE_FINISHED = 2,
            EVENT_TYPE_CACHE_TASK = 3,
            EVENT_TYPE_SYNC_SEG = 4,
            EVENT_TYPE_LPS_CHECKPOINT = 5,
            EVENT_TYPE_CACHE_WATCHER_NOTIFY = 6,
         };//enum EVENT_TYPE

      public:
         void release()
         {
            _type = EVENT_TYPE_INVALID;
            ossMemset(_msg, 0, BG_EVENT_MSG_BUFFER_SIZE);
            _responseEvent = NULL;
            _responseList = NULL;
         }

         OSS_INLINE BOOLEAN isQuitEvent()const
         {
            return EVENT_TYPE_QUIT == _type;
         }

         OSS_INLINE void setType(EVENT_TYPE type)
         {
            _type = type;
         }
         OSS_INLINE EVENT_TYPE getType()const
         {
            return _type;
         }
         OSS_INLINE void setEventMsg(UINT32 size, const void *msg)
         {
            SDB_ASSERT(size <= BG_EVENT_MSG_BUFFER_SIZE, "out of bound");
            SDB_ASSERT(NULL != msg, "can not be null");
            ossMemcpy(_msg, msg, size);
         }
         OSS_INLINE const CHAR *getEventMsg()const
         {
            return _msg;
         }
         OSS_INLINE void setResponseList(autoEventList<backgroundEvent> *list)
         {
            _responseList = list;
         }
         OSS_INLINE autoEventList<backgroundEvent> *getReponseList()
         {
            return _responseList;
         }
         OSS_INLINE BOOLEAN hasResponseList()const
         {
            return NULL != _responseList;
         }
         OSS_INLINE BOOLEAN hasResponseEvent()const
         {
            return NULL != _responseEvent;
         }
         OSS_INLINE ossEvent *getResponseEvent()
         {
            return _responseEvent;
         }
         OSS_INLINE void resetResponseEvent(ossEvent *e)
         {
            if (NULL != e)
            {
               e->reset();
            }
            _responseEvent = e;
            return;
         }

      private:
         EVENT_TYPE _type = EVENT_TYPE_INVALID;
         CHAR _msg[BG_EVENT_MSG_BUFFER_SIZE] = {};
         ossEvent *_responseEvent = NULL;
         autoEventList<backgroundEvent> *_responseList = NULL;
   };//class backgroundEvent
}//namespace vessel
}//namespace engine

#endif//VESSEL_BACKGROUND_EVENT_H_