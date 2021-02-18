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

   Source File Name = dataIDMapStatPage.h

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_DATA_ID_MAP_SPACE_STAT_PAGE_H_
#define VESSEL_DATA_ID_MAP_SPACE_STAT_PAGE_H_

#include "vessel/extentDef.h"

namespace engine
{
namespace vessel
{
   const PAGE_ID DATA_ID_MAP_STAT_PAGE = 1;

   const UINT32 INALID_DATA_ID_MAP_STAT_RECORD_VERSION = 0;
   const UINT32 DATA_ID_MAP_STAT_RECORD_VERSION_1 = 1;

   const UINT32 DATA_ID_MAP_STAT_RECORD_LEN = 1024;

   struct idMapStatRecord
   {
      UINT32 version;
      UINT32 idMapFilePageCount;
   
      OSS_INLINE idMapStatRecord():
      version(INALID_DATA_ID_MAP_STAT_RECORD_VERSION),
      idMapFilePageCount(0)
      {
      }

      void reset()
      {
         version = INALID_DATA_ID_MAP_STAT_RECORD_VERSION;
         idMapFilePageCount = 0;
      }
   };//struct idMapStatRecord
   const UINT32 ID_MAP_STAT_RECORD_LEN = sizeof(idMapStatRecord);
   const UINT32 ID_MAP_STAT_RECORD_ON_DISK_LEN = 1024;

   struct idMapStatRecordOnDisk
   {
      idMapStatRecordOnDisk()
      {
         ossMemset(pad, 0, sizeof(pad));
      }
      idMapStatRecord record;
      CHAR pad[ID_MAP_STAT_RECORD_ON_DISK_LEN-ID_MAP_STAT_RECORD_LEN];
      
   };//struct csMetaRecordOnDisk
}//namespace vessel
}//namespace engine


#endif//VESSEL_DATA_ID_MAP_SPACE_STAT_PAGE_H_