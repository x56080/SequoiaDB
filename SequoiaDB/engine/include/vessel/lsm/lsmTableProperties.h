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

   Source File Name = lsmTableProperties.h

   Descriptive Name = 

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/20/2022  LYC  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef VESSEL_LSM_TABLE_PROPERTIES_H_
#define VESSEL_LSM_TABLE_PROPERTIES_H_

#include "vessel/globalIndexID.h"
#include "rocksdb/table_properties.h"

namespace engine
{
namespace vessel
{
   #define LSM_TABLE_PROPERTIES_MIN_LSN "sdb_min_lsn"
   #define LSM_TABLE_PROPERTIES_MAX_LSN "sdb_max_lsn"
   #define LSM_TABLE_PROPERTIES_MIN_IDX_ID "sdb_min_index_id"
   #define LSM_TABLE_PROPERTIES_MAX_IDX_ID "sdb_max_index_id"

   class lsmTableProperties : public SDBObject
   {
      public:
         lsmTableProperties() = default;
         lsmTableProperties(const rocksdb::TableProperties *properties);
         ~lsmTableProperties() = default;

      public:
         void init(const rocksdb::TableProperties *properties);
         BOOLEAN isValid()const {return nullptr != _properties;}
         void reset() {_properties = nullptr;}
         BOOLEAN hasUserDefinedProperties()const;
         const rocksdb::TableProperties *getProperties()const {return _properties;}
         INT32 getLSNPair(UINT64 &minLSN, UINT64 &maxLSN)const;
         INT32 getIndexIdPair(globalIndexID &minId, globalIndexID &maxId)const;

      private:
         INT32 _extractLSN(const CHAR *name, UINT64 &lsn)const;
         INT32 _extractIndexId(const CHAR *name, globalIndexID &indexId)const;

      private:
         const rocksdb::TableProperties *_properties = nullptr;
   };//class lsmTableProperties
} // namespace vessel

} // namespace engine


#endif//VESSEL_LSM_TABLE_PROPERTIES_H_