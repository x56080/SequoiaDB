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

   Source File Name = lsmDB.cpp

   Descriptive Name = 

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/20/2022  LYC  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/lsm/lsmDB.h"
#include "vessel/lsm/lsmIndexKey.h"
#include "vessel/lsm/lsmLobcKeyComparator.h"
#include "vessel/lsm/lsmCompactionFilter.hpp"

namespace engine
{
namespace vessel
{
   lsmDB::~lsmDB()
   {
      close();
   }

   BOOLEAN lsmDB::isOpen()const
   {
      return _lsmDB &&
             !_lsmCFHandles.empty();
   }

   INT32 lsmDB::open(const CHAR *dbPath,
                     const rocksdb::Options *o)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != dbPath, "can not be null");
      rocksdb::Status s;
      rocksdb::Options opt;
      std::vector<rocksdb::ColumnFamilyDescriptor> descs;

      close();

      if (nullptr != o)
      {
         opt = *o;
      }
      opt.create_if_missing = TRUE;
      opt.create_missing_column_families = TRUE;
      opt.atomic_flush = TRUE;

      // configure column family names and options
      for (UINT32 i = LSM_DEFAULT_CF; i <= LSM_MAX_CF_TYPE; ++i)
      {
         descs.push_back(_getDescriptor(static_cast<LSM_CF_TYPE>(i), opt));
      }

      s = rocksdb::DB::Open(opt, dbPath, descs, &_lsmCFHandles, &_lsmDB);
      if (!s.ok())
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "open rocksdb failed, status info:[%s]",
                s.ToString().c_str());
         goto error;
      }

   done:
      return rc;
   error:
      close();
      goto done;
   }

   void lsmDB::close()
   {
      rocksdb::Status s;
      if (nullptr != _lsmDB)
      {
         for (UINT32 i = LSM_DEFAULT_CF; i <= LSM_MAX_CF_TYPE; ++i)
         {
            s = _lsmDB->DestroyColumnFamilyHandle(_lsmCFHandles[i]);
            SDB_ASSERT(s.ok(), "failed to destroy column family handle");
         }
         _lsmDB->Close();
         delete _lsmDB;
         _lsmDB = nullptr;
         _lsmCFHandles.clear();
      }

   }

   rocksdb::DB *lsmDB::getDBPtr()
   {
      SDB_ASSERT(isOpen(), "must be open");
      return _lsmDB;
   }

   lsmColumnFamily lsmDB::getIdxColumnFamily()
   {
      SDB_ASSERT(isOpen(), "must be open");
      return lsmColumnFamily(this, _lsmCFHandles[LSM_INDEX_CF]);
   }

   lsmColumnFamily lsmDB::getLobcColumnFamily()
   {
      SDB_ASSERT(isOpen(), "must be open");
      return lsmColumnFamily(this, _lsmCFHandles[LSM_LOB_CHUNK_CF]);
   }

   rocksdb::ColumnFamilyDescriptor lsmDB::_getDescriptor(LSM_CF_TYPE type,
                                                         const rocksdb::Options &opt)
   {
      std::string cfName;
      rocksdb::ColumnFamilyOptions cfOpt(opt);

      if (LSM_DEFAULT_CF == type)
      {
         cfName = LSM_DEFAULT_CF_NAME;
      }
      if (LSM_INDEX_CF == type)
      {
         cfName = LSM_INDEX_CF_NAME;
         cfOpt.comparator = lsmIdxKeyComparator();
         cfOpt.compaction_filter_factory = createIdxCompactionFilterFactory();
         ///TODO: prefix_extractor
      }
      else if (LSM_LOB_CHUNK_CF == type)
      {
         cfName = LSM_LOB_CHUNK_CF_NAME;
         cfOpt.comparator = lsmLobcKeyComparator();
      }

      return rocksdb::ColumnFamilyDescriptor(cfName, cfOpt);
   }

} // namespace vessel
} // namespace engine
