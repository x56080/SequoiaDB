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

   Source File Name = dataScanRow.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_DATA_SCAN_ROW_H_
#define VESSEL_DATA_SCAN_ROW_H_

#include "vessel/cursorRow.h"
#include "vessel/recordID.h"
#include "dpsTransID.hpp"
#include "vessel/slice.h"

namespace engine
{
namespace vessel
{
   class dataScanRow : public cursorRow
   {
      public:
         dataScanRow(){}
         virtual ~dataScanRow(){}
         dataScanRow(const dataScanRow &o):
         _rid(o._rid),
         _transID(o._transID),
         _record(o._record){}

         dataScanRow &operator=(const dataScanRow &o)
         {
            _rid = o._rid;
            _transID = o._transID;
            _record = o._record;
            return *this;
         }

      public:
         virtual CURSOR_ROW_TYPE getType()const
         {
            return CURSOR_ROW_TYPE_SCAN;
         }

         static const UINT32 MIN_CONTENT_SIZE = sizeof(recordID) + sizeof(DPS_TRANS_ID);

      public:
         OSS_INLINE const recordID &getRid()const
         {
            return _rid;
         }
         OSS_INLINE const DPS_TRANS_ID &getTransID()const
         {
            return _transID;
         }
         OSS_INLINE const slice &getRecord()const
         {
            return _record;
         }

         OSS_INLINE void reset()
         {
            _rid = recordID();
            _transID = DPS_TRANS_ID();
            _record.reset();
            return;
         }

         void shallowCopy(const recordID &rid,
                          const DPS_TRANS_ID &transID,
                          const slice &record)
         {
            _rid = rid;
            _transID = transID;
            _record = record;
            return;
         }

      private:
         recordID _rid;
         DPS_TRANS_ID _transID;
         slice _record;
   };//class dataScanRow
}//namespace vessel
}//namespace engine


#endif//VESSEL_DATA_SCAN_ROW_H_