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

namespace engine
{
namespace vessel
{
   class requestContext;
   class indexScanContext;

   class indexScanner : public SDBObject
   {
      public:
         indexScanner(){}
         ~indexScanner();

      public:
         OSS_INLINE BOOLEAN isOpen()const
         {
            return NULL != _context;
         }

      public:
         INT32 open(indexScanContext *context,
                    const indexObject &indexObj);

         void close();

         /// return SDB_IXM_EOC when hit the end.
         /// scanner will be closed after returning any error.
         INT32 next(recordID &rid);

         /// pause will not release rid lock which holding.
         void pause();

         INT32 resume();

         OSS_INLINE BOOLEAN isPaused()const
         {
            return _paused;
         }
      private:
         INT32 prepareToScan(rtnPredicateListIterator *predicate);
         INT32 prepareToScan(const slice &entry);
         INT32 matchCurrentOrSeekNext(recordID &rid);
         INT32 tryLockCurrentRid(BOOLEAN &locked);
         INT32 waitCurrentRid(UINT32 millis, BOOLEAN &timeout);
         
         
      private:
         indexScanContext *_context = NULL;
         indexIterator *_iterator = NULL;
         BOOLEAN _seeked = FALSE;
         BOOLEAN _paused = FALSE;
         bson::BufBuilder _builder;
   };//class indexScanner

} // namespace vessel
}// namespace engine

#endif//VESSEL_INDEX_SCANNER_H_