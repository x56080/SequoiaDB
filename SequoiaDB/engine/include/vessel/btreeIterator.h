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

   Source File Name = btreeIterator.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_BTREE_ITERATOR_H_
#define VESSEL_BTREE_ITERATOR_H_

#include "vessel/indexDef.h"
#include "vessel/btreeAccessContext.h"
#include "vessel/btreeKeyStringEntry.h"

namespace engine
{
namespace vessel
{
   class requestContext;
   class indexSpace;
   class indexObject;

   class btreeIterator : public SDBObject
   {
      public:
         btreeIterator() = default;
         ~btreeIterator();
         btreeIterator(const btreeIterator &) = delete;
         btreeIterator &operator=(const btreeIterator &) = delete;
         
      public:
         OSS_INLINE BOOLEAN isValid() const {return _bac.isValid();}
         INT32 init(requestContext *context,
                    indexSpace *is,
                    indexObject *obj);

         void reset(); 

         UINT32 getTransferTick()const;

         INT32 seek(const keyString &ks);

         INT32 seekForPrev(const keyString &ks);

         OSS_INLINE BOOLEAN isReadyToRead() const {return _current.isValid();}

      public:/// ensure is ready to read first
         INT32 next(BOOLEAN forward=TRUE);
         INT32 advance(const keyString &ks, BOOLEAN forPrev=FALSE);
         OSS_INLINE const btreeKeyStringEntry &getEntry() const
         {
            return _current;
         }
         DPS_TRANS_ID getTransID() const;
         UINT64 getLSN() const;

      public:
         class location : public SDBObject
         {
            friend class btreeIterator;
            public:
               location() = default;
               ~location() = default;
               location(const location &) = default;
               location &operator=(const location &) = default;
               location(location &&o):
               _transferTick(o._transferTick),
               _path(std::move(o._path)),
               _pos(o._pos)
               {
                  o.reset();
               }
               location &operator=(location &&o)
               {
                  reset();
                  _transferTick = o._transferTick;
                  _path = std::move(o._path);
                  _pos = o._pos;
                  o.reset();
                  return *this;
               }
         
            public:
               void reset()
               {
                  _transferTick = 0;
                  _path.clear();
                  _pos = INVALID_RECORD_SLOT_POS;
               }

               OSS_INLINE BOOLEAN isValid()const {return isValidRecordSlotPosition(_pos);}
               OSS_INLINE UINT32 getTransferTick()const {return _transferTick;}
               OSS_INLINE RECORD_SLOT_POS getPos() const {return _pos;}

            private:
               UINT32 _transferTick = 0;
               ossPoolVector<UINT64> _path;
               RECORD_SLOT_POS _pos = INVALID_RECORD_SLOT_POS;
         };//class location

         location getLocation() const;
         INT32 locate(const location &l);

      private:

         INT32 _seekFromPathEnd(const keyString &ks,
                                RECORD_SLOT_POS pos=0,
                                BOOLEAN forward=TRUE);

         INT32 _cacheOrMove(BOOLEAN forward);

         INT32 _moveToAncestor();

         INT32 _moveToNext(BOOLEAN forward);

         INT32 _nextFromLeaf(BOOLEAN forward);

         INT32 _locateForwardFromNonleaf();

         INT32 _locateBackwardFromNonleaf();

         INT32 _locateToBottomStart(PAGE_ID child, const btreePathFootprint &fp);

         INT32 _locateToBottomEnd(PAGE_ID child, const btreePathFootprint &fp);

         BOOLEAN _hasLocation() const;

         void _resetCacheAndLocation();

         void _relocateToAncestorNode(BOOLEAN forward);

         INT32 _restorePath(const ossPoolVector<UINT64> &path);

         INT32 _advance(const keyString &ks, BOOLEAN forward);

         OSS_INLINE RECORD_SLOT_POS _getNextPos(RECORD_SLOT_POS pos,
                                                BOOLEAN forward)
         {
            return forward ? ++pos : --pos;
         }

      private:
         btreeAccessContext _bac;
         RECORD_SLOT_POS _pos = INVALID_RECORD_SLOT_POS;
         btreeKeyStringEntry _current;
   };//class btreeIterator
} // namespace vessel

} // namespace engine


#endif//VESSEL_BTREE_ITERATOR_H_
