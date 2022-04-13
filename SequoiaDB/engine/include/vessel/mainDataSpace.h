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
#include "vessel/dataStorageFileCluster.h"
#include "vessel/csMetaBlockPage.h"

namespace engine
{
namespace vessel
{
   class fsmFile;
   class logRecordContext;

   class mainDataSpace : public logicalPageSpace
   {
      public:
         mainDataSpace();
         virtual ~mainDataSpace();

      public:
         virtual SPACE_TYPE getSpaceType()const override
         {
            return SPACE_TYPE_MAIN_DATA;
         }
         virtual BOOLEAN isCopyOnWrite()const override
         {
            return FALSE;
         }

      public:
         INT32 initMetaPageWhenCreateCS(requestContext *context,
                                        const csMetaBlock &block,
                                        const slice &options);

         INT32 readMetaBlockWhenOpen(requestContext *context,
                                     csMetaBlock &block);

         INT32 updateCSMetaBlock(requestContext *context,
                                 const csMetaBlock &block,
                                 UINT64 updateMask);

         fsmFile *getFsmFile()
         {
            return _fsm;
         }
      private:
         virtual UINT32 getReservedImpCount()const override
         {
            return 1;
         }
         virtual dataPageCluster *getDataStorageObj() override
         {
            return &_storage;
         }

         virtual INT32 _create() override;
         virtual INT32 _open(const storageFileLoader &loader) override;
         virtual void _close() override;
         virtual void _destroy() override;

      private:
         virtual INT32 getRuntimePageBuffer(requestContext *context,
                                            PAGE_ID pid,
                                            const ossSharedLatchMode &mode,
                                            runtimePageBuffer &rpb) override;

         virtual INT32 getRuntimePageBufferToReset(requestContext *context,
                                                   PAGE_ID pid,
                                                   runtimePageBuffer &rpb) override;

         virtual INT32 copyPageAndReinitBuffer(requestContext *context,
                                               PAGE_SNAPSHOT_VERION psv,
                                               PAGE_ID newPid,
                                               runtimePageBuffer &rpb) override;

      private:
         INT32 prepareCopyLog(requestContext *context,
                              UINT32 pageSize,
                              logRecordContext *lrc);

         INT32 commit(requestContext *context,
                        UINT32 pageSize,
                        const void *pageBuffer,
                        const GLOBAL_PAGE_ID &gpid,
                        PAGE_ID lpid,
                        logRecordContext *lrc);

      private:
         dataStorageFileCluster _storage;
         fsmFile *_fsm = NULL;
   };//class mainDataSpace
}//namespace vessel
}//namespace engine

#endif//VESSEL_MAIN_DATA_SPACE_H_