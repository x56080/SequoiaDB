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

   Source File Name = sortedStorageFileList.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/sortedStorageFileList.h"
#include "vessel/storageFile.h"
#include "ossLikely.hpp"
#include "ossLatchGuard.hpp"

namespace engine
{
namespace vessel
{
   sortedStorageFileList::~sortedStorageFileList()
   {
      close();
   }

   void sortedStorageFileList::close()
   {
      for (_FILE_LIST::const_iterator itr = _list.begin();
           itr != _list.end(); ++itr)
      {
         if (NULL != *itr)
         {
            (*itr)->close();
            SDB_OSS_DEL (*itr);
         }
      }
      _list.clear();
      return;
   }

   void sortedStorageFileList::destroy()
   {
      for (_FILE_LIST::const_iterator itr = _list.begin();
           itr != _list.end(); ++itr)
      {
         if (NULL != *itr)
         {
            PD_LOG(PDINFO, "will destroy file[%s]", (*itr)->getFullPath());
            (*itr)->destroy();
            SDB_OSS_DEL (*itr);
         }
      }
      _list.clear();
      return;
   }

   storageFile *sortedStorageFileList::getBack()
   {
      return _list.empty() ? NULL : _list.back();
   }

   const storageFile *sortedStorageFileList::getBack()const
   {
      return _list.empty() ? NULL : _list.back();
   }

   storageFile *sortedStorageFileList::getFront()
   {
      return _list.empty() ? NULL : _list.front();
   }

   const storageFile *sortedStorageFileList::getFront()const
   {
      return _list.empty() ? NULL : _list.front();
   }

   static BOOLEAN cmp(const storageFile *l, const storageFile *r)
   {
      SDB_ASSERT(NULL != l, "can not be null");
      SDB_ASSERT(NULL != r, "can not be null");
      return l->getSequence() < r->getSequence();
   }

   void sortedStorageFileList::resort()
   {
      _list.sort(cmp);
   }

   INT32 sortedStorageFileList::pushBack(storageFile *file)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(NULL == file ||
                       !file->isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (_list.empty())
      {
         _list.push_back(file);
      }
      else if (file->getSequence() <= _list.back()->getSequence())
      {
         PD_LOG(PDERROR, "invalid sequence[%lld], last sequence is[%lld]",
                file->getSequence(), _list.back()->getSequence());
         rc = SDB_INVALID_OPERATION;
         goto error;
      }
      else
      {
         _list.push_back(file);
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 sortedStorageFileList::unsortedPushBack(storageFile *file)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(NULL == file ||
                       !file->isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _list.push_back(file);
   done:
      return rc;
   error:
      goto done;
   }

   void sortedStorageFileList::destroyIfLess(UINT64 sequence)
   {
      while (!_list.empty())
      {
         storageFile *file = _list.front();
         if (file->getSequence() < sequence)
         {
            PD_LOG(PDINFO, "will destroy file[%s]", file->getFullPath());
            file->destroy();
            SDB_OSS_DEL file;
            _list.pop_front();
         }
         else
         {
            break;
         }
      }
      return;
   }

   void sortedStorageFileList::truncate(UINT32 minCount)
   {
      UINT32 size = _list.size();
      while (minCount < size)
      {
         storageFile *file = _list.front();
         PD_LOG(PDINFO, "will destroy file[%s]", file->getFullPath());
         file->destroy();
         SDB_OSS_DEL file;
         _list.pop_front();
         --size;
      }
      return;
   }

   storageFile *sortedStorageFileList::findFromBackToFront(UINT64 sequence)
   {
      storageFile *file = NULL;
      _FILE_LIST::const_reverse_iterator itr = _list.rbegin();
      for (;itr != _list.rend(); ++itr)
      {
         storageFile *tmp = *itr;
         if (sequence == tmp->getSequence())
         {
            file = tmp;
            break;
         }
         else if (sequence > tmp->getSequence())
         {
            break;
         }
      }
      return file;
   }

   storageFile *sortedStorageFileList::findFromFrontToBack(UINT64 sequence)
   {
      storageFile *file = NULL;
      _FILE_LIST::const_iterator itr = _list.begin();
      for (;itr != _list.end(); ++itr)
      {
         storageFile *tmp = *itr;
         if (sequence == tmp->getSequence())
         {
            file = tmp;
            break;
         }
         else if (sequence < tmp->getSequence())
         {
            break;
         }
      }
      return file;
   }

   BOOLEAN sortedStorageFileList::isEmpty()const
   {
      return _list.empty();
   }

   UINT32 sortedStorageFileList::getSize()const
   {
      return _list.size();
   }

   sortedStorageFileList::CONST_ITERATOR sortedStorageFileList::begin()const
   {
      return _list.begin();
   }
   
   sortedStorageFileList::CONST_ITERATOR sortedStorageFileList::end()const
   {
      return _list.end();
   }

   
}//namespace vessel
}//namespace engine
