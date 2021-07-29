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

   Source File Name = sortedStorageFileList.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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
      ossScopedRWLock lock(&_latch, SHARED);
      return _list.empty() ? NULL : _list.back();
   }

   storageFile *sortedStorageFileList::getFront()
   {
      ossScopedRWLock lock(&_latch, SHARED);
      return _list.empty() ? NULL : _list.front();
   }

   static BOOLEAN cmp(const storageFile *l, const storageFile *r)
   {
      SDB_ASSERT(NULL != l, "can not be null");
      SDB_ASSERT(NULL != r, "can not be null");
      return l->getCommonHeadInMem().sequence < r->getCommonHeadInMem().sequence;
   }

   void sortedStorageFileList::resort()
   {
      ossScopedRWLock lock(&_latch, EXCLUSIVE);
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

      {
      ossScopedRWLock lock(&_latch, EXCLUSIVE);
      if (_list.empty())
      {
         _list.push_back(file);
      }
      else if (file->getCommonHeadInMem().sequence <=
               _list.back()->getCommonHeadInMem().sequence)
      {
         PD_LOG(PDERROR, "invalid sequence[%lld], last sequence is[%lld]",
                file->getCommonHeadInMem().sequence,
                _list.back()->getCommonHeadInMem().sequence);
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }
      else
      {
         _list.push_back(file);
      }
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
      SDB_ASSERT(STORAGE_FILE_INVALID_SEQUENCE != sequence, "can not be invalid");
      ossScopedRWLock lock(&_latch, EXCLUSIVE);
      while (!_list.empty())
      {
         storageFile *file = _list.front();
         if (file->getCommonHeadInMem().sequence < sequence)
         {
            PD_LOG(PDINFO, "will destroy file[%s]", file->getFullPath());
            file->destroy();
            SDB_OSS_DEL file;
            _list.pop_front();
         }
      }
      return;
   }

   storageFile *sortedStorageFileList::findFromBackToFront(UINT64 sequence)
   {
      storageFile *file = NULL;
      ossScopedRWLock lock(&_latch, SHARED);
      _FILE_LIST::const_reverse_iterator itr = _list.rbegin();
      for (;itr != _list.rend(); ++itr)
      {
         storageFile *tmp = *itr;
         if (sequence == tmp->getCommonHeadInMem().sequence)
         {
            file = tmp;
            break;
         }
         else if (sequence > tmp->getCommonHeadInMem().sequence)
         {
            break;
         }
      }
      return file;
   }

   storageFile *sortedStorageFileList::findFromFrontToBack(UINT64 sequence)
   {
      storageFile *file = NULL;
      ossScopedRWLock lock(&_latch, SHARED);
      _FILE_LIST::const_iterator itr = _list.begin();
      for (;itr != _list.end(); ++itr)
      {
         storageFile *tmp = *itr;
         if (sequence == tmp->getCommonHeadInMem().sequence)
         {
            file = tmp;
            break;
         }
         else if (sequence < tmp->getCommonHeadInMem().sequence)
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

   BOOLEAN sortedStorageFileList::isEmpty(BOOLEAN lock)
   {
      ossRWMutexBase *mutex = lock ? &_latch : NULL;
      ossScopedRWLock guard(mutex, SHARED);
      return _list.empty();
   }
   UINT32 sortedStorageFileList::getSize(BOOLEAN lock)
   {
      ossRWMutexBase *mutex = lock ? &_latch : NULL;
      ossScopedRWLock guard(mutex, SHARED);
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
