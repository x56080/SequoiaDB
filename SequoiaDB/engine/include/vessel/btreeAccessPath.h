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

   Source File Name = btreeAccessPath.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_BTREE_ACCESS_PATH_H_
#define VESSEL_BTREE_ACCESS_PATH_H_

#include "ossMemPool.hpp"
#include "vessel/vesselIdDef.h"

namespace engine
{
namespace vessel
{
   class btreeAccessPath : public SDBObject
   {
      public:
         btreeAccessPath();
         ~btreeAccessPath();
         btreeAccessPath(const btreeAccessPath &) = delete;
         btreeAccessPath &operator=(const btreeAccessPath &) = delete;

      private:
         class _node : public SDBObject
         {
            public:
               OSS_INLINE _node(){}
               OSS_INLINE _node(const _node &o):
               _lpid(o._lpid),
               _splitedTimes(o._splitedTimes){}
               OSS_INLINE explicit _node(PAGE_ID lpid, UINT32 splitedTimes):
               _lpid(lpid),
               _splitedTimes(splitedTimes){}
               OSS_INLINE ~_node(){}

            public:
               OSS_INLINE _node &operator=(const _node &o)
               {
                  _lpid = o._lpid;
                  _splitedTimes = o._splitedTimes;
                  return *this;
               }
               OSS_INLINE BOOLEAN operator==(const _node &o)const
               {
                  return _lpid == o._lpid && _splitedTimes == o._splitedTimes;
               }
               OSS_INLINE BOOLEAN operator!=(const _node &o)const
               {
                  return _lpid != o._lpid || _splitedTimes != o._splitedTimes;
               }

            public:
               OSS_INLINE PAGE_ID getLpid()const {return _lpid;}
               OSS_INLINE UINT32 getSplitedTimes()const {return _splitedTimes;}
               OSS_INLINE BOOLEAN isValid()const{return INVALID_PAGE_ID != _lpid;}

            private:
               PAGE_ID _lpid = INVALID_PAGE_ID;
               UINT32 _splitedTimes = 0;
         };//class _node

      public:
         class pathNode : public SDBObject
         {
            friend class btreeAccessPath;
            public:
               OSS_INLINE pathNode(){}
               OSS_INLINE pathNode(const pathNode &o):
               _depth(o._depth),
               _lpid(o._lpid),
               _splitedTimes(o._splitedTimes){}
               OSS_INLINE ~pathNode(){}
               OSS_INLINE pathNode &operator=(const pathNode &o)
               {
                  _depth = o._depth;
                  _lpid = o._lpid;
                  _splitedTimes = o._splitedTimes;
                  return *this;
               }

            private:
               OSS_INLINE explicit pathNode(UINT32 depth,
                                            PAGE_ID lpid,
                                            UINT32 splitedTimes):
               _depth(depth),
               _lpid(lpid),
               _splitedTimes(splitedTimes)
               {}

            public:
               OSS_INLINE BOOLEAN isValid()const
               {
                  return INVALID_PAGE_ID != _lpid;
               }
               OSS_INLINE UINT32 getDepth()const
               {
                  return _depth;
               }
               OSS_INLINE PAGE_ID getLpid()const
               {
                  return _lpid;
               }
               OSS_INLINE UINT32 getSplitedTimes()const
               {
                  return _splitedTimes;
               }
               OSS_INLINE BOOLEAN isRoot()const
               {
                  return isValid() && 0 == _depth;
               }
            private:
               UINT32 _depth = 0;
               PAGE_ID _lpid = INVALID_PAGE_ID;
               UINT32 _splitedTimes = 0;
         };//class pathNode

      public:
         OSS_INLINE UINT32 getSize()const {return _size;}
         OSS_INLINE BOOLEAN isEmpty()const {return 0 == _size;}

      public:
         void clear();

         void push(PAGE_ID lpid, UINT32 splitedTimes);

         void pop();

         pathNode operator[](UINT32 pos)const;

         pathNode getCurrentNode()const;

         /// get father _node of current path node
         _node getFatherNode()const;

         /// get father _node of spcified pos.
         pathNode getFatherOfCurrentNode()const;

         pathNode getFatherNode(UINT32 depth)const;

      private:
         const _node &get(UINT32 pos)const;

      private:
         static const UINT32 _DEFAULT_CAPACITY = 4;

      private:
         UINT32 _size = 0;
         _node _staticNodes[_DEFAULT_CAPACITY];
         ossPoolVector<_node> _dynamicNodes;

   }; // class btreeAccessPath
} // namespace vessel

} // namespace engine


#endif//VESSEL_BTREE_ACCESS_PATH_H_
