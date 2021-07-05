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

   Source File Name = dataStorageCluster.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_DATA_STORAGE_CLUSTER_H_
#define VESSEL_DATA_STORAGE_CLUSTER_H_

#include "ossLatch.hpp"
#include "vessel/vesselFileDef.h"
#include "ossMemPool.hpp"
#include "vessel/storageFileDef.h"
#include "vessel/strSlice.h"
#include "vessel/pageDef.h"


namespace engine
{
namespace vessel
{
   class storageFile;

   class dataStorageCluster : public SDBObject
   {
      public:
         dataStorageCluster();
         ~dataStorageCluster();
         dataStorageCluster(const dataStorageCluster &) = delete;
         dataStorageCluster &operator=(const dataStorageCluster &) = delete;

      public:
         OSS_INLINE const storageCoreArgs &getInMemDataCoreArgs()const
         {
            return _dataArgs;
         }
         BOOLEAN isOpen()const
         {
            return _dataArgs.isValid();
         }

      public:
         
         INT32 open(const storageCoreArgs &args);
         void close();
         void destroy();

         INT32 depositDataStorageFile(storageFile *file);
         INT32 getDataStorageFile(UINT64 sequence, storageFile **file);

         /// not thread safe
         void removeDataStorageFilesOverCount(UINT32 count);

         INT32 getPagePtr(PAGE_ID pid, ossValuePtr &ptr);

         UINT32 getDataStorageFileCount();

      public:
         INT32 fsyncPages(UINT32 count,
                          PAGE_ID first,
                          BOOLEAN sync);

      private:
         void _close();
      private:
         typedef ossPoolVector<storageFile *> _FILE_VEC;
      private:
         ossSpinSLatch _latch;
         _FILE_VEC _dataFiles;
         storageCoreArgs _dataArgs;
   };//class dataStorageCluster
}//namespace vessel
}//namespace engine

#endif//VESSEL_STORAGE_FILE_CLUSTER_H_