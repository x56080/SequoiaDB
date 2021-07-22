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

#include "vessel/replicatedLPS.h"
#include "vessel/dataStorageFileCluster.h"
#include "vessel/collectionSpaceOptions.h"
#include "vessel/collectionSpaceGlobalPage.h"

namespace engine
{
namespace vessel
{
   class fsmFile;

   class mainDataSpace : public replicatedLPS
   {
      public:
         mainDataSpace();
         virtual ~mainDataSpace();

      public:
         virtual SPACE_TYPE getSpaceType()const
         {
            return SPACE_TYPE_MAIN_DATA;
         }

      public:
         INT32 initMetaPageWhenCreateCS(requestContext *context,
                                        const csMetaRecord &record,
                                        const slice &options);

         INT32 readMetaRecordWhenOpen(requestContext *context,
                                      csMetaRecord &record);

         INT32 ensureNameFile(const CHAR *csName)const;

         fsmFile *getFsmFile()
         {
            return _fsm;
         }
      private:
         virtual UINT32 getReservedImpCount()const
         {
            return 1;
         }
         virtual UINT32 getFreeBoundOfLpidAllocator()const 
         {
            return PAGE_COUNT_IN_EXTENT;
         }
         virtual UINT32 getFreeBoundOfPageStorage()const
         {
            return PAGE_COUNT_IN_EXTENT;
         }
         virtual dataPageCluster *getDataStorageObj()
         {
            return &_storage;
         }

         virtual INT32 _create(requestContext *context);
         virtual INT32 _open(requestContext *context,
                             const storageFileLoader &loader);
         virtual void _close();
         virtual void _destroy();

      private:
         INT32 mapGlobalMetaPageWhenCreating(PAGE_SNAPSHOT_VERION psv,
                                             PAGE_ID lpid,
                                             PAGE_ID pid);

         INT32 ensureNameFileRemoved();

      private:
         dataStorageFileCluster _storage;
         fsmFile *_fsm = NULL;
   };//class mainDataSpace
}//namespace vessel
}//namespace engine

#endif//VESSEL_MAIN_DATA_SPACE_H_