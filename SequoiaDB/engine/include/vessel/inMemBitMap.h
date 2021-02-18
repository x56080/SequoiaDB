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

   Source File Name = inMemBitMap.h

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_IN_MEM_BIT_MAP_H_
#define VESSEL_IN_MEM_BIT_MAP_H_

#include "ossLatch.hpp"

#include <map>

namespace engine
{
namespace vessel
{
   class requestContext;

   const UINT32 IMBM_FLAG_KEEP_NONFREE_PAGE = 0x01;

   class inMemBitMap : public SDBObject
   {
      private:
         class _inMemBitPage : public SDBObject
         {
            public:
               _inMemBitPage():
               _pageID(-1),
               _size(0), 
               _free(0), 
               _firstFreeBits(-1),
               _bitsBuf(NULL)
               {}

               ~_inMemBitPage();

            public:
               INT32 setup(INT32 pageID, UINT32 size, BOOLEAN noFree = FALSE, UINT32 occupied = 0);
               INT32 setup(INT32 pageID, UINT32 size, UINT32 free, INT32 firstFree, const CHAR *buf);
               INT32 teardown();

               INT32 allocate(UINT32 count, UINT32 *buf, UINT32 *stillFreeCount = NULL);
               INT32 free(UINT32 count, const UINT32 *buf);

               OSS_INLINE INT32 getPageID()const
               {
                  return _pageID;
               }
               OSS_INLINE UINT32 getFree()const
               {
                  return _free;
               }
               OSS_INLINE UINT32 getSize()const
               {
                  return _size;
               }

               OSS_INLINE INT32 getFirstFree()const
               {
                  return _firstFreeBits;
               }

               OSS_INLINE const UINT32 *getBuf()const
               {
                  return _bitsBuf;
               }

               OSS_INLINE UINT32 getBufSize()const
               {
                  return ossAlign32(_size) / 8; /// 8bit per byte
               }

            private:
               void updateFirstFreeAfterAllocating();

            private:
               INT32 _pageID;
               UINT32 _size;
               UINT32 _free; /// free <= _size
               INT32 _firstFreeBits;
               UINT32 *_bitsBuf;

         };// class _inMemBitPage

      public:
         inMemBitMap();
         ~inMemBitMap();

      public:
         /// _pageCount will never become smaller.no latch protected.
         OSS_INLINE UINT32 getPageCount()const
         {
            return _pageCount;
         }
         OSS_INLINE BOOLEAN isSetup()const
         {
            return 0 < _bitCountInPage;
         }
      public:
         /// bitCountInPage:
         /// you'd better make bitCountInPage equal to corresponding smp's(or id map page) capacity.
         /// inMemBitMap will always do allocating from only one page.
         /// it may cause a waste of resource, but can save the io times.
         /// freeBound:
         /// freeBound will be used to quickly skip bit pages with insufficient free count.
         INT32 setup(UINT32 bitCountInPage, UINT32 freeBound = 0);
         INT32 teardown();

         INT32 allocateNewBitPage(UINT32 occupied=0);

         INT32 allocateBits(UINT32 count, UINT32 *buf);

         INT32 releaseBits(UINT32 count, const UINT32 *buf);

      public:/// WARNING: user should ensure there is no changing when dumping.
         INT32 dumpToFile(requestContext *context,
                          const CHAR *fullPath,
                          BOOLEAN onlyWhenModified=TRUE,
                          BOOLEAN replace=TRUE);

         INT32 loadFromFile(requestContext *context,
                            const CHAR *fullPath);

      private:
         INT32 allocateBitsFromHFC(UINT32 count, UINT32 *buf);
         INT32 allocateBitsFromLFC(UINT32 count, UINT32 *buf);

         INT32 releaseBitFromHFC(UINT32 offset, _inMemBitPage *page);
         INT32 releaseBitFromLFC(UINT32 offset, _inMemBitPage *page);
         INT32 releaseAtDestroyedPage(INT32 pageID, UINT32 offset);

      private:
         OSS_INLINE UINT32 getDumpHeadSize()const
         {
            return  sizeof(UINT32)     /// version
                    + sizeof(UINT32) * 4; /// pagecount + flags + _bitCountInPage + free bound
         }

         OSS_INLINE UINT32 getDumpPageHeadSize()const
         {
            return sizeof(UINT32) * 4;/// _pageID + _size + _free + _firstFreeBits
         }

         OSS_INLINE UINT32 getPageDumpSize()const
         {
            
            return ossAlign32(_bitCountInPage) / 8; /// 8bit per byte
         }

         INT32 dumpHead(UINT32 bufferSize, void *buf)const;

         /// the actual dump size is not always be getMaxPageDumpSize.
         /// if free count is zero, it only dump page head to buffer.
         /// if 0 == size when return sdb_ok, means buffer size not enough.
         INT32 dumpPage(UINT32 pageID, UINT32 bufferSize, void *buf, UINT32 &size)const;

      private:
         ossSpinXLatch _mutex;
         UINT32 _flags;
         UINT32 _bitCountInPage;
         UINT32 _freeBound;
         BOOLEAN _modified;

         /// we will not keep bit pages in mem when it's free count is zero.
         /// _pageCount means total count of page we ever allocated.
         UINT32 _pageCount;
         typedef std::map<INT32, _inMemBitPage*> _PAGE_MAP;
         _PAGE_MAP _pagesWithLowFreeCount;
         _PAGE_MAP _pagesWithHighFreeCount;
   };//class inMemBitMap
} /// end of namespace vessel
} /// end of namespace engine
#endif // VESSEL_IN_MEM_BIT_MAP_H_