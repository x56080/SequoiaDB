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

   Source File Name = dataStorageCluster.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/dataStorageCluster.h"
#include "pdTrace.hpp"
#include "utilStr.hpp"
#include "vessel/storageFile.h"

namespace engine
{
namespace vessel
{
   dataStorageCluster::dataStorageCluster()
   {}

   dataStorageCluster::~dataStorageCluster()
   {
      close();
   }

   void dataStorageCluster::close()
   {
      _dataArgs.reset();
      for (_FILE_VEC::iterator itr = _dataFiles.begin();
           itr != _dataFiles.end(); ++itr)
      {
         storageFile *f = *itr;
         if (NULL != f)
         {
            f->close();
            SDB_OSS_DEL f;
         }
      }
      _dataFiles.clear();
      return;
   }

   void dataStorageCluster::destroy()
   {
      for (UINT32 i = 0; i < _dataFiles.size(); ++i)
      {
         storageFile *f = _dataFiles[i];
         if (NULL != f)
         {
            f->destroy();
            SDB_OSS_DEL f;
         }
      }
      _dataFiles.clear();
      _dataArgs.reset();
      return;
   }

   INT32 dataStorageCluster::open(const storageCoreArgs &args)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!isOpen(), "do not reinit");
      if (OSS_UNLIKELY(isOpen()))
      {
         close();
      }

      if (OSS_UNLIKELY(!args.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _dataArgs = args;

   done:
      return rc;
   error:
      goto done;
   }

   INT32 dataStorageCluster::depositDataStorageFile(storageFile *file)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == file ||
                            !file->isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(file->getCommonHeadInMem().fileType !=
                            FILE_TYPE_DATA_STORAGE))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(file->getCommonHeadInMem().pageSize != _dataArgs.pageSize ||
                            file->getCommonHeadInMem().maxPageCountPerSeg != _dataArgs.maxPageCountPerSeg ||
                            file->getCommonHeadInMem().maxSegmentCountPerFile != _dataArgs.maxSegmentCountPerFile))
      {
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      {
      ossScopedLock guard(&_latch, EXCLUSIVE);
      if (_dataFiles.size() <= file->getCommonHeadInMem().sequence)
      {
         _dataFiles.resize(file->getCommonHeadInMem().sequence, NULL);
         _dataFiles[file->getCommonHeadInMem().sequence] = file;
      }
      else if (NULL != _dataFiles.at(file->getCommonHeadInMem().sequence))
      {
         PD_LOG(PDERROR, "file with sequence[%lld] already exists",
                file->getCommonHeadInMem().sequence);
         rc = SDB_FE;
         goto error;
      }
      else
      {
         _dataFiles[file->getCommonHeadInMem().sequence] = file;
      }
      }

   done:
      return rc;
   error:
      goto done;
   }

   void dataStorageCluster::removeDataStorageFilesOverCount(UINT32 count)
   {
      while (count < _dataFiles.size())
      {
         storageFile *file = _dataFiles.back();
         file->destroy();
         SDB_OSS_DEL file;
         _dataFiles.pop_back();
      }

   done:
      return;
   }

   INT32 dataStorageCluster::getDataStorageFile(UINT64 sequence, storageFile **file)
   {
      INT32 rc = SDB_OK;
      storageFile *f = NULL;
      ossScopedLock guard(&_latch, SHARED);
   
      if (_dataFiles.size() <= sequence)
      {
         rc = SDB_FNE;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == file))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      f = _dataFiles.at(sequence);
      if (NULL == f)
      {
         rc = SDB_FNE;
         goto error;
      }

      *file = f;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 dataStorageCluster::getPagePtr(PAGE_ID pid, ossValuePtr &ptr)
   {
      INT32 rc = SDB_OK;
      PAGE_ID pidInFile = INVALID_PAGE_ID;
      UINT64 i = 0;
      UINT32 totalPageCountInFile = 0;
      storageFile *file = NULL;

      if (OSS_UNLIKELY(INVALID_PAGE_ID == pid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      totalPageCountInFile = _dataArgs.getMaxPageCountInFile();
      i = pid / totalPageCountInFile;
      pidInFile = (pid & (totalPageCountInFile - 1));
      rc = getDataStorageFile(i, &file);
      if (SDB_OK != rc)
      {
         goto error;
      }
      
      rc = file->getPagePtr(pidInFile, ptr);
      if (SDB_OK != rc)
      {
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   UINT32 dataStorageCluster::getDataStorageFileCount()
   {
      ossScopedLock guard(&_latch, SHARED);
      return _dataFiles.size();
   }

   INT32 dataStorageCluster::fsyncPages(UINT32 count,
                                        PAGE_ID first,
                                        BOOLEAN sync)
   {
      INT32 rc = SDB_OK;
      UINT32 totalPageCountPerFile = 0;
      UINT32 unflushed = count;
      PAGE_ID pid = first;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(0 == count ||
                            INVALID_PAGE_ID == first))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      totalPageCountPerFile = _dataArgs.getMaxPageCountInFile();

      do
      {
         UINT64 i = pid / totalPageCountPerFile;
         PAGE_ID pidInFile = (pid & (totalPageCountPerFile - 1));
         UINT32 pageCount = 0;
         storageFile *file = NULL;
         if ((pidInFile + unflushed) <= totalPageCountPerFile)
         {
            pageCount = unflushed;
         }
         else
         {
            pageCount = totalPageCountPerFile - pidInFile;
         }

         rc = getDataStorageFile(i, &file);
         if (SDB_OK != rc)
         {
            goto error;
         }

         rc = file->fsync(pidInFile, pageCount, sync);
         if (SDB_OK != rc)
         {
            goto error;
         }

         unflushed -= pageCount;
         pid += pageCount;

      } while (0 < unflushed);
      
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine