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

   Source File Name = sortedStorageFileList.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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
         BOOLEAN isEmpty()const;
         UINT32 getSize()const;

         void close();

         void destroy();

         void resort();

         INT32 unsortedPushBack(storageFile *file);

         storageFile *getBack();

         const storageFile *getBack()const;

         template <class T>
         T *getBack()
         {
            storageFile *file = getBack();
            return static_cast<T *>(file);
         }

         template <class T>
         const T *getBack()const
         {
            const storageFile *file = getBack();
            return static_cast<const T *>(file);
         }

         storageFile *getFront();

         const storageFile *getFront()const;

         /// new file's sequence must over current back file's sequence.
         INT32 pushBack(storageFile *file);

         void destroyIfLess(UINT64 sequence);

         void truncate(UINT32 minCount);

         /// files must be sorted
         storageFile *findFromBackToFront(UINT64 sequence);

         /// files must be sorted
         storageFile *findFromFrontToBack(UINT64 sequence);

      private:
         typedef ossPoolList<storageFile *> _FILE_LIST;

      public:
         typedef ossPoolList<storageFile *>::const_iterator CONST_ITERATOR;
         CONST_ITERATOR begin()const;
         CONST_ITERATOR end()const;
      private:         
         _FILE_LIST _list;
   };//class sortedStorageFileList
}//namespace vessel
}//namespace engine

#endif//VESSEL_SORTED_STORAGE_FILE_LIST_H_