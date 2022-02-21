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

   Source File Name = storageFileMap.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/storageFileMap.h"
#include "ossLikely.hpp"
#include "vessel/storageFile.h"

namespace engine
{
namespace vessel
{
   storageFileMap::~storageFileMap()
   {
      fini();
   }

   void storageFileMap::fini()
   {
      _FILE_MAP::iterator itr = _map.begin();
      for (; itr != _map.end(); ++itr)
      {
         if (OSS_LIKELY(NULL != itr->second))
         {
            storageFile *file = itr->second;
            file->close();
            SDB_OSS_DEL file;
            itr->second = NULL;
         }
      }
      _map.clear();
      return;
   }

   INT32 storageFileMap::insertFile(storageFile *file)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(NULL == file ||
                       !file->isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!_map.insert(std::make_pair(file->getSequence(), file)).second)
      {
         rc = SDB_VESSEL_DUPLICATED_KEY;
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   storageFile *storageFileMap::getFirst()const
   {
      return _map.begin() == _map.end() ? NULL : _map.begin()->second;
   }

   storageFile *storageFileMap::getLast()const
   {
      return _map.rbegin() == _map.rend() ? NULL : _map.rbegin()->second;
   }

   storageFile *storageFileMap::get(UINT64 s)const
   {
      storageFile *file = NULL;
      _FILE_MAP::const_iterator itr = _map.find(s);
      if (itr != _map.end())
      {
         file = itr->second;
      }
      return file;
   }

   void storageFileMap::destroy(UINT64 sequence)
   {
      storageFile *file = NULL;
      _FILE_MAP::const_iterator itr = _map.find(sequence);
      if (_map.end() != itr)
      {
         file = itr->second;
         _map.erase(itr);
         PD_LOG(PDINFO, "will destroy file[%s]", file->getFullPath());
         file->destroy();
         SDB_OSS_DEL file;
      }
      return;
   }

   void storageFileMap::destroyIfGreater(UINT64 sequence)
   {
      while (!_map.empty())
      {
         _FILE_MAP::const_reverse_iterator itr = _map.rbegin();
         if (sequence < itr->first)
         {
            storageFile *file = itr->second;
            PD_LOG(PDINFO, "will destroy file[%s] which sequence over[%lld]",
                   file->getFullPath(), sequence);
            _map.erase(itr.base());
            file->destroy();
            SDB_OSS_DEL file;
         }
         else
         {
            break;
         }
      }
      return;
   }

   void storageFileMap::destroy()
   {
      _FILE_MAP::const_iterator itr = _map.begin();
      for (; itr != _map.end(); ++itr)
      {
         storageFile *file = itr->second;
         PD_LOG(PDINFO, "will destroy file[%s]", file->getFullPath());
         file->destroy();
         SDB_OSS_DEL file;
      }
      _map.clear();
      return;
   }

   BOOLEAN storageFileMap::hasSequenceBreakpoint()const
   {
      BOOLEAN r = FALSE;
      UINT64 lastSequence = 0;
      _FILE_MAP::const_iterator itr = _map.begin();

      if (_map.end() == itr)
      {
         goto done;
      }

      lastSequence = itr->first;
      ++itr;
      for (; itr != _map.end(); ++itr)
      {
         if ((lastSequence + 1) != itr->first)
         {
            PD_LOG(PDINFO, "breakpoint found after between[%lld,%lld]",
                   lastSequence, itr->first);
            r = TRUE;
            goto done;
         }
         lastSequence = itr->first;
      }

   done:
      return r;
   }
}//namespace vessel
}//namespace engine