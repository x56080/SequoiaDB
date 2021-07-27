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

   Source File Name = objectLatchMap.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_OBJECT_LATCH_MAP_H_
#define VESSEL_OBJECT_LATCH_MAP_H_

#include "vessel/sharedObjectMap.hpp"
#include "vessel/vesselIdDef.h"
#include "ossLatch.hpp"
#include "vessel/recordID.h"
#include "vessel/vesselFileDef.h"

namespace engine
{
namespace vessel
{

   class logicalIdLatchKey : public SDBObject
   {
      public:
         logicalIdLatchKey(){}
         logicalIdLatchKey(SPACE_ID sid,
                           SPACE_TYPE type,
                           PAGE_ID lpid):
         _sid(sid),
         _type(type),
         _lpid(lpid){}

         ~logicalIdLatchKey(){}
         logicalIdLatchKey(const logicalIdLatchKey &o):
         _sid(o._sid),
         _type(o._type),
         _lpid(o._lpid){}
         logicalIdLatchKey &operator=(const logicalIdLatchKey &o)
         {
            _sid = o._sid;
            _type = o._type;
            _lpid = o._lpid;
            return *this;
         }
         OSS_INLINE BOOLEAN operator==(const logicalIdLatchKey &o)const
         {
            return _sid == o._sid &&
                   _type == o._type &&
                   _lpid == o._lpid;
         }
         OSS_INLINE UINT32 hash()const
         {
            return _sid + _type + _lpid;
         }

         OSS_INLINE BOOLEAN isValid()const
         {
            return INVALID_SPACE_ID != _sid &&
                   INVALID_SPACE_TYPE != _type &&
                   INVALID_PAGE_ID != _lpid;
         }
      public:
         SPACE_ID _sid = INVALID_SPACE_ID;
         SPACE_TYPE _type = INVALID_SPACE_TYPE;
         PAGE_ID _lpid = INVALID_PAGE_ID;
   };//class logicalIdLatchKey

   typedef class sharedObjectMap<logicalIdLatchKey, ossSharedLatch> LOGICAL_ID_LATCH_MAP;

   class recordIdLatchKey : public SDBObject
   {
      public:
         recordIdLatchKey(){}
         ~recordIdLatchKey(){}
         recordIdLatchKey(const recordIdLatchKey &o):
         _sid(o._sid),
         _slot(o._slot),
         _lpid(o._lpid){}
         recordIdLatchKey &operator=(const recordIdLatchKey &o)
         {
            _sid = o._sid;
            _slot = o._slot;
            _lpid = o._lpid;
            return *this;
         }
         OSS_INLINE BOOLEAN operator==(const recordIdLatchKey &o)const
         {
            return _sid == o._sid &&
                   _slot == o._slot &&
                   _lpid == o._lpid;
         }
         OSS_INLINE UINT32 hash()const
         {
            return _sid + _slot + _lpid;
         }

         OSS_INLINE BOOLEAN isValid()const
         {
            return INVALID_SPACE_ID != _sid &&
                   INVALID_RECORD_SLOT_ID != _slot &&
                   INVALID_PAGE_ID != _lpid;
         }

      public:
         SPACE_ID _sid = INVALID_SPACE_ID;
         RECORD_SLOT_ID _slot = INVALID_RECORD_SLOT_ID;
         PAGE_ID _lpid = INVALID_PAGE_ID;
   };//class recordIdLatchKey

   typedef class sharedObjectMap<recordIdLatchKey, ossSharedLatch> RECORD_ID_LATCH_MAP;

   class uniqueIndexLatchKey : public SDBObject
   {
      public:
         uniqueIndexLatchKey(){}
         ~uniqueIndexLatchKey(){}
         explicit uniqueIndexLatchKey(SPACE_ID sid,
                                      UINT16 hash):
                  _sid(sid),
                  _key(hash){}
         uniqueIndexLatchKey(const uniqueIndexLatchKey &o):
         _sid(o._sid),
         _key(o._key)
         {}
         uniqueIndexLatchKey &operator=(const uniqueIndexLatchKey &o)
         {
            _sid = o._sid;
            _key = o._key;
            return *this;
         }

      public:
         OSS_INLINE BOOLEAN operator==(const uniqueIndexLatchKey &o)const
         {
            return _sid == o._sid && _key == o._key;
         }
         OSS_INLINE UINT32 hash()const
         {
            return _sid + _key;
         }
         OSS_INLINE SPACE_ID getSpaceID()const
         {
            return _sid;
         }
         OSS_INLINE UINT16 getKeyHash()const
         {
            return _key;
         }
      private:
         SPACE_ID _sid = INVALID_SPACE_ID;
         UINT16 _key = 0;
   };//class uniqueIndexLatchKey

   ///WARNING: UNIQUE_INDEX_LATCH_MAP's object is x latch, do not use objectSharedLatchContext.
   typedef class sharedObjectMap<uniqueIndexLatchKey, ossSpinXLatch> UNIQUE_INDEX_LATCH_MAP;

   template <typename KEY>
   class objectSharedLatchContext : public SDBObject
   {
      public:
         objectSharedLatchContext(){}
         ~objectSharedLatchContext()
         {
            fini();
         }
         objectSharedLatchContext(const objectSharedLatchContext &) = delete;
         objectSharedLatchContext &operator=(const objectSharedLatchContext &) = delete;

      private:
         typedef class sharedObjectMap<KEY, ossSharedLatch>::object LATCH_OBJECT;

      private:
         struct _latchSlot : public SDBObject
         {
            _latchSlot(){}
            ~_latchSlot(){}
            _latchSlot(const _latchSlot &) = delete;

            _latchSlot &operator=(const _latchSlot &o)
            {
               obj = o.obj;
               mode = o.mode;
               return *this;
            }

            LATCH_OBJECT obj;
            OSS_SHARED_LATCH_MODE mode = OSS_SHARED_LATCH_MODE_NONE;
         };//struct _latchSlot

      public:
         OSS_INLINE UINT32 getSize()const
         {
            return _size;
         }
         OSS_INLINE BOOLEAN isEmpty()const
         {
            return 0 == _size;
         }

         void fini()
         {
            if (_slots != _staticBuf)
            {
               SDB_OSS_DEL []_slots;
               _slots = _staticBuf;
            }
            _capacity = DEFAULT_CAPACITY;
            _size = 0;
            return;
         }

         INT32 push(const LATCH_OBJECT &obj,
                    OSS_SHARED_LATCH_MODE mode)
         {
            INT32 rc = SDB_OK;
            if (OSS_UNLIKELY(!obj.isValid() || OSS_SHARED_LATCH_MODE_NONE == mode))
            {
               rc = SDB_INVALIDARG;
               goto error;
            }

            rc = ensureBuf(_size + 1);
            if (SDB_OK != rc)
            {
               goto error;
            }

            _slots[_size].obj = obj;
            _slots[_size].mode = mode;
            ++_size;
         done:
            return rc;
         error:
            goto done;
         }

         INT32 pop(const KEY &key,
                   LATCH_OBJECT &obj,
                   OSS_SHARED_LATCH_MODE &mode)
         {
            INT32 rc = SDB_OK;
            _latchSlot *slot = NULL;
            UINT32 pos = 0;
            if (OSS_UNLIKELY(!key.isValid()))
            {
               rc = SDB_INVALIDARG;
               goto error;
            }

            slot = find(key, pos);
            if (NULL == slot)
            {
               rc = SDB_VESSEL_KEY_NOT_FOUND;
               goto error;
            }

            obj = slot->obj;
            mode = slot->mode;
            remove(pos);
         done:
            return rc;
         error:
            goto done;
         }

         BOOLEAN pop(LATCH_OBJECT &obj, OSS_SHARED_LATCH_MODE &mode)
         {
            BOOLEAN r = FALSE;
            if (0 == _size)
            {
               goto done;
            }

            obj = _slots[_size - 1].obj;
            mode = _slots[_size - 1].mode;
            remove(_size - 1);
            r = TRUE;

         done:
            return r;
         }

         INT32 findUpgradeAndSetExclusive(const KEY &key,
                                          LATCH_OBJECT &obj)
         {
            INT32 rc = SDB_OK;
            _latchSlot *slot = NULL;
            UINT32 pos = 0;
            if (OSS_UNLIKELY(!key.isValid()))
            {
               rc = SDB_INVALIDARG;
               goto error;
            }

            slot = find(key, pos);
            if (NULL == slot)
            {
               rc = SDB_VESSEL_KEY_NOT_FOUND;
               goto error;
            }
            if (OSS_SHARED_LATCH_MODE_UPGRADE != slot->mode)
            {
               rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
               goto error;
            }

            slot->mode = OSS_SHARED_LATCH_MODE_EXCLUSIVE;
            obj = slot->obj;
         done:
            return rc;
         error:
            goto done;
         }

         BOOLEAN test(const KEY &key,
                      OSS_SHARED_LATCH_MODE *mode)
         {
            BOOLEAN r = FALSE;
            _latchSlot *slot = NULL;
            UINT32 pos = 0;
            slot = find(key, pos);
            if (NULL != slot)
            {
               r = TRUE;
               if (NULL != mode)
               {
                  *mode = slot->mode;
               }
            }
            return r;
         }

      private:
         INT32 ensureBuf(UINT32 size)
         {
            INT32 rc = SDB_OK;
            _latchSlot *tmp = NULL;

            if (size <= _capacity)
            {
               goto done;
            }

            tmp = SDB_OSS_NEW _latchSlot[_capacity << 1];
            if (NULL == tmp)
            {
               rc = SDB_OOM;
               PD_LOG(PDERROR, "failed to allocate mem");
               goto error;
            }

            for (UINT32 i = 0; i < _size; ++i)
            {
               tmp[i] = _slots[i];
            }

            if (_staticBuf != _slots)
            {
               SDB_OSS_DEL []_slots;
            }

            _slots = tmp;
            _capacity = (_capacity << 1);
         done:
            return rc;
         error:
            goto done;
         }

         _latchSlot *find(const logicalIdLatchKey &key,
                          UINT32 &pos)
         {
            _latchSlot *out = NULL;
            SDB_ASSERT(key.isValid(), "can not be invalid");

            for (INT32 i = ((INT32)_size - 1); i >= 0; --i)
            {
               out = _slots + i;
               if (out->obj.getKey() == key)
               {
                  pos = i;
                  break;
               }
               out = NULL;
            }
         
            return out;
         }

         void remove(UINT32 pos)
         {
            SDB_ASSERT(pos < _size, "out of bound");
            for (UINT32 i = pos; (i + 1) < _size; ++i)
            {
               _slots[i] = _slots[i + 1];
            }

            if (0 < _size)
            {
               _slots[_size - 1] = _latchSlot();
               --_size;
            }
            return;
         }

      private:
         static constexpr UINT32 DEFAULT_CAPACITY = 4;
         UINT32 _capacity = DEFAULT_CAPACITY;
         UINT32 _size = 0;
         _latchSlot _staticBuf[DEFAULT_CAPACITY];
         _latchSlot *_slots = _staticBuf;

   };//class objectSharedLatchContext
}//namespace vessel
}//namespace engine

#endif//VESSEL_OBJECT_LATCH_MAP_H_