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

   Source File Name = indexScanEntryBatch.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_INDEX_SCAN_ENTRY_BATCH_H_
#define VESSEL_INDEX_SCAN_ENTRY_BATCH_H_

#include "vessel/indexScanEntry.h"
#include "../bson/util/builder.h"
#include "utilArray.hpp"

namespace engine
{
namespace vessel
{
   class indexScanEntryBatch : public SDBObject
   {
      public:
         indexScanEntryBatch(){}
         ~indexScanEntryBatch(){}
         indexScanEntryBatch(const indexScanEntryBatch &) = delete;
         indexScanEntryBatch &operator=(const indexScanEntryBatch &) = delete;
      public:
         OSS_INLINE UINT32 getEntryCount()const
         {
            return _entries.size();
         }
         OSS_INLINE BOOLEAN isEmpty()const
         {
            return _entries.empty();
         }
         void reset();

         INT32 addFragmentsOfOneEntry(std::initializer_list<slice> il);

         INT32 addEntry(const slice &entry);

         slice operator[](UINT32 pos)const;

         slice getLastEntry()const;

      private:
         typedef UINT32 _ENTRY_OFFSET;
         typedef UINT32 _ENTRY_SIZE;
         typedef std::pair<_ENTRY_OFFSET, _ENTRY_SIZE> _ENTRY_INFO;

      private:
         _utilArray<_ENTRY_INFO, 8> _entries;
         bson::StackBufBuilder _buffer;
   };//class indexScanEntryBatch
} // namespace vessel

} // namespace engine


#endif//VESSEL_INDEX_SCAN_ENTRY_BATCH_H_