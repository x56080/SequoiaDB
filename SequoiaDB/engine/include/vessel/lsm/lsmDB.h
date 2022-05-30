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

   Source File Name = lsmDB.h

   Descriptive Name = 

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/20/2022  LYC  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_LSM_DB_H_
#define VESSEL_LSM_DB_H_

#include "vessel/lsm/lsmDBDef.h"
#include "vessel/lsm/lsmColumnFamily.h"
#include "rocksdb/db.h"

namespace engine
{
namespace vessel
{
   class lsmDB : public SDBObject
   {
      public:
         lsmDB() = default;
         ~lsmDB();
         lsmDB(const lsmDB &) = delete;
         lsmDB &operator= (const lsmDB &) = delete;

      public:
         INT32 open(const CHAR* dbPath,
                    const rocksdb::Options *o = nullptr);
         void close();

      public:
         BOOLEAN isOpen()const;
         rocksdb::DB *getDBPtr();
         lsmColumnFamily getIdxColumnFamily();
         lsmColumnFamily getLobcColumnFamily();

      private:
         rocksdb::ColumnFamilyDescriptor _getDescriptor(LSM_CF_TYPE type,
                                                        const rocksdb::Options &opt);

      private:
         rocksdb::DB *_lsmDB = nullptr;
         std::vector<rocksdb::ColumnFamilyHandle *> _lsmCFHandles;
   };
} // namespace vessel
} // namespace engine

#endif // VESSEL_LSM_DB_H_