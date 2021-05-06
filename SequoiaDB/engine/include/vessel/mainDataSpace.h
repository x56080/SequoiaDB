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

   Source File Name = mainDataSpace.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_MAIN_DATA_SPACE_H_
#define VESSEL_MAIN_DATA_SPACE_H_

#include "vessel/logicalPageSpace.h"
#include "vessel/collectionSpaceGlobalPage.h"
#include "vessel/vesselOptions.h"

namespace engine
{
namespace vessel
{
   constexpr UINT32 MD_SPACE_SYSTEM_PAGE_COUNT = 1;
   constexpr UINT32 MD_SPACE_RESERVED_IMP_COUNT = 1;

   class mainDataSpace : public logicalPageSpace
   {
      public:
         mainDataSpace();
         virtual ~mainDataSpace();

      public:
         BOOLEAN isOpen()const;

         INT32 create(requestContext *context,
                      storageUnit *su,
                      const strSlice &csName,
                      UINT32 logicalID,
                      const createCSOptions &options);

      public:
         INT32 getLpidOfCollectionRecord(CL_MB_ID mbID, PAGE_ID &lpid)const;
         PAGE_ID getSystemImpPid()const;
         INT32 readGlobalMetaData(requestContext *context,
                                  BOOLEAN cacheMode,
                                  csMetaRecord &record);

      public:
         virtual INT32 allocatePages(requestContext *context,
                                     PAGE_TYPE pageType,
                                     UINT32 count,
                                     const PAGE_ID *lpids,
                                     const PAGE_ID *pids,
                                     const slice &args,
                                     DPS_LSN_OFFSET *oplist);

         virtual INT32 releasePages(requestContext *context,
                                    UINT32 count,
                                    const PAGE_ID *lpids,
                                    DPS_LSN_OFFSET oplist);

         virtual INT32 getPhysicalPid(requestContext *context,
                                      PAGE_ID lpid,
                                      PAGE_ID &pid,
                                      SNAPSHOT_ID *snap);

         virtual INT32 getPhysicalPidToWrite(requestContext *context,
                                             PAGE_ID lpid,
                                             PAGE_ID &pid,
                                             SNAPSHOT_ID *snap);

         virtual INT32 copyOnWirte(requestContext *context,
                                   PAGE_ID lpid,
                                   PAGE_ID oldPid,
                                   PAGE_ID &newPid,
                                   SNAPSHOT_ID *snap);

      private:
         PAGE_ID getGlobalMetaPid()const;
         PAGE_ID getDataSmpPid(PAGE_ID pid)const;

      private:
         virtual UINT32 getSystemPageCount()const
         {
            return MD_SPACE_SYSTEM_PAGE_COUNT;
         }
         virtual UINT32 getReservedImpCount()const 
         {
            return MD_SPACE_RESERVED_IMP_COUNT;
         }
         virtual FILE_TYPE getTypeOfMetaFile()const
         {
            return FILE_TYPE_DM;
         }
         virtual FILE_TYPE getTypeOfDataFile()const
         {
            return FILE_TYPE_DD;
         }
         virtual UINT32 getFreeBoundOfLpidPool()const
         {
            return PAGE_COUNT_IN_EXTENT;
         }
         virtual UINT32 getFreeBoundOfPpidPool()const
         {
            return PAGE_COUNT_IN_EXTENT;
         }
      private:
         virtual INT32 allocateIdMapPagesOnDisk(requestContext *context,
                                                PAGE_ID first,
                                                UINT32 count);
         virtual INT32 createDataFile(requestContext *context,
                                      UINT64 sequence);
         virtual UINT32 getDataFileCount();
         virtual INT32 getDataSMPOfFile(UINT32 sequence, UINT32 i, PAGE_ID &pid);

      private:
         INT32 allocateNewIMPInSMP(requestContext *context,
                                   PAGE_ID imp,
                                   PAGE_ID pid,
                                   UINT32 count);

         INT32 allocateNewDataPagesOnSMP(requestContext *context,
                                         PAGE_ID smpPid,
                                         PAGE_TYPE type,
                                         UINT32 count,
                                         const PAGE_ID *lpids,
                                         const PAGE_ID *pids,
                                         const slice &args,
                                         DPS_LSN_OFFSET *oplist);

         INT32 releaseDataPagesOnSMP(requestContext *context,
                                     UINT32 count,
                                     const PAGE_ID *pids,
                                     DPS_LSN_OFFSET oplist=DPS_INVALID_LSN_OFFSET,
                                     BOOLEAN oplistTail=FALSE);

         INT32 mapNewPagesToIdMap(requestContext *context,
                                  PAGE_ID imp,
                                  UINT32 count,
                                  const PAGE_ID *lpids,
                                  const PAGE_ID *pids,
                                  DPS_LSN_OFFSET oplist=DPS_INVALID_LSN_OFFSET,
                                  BOOLEAN oplistTail=FALSE);

      private:
         
         INT32 initNecessaryPagesWhenCreating(requestContext *context,
                                              storageUnit *su,
                                              const strSlice &csName,
                                              UINT32 logicalID,
                                              UINT32 uniqueID);
         INT32 updateStatusToOnlineWhenCreating(requestContext *context);
   };//class mainDataSpace
}//namespace vessel
}//namespace engine

#endif//VESSEL_MAIN_DATA_SPACE_H_