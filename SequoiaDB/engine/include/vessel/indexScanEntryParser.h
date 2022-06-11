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

   Source File Name = indexScanEntryParser.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_INDEX_SCAN_ENTRY_PARSER_H_
#define VESSEL_INDEX_SCAN_ENTRY_PARSER_H_

#include "vessel/indexDef.h"
#include "vessel/slice.h"
#include "vessel/recordID.h"

namespace engine
{
namespace vessel
{
   class indexScanEntryParser : public SDBObject
   {
      public:
         indexScanEntryParser(){}
         virtual ~indexScanEntryParser(){}

      public:
         virtual void reset() = 0;
         virtual INT32 parse(const slice &entryData) = 0;
         virtual INDEX_TYPE getType()const = 0;
         virtual recordID getRid()const = 0;
         virtual slice getKeySlice()const = 0;

   };//class indexScanEntryParser
} // namespace vessel

} // namespace engine


#endif//VESSEL_INDEX_SCAN_ENTRY_PARSER_H_
