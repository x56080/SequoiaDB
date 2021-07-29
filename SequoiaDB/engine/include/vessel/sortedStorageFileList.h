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

   Source File Name = sortedStorageFileList.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_SORTED_STORAGE_FILE_LIST_H_
#define VESSEL_SORTED_STORAGE_FILE_LIST_H_

#include "ossMemPool.hpp"
#include "ossRWMutex.hpp"

namespace engine
{
namespace vessel
{
   class storageFile;

   class sortedStorageFileList : public SDBObject
   {
      public:
         sortedStorageFileList(){}
         ~sortedStorageFileList();
         sortedStorageFileList(const sortedStorageFileList &) = delete;
         sortedStorageFileList &operator=(const sortedStorageFileList &) = delete;

      public:
         /// not thread safe
         BOOLEAN isEmpty()const;

         BOOLEAN isEmpty(BOOLEAN lock);
         UINT32 getSize(BOOLEAN lock);

         /// not thread safe
         void close();

         /// not thread safe
         void destroy();

         /// not thread safe
         void resort();

         /// not thread safe
         INT32 unsortedPushBack(storageFile *file);

         /// thread safe
         storageFile *getBack();

         /// thread safe
         storageFile *getFront();

         /// thread safe
         /// new file's sequence must over current back file's sequence.
         INT32 pushBack(storageFile *file);

         /// thread safe
         void destroyIfLess(UINT64 sequence);

         /// thread safe
         /// files must be sorted
         storageFile *findFromBackToFront(UINT64 sequence);

         /// thread safe
         /// files must be sorted
         storageFile *findFromFrontToBack(UINT64 sequence);

      private:
         typedef ossPoolList<storageFile *> _FILE_LIST;

      public:
         /// not thread safe
         typedef ossPoolList<storageFile *>::const_iterator CONST_ITERATOR;
         CONST_ITERATOR begin()const;
         CONST_ITERATOR end()const;
      private:
         ossRWMutex _latch;         
         _FILE_LIST _list;
   };//class sortedStorageFileList
}//namespace vessel
}//namespace engine

#endif//VESSEL_SORTED_STORAGE_FILE_LIST_H_