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

   Source File Name = dataStorageFileCluster.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_DATA_STORAGE_FILE_CLUSTER_H_
#define VESSEL_DATA_STORAGE_FILE_CLUSTER_H_

#include "vessel/dataPageCluster.h"
#include "vessel/keepHistoryPointerArray.h"

namespace engine
{
namespace vessel
{
   class storageFile;

   class dataStorageFileCluster : public dataPageCluster
   {
      public:
         dataStorageFileCluster();
         virtual ~dataStorageFileCluster();

      public:
         virtual INT32 fsyncSegment(UINT32 globalSegmentId)const;

         virtual INT32 fysncPage(PAGE_ID pid)const;

         virtual INT32 getDataPagePtr(PAGE_ID pid, mmapPagePointer &ptr)const;

         virtual FILE_TYPE getDataFileType()const
         {
            return FILE_TYPE_DATA_STORAGE;
         }
         virtual UINT32 getTotalSegmentCountAllocated()const;

      private:
         virtual INT32 openFiles(const storageFileLoader *loader);
         virtual void closeFiles();
         virtual void destroyFiles();

         virtual INT32 allocateNewSegment();
         virtual INT32 ensureSegmentNotSparse(UINT32 globalSegmentId);
         virtual INT32 isSparseSegment(UINT32 globalSegmentId,
                                       BOOLEAN &isSparse)const;

         virtual BOOLEAN mayBeSparse()const {return TRUE;}

         virtual BOOLEAN hasSparseFile()const {return FALSE;}

      private:
         INT32 createNewFile();

         INT32 createFileEverShrinked(UINT32 sequence);

         void _close();

         UINT32 getFileIdByGlobalSegmentId(UINT32 globalSegment,
                                           UINT32 *segmentInFile)const;
         UINT32 getFileIdByGlobalPageId(PAGE_ID pid,
                                        PAGE_ID *pidInFile)const;
      private:
         keepHistoryPointerArray _files;

   };//class dataStorageFileCluster

}//namespace vessel
}//namespace engine

#endif//VESSEL_DATA_STORAGE_FILE_CLUSTER_H_