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

   Source File Name = indexScanEntry.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_INDEX_SCAN_ENTRY_H_
#define VESSEL_INDEX_SCAN_ENTRY_H_

#include "vessel/recordID.h"
#include "vessel/indexDef.h"
#include "dpsTransID.hpp"
#include "vessel/slice.h"
#include "vessel/indexScanEntryParser.h"

namespace engine
{
namespace vessel
{
   class indexScanEntry : public SDBObject
   {
      public:
         indexScanEntry(){}
         virtual ~indexScanEntry(){}
         indexScanEntry(const indexScanEntry &o):
         _type(o._type),
         _rid(o._rid),
         _transID(o._transID),
         _keySlice(o._keySlice),
         _entryData(o._entryData)
         {}
         indexScanEntry &operator=(const indexScanEntry &o)
         {
            _type = o._type;
            _rid = o._rid;
            _transID = o._transID;
            _keySlice = o._keySlice;
            _entryData = o._entryData;
            return *this;
         }

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return INVALID_INDEX_TYPE != _type;
         }
         OSS_INLINE INDEX_TYPE getType()const
         {
            return _type;
         }

         OSS_INLINE const recordID &getRid()const
         {
            return _rid;
         }
         OSS_INLINE DPS_TRANS_ID getTransID()const
         {
            return _transID;
         }
         OSS_INLINE const slice &getKeySlice()const
         {
            return _keySlice;
         }
         OSS_INLINE const slice getEntryData()const
         {
            return _entryData;
         }

         INT32 init(INDEX_TYPE type, const slice &entryData);
         INT32 init(const slice &data, indexScanEntryParser *parser);
         void reset();


      private:
         INDEX_TYPE _type = INVALID_INDEX_TYPE;
         recordID _rid;
         DPS_TRANS_ID _transID;
         slice _keySlice;
         slice _entryData;
   };//class indexScanEntry
} // namespace vessel

} // namespace engine


#endif//VESSEL_INDEX_SCAN_ENTRY_H_