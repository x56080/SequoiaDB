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
#include "vessel/dataStorageFileCluster.h"

namespace engine
{
namespace vessel
{  
   class indexSpace : public logicalPageSpace
   {
      public:
         indexSpace(){}
         virtual ~indexSpace(){}

      public:
         virtual SPACE_TYPE getSpaceType()const override
         {
            return SPACE_TYPE_IDX;
         }
         virtual BOOLEAN isCopyOnWrite()const override
         {
            return TRUE;
         }

      protected:
         virtual dataPageCluster *getDataStorageObj() override {return &_storage;}

      private:
         virtual UINT32 getReservedImpCount()const override {return 1;};
         virtual dataPageCluster::options getStorageOptions()const override;

      private:
         virtual INT32 getRuntimePageBuffer(requestContext *context,
                                            PAGE_ID pid,
                                            const ossSharedLatchMode &mode,
                                            runtimePageBuffer &rpb) override;

         /// rpb must be writable at last
         virtual INT32 getRuntimePageBufferToReset(requestContext *context,
                                                   PAGE_ID pid,
                                                   runtimePageBuffer &rpb) override;

         /// rpb must be writable at last
         virtual INT32 copyPageAndReinitBuffer(requestContext *context,
                                               PAGE_SNAPSHOT_VERION psv,
                                               PAGE_ID newPid,
                                               runtimePageBuffer &rpb) override;

      private:
         virtual INT32 _create() override;
         virtual INT32 _open(const storageFileLoader &loader) override;
         virtual void _close() override;
         virtual void _destroy() override;

      private:
         virtual INT32 getMinUncompletedLSN(requestContext *context,
                                            DPS_LSN_OFFSET &lsn) override;

      private:
         dataStorageFileCluster _storage;
   };//class indexSpace
}//namespace vessel
}//namespace engine

#endif//VESSEL_INDEX_SPACE_H_