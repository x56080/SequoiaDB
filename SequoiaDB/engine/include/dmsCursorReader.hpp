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

   Source File Name = dmsCursorReader.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SDB_DMS_BSON_RECORD_READER_HPP_
#define SDB_DMS_BSON_RECORD_READER_HPP_

#include "interface/IDataCursor.h"
#include "dms.hpp"
#include "dpsTransID.hpp"
#include "../bson/bson.hpp"

namespace engine
{
   class _dmsBsonCursorReader : public SDBObject
   {
      public:
         _dmsBsonCursorReader(){}
         ~_dmsBsonCursorReader(){}
         _dmsBsonCursorReader(const _dmsBsonCursorReader &) = delete;
         _dmsBsonCursorReader &operator=(const _dmsBsonCursorReader &) = delete;

      public:
         void init(const DATA_CURSOR_PTR &cursor);
         void fini(BOOLEAN closeCursor=TRUE);
         INT32 fetchNext(IExecutor *executor);

         const bson::BSONObj &getRecord()const {return _record;}
         const dmsRecordID &getRid()const {return _rid;}
         const DPS_TRANS_ID &getTransID()const {return _transID;}

      private:
         void clearDataFetched();

      private:
         DATA_CURSOR_PTR _cursor;
         bson::BSONObj _record;
         dmsRecordID _rid;
         DPS_TRANS_ID _transID;
   };//class _dmsBsonCursorReader

   typedef class _dmsBsonCursorReader dmsBsonCursorReader;
} // namespace engine


#endif//SDB_DMS_BSON_RECORD_READER_HPP_