/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = utilSQLiteDB.hpp

   Descriptive Name = embedded SQLite database

   When/how to use: this program may be used on binary and text-formatted
   versions of util component. This file contains structure for embedded
   SQLite database.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/20/2020  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_SQLITE_DB_HPP__
#define UTIL_SQLITE_DB_HPP__

#include "core.hpp"
#include "ossUtil.hpp"
#include "utilPooledObject.hpp"
#include "ossMemPool.hpp"

#include "sqlite3.h"

namespace engine
{

   // maximum size of SQL for query or command
   #define UTIL_MAX_SQL_SIZE ( 512 )

   /*
      _utilSQLiteStmtBase define
    */
   // base class for statement ( can be a query or command )
   // WARNING: a base class, should not be instance
   class _utilSQLiteStmtBase : public utilPooledObject
   {
   protected:
      _utilSQLiteStmtBase() ;
      ~_utilSQLiteStmtBase() ;

      // open statement
      INT32 _open( sqlite3 *database, sqlite3_stmt *stmt ) ;
      // close statement
      void _close() ;

   public:
      // check if statement is closed
      BOOLEAN isClosed() const
      {
         return NULL == _stmt ? TRUE : FALSE ;
      }

      // check if statement is opened
      BOOLEAN isOpened() const
      {
         return NULL != _stmt ? TRUE : FALSE ;
      }

   protected:
      // pointer to SQLite database
      sqlite3 *      _database ;
      // pointer to SQLite statement
      sqlite3_stmt * _stmt ;
   } ;

   typedef class _utilSQLiteStmtBase utilSQLiteStmtBase ;

   /*
      _utilSQLiteStatement define
    */
   // prepared statement holds a single SQL statement which has been compiled
   // into binary form and is ready to be evaluated
   // the life-cycle of a prepared statement:
   // 1. create the prepared statement by _utilSQLiteDB::prepareCommand()
   // 2. bind values to parameters using _utilSQLiteStatement::bindXXX()
   //    interfaces
   // 3. run statement by calling _utilSQLiteStatement::execute()
   // 4. reset statement by _utilSQLiteStatement::reset() to restart,
   //    and loop again from step 2
   // 5. finalize statement by _utilSQLiteStatement::closeStatement()
   class _utilSQLiteStatement : public _utilSQLiteStmtBase
   {
   public:
      _utilSQLiteStatement() ;
      ~_utilSQLiteStatement() ;

      // open statement
      INT32 openStatement( sqlite3 *database, sqlite3_stmt *stmt ) ;
      // close statement
      void closeStatement() ;

      // execute statement
      INT32 execute() ;

      // reset statement
      INT32 reset() ;

      // bind double parameter
      // NOTE: position starts from 1
      INT32 bindFLOAT64At( UINT32 position, FLOAT64 value ) ;
      // bind 32-bit integer parameter
      // NOTE: position starts from 1
      INT32 bindINT32At( UINT32 position, INT32 value ) ;
      // bind 64-bit integer parameter
      // NOTE: position starts from 1
      INT32 bindINT64At( UINT32 position, INT64 value ) ;
      // bind text parameter
      // NOTE: position starts from 1
      INT32 bindTEXTAt( UINT32 position, const CHAR *value ) ;
   } ;

   typedef class _utilSQLiteStatement utilSQLiteStatement ;

   /*
      _utilSQLiteContext define
    */
   // result context for query holds the result set of a query, and provides
   // interfaces to extract values from result
   // the life-cycle of a query context:
   // 1. create the context by _utilSQLiteDB::execQuery()
   // 2. move to next available row by _utilSQLiteContext::moveNext()
   // 3. get values of columns by _utilSQLiteContext::getXXX() interfaces
   // 4. loop again from step 2
   // 5. finalize context by _utilSQLiteContext::closeContext()
   class _utilSQLiteContext : public _utilSQLiteStmtBase
   {
   public:
      _utilSQLiteContext() ;
      ~_utilSQLiteContext() ;

      // open context
      INT32 openContext( sqlite3 *database, sqlite3_stmt *stmt ) ;
      // close context
      void closeContext() ;

      // move to the next row
      INT32 moveNext() ;

      // get column name at position
      // NOTE: position starts from 0
      INT32 getColumnName( UINT32 position, const CHAR *&name ) ;

      // get double value at given position
      // NOTE: position starts from 0
      INT32 getFLOAT64At( UINT32 position, FLOAT64 &value ) ;
      // get 32-bit integer value at given position
      // NOTE: position starts from 0
      INT32 getINT32At( UINT32 position, INT32 &value ) ;
      // get 64-bit integer value at given position
      // NOTE: position starts from 0
      INT32 getINT64At( UINT32 position, INT64 &value ) ;
      // get text value at given position
      // NOTE: position starts from 0
      INT32 getTEXTAt( UINT32 position, const CHAR *&value ) ;

      // get double value at given column
      INT32 getFLOAT64( const CHAR *name, FLOAT64 &value ) ;
      // get 32-bit integer value at given column
      INT32 getINT32( const CHAR *name, INT32 &value ) ;
      // get 64-bit integer value at given column
      INT32 getINT64( const CHAR *name, INT64 &value ) ;
      // get text value at given column
      INT32 getTEXT( const CHAR *name, const CHAR *&value ) ;

      // check if context is ended
      BOOLEAN isEnded() const
      {
         return _hitEnd ;
      }

      // get count of column
      UINT32 getColumnCount() const
      {
         return _columnCount ;
      }

   protected:
      // get position of column
      INT32 _getColumnPosition( const CHAR *name, UINT32 &position ) ;
      // map columns and positions
      INT32 _mapColumns() ;

      // check if columns are mapped with positions
      BOOLEAN _isColumnMapped() const
      {
         return _columnMap.empty() ? FALSE : TRUE ;
      }

   protected:
      typedef ossPoolMap< ossPoolString, UINT32 > UTIL_SQLITE_COLUMN_MAP ;

      // flag to hit end
      BOOLEAN              _hitEnd ;
      // count of columns
      UINT32               _columnCount ;
      // mapping between columns and positions
      UTIL_SQLITE_COLUMN_MAP _columnMap ;
   } ;

   typedef class _utilSQLiteContext utilSQLiteContext ;

   /*
      _utilSQLiteDB define
    */
   // SQLite database holds SQLite database file, and provides interfaces to
   // manipulate data in database
   // the life-cycle of a SQLite database:
   // 1. initialize the database by _utilSQLiteDB::initialize()
   //    if database file doesn't exist, will create a new one
   // 2. execute command, query by _utilSQLiteDB::execCommand(),
   //    _utilSQLiteDB::prepareCommon(), or _utilSQLiteDB::execQuery()
   // 3. finalize context by _utilSQLiteDB::finalize()
   class _utilSQLiteDB : public SDBObject
   {
   public:
      _utilSQLiteDB() ;
      ~_utilSQLiteDB() ;

      // initialize database
      INT32 initialize( const CHAR *databasePath ) ;
      // finalize database
      void finalize() ;

      // execute command
      INT32 execCommand( const CHAR *command ) ;
      // prepare command into a prepared statement
      INT32 prepareCommand( const CHAR *command,
                            utilSQLiteStatement &statement ) ;
      // execute a query and put results into a context
      INT32 execQuery( const CHAR *query,
                       utilSQLiteContext &context ) ;

      // check if database is closed
      BOOLEAN isClosed() const
      {
         return NULL == _database ? TRUE : FALSE ;
      }

      // check if database is opened
      BOOLEAN isOpened() const
      {
         return NULL != _database ? TRUE : FALSE ;
      }

      // get path of database
      const CHAR *getDatabasePath() const
      {
         return _databasePath ;
      }

   protected:
      // open SQLite database from given database path
      INT32 _openDatabase( const CHAR *databasePath ) ;
      // close SQLite database
      void  _closeDatabase() ;

   protected:
      // copy of database path
      CHAR        _databasePath[ OSS_MAX_PATHSIZE + 1 ] ;
      // pointer to database
      sqlite3 *   _database ;
   } ;

   typedef class _utilSQLiteDB utilSQLiteDB ;

}

#endif // UTIL_SQLITE_DB_HPP__
