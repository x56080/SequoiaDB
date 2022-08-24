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

   Source File Name = btreeIterator.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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
               OSS_INLINE UINT64 getTransferTick()const {return _transferTick;}
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
