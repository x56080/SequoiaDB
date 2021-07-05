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

   Source File Name = storageFileSortedList.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/storageFileSortedList.h"
#include "vessel/storageFile.h"

namespace engine
{
namespace vessel
{
   storageFileSortedList::storageFileSortedList()
   {}

   storageFileSortedList::~storageFileSortedList()
   {
      fini();
   }

   void storageFileSortedList::fini()
   {
      _FILE_LIST::iterator itr = _list.begin();
      for (; itr != _list.end(); ++itr)
      {
         if (OSS_LIKELY(NULL != *itr))
         {
            (*itr)->close();
            SDB_OSS_DEL (*itr);
         }
      }
      _list.clear();
      return;
   }

   INT32 storageFileSortedList::insert(storageFile *f)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(NULL == f ||
                       !f->isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (_list.empty())
      {
         _list.push_back(f);
      }
      else if (f->getCommonHeadInMem().sequence >
               _list.back()->getCommonHeadInMem().sequence)
      {
         _list.push_back(f);
      }
      else
      {
         UINT64 sequence = f->getCommonHeadInMem().sequence;
         _FILE_LIST::iterator itr = _list.begin();
         for (; itr != _list.end(); ++itr)
         {
            if (sequence < (*itr)->getCommonHeadInMem().sequence)
            {
               _list.insert(itr, f);
               goto done;
            }
            else if (sequence == (*itr)->getCommonHeadInMem().sequence)
            {
               rc = SDB_FE;
               goto error;
            }
            else
            {
               continue;
            }
         }
         SDB_ASSERT(FALSE, "impossible");
      }
   done:
      return rc;
   error:
      goto done;
   }

   storageFile *storageFileSortedList::getFirst()const
   {
      return _list.empty() ? NULL : _list.front();
   }

   storageFile *storageFileSortedList::getLast()const
   {
      return _list.empty() ? NULL : _list.back();
   }

   storageFile *storageFileSortedList::searchFromFront(UINT64 sequence)const
   {
      storageFile *out = NULL;
      _FILE_LIST::const_iterator itr = _list.begin();
      for (; itr != _list.end(); ++itr)
      {
         if ((*itr)->getCommonHeadInMem().sequence == sequence)
         {
            out = *itr;
            goto done;
         }
      }
   done:
      return out;
   }

   storageFile *storageFileSortedList::searchFromBack(UINT64 sequence)const
   {
      storageFile *out = NULL;
      _FILE_LIST::const_reverse_iterator itr = _list.rbegin();
      for (; itr != _list.rend(); ++itr)
      {
         if ((*itr)->getCommonHeadInMem().sequence == sequence)
         {
            out = *itr;
            goto done;
         }
      }
   done:
      return out;
   }

   void storageFileSortedList::closeIfLess(UINT64 sequence,
                                           FILE_NAME_LIST *list)
   {
      SDB_ASSERT(STORAGE_FILE_INVALID_SEQUENCE != sequence, "can not be invalid");
      while (!_list.empty())
      {
         storageFile *file = _list.front();
         if (file->getCommonHeadInMem().sequence < sequence)
         {
            PD_LOG(PDINFO, "will close file[%s] which sequence[%lld] less than[%lld]",
                   file->getFullPath(), file->getCommonHeadInMem().sequence, sequence);
            
            if (NULL != list)
            {
               vesselFileName fn;
               BOOLEAN r = fn.build(file->getCommonHeadInMem().spaceID,
                                    file->getCommonHeadInMem().fileType,
                                    file->getCommonHeadInMem().spaceType,
                                    file->getCommonHeadInMem().sequence);
               list->push_back(fn);
               
            }
            file->close();
         }
      }
      return;
   }

   void storageFileSortedList::destroyIfLess(UINT64 sequence)
   {
      SDB_ASSERT(STORAGE_FILE_INVALID_SEQUENCE != sequence, "can not be invalid");
      while (!_list.empty())
      {
         storageFile *file = _list.front();
         if (file->getCommonHeadInMem().sequence < sequence)
         {
            PD_LOG(PDINFO, "will destroy file[%s] which sequence[%lld] less than[%lld]",
                   file->getFullPath(), file->getCommonHeadInMem().sequence, sequence);
            file->destroy();
            SDB_OSS_DEL file;
            _list.pop_front();
         }
         else
         {
            goto done;
         }
      }
   done:
      return;
   }

   void storageFileSortedList::destroyIfGreater(UINT64 sequence)
   {
      SDB_ASSERT(STORAGE_FILE_INVALID_SEQUENCE != sequence, "can not be invalid");
      while (!_list.empty())
      {
         storageFile *file = _list.back();
         if (sequence < file->getCommonHeadInMem().sequence)
         {
            PD_LOG(PDINFO, "will destroy file[%s] which sequence[%lld] greater than[%lld]",
                   file->getFullPath(), file->getCommonHeadInMem().sequence, sequence);
            file->destroy();
            SDB_OSS_DEL file;
            _list.pop_back();
         }
         else
         {
            goto done;
         }
      }
   done:
      return;
   }

   void storageFileSortedList::destroy()
   {
      while (!_list.empty())
      {
         storageFile *file = _list.front();
         PD_LOG(PDINFO, "will destroy file[%s]", file->getFullPath());
         file->destroy();
         SDB_OSS_DEL file;
         _list.pop_front();
      }
      return;
   }

   BOOLEAN storageFileSortedList::hasSequenceBreakpoint()const
   {
      BOOLEAN r = FALSE;
      UINT64 lastSequence = 0;
      _FILE_LIST::const_iterator itr = _list.begin();

      if (_list.end() == itr)
      {
         goto done;
      }

      lastSequence = (*itr)->getCommonHeadInMem().sequence;
      ++itr;
      for (; itr != _list.end(); ++itr)
      {
         UINT64 s = (*itr)->getCommonHeadInMem().sequence;
         if ((lastSequence + 1) != s)
         {
            r = FALSE;
            goto done;
         }
         lastSequence = s;
      }

   done:
      return r;
   }
}//namespace vessel
}//namespace engine