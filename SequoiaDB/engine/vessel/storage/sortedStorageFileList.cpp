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
      return l->getFileNameInMem().getSequence() < r->getFileNameInMem().getSequence();
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
      else if (file->getFileNameInMem().getSequence() <=
               _list.back()->getFileNameInMem().getSequence())
      {
         PD_LOG(PDERROR, "invalid sequence[%lld], last sequence is[%lld]",
                file->getFileNameInMem().getSequence(),
                _list.back()->getFileNameInMem().getSequence());
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
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
      SDB_ASSERT(STORAGE_FILE_INVALID_SEQUENCE != sequence, "can not be invalid");
      while (!_list.empty())
      {
         storageFile *file = _list.front();
         if (file->getFileNameInMem().getSequence() < sequence)
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
         if (sequence == tmp->getFileNameInMem().getSequence())
         {
            file = tmp;
            break;
         }
         else if (sequence > tmp->getFileNameInMem().getSequence())
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
         if (sequence == tmp->getFileNameInMem().getSequence())
         {
            file = tmp;
            break;
         }
         else if (sequence < tmp->getFileNameInMem().getSequence())
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
