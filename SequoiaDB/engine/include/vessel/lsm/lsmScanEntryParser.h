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

   Source File Name = lsmScanEntryParser.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_LSM_SCAN_ENTRY_PARSER_H_
#define VESSEL_LSM_SCAN_ENTRY_PARSER_H_

#include "vessel/indexScanEntryParser.h"
#include "vessel/lsm/lsmIdxKey.hpp"

namespace engine
{
namespace vessel
{
   class lsmScanEntryParser : public indexScanEntryParser
   {
      public:  
         lsmScanEntryParser(){}
         virtual ~lsmScanEntryParser(){}

      public:
         virtual void reset()
         {
            _lsmEntry.reset();
            _fullEntry.reset();
         }
         virtual INT32 parse(const slice &entryData);
         virtual INDEX_TYPE getType()const
         {
            return INDEX_TYPE_LSM;
         }
         virtual recordID getRid()const
         {
            return _lsmEntry.getRid();
         }
         virtual DPS_TRANS_ID getTransID()const
         {
            return _lsmEntry.getTransID();
         }
         
         virtual slice getKeySlice()const
         {
            return isValid() ?
                   slice(_lsmEntry.getKey().dataSize(), _lsmEntry.getKey().data()):
                   slice();
         }

      public:
         OSS_INLINE const ixmKey &getKey()const
         {
            return _lsmEntry.getKey();
         }
         OSS_INLINE DPS_LSN_OFFSET getLsn()const
         {
            return _lsmEntry.getDataLsn();
         }
         OSS_INLINE BOOLEAN isValid()const
         {
            return _lsmEntry.isValid();
         }
      private:
         lsmKeyEntry _lsmEntry;
         slice _fullEntry;
   };//class lsmScanEntryParser
} // namespace vessel

} // namespace engine


#endif//VESSEL_LSM_SCAN_ENTRY_PARSER_H_
