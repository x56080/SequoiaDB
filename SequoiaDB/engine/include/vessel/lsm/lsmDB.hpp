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

   Source File Name = lsmDB.hpp

   Descriptive Name = LSM DB APIs, rocksdb::DB interface wrapper

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/21/2021  JT  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef LSMDB_HPP_
#define LSMDB_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "pd.hpp"

#include "rocksdb/rocksdb_namespace.h"
#include "rocksdb/options.h"
#include "rocksdb/db.h"
#include "rocksdb/utilities/transaction_db.h"

using namespace rocksdb ;
namespace engine
{
namespace vessel
{
   enum LSM_DB_TYPE
   {
      LSM_DB_INVALID = 0,
      LSM_DB ,         // rocksdb::DB
      LSM_DB_TX,       // rocksdb::TransactionDB
      LSM_DB_READONLY  // rocksdb::DB for read only
   };

   // LSM Configuration
   #define LSM_MAX_FILE_PATH_LEN 255
   class LSMConfig
   {
   public:
      std::string dbPath;
      // rocksdb::Options
      BOOLEAN createDBIfMissing = FALSE;
      BOOLEAN createCFIfMissing = FALSE;
      BOOLEAN increaseParallelism = FALSE ;
      BOOLEAN optimizeLevelStyleCompaction = FALSE ;
      BOOLEAN atomic_flush = TRUE;
      BOOLEAN enablePrefixSeek = TRUE ;
      UINT32  db_write_buffer_size = 0 ;
      // rocksdb::ColumnFamilyOptions
      UINT32  write_buffer_size = 0 ;
      UINT32  max_write_buffer_number = 2 ;
      // rocksdb::TransactionDBOptions
      UINT32  num_stripes = 16;
      // rocksdb::WriteOption
      BOOLEAN writeSync  = FALSE ;
      BOOLEAN disableWAL = TRUE;
      // rocksdb::FlushOption
      BOOLEAN wait = TRUE;
      BOOLEAN allow_write_stall = FALSE;
      // rocksdb::ReadOption
      UINT32 readahead_size = 0 ;
      BOOLEAN fill_cache = TRUE;
   };

   // LSM Column Familes
   enum LSM_CF
   {
      LSM_CF_INDEX = 0,
      LSM_CF_MAX   = LSM_CF_INDEX
   };

   // rocksdb::DB wrapper
   class LSMDB : public SDBObject
   {
   public:
      LSMDB(): _dbType( LSM_DB_INVALID ), _lsmDB( NULL )
      {
         _lsmCFHandles.clear();
         _lsmCFDescriptors.clear();
         _lsmCFOptions.clear();
         _lsmCFNames.clear();
      }

      // No copying allowed
      LSMDB( const LSMDB & ) = delete;
      void operator= ( const LSMDB & ) = delete;

      virtual ~LSMDB();

      OSS_INLINE LSM_DB_TYPE getDBType() const { return _dbType; }

      OSS_INLINE rocksdb::FlushOptions getFlsOpt() const { return _lsmFlushOpt;}

      OSS_INLINE rocksdb::WriteOptions getWrtOpt() const { return _lsmWriteOpt;}

      OSS_INLINE rocksdb::ReadOptions getReadOpt() const { return _lsmReadOpt; }

      OSS_INLINE std::string getCFName( LSM_CF hId ) const
      {
         return _lsmCFNames[ hId ] ;
      }

      OSS_INLINE rocksdb::ColumnFamilyOptions getCFOption( LSM_CF hId ) const
      {
         return _lsmCFOptions[ hId ] ;
      }

      OSS_INLINE rocksdb::ColumnFamilyHandle* getCFHdl( LSM_CF hId ) const
      {
         return _lsmCFHandles[ hId ] ;
      }

      OSS_INLINE BOOLEAN isDBOpened() const { return _lsmDB ? TRUE : FALSE ; }

      // initialze default options regarding on the passed in configuration
      virtual void initLsmDB( const LSMConfig & config ) = 0 ;
      virtual rocksdb::Status openLsmDB() = 0;

      virtual rocksdb::Status flushLsmDB( const rocksdb::FlushOptions& fOpt )=0;

      virtual rocksdb::Status closeLsmDB
      (
         const rocksdb::FlushOptions & fOpt,
         BOOLEAN bFlushDB = FALSE,
         BOOLEAN bForceClose = TRUE // ingore errors during flushing and
                                    // dropping column family handles
      ) = 0;

      // ---- Wrapper of the rocksdb::DB interface ----

      // Set the database entry for "key" to "value".
      // If "key" already exists, it will be overwritten.
      // Returns OK on success, and a non-OK status on error.
      // Note: consider setting WriteOptions.sync = true.
      rocksdb::Status Put( const rocksdb::WriteOptions & wOpt,
                           LSM_CF hId,
                           const rocksdb::Slice & key,
                           const rocksdb::Slice & value ) ;

      // Put K-V in default column family with default writeOption,
      rocksdb::Status Put( const rocksdb::Slice & key,
                           const rocksdb::Slice & value )
      {
         return this->Put( _lsmWriteOpt, LSM_CF_INDEX, key, value );
      }

      // Remove the database entry (if any) for "key".  Returns OK on
      // success, and a non-OK status on error.  It is not an error if "key"
      // did not exist in the database.
      // Note: consider setting WriteOptions.sync = true.
      rocksdb::Status Delete( const rocksdb::WriteOptions & wOpt,
                              LSM_CF hId, const rocksdb::Slice & key ) ;
      // Remove K-V from default column family with default writeOption
      rocksdb::Status Delete( const rocksdb::Slice & key )
      {
         return this->Delete( _lsmWriteOpt, LSM_CF_INDEX, key ) ;
      }

      // Removes the database entries in the range ["begin_key", "end_key"),
      // i.e., including "begin_key" and excluding "end_key". Returns OK on
      // success, and a non-OK status on error. It is not an error if the
      // database does not contain any existing data in the range ["begin_key",
      // "end_key").
      //
      // If "end_key" comes before "start_key" according to the user's
      // comparator,  a `Status::InvalidArgument` is returned.
      //
      // This feature is now usable in production, with the following caveats:
      // 1) Accumulating many range tombstones in the memtable will degrade read
      // performance; this can be avoided by manually flushing occasionally.
      // 2) Limiting the maximum number of open files in the presence of range
      // tombstones can degrade read performance. To avoid this problem, set
      // max_open_files to -1 whenever possible.
      rocksdb::Status DeleteRange( const rocksdb::WriteOptions & wOpt,
                                   LSM_CF hId,
                                   const rocksdb::Slice & begin_key,
                                   const rocksdb::Slice & end_key );

      // DeleteRange from default column family with default WriteOption
      rocksdb::Status DeleteRange( const rocksdb::Slice & begin_key,
                                   const rocksdb::Slice & end_key )
      {
         return this->DeleteRange( _lsmWriteOpt, LSM_CF_INDEX,
                                   begin_key, end_key );
      }

      // Apply the specified updates to the database.
      // If `updates` contains no update, WAL will still be synced if
      // options.sync=true.
      // Returns OK on success, non-OK on failure.
      // Note: consider setting options.sync = true.
      rocksdb::Status Write( const rocksdb::WriteOptions & wOpt,
                             rocksdb:: WriteBatch* updates ) ;
      // write with default WriteOption
      rocksdb::Status Write( rocksdb:: WriteBatch* updates )
      {
         return this->Write( _lsmWriteOpt, updates ) ;
      }

      // If the database contains an entry for "key" store the
      // corresponding value in *value and return OK.
      //
      // If there is no entry for "key" leave *value unchanged and return
      // a status for which Status::IsNotFound() returns true.
      //
      // May return some other Status on an error.
      rocksdb::Status Get( const rocksdb::ReadOptions & rOpt,
                           LSM_CF hId,
                           const rocksdb::Slice& key,
                           rocksdb::PinnableSlice* value ) ;

      // Get K-V from default column family
      rocksdb::Status Get( const rocksdb::Slice & key,
                           rocksdb::PinnableSlice* value )
      {
         return this->Get( _lsmReadOpt, LSM_CF_INDEX, key, value );
      }

      // Consistent Get of many keys across column families without the need
      // for an explicit snapshot. NOTE: the implementation of this MultiGet API
      // does not have the performance benefits of the void-returning MultiGet
      // functions.
      //
      // If keys[i] does not exist in the database, then the i'th returned
      // status will be one for which Status::IsNotFound() is true, and
      // (*values)[i] will be set to some arbitrary value (often ""). Otherwise,
      // the i'th returned status will have Status::ok() true, and (*values)[i]
      // will store the value associated with keys[i].
      //
      // (*values) will always be resized to be the same size as (keys).
      // Similarly, the number of returned statuses will be the number of keys.
      // Note: keys will not be "de-duplicated". Duplicate keys will return
      // duplicate values in order.
      std::vector<rocksdb::Status> MultiGet
      (
         const rocksdb::ReadOptions & rOpt,
         const std::vector<rocksdb::ColumnFamilyHandle*> & column_family,
         const std::vector<rocksdb::Slice> & keys,
         std::vector<std::string> * values
      ) ;

      // get keys from default column family
      std::vector<rocksdb::Status> MultiGet
      (
         const std::vector<Slice>   & keys,
         std::vector<std::string>   * values
      )
      {
         return this->MultiGet(
           _lsmReadOpt,
           std::vector<rocksdb::ColumnFamilyHandle*>(keys.size(),
                                                     getCFHdl(LSM_CF_INDEX)),
           keys,
           values );
      }

      // Overloaded MultiGet API that improves performance by batching
      // operations in the read path for greater efficiency. Currently,
      // only the block based table format with full filters are supported.
      // Other table formats such as plain table, block based table with block
      // based filters and partitioned indexes will still work, but will not
      // get any performance benefits.
      // Parameters -
      // options       - ReadOptions
      // column_family - ColumnFamilyHandle* that the keys belong to.
      //                 All the keys passed to the API are restricted to
      //                 a single column family
      // num_keys - Number of keys to lookup
      // keys     - Pointer to C style array of key Slices with num_keys
      //            elements
      // values   - Pointer to C style array of PinnableSlices with num_keys
      //            elements
      // statuses - Pointer to C style array of Status with num_keys elements
      // sorted_input - If true, it means the input keys are already sorted
      //                by key order, so the MultiGet() API doesn't have to
      //                sort them again. If false, the keys will be copied and
      //                sorted internally by the API - the input array will not
      //                be modified
      void MultiGet
      (
         const rocksdb::ReadOptions& options,
         rocksdb::ColumnFamilyHandle* column_family,
         const size_t num_keys,
         const rocksdb::Slice* keys,
         rocksdb::PinnableSlice* values,
         rocksdb:: Status* statuses,
         const bool sorted_input = false
      ) ;

      void MultiGet
      (
         const rocksdb::ReadOptions& options,
         const size_t num_keys,
         rocksdb::ColumnFamilyHandle** column_families,
         const rocksdb::Slice* keys,
         rocksdb::PinnableSlice* values,
         rocksdb:: Status* statuses,
         const bool sorted_input = false
      ) ;

      // If the key definitely does not exist in the database, then this method
      // returns false, else true. If the caller wants to obtain value when the
      // key is found in memory, a bool for 'value_found' must be passed.
      // 'value_found' will be true on return if value has been set properly.
      // This check is potentially lighter-weight than invoking DB::Get().
      // One way to make this lighter weight is to avoid doing any IOs.
      // Default implementation here returns true and sets 'value_found'
      // to false
      BOOLEAN KeyMayExist( const rocksdb::ReadOptions & rOpt,
                           LSM_CF hId,
                           const rocksdb::Slice & key,
                           std::string* value,
                           BOOLEAN * value_found = NULL ) ;

      // search Key in default column family
      BOOLEAN KeyMayExist( const rocksdb::Slice & key,
                           std::string* value,
                           BOOLEAN * value_found = NULL )
      {
         return this->KeyMayExist( _lsmReadOpt, LSM_CF_INDEX, key, value,
                                   value_found );
      }

      // Return a heap-allocated iterator over the contents of the database.
      // The result of NewIterator() is initially invalid (caller must
      // call one of the Seek methods on the iterator before using it).
      //
      // Caller should delete the iterator when it is no longer needed.
      // The returned iterator should be deleted before this db is deleted.
      rocksdb::Iterator* NewIterator( const rocksdb::ReadOptions & rOpt,
                                      LSM_CF hId );

      // create iterator against default column family
      rocksdb::Iterator* NewIterator()
      {
         return this->NewIterator( _lsmReadOpt, LSM_CF_INDEX ) ;
      }

      // Return a handle to the current DB state.  Iterators created with
      // this handle will all observe a stable snapshot of the current DB
      // state.  The caller must call ReleaseSnapshot(result) when the
      // snapshot is no longer needed.
      //
      // nullptr will be returned if the DB fails to take a snapshot or does
      // not support snapshot.
      const rocksdb::Snapshot* GetSnapshot() ;

      // Release a previously acquired snapshot.  The caller must not
      // use "snapshot" after this call.
      void ReleaseSnapshot(const rocksdb::Snapshot* snapshot );

      rocksdb::Status CompactRange(rocksdb::CompactRangeOptions &options,
                                   rocksdb::Slice *begin,
                                   rocksdb::Slice *end);
   protected:
      LSM_DB_TYPE      _dbType ;
      std::string      _dbPath ;
      rocksdb::DB *    _lsmDB ;
      rocksdb::Options _lsmOption ;
      rocksdb::ReadOptions  _lsmReadOpt;
      rocksdb::WriteOptions _lsmWriteOpt;
      rocksdb::FlushOptions _lsmFlushOpt;

      std::vector<std::string>                     _lsmCFNames ;
      std::vector<rocksdb::ColumnFamilyOptions>    _lsmCFOptions ;
      std::vector<rocksdb::ColumnFamilyDescriptor> _lsmCFDescriptors;
      std::vector<rocksdb::ColumnFamilyHandle*>    _lsmCFHandles ;

      virtual void _initCFNamesAndOptions( const LSMConfig & config ) = 0;

      void _initCFDescriptors()
      {
         _lsmCFDescriptors.clear();

         for ( int cfId = LSM_CF_INDEX; cfId <= LSM_CF_MAX; cfId++ )
         {
            _lsmCFDescriptors.push_back( rocksdb::ColumnFamilyDescriptor(
               _lsmCFNames[cfId], _lsmCFOptions[cfId] ) ) ;
         }
      }
   };


   // LSMDB implementation, read and write
   class lsmDB : public LSMDB
   {
   public:
      lsmDB(): LSMDB() { _dbType = LSM_DB ; }

      // No copying allowed
      lsmDB( const lsmDB & ) = delete;
      void operator= ( const lsmDB & ) = delete;

      virtual ~lsmDB();

      // initialze default options regarding on the passed in configuration
      void initLsmDB( const LSMConfig & config );
      rocksdb::Status openLsmDB();

      rocksdb::Status flushLsmDB( const rocksdb::FlushOptions& fOpt );
      rocksdb::Status flushLsmDB() { return this->flushLsmDB( _lsmFlushOpt ); }

      rocksdb::Status closeLsmDB
      (
         const rocksdb::FlushOptions &fOpt,
         BOOLEAN bFlushDB = FALSE,
         BOOLEAN bForceClose = TRUE // ingore errors during flushing and
                                    // dropping column family handles
      );
      rocksdb::Status closeLsmDB
      (
         BOOLEAN bFlushDB = FALSE,
         BOOLEAN bForceClose = TRUE
      )
      {
         return this->closeLsmDB( _lsmFlushOpt, bFlushDB, bForceClose );
      }

   protected:
      void _initCFNamesAndOptions( const LSMConfig & config ) ;
   };


   // LSMDB implementation
   //   read and write, with rocksdb::Transaction supported
   class lsmTxDB : public LSMDB
   {
   public:
      lsmTxDB(): LSMDB() { _dbType = LSM_DB_TX ; _lsmTxDB = NULL ; }

      // No copying allowed
      lsmTxDB( const lsmTxDB & ) = delete;
      void operator= ( const lsmTxDB & ) = delete;

      virtual ~lsmTxDB();

      // initialze default options regarding on the passed in configuration
      void initLsmDB( const LSMConfig & config );
      rocksdb::Status openLsmDB();

      rocksdb::Status flushLsmDB( const rocksdb::FlushOptions& fOpt );
      rocksdb::Status flushLsmDB() { return this->flushLsmDB( _lsmFlushOpt ); }

      rocksdb::Status closeLsmDB
      (
         const rocksdb::FlushOptions &fOpt,
         BOOLEAN bFlushDB = FALSE,
         BOOLEAN bForceClose = TRUE // ingore errors during flushing and
                                    // dropping column family handles
      );

      rocksdb::Status closeLsmDB
      (
         BOOLEAN bFlushDB = FALSE,
         BOOLEAN bForceClose = TRUE
      )
      {
         return this->closeLsmDB( _lsmFlushOpt, bFlushDB, bForceClose );
      }

      //
      // Following functions are only supported by rocksdb::TransactionDB,
      // i.e., only when the DB is opened as LSM_DB_TX type
      //
      // Starts a new Transaction.
      //
      // Caller is responsible for deleting the returned transaction when no
      // longer needed.
      //
      // If old_txn is not null, BeginTransaction will reuse this Transaction
      // handle instead of allocating a new one.  This is an optimization to
      // avoid extra allocations when repeatedly creating transactions.
      rocksdb::Transaction* BeginTransaction
      (
         const rocksdb::WriteOptions& wOp,
         const rocksdb::TransactionOptions& tOp,
         rocksdb::Transaction* old_txn = NULL
      ) ;

      rocksdb::Transaction* GetTransactionByName
      (
         const rocksdb::TransactionName & name
      ) ;

      rocksdb::TransactionDBOptions getTxDBOpt() const { return _lsmTxDBOpt; }
      rocksdb::TransactionOptions getTxOpt() const { return _lsmTxOpt ; }

   protected:
      rocksdb::TransactionDB *      _lsmTxDB ;
      rocksdb::TransactionDBOptions _lsmTxDBOpt ;
      rocksdb::TransactionOptions   _lsmTxOpt ;
      void _initCFNamesAndOptions( const LSMConfig & config ) ;
   };


   // LSMDB implementation, read only
   const rocksdb::Status LSMDB_NOT_SUPPORTED_IN_READ_MODE =
    rocksdb::Status::NotSupported("Not supported operation in read only mode.");

   class lsmDBReadOnly : public LSMDB
   {
   public:
      lsmDBReadOnly(): LSMDB() { _dbType = LSM_DB_READONLY; }

      // No copying allowed
      lsmDBReadOnly( const lsmDBReadOnly & ) = delete;
      void operator= ( const lsmDBReadOnly & ) = delete;

      virtual ~lsmDBReadOnly();

      // initialze default options regarding on the passed in configuration
      void initLsmDB( const LSMConfig & config );
      rocksdb::Status openLsmDB();

      rocksdb::Status flushLsmDB( const rocksdb::FlushOptions & fOpt )
      {
         return LSMDB_NOT_SUPPORTED_IN_READ_MODE ;
      }

      rocksdb::Status closeLsmDB
      (
         const rocksdb::FlushOptions &fOpt,
         BOOLEAN bFlushDB = FALSE,
         BOOLEAN bForceClose = TRUE // ingore errors during flushing and
                                    // dropping column family handles
      )
      {
         return LSMDB_NOT_SUPPORTED_IN_READ_MODE ;
      }

      rocksdb::Status closeLsmDB
      (
         BOOLEAN bForceClose = TRUE // ingore errors during flushing and
                                    // dropping column family handles
      );

      rocksdb::Status Put
      (
         const rocksdb::WriteOptions & wOpt,
         LSM_CF hId,
         const rocksdb::Slice & key,
         const rocksdb::Slice & value
      )
      {
         return LSMDB_NOT_SUPPORTED_IN_READ_MODE ;
      }

      rocksdb::Status Put( const rocksdb::Slice & key,
                           const rocksdb::Slice & value )
      {
         return LSMDB_NOT_SUPPORTED_IN_READ_MODE ;
      }


      rocksdb::Status Delete
      (
         const rocksdb::WriteOptions & wOpt,
         LSM_CF hId,
         const rocksdb::Slice & key
      )
      {
         return LSMDB_NOT_SUPPORTED_IN_READ_MODE ;
      }

      rocksdb::Status Delete( const rocksdb::Slice & key )
      {
         return LSMDB_NOT_SUPPORTED_IN_READ_MODE ;
      }

      rocksdb::Status DeleteRange
      (
         const rocksdb::WriteOptions & wOpt,
         LSM_CF hId,
         const rocksdb::Slice & begin_key,
         const rocksdb::Slice & end_key
      )
      {
         return LSMDB_NOT_SUPPORTED_IN_READ_MODE ;
      }

      rocksdb::Status DeleteRange( const rocksdb::Slice & begin_key,
                                   const rocksdb::Slice & end_key )
      {
         return LSMDB_NOT_SUPPORTED_IN_READ_MODE ;
      }

      rocksdb::Status Write
      (
         const rocksdb::WriteOptions & wOpt,
         rocksdb:: WriteBatch* updates
      )
      {
         return LSMDB_NOT_SUPPORTED_IN_READ_MODE ;
      }

      rocksdb::Status Write( rocksdb:: WriteBatch* updates )
      {
         return LSMDB_NOT_SUPPORTED_IN_READ_MODE ;
      }

   protected:
      void _initCFNamesAndOptions( const LSMConfig & config ) ;
   };



} // namescape vessel
} // namespace engine
#endif  // LSMDB_HPP_
