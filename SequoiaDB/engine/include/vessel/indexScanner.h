/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = indexScanner.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_INDEX_SCANNER_H_
#define VESSEL_INDEX_SCANNER_H_

#include "vessel/recordID.h"
#include "vessel/indexIterator.h"
#include "vessel/indexScanContext.h"

namespace engine
{
namespace vessel
{
   class indexScanCursor;
   class requestContext;

   class indexScanner : public SDBObject
   {
      public:
         indexScanner() = default;
         ~indexScanner();
         indexScanner(const indexScanner &) = delete;
         indexScanner &operator=(const indexScanner &) = delete;

      public:
         OSS_INLINE BOOLEAN isOpen()const
         {
            return nullptr != _cursor;
         }
      public:
         ///WARNING: scanner does not own the cursor
         INT32 open(indexScanCursor *ctx,
                    INDEX_ITERATOR_UPTR &&iterator);

         void close();

         /// return SDB_IXM_EOC when hit the end.
         INT32 next(requestContext *context);

      public:
         const indexIterator *current()const {return _iterator.get();}

         INT32 saveLocation();

      private:
         INT32 _pauseUntilRidReady(requestContext *context,
                                   const recordID &rid);

         INT32 _beginToScan();
         INT32 _fetchNextAndLock(requestContext *context);

         INT32 _tryLockRecord(requestContext *context,
                              const recordID &rid,
                              BOOLEAN &locked);
      
         INT32 _waitRecord(requestContext *context,
                           const recordID &rid);        
      private:
         indexScanCursor *_cursor = nullptr;
         INDEX_ITERATOR_UPTR _iterator;
         BOOLEAN _scanning = FALSE;
         bson::BufBuilder _keyBuilder;
   };//class indexScanner

} // namespace vessel
}// namespace engine

#endif//VESSEL_INDEX_SCANNER_H_