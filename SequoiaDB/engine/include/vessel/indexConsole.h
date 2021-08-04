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

   Source File Name = indexConsole.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_INDEX_CONSOLE_H_
#define VESSEL_INDEX_CONSOLE_H_

#include "vessel/indexOptions.h"
#include "vessel/indexKeyPattern.h"
#include "vessel/slice.h"
#include "vessel/strSlice.h"

namespace engine
{
namespace vessel
{
   struct collectionRecord;
   class indexSpace;
   class requestContext;

   class indexConsole : public SDBObject
   {
      public:
         indexConsole(){}
         indexConsole(const indexConsole &) = delete;
         indexConsole &operator=(const indexConsole &) = delete;
         ~indexConsole(){}

      public:
         OSS_INLINE BOOLEAN isInitialized()const
         {
            return NULL != _record;
         }
      public:
         INT32 init(const collectionRecord *record,
                    indexSpace *is);

         INT32 allocateIndexSlot(INT32 &indexSlot)const;

         INT32 testIfDuplicated(requestContext *context,
                                const strSlice &indexName,
                                const indexKeyPattern &pattern,
                                BOOLEAN &duplicated)const;

         INT32 createIndex(requestContext *context,
                           INT32 indexSlot,
                           UINT32 indexId,
                           const slice &defObj)const;

         INT32 markIndexRemoving(requestContext *context,
                                 INT32 indexSlot,
                                 UINT32 indexId)const;

         INT32 destroyIndexDefPage(requestContext *context,
                                   INT32 indexSlot)const;

         INT32 listIndexes(requestContext *context,
                           ossPoolVector<bson::BSONObj> &indexes)const;

         INT32 dumpIndex(requestContext *context,
                         INT32 indexSlot,
                         bson::BSONObj &obj)const;

      private:
         void fini();

         INT32 testIfDuplicated(requestContext *context,
                                INT32 indexSlot,
                                const strSlice &indexName,
                                const indexKeyPattern &pattern,
                                BOOLEAN &duplicated)const;

         INT32 createDirectMappedIndex(requestContext *context,
                                       INT32 indexSlot,
                                       UINT32 indexId,
                                       const slice &defObj)const;

         INT32 createDoubleMappedIndex(requestContext *context,
                                       INT32 indexSlot,
                                       UINT32 indexId,
                                       const slice &defObj)const;

      private:
         const collectionRecord *_record = NULL;
         indexSpace *_is = NULL;
   };//class indexConsole
}//namespace vessel
}//namespace engine

#endif//VESSEL_INDEX_CONSOLE_H_