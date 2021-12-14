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

   Source File Name = indexScanner.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_INDEX_SCANNER_H_
#define VESSEL_INDEX_SCANNER_H_

#include "vessel/indexHandle.h"
#include "vessel/indexObject.h"
#include "vessel/recordID.h"
#include "vessel/indexIterator.h"
#include "vessel/collectionOptions.h"

namespace engine
{
namespace vessel
{
   class indexScanContext;
   class indexContext;

   class indexScanner : public SDBObject
   {
      public:
         indexScanner(){}
         ~indexScanner();
         indexScanner(const indexScanner &) = delete;
         indexScanner &operator=(const indexScanner &) = delete;

      public:
         OSS_INLINE BOOLEAN isOpen()const
         {
            return NULL != _iterator;
         }
      public:
         INT32 open(indexScanContext *context,
                    indexContext *ic);

         void close();

         /// hold rid latch or record lock and put entry into batch.
         /// return SDB_IXM_EOC when hit the end.
         /// always clear batch outside first
         /// init row limit outside first
         INT32 batchNext(rowBatch &entryBatch);

      private:
         INT32 fillBatch(rowBatch &entryBatch);

         INT32 pauseAndRescan();

         INT32 beginToScan();
         INT32 moveIterator();

         INT32 tryLockRecord(const recordID &rid, BOOLEAN &locked);
      
         INT32 waitRecord(const recordID &rid);        
      private:
         indexScanContext *_context = NULL;
         indexIterator *_iterator = NULL;
         const indexContext *_ic = NULL;
         bson::BufBuilder _keyBuilder;
   };//class indexScanner

} // namespace vessel
}// namespace engine

#endif//VESSEL_INDEX_SCANNER_H_