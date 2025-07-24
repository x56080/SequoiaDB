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

   Source File Name = objectLatchMap.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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
#include "vessel/lobChunkKey.h"
#include "../../bson/util/builder.h"

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
            bson::StringBuilder str(64);
            str << "{lpidlatch:" << _sid << ',' << _type << ',' << _lpid << '}';
            return std::move(str.poolStr());
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
         explicit recordIdLatchKey(SPACE_ID sid,
                                   CL_MB_ID mbID,
                                   const recordID &rid):
                  _sid(sid), _mbID(mbID),
                  _rid(rid), _pad(0){}

         recordIdLatchKey(const recordIdLatchKey &o):
         _sid(o._sid),
         _mbID(o._mbID),
         _rid(o._rid),
         _pad(o._pad){}

         recordIdLatchKey &operator=(const recordIdLatchKey &o)
         {
            _sid = o._sid;
            _mbID = o._mbID;
            _rid = o._rid;
            _pad = o._pad;
            return *this;
         }
         OSS_INLINE BOOLEAN operator==(const recordIdLatchKey &o)const
         {
            return _sid == o._sid &&
                   _mbID == o._mbID &&
                   _rid == o._rid;
         }
         OSS_INLINE UINT32 hash()const
         {
            return XXH3_64bits(this, sizeof(recordIdLatchKey));
         }

         OSS_INLINE BOOLEAN isValid()const
         {
            return INVALID_SPACE_ID != _sid &&
                   INVALID_CL_MB_ID != _mbID &&
                   _rid.isValid() &&
                   0 == _pad;
         }

         ossPoolString toString()const
         {
            bson::StringBuilder str(64);
            str << "{ridlatch:" << _sid
                << ',' << _mbID
                << ',' << _rid.getPid()
                << ',' << _rid.getPos()
                << '}';
            return std::move(str.poolStr());
         }

      public:
         SPACE_ID _sid = INVALID_SPACE_ID;
         CL_MB_ID _mbID = INVALID_CL_MB_ID;
         recordID _rid;
         UINT16 _pad = 0;
   };//class recordIdLatchKey

   typedef class sharedObjectMap<recordIdLatchKey, ossSpinSLatchPOSIX> RECORD_ID_LATCH_MAP;

   class uniqueIndexLatchKey : public SDBObject
   {
      public:
         uniqueIndexLatchKey(){}
         ~uniqueIndexLatchKey(){}
         explicit uniqueIndexLatchKey(SPACE_ID sid,
                                      CL_MB_ID mbID,
                                      UINT32 hash):
                  _sid(sid),
                  _mbID(mbID),
                  _hash(hash){}

         uniqueIndexLatchKey(const uniqueIndexLatchKey &o):
         _sid(o._sid),
         _mbID(o._mbID),
         _hash(o._hash)
         {}
         uniqueIndexLatchKey &operator=(const uniqueIndexLatchKey &o)
         {
            _sid = o._sid;
            _mbID = o._mbID;
            _hash = o._hash;
            return *this;
         }

      public:
         OSS_INLINE BOOLEAN operator==(const uniqueIndexLatchKey &o)const
         {
            return _sid == o._sid &&
                   _mbID == o._mbID &&
                   _hash == o._hash;
         }
         OSS_INLINE UINT32 hash()const
         {
            return _sid + _mbID + _hash;
         }

         OSS_INLINE BOOLEAN isValid()const
         {
            return INVALID_SPACE_ID != _sid &&
                   INVALID_CL_MB_ID != _mbID;
         }

         ossPoolString toString()const
         {
            bson::StringBuilder str(64);
            str << "{uhashlatch:" << _sid
                << ',' << _mbID << ',' << _hash << '}';
            return std::move(str.poolStr());
         }

      private:
         SPACE_ID _sid = INVALID_SPACE_ID;
         CL_MB_ID _mbID = INVALID_CL_MB_ID;
         UINT32 _hash = 0;
   };//class uniqueIndexLatchKey

   ///WARNING: UNIQUE_INDEX_LATCH_MAP's object is x latch, do not use objectSharedLatchContext.
   typedef class sharedObjectMap<uniqueIndexLatchKey, ossSpinXLatch> UNIQUE_INDEX_LATCH_MAP;


   template <class KEY, class LATCH=ossSharedLatch>
   class objectSharedLatchContext : public SDBObject
   {
      public:
         objectSharedLatchContext(){}
         ~objectSharedLatchContext(){}
         objectSharedLatchContext(const objectSharedLatchContext &) = delete;
         objectSharedLatchContext &operator=(const objectSharedLatchContext &) = delete;

      private:
         typedef class sharedObjectMap<KEY, LATCH>::object LATCH_OBJECT;

      public:
         class item : public SDBObject
         {
            public:
            
            item(){}
            ~item(){}
            explicit item(const LATCH_OBJECT &o,
                          const ossSharedLatchMode &m):
                     obj(o), mode(m){}
            item(const item &o):
            obj(o.obj),
            mode(o.mode){}

            item &operator=(const item &o)
            {
               obj = o.obj;
               mode = o.mode;
               return *this;
            }

            OSS_INLINE BOOLEAN isValid()const
            {
               return obj.isValid();
            }

            LATCH_OBJECT obj;
            ossSharedLatchMode mode;
         };//struct _latchSlot

      private:
         typedef ossPoolList<item> _ITEM_CONTAINER;

      public:
         OSS_INLINE UINT32 getSize()const
         {
            return _items.size();
         }
         OSS_INLINE BOOLEAN isEmpty()const
         {
            return _items.empty();
         }

         void fini()
         {
            _items.clear();
            return;
         }

         void pushBack(const LATCH_OBJECT &obj,
                       const ossSharedLatchMode &mode)
         {
            SDB_ASSERT(obj.isValid(), "can not be invalid");
            SDB_ASSERT(!mode.isNone(), "can not be none");
         
            _items.push_back(item(obj, mode));
         }

         BOOLEAN findAndPop(const KEY &key,
                            item &out)
         {
            SDB_ASSERT(key.isValid(), "can not be invalid");
            out = item();
            for (typename _ITEM_CONTAINER::iterator itr = _items.begin();
                 itr != _items.end(); ++itr)
            {
               SDB_ASSERT(itr->isValid(), "impossible");
               if (itr->isValid() && itr->obj.getKey() == key)
               {
                  out = *itr;
                  _items.erase(itr);
                  break;
               }
            }

            return out.isValid();
         }

         BOOLEAN popBack(item &out)
         {
            out = item();
            if (!isEmpty())
            {
               out = _items.back();
               SDB_ASSERT(out.isValid(), "impossible");
               _items.pop_back();
            }
            return out.isValid();
         }

         BOOLEAN test(const KEY &key,
                      ossSharedLatchMode *mode)const
         {
            SDB_ASSERT(key.isValid(), "can not be invalid");
            BOOLEAN r = FALSE;
            for (typename _ITEM_CONTAINER::const_reverse_iterator itr = _items.crbegin();
                 itr != _items.crend(); ++itr)
            {
               SDB_ASSERT(itr->isValid(), "impossible");
               if (itr->isValid() && itr->obj.getKey() == key)
               {
                  r = TRUE;
                  if (NULL != mode)
                  {
                     *mode = itr->mode;
                  }
                  break;
               }
            }

            return r;
         }

         item findToUpdate(const KEY &key, ossSharedLatchMode **mode)
         {
            SDB_ASSERT(key.isValid(), "can not be invalid");
            SDB_ASSERT(NULL != mode, "can not be null");
            item out;
            for (typename _ITEM_CONTAINER::iterator itr = _items.begin();
                 itr != _items.end(); ++itr)
            {
               SDB_ASSERT(itr->isValid(), "impossible");
               if (itr->isValid() && itr->obj.getKey() == key)
               {
                  out = *itr;
                  *mode = &(itr->mode);
                  break;
               }
            }

            return out;
         }

      private:
         _ITEM_CONTAINER _items;

   };//class objectSharedLatchContext

   #pragma pack()

   typedef objectSharedLatchContext<recordIdLatchKey, ossSpinSLatchPOSIX> RID_LATCH_CONTEXT;
   typedef objectSharedLatchContext<logicalPidLatchKey> LPID_LATCH_CONTEXT;

   typedef class globalLobChunkKey LOB_CHUNK_LATCH_KEY;
   typedef class sharedObjectMap<LOB_CHUNK_LATCH_KEY, ossSpinSLatchPOSIX> LOBC_LATCH_MAP;
}//namespace vessel
}//namespace engine

#endif//VESSEL_OBJECT_LATCH_MAP_H_