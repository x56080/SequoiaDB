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
#include "ossSharedLatch.hpp"
#include "vessel/recordID.h"
#include "vessel/vesselFileDef.h"
#include "pdTrace.hpp"
#include "xxHashInc.h"
#include "ossMemPool.hpp"
#include "xxHashInc.h"

namespace engine
{
namespace vessel
{
#pragma pack(4)
   class logicalPidLatchKey : public SDBObject
   {
      public:
         logicalPidLatchKey(){}
         explicit logicalPidLatchKey(SPACE_ID sid,
                                     SPACE_TYPE type,
                                     PAGE_ID lpid):
         _sid(sid),
         _type(type),
         _pad(0),
         _lpid(lpid){}

         ~logicalPidLatchKey(){}
         logicalPidLatchKey(const logicalPidLatchKey &o):
         _sid(o._sid),
         _type(o._type),
         _pad(o._pad),
         _lpid(o._lpid){}
         logicalPidLatchKey &operator=(const logicalPidLatchKey &o)
         {
            _sid = o._sid;
            _type = o._type;
            _pad = o._pad;
            _lpid = o._lpid;
            return *this;
         }
         OSS_INLINE BOOLEAN operator==(const logicalPidLatchKey &o)const
         {
            return _sid == o._sid &&
                   _type == o._type &&
                   _pad == o._pad &&
                   _lpid == o._lpid;
         }
         OSS_INLINE UINT32 hash()const
         {
            return XXH3_64bits(this, sizeof(logicalPidLatchKey));
         }

         OSS_INLINE BOOLEAN isValid()const
         {
            return INVALID_SPACE_ID != _sid &&
                   INVALID_SPACE_TYPE != _type &&
                   0 == _pad &&
                   INVALID_PAGE_ID != _lpid;
         }

         ossPoolString toString()const
         {
            static const UINT32 _BUF_SIZE = 16;
            CHAR buf[_BUF_SIZE] = {};
            ossPoolString str;
            str.reserve(64);
            str.append("{sid");
            ossItoa(_sid, buf, _BUF_SIZE);
            str.append(buf);
            str.append(", type:");
            ossItoa(_type, buf, _BUF_SIZE);
            str.append(buf);
            str.append(", lpid:");
            ossItoa(_lpid, buf, _BUF_SIZE);
            str.append(buf);
            str.append("}");
            return str;
         }
      public:
         UINT16 _sid = INVALID_SPACE_ID;
         UINT8 _type = INVALID_SPACE_TYPE;
         UINT8 _pad = 0;
         UINT32 _lpid = INVALID_PAGE_ID;
   };//class logicalPidLatchKey

   typedef class sharedObjectMap<logicalPidLatchKey, ossSharedLatch> LOGICAL_PID_LATCH_MAP;

   class recordIdLatchKey : public SDBObject
   {
      public:
         recordIdLatchKey(){}
         ~recordIdLatchKey(){}
         explicit recordIdLatchKey(UINT32 lcs,
                                   UINT32 lcl,
                                   const recordID &rid):
                  _lcs(lcs), _lcl(lcl), _rid(rid){}

         recordIdLatchKey(const recordIdLatchKey &o):
         _lcs(o._lcs),
         _lcl(o._lcl),
         _rid(o._rid){}

         recordIdLatchKey &operator=(const recordIdLatchKey &o)
         {
            _lcs = o._lcs;
            _lcl = o._lcl;
            _rid = o._rid;
            return *this;
         }
         OSS_INLINE BOOLEAN operator==(const recordIdLatchKey &o)const
         {
            return _lcs == o._lcs &&
                   _lcl == o._lcl &&
                   _rid == o._rid;
         }
         OSS_INLINE UINT32 hash()const
         {
            //return _sid + _mbID + _rid.getPageID() + _rid.getSlotID();
            return _lcs + _lcl + _rid.hash();
         }

         OSS_INLINE BOOLEAN isValid()const
         {
            return DMS_INVALID_LOGICCSID != _lcs &&
                   DMS_INVALID_LOGICCLID != _lcl &&
                   _rid.isValid();
         }

         ossPoolString toString()const
         {
            static const UINT32 _BUF_SIZE = 16;
            CHAR buf[_BUF_SIZE] = {};
            ossPoolString str;
            str.reserve(64);
            str.append("{lcs:");
            ossItoa(_lcs, buf, _BUF_SIZE);
            str.append(buf);
            str.append(", lcl:");
            ossItoa(_lcl, buf, _BUF_SIZE);
            str.append(buf);
            str.append(", lpid:");
            ossItoa(_rid.getPageID(), buf, _BUF_SIZE);
            str.append(buf);
            str.append(", slot:");
            ossItoa(_rid.getSlotID(), buf, _BUF_SIZE);
            str.append(buf);
            str.append("}");
            return str;
         }

      public:
         UINT32 _lcs = DMS_INVALID_LOGICCSID;
         UINT32 _lcl = DMS_INVALID_LOGICCLID;
         recordID _rid;
   };//class recordIdLatchKey

   typedef class sharedObjectMap<recordIdLatchKey, ossSharedLatch> RECORD_ID_LATCH_MAP;

   class uniqueIndexLatchKey : public SDBObject
   {
      public:
         uniqueIndexLatchKey(){}
         ~uniqueIndexLatchKey(){}
         explicit uniqueIndexLatchKey(UINT32 lcs,
                                      UINT32 lcl,
                                      UINT32 hash):
                  _lcs(lcs),
                  _lcl(lcl),
                  _hash(hash){}

         uniqueIndexLatchKey(const uniqueIndexLatchKey &o):
         _lcs(o._lcs),
         _lcl(o._lcl),
         _hash(o._hash)
         {}
         uniqueIndexLatchKey &operator=(const uniqueIndexLatchKey &o)
         {
            _lcs = o._lcs;
            _lcl = o._lcl;
            _hash = o._hash;
            return *this;
         }

      public:
         OSS_INLINE BOOLEAN operator==(const uniqueIndexLatchKey &o)const
         {
            return _lcs == o._lcs &&
                   _lcl == o._lcl &&
                   _hash == o._hash;
         }
         OSS_INLINE UINT32 hash()const
         {
            return _lcs + _lcl + _hash;
         }

         OSS_INLINE BOOLEAN isValid()const
         {
            return DMS_INVALID_LOGICCSID != _lcs &&
                   DMS_INVALID_LOGICCLID != _lcl;
         }

         ossPoolString toString()const
         {
            static const UINT32 _BUF_SIZE = 16;
            CHAR buf[_BUF_SIZE] = {};
            ossPoolString str;
            str.reserve(64);
            str.append("{lcs:");
            ossItoa(_lcs, buf, _BUF_SIZE);
            str.append(buf);
            str.append(", lcl:");
            ossItoa(_lcl, buf, _BUF_SIZE);
            str.append(buf);
            str.append(", hash:");
            ossItoa(_hash, buf, _BUF_SIZE);
            str.append(buf);
            str.append("}");
            return str;
         }

      private:
         UINT32 _lcs = DMS_INVALID_LOGICCSID;
         UINT32 _lcl = DMS_INVALID_LOGICCLID;
         UINT32 _hash = 0;
   };//class uniqueIndexLatchKey

   ///WARNING: UNIQUE_INDEX_LATCH_MAP's object is x latch, do not use objectSharedLatchContext.
   typedef class sharedObjectMap<uniqueIndexLatchKey, ossSpinXLatch> UNIQUE_INDEX_LATCH_MAP;

   template <typename KEY>
   class objectSharedLatchContext : public SDBObject
   {
      public:
         objectSharedLatchContext(){}
         ~objectSharedLatchContext(){}
         objectSharedLatchContext(const objectSharedLatchContext &) = delete;
         objectSharedLatchContext &operator=(const objectSharedLatchContext &) = delete;

      private:
         typedef class sharedObjectMap<KEY, ossSharedLatch>::object LATCH_OBJECT;

      private:
         struct _latchSlot : public SDBObject
         {
            _latchSlot(){}
            ~_latchSlot(){}
            _latchSlot(const _latchSlot &o):
            obj(o.obj),
            mode(o.mode){}

            _latchSlot &operator=(const _latchSlot &o)
            {
               obj = o.obj;
               mode = o.mode;
               return *this;
            }

            LATCH_OBJECT obj;
            ossSharedLatchMode mode;
         };//struct _latchSlot

      public:
         OSS_INLINE UINT32 getSize()const
         {
            return _data.size();
         }
         OSS_INLINE BOOLEAN isEmpty()const
         {
            return _data.empty();
         }

         void fini()
         {
            _data.clear();
            return;
         }

         void pushBack(const LATCH_OBJECT &obj,
                       const ossSharedLatchMode &mode)
         {
            SDB_ASSERT(obj.isValid(), "can not be invalid");
            SDB_ASSERT(!mode.isNone(), "can not be none");
            _latchSlot slot;
            slot.obj = obj;
            slot.mode = mode;
            _data.push_back(slot);
         }

         BOOLEAN findAndPop(const KEY &key,
                            LATCH_OBJECT &obj,
                            ossSharedLatchMode &mode)
         {
            _latchSlot *slot = NULL;
            UINT32 pos = 0;
            obj = LATCH_OBJECT();
            mode.setNone();

            slot = find(key, pos);
            if (NULL != slot)
            {
               obj = slot->obj;
               mode = slot->mode;
               remove(pos);
            }

            return NULL != slot;
         }

         BOOLEAN popBack(LATCH_OBJECT &obj, ossSharedLatchMode &mode)
         {
            BOOLEAN r = FALSE;
            if (!isEmpty())
            {
               const _latchSlot &slot = _data.back();
               obj = slot.obj;
               mode = slot.mode;
               _data.pop_back();
               r = TRUE;
            }
            return r;
         }

         BOOLEAN findToUpdate(const KEY &key,
                              LATCH_OBJECT &obj,
                              ossSharedLatchMode **mode)
         {
            _latchSlot *slot = NULL;
            UINT32 pos = 0;
            obj = LATCH_OBJECT();
            if (NULL != mode)
            {
               *mode = NULL;
            }

            slot = find(key, pos);
            if (NULL != slot)
            {
               obj = slot->obj;
               if (NULL != mode)
               {
                  *mode = &(slot->mode);
               }
            }

            return NULL != slot;
         }

         BOOLEAN test(const KEY &key,
                      ossSharedLatchMode *mode)const
         {
            if (NULL != mode)
            {
               mode->setNone();
            }
            const _latchSlot *slot = NULL;
            UINT32 pos = 0;
            slot = find(key, pos);
            if (NULL != slot)
            {
               if (NULL != mode)
               {
                  *mode = slot->mode;
               }
            }
            return NULL != slot;
         }

      private:
         _latchSlot *find(const KEY &key,
                          UINT32 &pos)
         {
            _latchSlot *out = NULL;
            SDB_ASSERT(key.isValid(), "can not be invalid");
            for (INT32 i = ((INT32)(_data.size()) - 1); i >= 0; --i)
            {
               if (_data[i].obj.getKey() == key)
               {
                  out = _data.data() + i;
                  pos = i;
                  break;
               }
            }
         
            return out;
         }

         const _latchSlot *find(const KEY &key,
                          UINT32 &pos)const
         {
            const _latchSlot *out = NULL;
            SDB_ASSERT(key.isValid(), "can not be invalid");
            for (INT32 i = ((INT32)(_data.size()) - 1); i >= 0; --i)
            {
               if (_data[i].obj.getKey() == key)
               {
                  out = _data.data() + i;
                  pos = i;
                  break;
               }
            }
         
            return out;
         }

         void remove(UINT32 pos)
         {
            if (OSS_LIKELY(pos < _data.size()))
            {
               for (UINT32 i = pos; (i + 1) < _data.size(); ++i)
               {
                  _data[i] = _data[i + 1];
               }
               _data.pop_back();
            }
            else
            {
               SDB_ASSERT(FALSE, "out of bound");
            }
      
            return;
         }

      private:
         ossPoolVector<_latchSlot> _data;

   };//class objectSharedLatchContext

   #pragma pack()

   typedef objectSharedLatchContext<recordIdLatchKey> RID_LATCH_CONTEXT;
   typedef objectSharedLatchContext<logicalPidLatchKey> LPID_LATCH_CONTEXT;
}//namespace vessel
}//namespace engine

#endif//VESSEL_OBJECT_LATCH_MAP_H_