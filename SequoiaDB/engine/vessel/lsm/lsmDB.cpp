#include "vessel/lsm/lsmDB.hpp"
#include "vessel/lsm/lsmIdxKey.hpp"
#include "pd.hpp"
#include "rocksdb/status.h"
#include "rocksdb/slice_transform.h"

using namespace std ;
using namespace rocksdb ;

namespace engine
{
namespace vessel
{

////////////////////////////////////////////////////////////////////
//  LSMDB interface, rocksdb::DB wrapper
////////////////////////////////////////////////////////////////////
LSMDB::~LSMDB()
{
   if ( _lsmDB )
   {
      delete _lsmDB ;
      _lsmDB = NULL ;
   }
   _lsmCFHandles.clear();
   _lsmCFDescriptors.clear();
   _lsmCFOptions.clear();
   _lsmCFNames.clear();
}


// Set the database entry for "key" to "value".
// If "key" already exists, it will be overwritten.
// Returns OK on success, and a non-OK status on error.
// Note: consider setting options.sync = true.
rocksdb::Status LSMDB::Put
(
   const rocksdb::WriteOptions & wOpt,
   LSM_CF hId,
   const rocksdb::Slice & key,
   const rocksdb::Slice & value
)
{
   SDB_ASSERT( _lsmDB, "Database hasn't been opened!" );
   if ( LSM_DB_TX == _dbType )
   {
      rocksdb::TransactionDB * pTxDB =
            dynamic_cast<rocksdb::TransactionDB*>(_lsmDB) ;
      return  pTxDB->Put( wOpt, getCFHdl( hId ), key, value );
   }
   else
   {
      return _lsmDB->Put( wOpt, getCFHdl( hId ), key, value );
   }
}


// Remove the database entry (if any) for "key".  Returns OK on
// success, and a non-OK status on error.  It is not an error if "key"
// did not exist in the database.
// Note: consider setting options.sync = true.
rocksdb::Status LSMDB::Delete
(
   const rocksdb::WriteOptions & wOpt,
   LSM_CF hId,
   const rocksdb::Slice & key
)
{
   SDB_ASSERT( _lsmDB, "Database hasn't been opened!" );
   if ( LSM_DB_TX == _dbType )
   {
      rocksdb::TransactionDB * pTxDB =
            dynamic_cast<rocksdb::TransactionDB*>(_lsmDB) ;
      return  pTxDB->Delete( wOpt, getCFHdl( hId ), key );
   }
   else
   {
      return _lsmDB->Delete( wOpt, getCFHdl( hId ), key );
   }
}

// Removes the database entries in the range ["begin_key", "end_key"), i.e.,
// including "begin_key" and excluding "end_key". Returns OK on success, and
// a non-OK status on error. It is not an error if the database does not
// contain any existing data in the range ["begin_key", "end_key").
//
// If "end_key" comes before "start_key" according to the user's comparator,
// a `Status::InvalidArgument` is returned.
//
// This feature is now usable in production, with the following caveats:
// 1) Accumulating many range tombstones in the memtable will degrade read
// performance; this can be avoided by manually flushing occasionally.
// 2) Limiting the maximum number of open files in the presence of range
// tombstones can degrade read performance. To avoid this problem, set
// max_open_files to -1 whenever possible.
rocksdb::Status LSMDB::DeleteRange
(
   const rocksdb::WriteOptions & wOpt,
   LSM_CF hId,
   const rocksdb::Slice & begin_key,
   const rocksdb::Slice & end_key
)
{
   SDB_ASSERT( _lsmDB, "Database hasn't been opened!" );
   if ( LSM_DB_TX == _dbType )
   {
      rocksdb::TransactionDB * pTxDB =
            dynamic_cast<rocksdb::TransactionDB*>(_lsmDB) ;
      return  pTxDB->DeleteRange( wOpt, getCFHdl( hId ), begin_key, end_key );
   }
   else
   {
      return _lsmDB->DeleteRange( wOpt, getCFHdl( hId ), begin_key, end_key );
   }
}

// Apply the specified updates to the database.
// If `updates` contains no update, WAL will still be synced if
// options.sync=true.
// Returns OK on success, non-OK on failure.
// Note: consider setting options.sync = true.
rocksdb::Status LSMDB::Write
(
   const rocksdb::WriteOptions & wOpt,
   rocksdb:: WriteBatch* updates
)
{
   SDB_ASSERT( _lsmDB, "Database hasn't been opened!" );
   if ( LSM_DB_TX == _dbType )
   {
      rocksdb::TransactionDB * pTxDB =
            dynamic_cast<rocksdb::TransactionDB*>(_lsmDB) ;
      return  pTxDB->Write( wOpt, updates ) ;
   }
   else
   {
      return _lsmDB->Write( wOpt, updates ) ;
   }
}

// If the database contains an entry for "key" store the
// corresponding value in *value and return OK.
//
// If timestamp is enabled and a non-null timestamp pointer is passed in,
// timestamp is returned.
//
// If there is no entry for "key" leave *value unchanged and return
// a status for which Status::IsNotFound() returns true.
//
// May return some other Status on an error.
rocksdb::Status LSMDB::Get
(
   const rocksdb::ReadOptions & rOpt,
   LSM_CF hId,
   const rocksdb::Slice& key,
   rocksdb::PinnableSlice* value
)
{
   SDB_ASSERT( _lsmDB, "Database hasn't been opened!" );
   if ( LSM_DB_TX == _dbType )
   {
      rocksdb::TransactionDB * pTxDB =
            dynamic_cast<rocksdb::TransactionDB*>(_lsmDB) ;
      return  pTxDB->Get( rOpt, getCFHdl( hId ), key, value );
   }
   else
   {
      return _lsmDB->Get( rOpt, getCFHdl( hId ), key, value );
   }
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
std::vector<rocksdb::Status> LSMDB::MultiGet
(
   const rocksdb::ReadOptions & rOpt,
   const std::vector<rocksdb::ColumnFamilyHandle*>& column_family,
   const std::vector<rocksdb::Slice> & keys,
   std::vector<std::string>* values
)
{
   SDB_ASSERT( _lsmDB, "Database hasn't been opened!" );
   if ( LSM_DB_TX == _dbType )
   {
      rocksdb::TransactionDB * pTxDB =
            dynamic_cast<rocksdb::TransactionDB*>(_lsmDB) ;
      return  pTxDB->MultiGet( rOpt, column_family, keys, values ) ;
   }
   else
   {
      return _lsmDB->MultiGet( rOpt, column_family, keys, values ) ;
   }
}

// Overloaded MultiGet API that improves performance by batching operations
// in the read path for greater efficiency. Currently, only the block based
// table format with full filters are supported. Other table formats such
// as plain table, block based table with block based filters and
// partitioned indexes will still work, but will not get any performance
// benefits.
// Parameters -
// options - ReadOptions
// column_family - ColumnFamilyHandle* that the keys belong to. All the keys
//                 passed to the API are restricted to a single column family
// num_keys - Number of keys to lookup
// keys - Pointer to C style array of key Slices with num_keys elements
// values - Pointer to C style array of PinnableSlices with num_keys elements
// statuses - Pointer to C style array of Status with num_keys elements
// sorted_input - If true, it means the input keys are already sorted by key
//                order, so the MultiGet() API doesn't have to sort them
//                again. If false, the keys will be copied and sorted
//                internally by the API - the input array will not be
//                modified
void LSMDB::MultiGet
(
   const rocksdb::ReadOptions& options,
   rocksdb::ColumnFamilyHandle* column_family,
   const size_t num_keys,
   const rocksdb::Slice* keys,
   rocksdb::PinnableSlice* values,
   rocksdb:: Status* statuses,
   const bool sorted_input
)
{
   SDB_ASSERT( _lsmDB, "Database hasn't been opened!" );
   if ( LSM_DB_TX == _dbType )
   {
      rocksdb::TransactionDB * pTxDB =
            dynamic_cast<rocksdb::TransactionDB*>(_lsmDB) ;
      pTxDB->MultiGet ( options, column_family, num_keys, keys, values,
                        statuses, sorted_input ) ;
   }
   else
   {
      _lsmDB->MultiGet( options, column_family, num_keys, keys, values,
                        statuses, sorted_input ) ;
   }
}

void LSMDB::MultiGet
(
   const rocksdb::ReadOptions& options,
   const size_t num_keys,
   rocksdb::ColumnFamilyHandle** column_families,
   const rocksdb::Slice* keys,
   rocksdb::PinnableSlice* values,
   rocksdb:: Status* statuses,
   const bool sorted_input
)
{
   SDB_ASSERT( _lsmDB, "Database hasn't been opened!" );
   if ( LSM_DB_TX == _dbType )
   {
      rocksdb::TransactionDB * pTxDB =
            dynamic_cast<rocksdb::TransactionDB*>(_lsmDB) ;
      pTxDB->MultiGet ( options, num_keys, column_families, keys, values,
                        statuses, sorted_input ) ;
   }
   else
   {
      _lsmDB->MultiGet( options, num_keys, column_families, keys, values,
                        statuses, sorted_input ) ;
   }
}


// If the key definitely does not exist in the database, then this method
// returns false, else true. If the caller wants to obtain value when the key
// is found in memory, a bool for 'value_found' must be passed. 'value_found'
// will be true on return if value has been set properly.
// This check is potentially lighter-weight than invoking DB::Get(). One way
// to make this lighter weight is to avoid doing any IOs.
// Default implementation here returns true and sets 'value_found' to false
BOOLEAN LSMDB::KeyMayExist
(
   const rocksdb::ReadOptions & rOpt,
   LSM_CF hId,
   const rocksdb::Slice & key,
   std::string* value,
   BOOLEAN * value_found
)
{
   SDB_ASSERT( _lsmDB, "Database hasn't been opened!" );
   if ( LSM_DB_TX == _dbType )
   {
      rocksdb::TransactionDB * pTxDB =
            dynamic_cast<rocksdb::TransactionDB*>(_lsmDB) ;
      return  pTxDB->KeyMayExist( rOpt, getCFHdl( hId ), key, value,
                                  (bool*)value_found ) ? TRUE : FALSE ;
   }
   else
   {
      return _lsmDB->KeyMayExist( rOpt, getCFHdl( hId ), key, value,
                                  (bool*)value_found ) ? TRUE : FALSE ;
   }
}


// Return a heap-allocated iterator over the contents of the database.
// The result of NewIterator() is initially invalid (caller must
// call one of the Seek methods on the iterator before using it).
//
// Caller should delete the iterator when it is no longer needed.
// The returned iterator should be deleted before this db is deleted.
rocksdb::Iterator* LSMDB::NewIterator
(
   const rocksdb::ReadOptions& rOpt,
   LSM_CF hId
)
{
   SDB_ASSERT( _lsmDB, "Database hasn't been opened!" );
   if ( LSM_DB_TX == _dbType )
   {
      rocksdb::TransactionDB * pTxDB =
            dynamic_cast<rocksdb::TransactionDB*>(_lsmDB) ;
      return  pTxDB->NewIterator( rOpt, getCFHdl( hId ) ) ;
   }
   else
   {
      return _lsmDB->NewIterator( rOpt, getCFHdl( hId ) ) ;
   }
}


// Return a handle to the current DB state.  Iterators created with
// this handle will all observe a stable snapshot of the current DB
// state.  The caller must call ReleaseSnapshot(result) when the
// snapshot is no longer needed.
//
// nullptr will be returned if the DB fails to take a snapshot or does
// not support snapshot.
const rocksdb::Snapshot* LSMDB::GetSnapshot()
{
   SDB_ASSERT( _lsmDB, "Database hasn't been opened!" );
   if ( LSM_DB_TX == _dbType )
   {
      rocksdb::TransactionDB * pTxDB =
            dynamic_cast<rocksdb::TransactionDB*>(_lsmDB) ;
      return  pTxDB->GetSnapshot();
   }
   else
   {
      return _lsmDB->GetSnapshot();
   }
}


// Release a previously acquired snapshot.  The caller must not
// use "snapshot" after this call.
void LSMDB::ReleaseSnapshot(const rocksdb::Snapshot* snapshot )
{
   SDB_ASSERT( _lsmDB, "Database hasn't been opened!" );
   if ( LSM_DB_TX == _dbType )
   {
      rocksdb::TransactionDB * pTxDB =
            dynamic_cast<rocksdb::TransactionDB*>(_lsmDB) ;
      pTxDB->ReleaseSnapshot ( snapshot ) ;
   }
   else
   {
      _lsmDB->ReleaseSnapshot( snapshot ) ;
   }
}


////////////////////////////////////////////////////////////////////
// lsmDB, LSMDB interface implementation, read and write mode
////////////////////////////////////////////////////////////////////
void lsmDB::initLsmDB( const LSMConfig & config )
{
   //
   // Initialize Options from input config
   //
   SDB_ASSERT( ( LSM_DB == _dbType ), "Invalid DB type!" ) ;

   // create the DB if it's not already present
   _lsmOption.create_if_missing = (config.createDBIfMissing) ? true : false;
   // create the CF if it's not already present
   _lsmOption.create_missing_column_families = (config.createCFIfMissing)
                                               ? true : false ;
   // atomic flush CF
   _lsmOption.atomic_flush = (config.atomic_flush) ? true : false ;

   // set _dbPath
   _dbPath.assign( config.dbPath );
   // set comparator
   _lsmOption.comparator = lsmKeyComparator();
   // set prefix extractor
   if ( config.enablePrefixSeek )
   {
      _lsmOption.prefix_extractor.reset(
         NewFixedPrefixTransform( lsmEntryTypeSz + lsmRidSz ));
   }

   // Optimize RocksDB.
   // This is the easiest way to get RocksDB to perform well
   if ( config.increaseParallelism )
   {
      _lsmOption.IncreaseParallelism();
   }
   if ( config.optimizeLevelStyleCompaction )
   {
      _lsmOption.OptimizeLevelStyleCompaction();
   }

   // amount of data to build up in memtables across all column families
   // before writing to disk.
   if ( config.db_write_buffer_size > 0 )
   {
      // This feature is disabled by default. Specify a non-zero value to
      // enable it.
      // Default: 0 ( disabled )
      _lsmOption.db_write_buffer_size = config.db_write_buffer_size ;
   }

   // Initialize WriteOption
   _lsmWriteOpt.sync = (config.writeSync) ? true : false ;
   _lsmWriteOpt.disableWAL = (config.disableWAL) ? true : false ;

   // Initialize ReadOption
   _lsmReadOpt.readahead_size = ( config.readahead_size )
                                ? config.readahead_size : 0 ;
   _lsmReadOpt.fill_cache = ( config.fill_cache ) ? true : false ;

   // Initialize _LsmCFNames and _lsmCFOptions vector
   // the first one MUST be 'default' ColumnFamily
   _initCFNamesAndOptions( config ) ;

   // Initialize ColumnFamily descriptors
   _initCFDescriptors();
}


void lsmDB::_initCFNamesAndOptions( const LSMConfig & config )
{
    // house clean, should be done in ctor already
    //_lsmCFNames.clear() ;
    //_lsmCFOptions.clear();

    // the first CF name MUST be 'default' ColumnFamily
    _lsmCFNames.push_back( rocksdb::kDefaultColumnFamilyName ) ;
    // initial column family options before put it in CF option array
    rocksdb::ColumnFamilyOptions defaultCFOpt( _lsmOption ) ;

    // a single memtable size
    if ( config.write_buffer_size > 0 )
    {
       defaultCFOpt.write_buffer_size = config.write_buffer_size ;
    }
    else
    {
       defaultCFOpt.write_buffer_size = 64 * 1024 * 1024; // 64MB
    }
    // max number of memtables
    if ( config.max_write_buffer_number > 0 )
    {
       defaultCFOpt.max_write_buffer_number = config.max_write_buffer_number ;
    }

    // save default column family option before open db
    _lsmCFOptions.push_back( defaultCFOpt ) ;

    // initial other CF name and CF options here
}


rocksdb::Status lsmDB::openLsmDB()
{
   rocksdb::Status s ;

   if ( NULL == _lsmDB )
   {
      // clean up CF handles before open DB, should be done in ctor already
      // _lsmCFHandles.clear();

      // open Rocksdb in read and write mode
      s = rocksdb::DB::Open( _lsmOption, _dbPath,
                             _lsmCFDescriptors,
                             &_lsmCFHandles,
                             &_lsmDB ) ;
      if ( s.ok() )
      {
         SDB_ASSERT( _lsmDB, "Failed open LSMDB !" ) ;
      }
      else
      {
         // Log error messsage
      }
   }
   return s ;
}


rocksdb::Status lsmDB::closeLsmDB
(
   const rocksdb::FlushOptions & fOpt,
   BOOLEAN bFlushDB,
   BOOLEAN bForceClose
)
{
   rocksdb::Status s ;

   SDB_ASSERT( _lsmDB, "Database hasn't been opened!" );

   // Flush if required
   if ( bFlushDB )
   {
      s = flushLsmDB( fOpt ) ;
      SDB_ASSERT( s.ok(), "lsmDB was not flushed properly!" ) ;
   }
   // drop column family handles
   if ( bForceClose || s.ok() )
   {
      for ( auto cf : _lsmCFHandles )
      {
         s = _lsmDB->DestroyColumnFamilyHandle( cf ) ;
         SDB_ASSERT( s.ok(), "Cloumn Family handle wasn't dropped properly!" ) ;
      }
   }
   // close DB
   if ( bForceClose || s.ok() )
   {
      _lsmDB->Close() ;
      delete _lsmDB ;
      _lsmDB = NULL ;

      _lsmCFHandles.clear();
      _lsmCFDescriptors.clear();
      _lsmCFOptions.clear();
      _lsmCFNames.clear();
   }
   return s ;
}


rocksdb::Status lsmDB::flushLsmDB( const rocksdb::FlushOptions & fOpt )
{
   rocksdb::Status s ;

   SDB_ASSERT( _lsmDB, "Database hasn't been opened!" );
   if ( _lsmDB )
   {
      s = _lsmDB->Flush( fOpt, _lsmCFHandles );
   }
   return s ;
}


lsmDB::~lsmDB()
{
   if ( _lsmDB )
   {
      closeLsmDB( _lsmFlushOpt ) ;
      if ( _lsmDB )
      {
         delete _lsmDB ;
         _lsmDB = NULL ;
      }
   }
   _lsmCFHandles.clear();
   _lsmCFDescriptors.clear();
   _lsmCFOptions.clear();
   _lsmCFNames.clear();
}


////////////////////////////////////////////////////////////////////
// lsmTxDB, LSMDB interface implementation, read and write mode,
// with rocksdb::Transaction supported
////////////////////////////////////////////////////////////////////
void lsmTxDB::initLsmDB( const LSMConfig & config )
{
   //
   // Initialize Options from input config
   //
   SDB_ASSERT( ( LSM_DB_TX == _dbType ), "Invalid DB type!" ) ;

   // create the DB if it's not already present
   _lsmOption.create_if_missing = (config.createDBIfMissing) ? true : false;
   // create the CF if it's not already present
   _lsmOption.create_missing_column_families = (config.createCFIfMissing)
                                                  ? true : false ;
   // atomic flush CF
   _lsmOption.atomic_flush = (config.atomic_flush) ? true : false ;

   // set _dbPath
   _dbPath.assign( config.dbPath );
   // set comparator
   _lsmOption.comparator = lsmKeyComparator();
   // set prefix extractor
   if ( config.enablePrefixSeek )
   {
      _lsmOption.prefix_extractor.reset(
         NewFixedPrefixTransform( lsmEntryTypeSz + lsmRidSz ));
   }

   // Optimize RocksDB.
   // This is the easiest way to get RocksDB to perform well
   if ( config.increaseParallelism )
   {
      _lsmOption.IncreaseParallelism();
   }
   if ( config.optimizeLevelStyleCompaction )
   {
      _lsmOption.OptimizeLevelStyleCompaction();
   }

   // amount of data to build up in memtables across all column families
   // before writing to disk.
   if ( config.db_write_buffer_size > 0 )
   {
      // This feature is disabled by default. Specify a non-zero value to
      // enable it.
      // Default: 0 ( disabled )
      _lsmOption.db_write_buffer_size = config.db_write_buffer_size ;
   }

   // Initialize TransactionDBOptions
   // To increase the concurrency, rocksdb lock table ( per column family )
   // is divided ( by this number ) into more sub-tablels, each with their
   // own separate mutex.
   if ( config.num_stripes > 0)
   {
      _lsmTxDBOpt.num_stripes = config.num_stripes ;
   }
   else
   {
      _lsmTxDBOpt.num_stripes = 8191 ;
   }

   // Initialize WriteOption
   _lsmWriteOpt.sync = (config.writeSync) ? true : false ;
   _lsmWriteOpt.disableWAL = (config.disableWAL) ? true : false ;

   // Initialize ReadOption
   _lsmReadOpt.readahead_size = ( config.readahead_size )
                                ? config.readahead_size : 0 ;
   _lsmReadOpt.fill_cache = ( config.fill_cache ) ? true : false ;

   // Initialize _LsmCFNames and _lsmCFOptions vector
   // the first one MUST be 'default' ColumnFamily
   _initCFNamesAndOptions( config ) ;

   // Initialize ColumnFamily descriptors
   _initCFDescriptors();
}


void lsmTxDB::_initCFNamesAndOptions( const LSMConfig & config )
{
    // house clean, should be done in ctor already
    // _lsmCFNames.clear() ;
    // _lsmCFOptions.clear();

    // the first CF name MUST be 'default' ColumnFamily
    _lsmCFNames.push_back( rocksdb::kDefaultColumnFamilyName ) ;
    // initial column family options before put it in CF option array
    rocksdb::ColumnFamilyOptions defaultCFOpt( _lsmOption ) ;

    // a single memtable size
    if ( config.write_buffer_size > 0 )
    {
       defaultCFOpt.write_buffer_size = config.write_buffer_size ;
    }
    else
    {
       defaultCFOpt.write_buffer_size = 64 * 1024 * 1024; // 64MB
    }
    // max number of memtables
    if ( config.max_write_buffer_number > 0 )
    {
       defaultCFOpt.max_write_buffer_number = config.max_write_buffer_number ;
    }

    // save default column family option before open db
    _lsmCFOptions.push_back( defaultCFOpt ) ;

    // initial other CF name and CF options here
}


rocksdb::Status lsmTxDB::openLsmDB()
{
   rocksdb::Status s ;

   if ( NULL == _lsmDB )
   {
      // clean up CF handles before open DB, should be done in ctor already
      // _lsmCFHandles.clear();

      // open Rocksdb for read and write with transaction supported
      s = rocksdb::TransactionDB::Open( _lsmOption, _lsmTxDBOpt,
                                        _dbPath,
                                        _lsmCFDescriptors,
                                        &_lsmCFHandles,
                                        &_lsmTxDB ) ;
      if ( s.ok() )
      {
         SDB_ASSERT( _lsmTxDB, "Failed open lsmTxDB !" ) ;
         _lsmDB = _lsmTxDB ;
         SDB_ASSERT( ( ( _lsmDB == _lsmTxDB ) && _lsmDB ),
                     "lsmTxDB was not opened correctly!" );
      }
      else
      {
         // Log error messsage
      }
   }
   return s ;
}


rocksdb::Status lsmTxDB::flushLsmDB( const rocksdb::FlushOptions & fOpt )
{
   rocksdb::Status s ;

   SDB_ASSERT( ( ( _lsmDB == _lsmTxDB ) && _lsmDB ),
               "lsmTxDB was not opened yet!" );

   if ( _lsmTxDB )
   {
      s = _lsmTxDB->Flush( fOpt, _lsmCFHandles );
   }
   return s ;
}


rocksdb::Status lsmTxDB::closeLsmDB
(
   const rocksdb::FlushOptions & fOpt,
   BOOLEAN bFlushDB,
   BOOLEAN bForceClose
)
{
   rocksdb::Status s ;

   SDB_ASSERT( ( ( _lsmDB == _lsmTxDB ) && _lsmDB ),
               "lsmTxDB was not opened yet!" );

   // Flush if required
   if ( bFlushDB )
   {
      s = flushLsmDB( fOpt ) ;
      SDB_ASSERT( s.ok(), "lsmTxDB was not flushed properly!" ) ;
   }

   // drop column family handles
   if ( bForceClose || s.ok() )
   {
      for ( auto cf : _lsmCFHandles )
      {
         s = _lsmTxDB->DestroyColumnFamilyHandle( cf ) ;
         SDB_ASSERT( s.ok(), "Cloumn Family handle wasn't dropped properly!" ) ;
      }
   }

   // close DB
   if ( bForceClose || s.ok() )
   {
      _lsmTxDB->Close() ;
      delete _lsmTxDB ;
      _lsmTxDB = NULL ;
      _lsmDB = NULL ;

      _lsmCFHandles.clear();
      _lsmCFDescriptors.clear();
      _lsmCFOptions.clear();
      _lsmCFNames.clear();
   }
   return s ;
}


lsmTxDB::~lsmTxDB()
{
   if ( _lsmTxDB )
   {
      closeLsmDB( _lsmFlushOpt ) ;
      if ( _lsmTxDB )
      {
         delete _lsmTxDB ;
         _lsmTxDB = NULL ;
         _lsmDB = NULL ;
      }
   }
   _lsmCFHandles.clear();
   _lsmCFDescriptors.clear();
   _lsmCFOptions.clear();
   _lsmCFNames.clear();
}



// rocksdb::TransactionDB, i.e., LSM_DB_TX _dbType only
rocksdb::Transaction* lsmTxDB::BeginTransaction
(
   const rocksdb::WriteOptions& wOpt,
   const rocksdb::TransactionOptions& tOpt,
   rocksdb::Transaction* old_txn
)
{
   SDB_ASSERT( _lsmTxDB, "Database hasn't been opened!" );
   return _lsmTxDB->BeginTransaction( wOpt, tOpt, old_txn ) ;
}


// rocksdb::TransactionDB, i.e., LSM_DB_TX _dbType only
rocksdb::Transaction* lsmTxDB::GetTransactionByName
(
   const rocksdb::TransactionName & name
)
{
   SDB_ASSERT( _lsmTxDB, "Database hasn't been opened!" );
   return _lsmTxDB->GetTransactionByName( name ) ;
}


////////////////////////////////////////////////////////////////////
// lsmDBReadOnly, LSMDB interface implementation, read only mode
////////////////////////////////////////////////////////////////////
void lsmDBReadOnly::initLsmDB( const LSMConfig & config )
{
   //
   // Initialize Options from input config
   //
   SDB_ASSERT( ( LSM_DB_READONLY == _dbType ), "Invalid DB type!" ) ;

   _lsmOption.create_if_missing = false;
   _lsmOption.create_missing_column_families = false;

   // set _dbPath
   _dbPath.assign( config.dbPath );
   // set comparator
   _lsmOption.comparator = lsmKeyComparator();
   // set prefix extractor
   if ( config.enablePrefixSeek )
   {
      _lsmOption.prefix_extractor.reset(
         NewFixedPrefixTransform( lsmEntryTypeSz + lsmRidSz ));
   }

   // Optimize RocksDB.
   // This is the easiest way to get RocksDB to perform well
   if ( config.increaseParallelism )
   {
      _lsmOption.IncreaseParallelism();
   }
   if ( config.optimizeLevelStyleCompaction )
   {
      _lsmOption.OptimizeLevelStyleCompaction();
   }

   // amount of data to build up in memtables across all column families
   // before writing to disk.
   if ( config.db_write_buffer_size > 0 )
   {
      // This feature is disabled by default. Specify a non-zero value to
      // enable it.
      // Default: 0 ( disabled )
      _lsmOption.db_write_buffer_size = config.db_write_buffer_size ;
   }

   // Initialize WriteOption
   _lsmWriteOpt.sync = (config.writeSync) ? true : false ;
   _lsmWriteOpt.disableWAL = true ;

   // Initialize ReadOption
   _lsmReadOpt.readahead_size = ( config.readahead_size )
                                ? config.readahead_size : 0 ;
   _lsmReadOpt.fill_cache = ( config.fill_cache ) ? true : false ;

   // Initialize _LsmCFNames and _lsmCFOptions vector
   // the first one MUST be 'default' ColumnFamily
   _initCFNamesAndOptions( config ) ;

   // Initialize ColumnFamily descriptors
   _initCFDescriptors();
}


void lsmDBReadOnly::_initCFNamesAndOptions( const LSMConfig & config )
{
    // house clean, should be done in ctor already
    // _lsmCFNames.clear() ;
    // _lsmCFOptions.clear();

    // the first CF name MUST be 'default' ColumnFamily
    _lsmCFNames.push_back( rocksdb::kDefaultColumnFamilyName ) ;
    // initial column family options before put it in CF option array
    rocksdb::ColumnFamilyOptions defaultCFOpt( _lsmOption ) ;

    // a single memtable size
    if ( config.write_buffer_size > 0 )
    {
       defaultCFOpt.write_buffer_size = config.write_buffer_size ;
    }
    else
    {
       // read only db doesn't write memtable, so can we make it small
       defaultCFOpt.write_buffer_size = 1024 ; // 1K bytes
    }
    // max number of memtables
    if ( config.max_write_buffer_number > 0 )
    {
       defaultCFOpt.max_write_buffer_number = config.max_write_buffer_number ;
    }

    // save default column family option before open db
    _lsmCFOptions.push_back( defaultCFOpt ) ;

    // initial other CF name and CF options here
}


rocksdb::Status lsmDBReadOnly::openLsmDB()
{
   rocksdb::Status s ;

   if ( NULL == _lsmDB )
   {
      // clean up CF handles before open DB, should be done in ctor already
      // _lsmCFHandles.clear();

      // open Rocksdb in read only mode
      s = rocksdb::DB::OpenForReadOnly( _lsmOption, _dbPath,
                                        _lsmCFDescriptors,
                                        &_lsmCFHandles,
                                        &_lsmDB ) ;
      if ( s.ok() )
      {
         SDB_ASSERT( _lsmDB, "Failed open LSMDB !" ) ;
      }
      else
      {
         // Log error messsage
      }
   }
   return s ;
}


rocksdb::Status lsmDBReadOnly::closeLsmDB( BOOLEAN bForceClose )
{
   rocksdb::Status s ;

   SDB_ASSERT( _lsmDB, "Database hasn't been opened!" );

   // drop column family handles
   if ( bForceClose || s.ok() )
   {
      for ( auto cf : _lsmCFHandles )
      {
         s = _lsmDB->DestroyColumnFamilyHandle( cf ) ;
         SDB_ASSERT( s.ok(), "Cloumn Family handle wasn't dropped properly!" ) ;
      }
   }
   // close DB
   if ( bForceClose || s.ok() )
   {
      _lsmDB->Close() ;
      delete _lsmDB ;
      _lsmDB = NULL ;
      _lsmCFHandles.clear();
      _lsmCFDescriptors.clear();
      _lsmCFOptions.clear();
      _lsmCFNames.clear();
   }
   return s ;
}


lsmDBReadOnly::~lsmDBReadOnly()
{
   if ( _lsmDB )
   {
      closeLsmDB() ;
      if ( _lsmDB )
      {
         delete _lsmDB ;
         _lsmDB = NULL ;
      }
   }
   _lsmCFHandles.clear();
   _lsmCFDescriptors.clear();
   _lsmCFOptions.clear();
   _lsmCFNames.clear();
}


} // namespace vessel
} // namespace engine
