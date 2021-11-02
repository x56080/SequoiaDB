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

   Source File Name = btreeScanEntryParser.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_BTREE_SCAN_ENTRY_PARSER_H_
#define VESSEL_BTREE_SCAN_ENTRY_PARSER_H_

#include "vessel/indexScanEntryParser.h"

namespace engine
{
namespace vessel
{

   class btreeScanEntryParser : public indexScanEntryParser
   {
      public:
         btreeScanEntryParser(){}
         virtual ~btreeScanEntryParser(){}
         btreeScanEntryParser(const btreeScanEntryParser &) = delete;
         btreeScanEntryParser &operator=(const btreeScanEntryParser &o)
         {
            _fields = o._fields;
            _keySlice = o._keySlice;
            return *this;
         }

      public:
         virtual void reset()
         {
            _fields = _fixedSizeFields();
            _keySlice.reset();
            return;
         }
         
         virtual INT32 parse(const slice &entryData);
         virtual INDEX_TYPE getType()const
         {
            return INDEX_TYPE_BTREE;
         }
         virtual recordID getRid()const
         {
            return _fields.rid;
         }
         virtual DPS_TRANS_ID getTransID()const
         {
            return _fields.transID;
         }
         virtual slice getKeySlice()const
         {
            return _keySlice;
         }

      public:
         OSS_INLINE recordID getIndexRid()const
         {
            return _fields.indexRid;
         }

         OSS_INLINE BOOLEAN isValid()const
         {
            return _keySlice.isValid();
         }

         void init(const recordID &indexRid,
                   const recordID &rid,
                   const DPS_TRANS_ID &transID,
                   const slice &keySlice);

         OSS_INLINE slice getFixSizedFields()const
         {
            return slice(sizeof(_fixedSizeFields), &_fields);
         }

#pragma pack(2)
      private:
         struct _fixedSizeFields
         {
            _fixedSizeFields &operator=(const _fixedSizeFields &o)
            {
               indexRid = o.indexRid;
               rid = o.rid;
               transID = o.transID;
               return *this;
            }

            recordID indexRid;
            recordID rid;
            DPS_TRANS_ID transID;
         };//struct _fixSizedFields

#pragma pack()
      private:
         _fixedSizeFields _fields;
         slice _keySlice;

   };//class btreeScanEntryParser


} // namespace vessel

} // namespace engine

#endif//VESSEL_BTREE_SCAN_ENTRY_H_
