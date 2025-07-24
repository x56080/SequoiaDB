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

   Source File Name = lobcExtentChain.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_LOBC_EXTENT_CHAIN_H_
#define VESSEL_LOBC_EXTENT_CHAIN_H_

#include "vessel/lextentDescriptor.h"
#include "vessel/objectIdentifier.h"
#include "ossMemPool.hpp"


namespace engine
{
namespace vessel
{
   class lobcExtentChain : public SDBObject
   {
      public:
         lobcExtentChain(){}
         ~lobcExtentChain(){}
         lobcExtentChain(const lobcExtentChain &o):
         _pageSize(o._pageSize),
         _size(o._size),
         _chain(o._chain){}
         lobcExtentChain &operator=(const lobcExtentChain &o)
         {
            _pageSize = o._pageSize;
            _size = o._size;
            _chain = o._chain;
            return *this;
         }

      private:
         typedef class ossPoolVector<lextentDescriptor> _EXTENT_VEC;

      public:
         OSS_INLINE BOOLEAN isValid()const {return 0 < _pageSize;}
         OSS_INLINE BOOLEAN isEmpty()const {return 0 == _size;}
         OSS_INLINE UINT32 getChunkSize()const {return _size;}
         OSS_INLINE UINT32 getChainSize()const {return _chain.size();}
         OSS_INLINE UINT32 getPageSize()const {return _pageSize;}
         OSS_INLINE const lextentDescriptor &getChainItem(UINT32 pos)const
         {
            return _chain.at(pos);
         }
         OSS_INLINE void reset()
         {
            _pageSize = 0;
            _size = 0;
            _chain.clear();
            return;
         }

         UINT32 getCurrentCapacity()const;

      public:
         void init(UINT32 pageSize);
         INT32 pushBack(const lextentDescriptor &desc);
         BOOLEAN isValidAccessing(UINT32 offset, UINT32 size)const;

         /// return real size extended.
         UINT32 extendLastExtent(UINT32 deltaSize);

         UINT32 getFreeSizeInLastExtent()const;

      public:
         class extentRoadmap : public SDBObject
         {
            friend class lobcExtentChain;
            public:
               OSS_INLINE BOOLEAN isValid()const
               {
                  return INVALID_PAGE_ID != _pid;
               }
               OSS_INLINE PAGE_ID getPid(UINT32 pos)const
               {
                  SDB_ASSERT(pos < _pcnt, "out of bound");
                  return _pid + pos;
               }
               OSS_INLINE UINT32 getOffset(UINT32 pos)const
               {
                  SDB_ASSERT(pos < _pcnt, "out of bound");
                  return 0 == pos ? _boffset : 0;
               }
               OSS_INLINE UINT32 getSize(UINT32 pos)const
               {
                  SDB_ASSERT(pos < _pcnt, "out of bound");
                  if (1 == _pcnt)
                  {
                     return _eoffset - _boffset;
                  }
                  else
                  {
                     if (0 == pos)
                     {
                        return _pageSize - _boffset;
                     }
                     else if ((pos + 1) == _pcnt)
                     {
                        UINT32 mod = _eoffset % _pageSize;
                        return 0 == mod ? _pageSize : mod;
                     }
                     else
                     {
                        return _pageSize;
                     }
                  }
               }
               OSS_INLINE UINT32 getPcnt()const
               {
                  return _pcnt;
               }
               OSS_INLINE UINT32 getSize()const {return _eoffset - _boffset;}

               void reset()
               {
                  _pageSize = 0;
                  _pid = INVALID_PAGE_ID;
                  _pcnt = 0;
                  _boffset = 0;
                  _eoffset = 0;
               }
            private:
               UINT32 _pageSize = 0;
               /// WARNING: _pid may be not the first pid of extent.
               PAGE_ID _pid = INVALID_PAGE_ID;
               UINT32 _pcnt = 0;
               UINT32 _boffset = 0;
               UINT32 _eoffset = 0;
         };//struct extentRoadmap

         INT32 createExtentRoadmap(UINT32 offset, UINT32 size, extentRoadmap &roadmap)const;
         INT32 createExtentRoadmaps(UINT32 offset, UINT32 size,
                                    ossPoolList<extentRoadmap> &roadmaps)const;

      private:
         INT32 _createExtentRoadmap(UINT32 offset,
                                    UINT32 size,
                                    UINT32 chainPos,
                                    extentRoadmap &roadmap);
      private:
         UINT32 _pageSize = 0;
         UINT32 _size = 0;
         _EXTENT_VEC _chain;
   };//class lobcExtentChain

} // namespace vessel

} // namespace engine


#endif//VESSEL_LOBC_EXTENT_CHAIN_H_