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

   Source File Name = indexSpace.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_INDEX_SPACE_H_
#define VESSEL_INDEX_SPACE_H_

#include "vessel/logicalPageSpace.h"

namespace engine
{
namespace vessel
{  
   class copyOnWriteSpaceEnv;
   class idxDataFile;

   class indexSpace : public logicalPageSpace
   {
      public:
         indexSpace(){}
         virtual ~indexSpace();

      public:
         INT32 mapNewLpids(requestContext *context,
                           UINT32 count,
                           const PAGE_ID *lpids,
                           const PAGE_ID *pids);

      private:
         virtual UINT32 getSystemPageCount()const {return 0;}
         virtual UINT32 getReservedImpCount()const {return 2;}
         virtual FILE_TYPE getTypeOfMetaFile()const{return FILE_TYPE_IDX_M;}
         virtual FILE_TYPE getTypeOfDataFile()const{return FILE_TYPE_IDX_D;}
         virtual UINT32 getFreeBoundOfLpidPool()const{return 0;}
         virtual UINT32 getFreeBoundOfPpidPool()const{return 0;}

      private:
         virtual INT32 allocateIdMapPagesOnDisk(requestContext *context,
                                                PAGE_ID first,
                                                UINT32 count);

         virtual INT32 createDataFile(requestContext *context,
                                      UINT64 sequence);

         virtual UINT32 getDataFileCount();

         virtual INT32 getDataSMPOfFile(UINT32 sequence, UINT32 i, PAGE_ID &pid);

      private:
         INT32 ensureDataFile(requestContext *context,
                              UINT64 sequence,
                              idxDataFile **out);

         INT32 getMaxDataFileCount(UINT32 &cnt)const;

      private:
         copyOnWriteSpaceEnv *_env = NULL;
   };//class indexSpace
}//namespace vessel
}//namespace engine

#endif//VESSEL_INDEX_SPACE_H_