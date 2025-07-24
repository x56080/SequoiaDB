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

   Source File Name = hybridTreeIterator.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_HYBRID_TREE_ITERATOR_H_
#define VESSEL_HYBRID_TREE_ITERATOR_H_

#include "vessel/indexIterator.h"
#include "vessel/lsm/lsmTreeIterator.h"
#include "vessel/btreeIterator.h"
#include "vessel/keyString.h"
#include "vessel/lsm/lsmIteratorBound.h"
#include "vessel/lpsPteViewer.h"

#include <array>

namespace engine
{
namespace vessel
{
   class hybridTreeIterator : public indexIterator
   {
      public:
         hybridTreeIterator() = default;
         virtual ~hybridTreeIterator();

      public:
         INT32 init(requestContext *context,
                    indexSpace *is,
                    const lsmColumnFamily &cf,
                    indexObject *obj);

         OSS_INLINE BOOLEAN isValid() const {return nullptr != _obj;}

      public:
         virtual INDEX_ITERATOR_TYPE getType() const override
         {
            return INDEX_ITERATOR_TYPE::HYBRID_TREE;
         } 
         virtual void reset() override;

      public:/// init iterator location. 
         virtual INT32 seek(const VEC_ELE_CMP &eles,
                            const inclusiveVec &iv,
                            const options &o) override;
                            
         virtual INT32 seek(const bson::BSONObj &key,
                            const inclusiveVec &iv,
                            const options &o) override;

         /// locate
         virtual INT32 locateNext(const indexEntryLocation *location,
                                  const options &o) override;

         virtual INT32 pause() override;

         virtual INT32 resume() override;

      public:
         virtual INT32 next();

         /// reseek from current pos, to fast skip unmatched entries.
         virtual INT32 advance(const bson::BSONObj &prevKey,
                               INT32 fieldCountToCmpInPrev,
                               const VEC_ELE_CMP &matchEles,
                               const inclusiveVec &iv) override;

         virtual BOOLEAN isReadyToRead()const override
         {
            return _isReadyToRead();
         }

      public:
         virtual bson::BSONObj getKeyObj(BOOLEAN withFieldName,
                                         bson::BufBuilder *buf)const override;
         virtual DPS_LSN_OFFSET getLSN()const override;
         virtual DPS_TRANS_ID getTransID()const override;
         virtual recordID getRid()const override;
         virtual INT32 initOrUpdateLocation(IDX_ENTRY_LOCATION_UPTR &location) const override;

      public:
         INT32 seek(const keyString &key, const options &o);
         keyString getCurrentKeyString()const;

      private:
         enum _FILLING_STATUS : INT32
         {
            NONE_EXPECTED = 0x0,
            LSM_EXPECTED = 0x01,
            BTREE_EXPECTED = 0x02,
            BOTH_EXPECTED = 0x03,
         };
         enum _RING_POS : INT32
         {
            INVALID = -1,
            LSM = 0,
            BTREE = 1,
         };

         struct _RING_PICK_RES : public SDBObject
         {
            OSS_INLINE BOOLEAN isPicked() const
            {
               return _RING_POS::INVALID != pos;
            }
            OSS_INLINE BOOLEAN needRefill() const
            {
               return _FILLING_STATUS::NONE_EXPECTED != status;
            }

            _RING_POS pos = _RING_POS::INVALID;
            _FILLING_STATUS status = _FILLING_STATUS::NONE_EXPECTED;
         };

         static constexpr UINT32 _RING_SIZE = 2;
         using _ENTRY_RING = std::array<keyString, _RING_SIZE>;

      private:
         class _location : public indexEntryLocation
         {
            public:
               _location() = default;
               virtual ~_location() = default;

            public:
               virtual IDX_ENTRY_LOCATION_TYPE getType() const override
               {
                  return IDX_ENTRY_LOCATION_TYPE::HIT;
               }
               virtual BOOLEAN isValid() const override
               {
                  return _RING_POS::INVALID != current &&
                         !entry.empty();
               }

               void reset()
               {
                  current = _RING_POS::INVALID;
                  entry.clear();
                  bl.reset();
               }

            public:
               _RING_POS current = _RING_POS::INVALID;
               ossPoolString entry;
               btreeIterator::location bl;
         };//class _location

      private:
         INT32 _reinitInternalItrs();
         void _resetInternalItrs();
         INT32 _seek(const keyString &ks);
         INT32 _seekForPrev(const keyString &ks);
         void _resetRing();
         INT32 _refillRingAndPick(BOOLEAN fetchNext);
         INT32 _refillRing(BOOLEAN fetchNext);
         _RING_PICK_RES _pickFromRing() const;
         INT32 _locateNext(const _location *l);
         INT32 _resumeBtreeLocation(const _location &l,
                                    const keyString &ks);
         INT32 _advance(const keyString &ks);

         OSS_INLINE BOOLEAN _isReadyToRead() const
         {
            return _RING_POS::INVALID != _pos;
         }
         OSS_INLINE BOOLEAN _isPaused() const
         {
            return !_lsm.isValid();
         }
         OSS_INLINE const keyString &_getCurrent()const
         {
            return _ring[_pos];
         }

      private:
         indexObject *_obj = nullptr;
         indexSpace *_is = nullptr;
         requestContext *_context = nullptr;
         lsmColumnFamily _cf;
         options _o;
         lsmIteratorBound _bound;
         lsmTreeIterator _lsm;
         lpsPteViewer _viewer;
         btreeIterator _btree;
         INT32 _status = _FILLING_STATUS::BOTH_EXPECTED;
         _RING_POS _pos = _RING_POS::INVALID;
         _ENTRY_RING _ring;
   };//class hybridTreeIterator
} // namespace vessel

} // namespace engine


#endif//VESSEL_HYBRID_TREE_ITERATOR_H_