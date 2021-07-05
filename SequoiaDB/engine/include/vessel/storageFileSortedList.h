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

   Source File Name = storageFileSortedList.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_STORAGE_FILE_SORTED_LIST_H_
#define VESSEL_STORAGE_FILE_SORTED_LIST_H_

#include "ossMemPool.hpp"

namespace engine
{
namespace vessel
{
   class storageFile;
   class storageFileSortedList : public SDBObject
   {
      public:
         storageFileSortedList();
         ~storageFileSortedList();
         storageFileSortedList(const storageFileSortedList &) = delete;
         storageFileSortedList &operator=(const storageFileSortedList &) = delete;

      public:
         OSS_INLINE BOOLEAN isEmpty()const
         {
            return _list.empty();
         }
         OSS_INLINE UINT32 getSize()const
         {
            return _list.size();
         }

      public:
         void fini();
         INT32 insert(storageFile *f);

         /// will not dump closed file name if list is null.
         void closeIfLess(UINT64 sequence,
                          FILE_NAME_LIST *list);

         void destroyIfLess(UINT64 sequence);

         void destroyIfGreater(UINT64 sequence);

         void destroy();

         BOOLEAN hasSequenceBreakpoint()const;

         storageFile *getFirst()const;

         ///return null if empty
         storageFile *getLast()const;

         ///return null if not exists
         storageFile *searchFromFront(UINT64 sequence)const;

         ///return null if not exists
         storageFile *searchFromBack(UINT64 sequence)const;

      private:
         typedef ossPoolList<storageFile *> _FILE_LIST;

      public:
         typedef _FILE_LIST::iterator ITERATOR;
         typedef _FILE_LIST::reverse_iterator REVERSE_ITERATOR;

         ITERATOR begin()
         {
            return _list.begin();
         }

         ITERATOR end()
         {
            return _list.end();
         }

         REVERSE_ITERATOR rbegin()
         {
            return _list.rbegin();
         }
         REVERSE_ITERATOR rend()
         {
            return _list.rend();
         }

      private:
         _FILE_LIST _list;

   };//class storageFileSortedList
}//namespace vessel
}//namespace engine

#endif//VESSEL_STORAGE_FILE_SORTED_LIST_H_