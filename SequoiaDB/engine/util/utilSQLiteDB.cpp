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

   Source File Name = utilSQLiteDB.cpp

   Descriptive Name = embedded SQLite database

   When/how to use: this program may be used on binary and text-formatted
   versions of util component. This file contains structure for embedded
   SQLite database.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/01/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#include "utilSQLiteDB.hpp"
#include "pdTrace.hpp"
#include "utilTrace.hpp"
#include "pmd.hpp"

namespace engine
{

   #define DB_RC_CHECK( dbRC, expRC, rc, label, level, fmt, ... ) \
      do {                                                        \
         if ( ( expRC ) != ( dbRC ) )                             \
         {                                                        \
            ( rc ) = _utilSQLiteConvertRC( dbRC ) ;                \
            PD_LOG( ( level ), ( fmt ), ##__VA_ARGS__ ) ;         \
            goto label ;                                          \
         }                                                        \
      } while ( FALSE )

   static INT32 _utilSQLiteConvertRC( INT32 dbRC )
   {
      INT32 rc = SDB_OK ;
      switch ( dbRC )
      {
         case SQLITE_OK :
            rc = SDB_OK ;
            break ;
         case SQLITE_ROW :
            rc = SDB_OK ;
            break ;
         case SQLITE_DONE :
            rc = SDB_DMS_EOC ;
            break ;
         default :
            rc = SDB_SYS ;
            break ;
      }
      return rc ;
   }

   /*
      _utilSQLiteStmtBase implement
    */
   _utilSQLiteStmtBase::_utilSQLiteStmtBase()
   : _database( NULL ),
     _stmt( NULL )
   {
   }

   _utilSQLiteStmtBase::~_utilSQLiteStmtBase()
   {
      _close() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSQLITESTMTBASE__OPEN, "_utilSQLiteStmtBase::_open" )
   INT32 _utilSQLiteStmtBase::_open( sqlite3 *database, sqlite3_stmt *stmt )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSQLITESTMTBASE__OPEN ) ;

      SDB_ASSERT( NULL != database, "database is invalid" ) ;
      SDB_ASSERT( NULL != stmt, "statement is invalid" ) ;

      PD_CHECK( NULL != database, SDB_SYS, error, PDERROR,
                "Failed to open statement, database is invalid" ) ;
      PD_CHECK( NULL != stmt, SDB_SYS, error, PDERROR,
                "Failed to open statement, prepared statement is invalid" ) ;
      PD_CHECK( NULL == _database, SDB_SYS, error, PDERROR,
                "Failed to open statement, statement is already opened" ) ;
      PD_CHECK( NULL == _stmt, SDB_SYS, error, PDERROR,
                "Failed to open statement, statement is already opened" ) ;

      _database = database ;
      _stmt = stmt ;

   done:
      PD_TRACE_EXITRC( SDB__UTILSQLITESTMTBASE__OPEN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   void _utilSQLiteStmtBase::_close()
   {
      if ( NULL != _stmt )
      {
         sqlite3_finalize( _stmt ) ;
         _stmt = NULL ;
      }
      _database = NULL ;
   }

   /*
      _utilSQLiteStatement implement
    */
   _utilSQLiteStatement::_utilSQLiteStatement()
   : _utilSQLiteStmtBase()
   {
   }

   _utilSQLiteStatement::~_utilSQLiteStatement()
   {
      closeStatement() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSQLITESTMT_OPENSTMT, "_utilSQLiteStatement::openStatement" )
   INT32 _utilSQLiteStatement::openStatement( sqlite3 *database,
                                             sqlite3_stmt *stmt )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSQLITESTMT_OPENSTMT ) ;

      rc = _open( database, stmt ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open statement, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__UTILSQLITESTMT_OPENSTMT, rc ) ;
      return rc ;

   error:
      closeStatement() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSQLITESTMT_CLOSESTMT, "_utilSQLiteStatement::closeStatement" )
   void _utilSQLiteStatement::closeStatement()
   {
      PD_TRACE_ENTRY( SDB__UTILSQLITESTMT_CLOSESTMT ) ;

      _close() ;

      PD_TRACE_EXIT( SDB__UTILSQLITESTMT_CLOSESTMT ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSQLITESTMT_EXEC, "_utilSQLiteStatement::execute" )
   INT32 _utilSQLiteStatement::execute()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSQLITESTMT_EXEC ) ;

      INT32 dbRC = SQLITE_OK ;

      PD_CHECK( NULL != _stmt, SDB_SYS, error, PDERROR,
                "Failed to execute statement, statement is not opened" ) ;

      dbRC = sqlite3_step( _stmt ) ;
      DB_RC_CHECK( dbRC, SQLITE_DONE, rc, error, PDERROR,
                   "Failed to execute command, rc: %d, err: %s",
                   dbRC, sqlite3_errmsg( _database ) ) ;

   done:
      PD_TRACE_EXITRC( SDB__UTILSQLITESTMT_EXEC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSQLITESTMT_RESET, "_utilSQLiteStatement::reset" )
   INT32 _utilSQLiteStatement::reset()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSQLITESTMT_RESET ) ;

      INT32 dbRC = SQLITE_OK ;

      PD_CHECK( NULL != _stmt, SDB_SYS, error, PDERROR,
                "Failed to execute statement, statement is not opened" ) ;

      dbRC = sqlite3_reset( _stmt ) ;
      DB_RC_CHECK( dbRC, SQLITE_DONE, rc, error, PDERROR,
                   "Failed to reset command, rc: %d, err: %s",
                   dbRC, sqlite3_errmsg( _database ) ) ;

   done:
      PD_TRACE_EXITRC( SDB__UTILSQLITESTMT_RESET, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSQLITESTMT_BINDFLOAT64AT, "_utilSQLiteStatement::bindFLOAT64At" )
   INT32 _utilSQLiteStatement::bindFLOAT64At( UINT32 position, FLOAT64 value )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSQLITESTMT_BINDFLOAT64AT ) ;

      INT32 dbRC = SQLITE_OK ;

      PD_CHECK( NULL != _stmt, SDB_SYS, error, PDERROR,
                "Failed to bind float, statement is not opened" ) ;

      dbRC = sqlite3_bind_double( _stmt, position, value ) ;
      DB_RC_CHECK( dbRC, SQLITE_OK, rc, error, PDERROR,
                   "Failed to bind float [%u], rc: %d, err: %s",
                   position, dbRC, sqlite3_errmsg( _database ) ) ;

   done:
      PD_TRACE_EXITRC( SDB__UTILSQLITESTMT_BINDFLOAT64AT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSQLITESTMT_BINDINT32AT, "_utilSQLiteStatement::bindINT32At" )
   INT32 _utilSQLiteStatement::bindINT32At( UINT32 position, INT32 value )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSQLITESTMT_BINDINT32AT ) ;

      INT32 dbRC = SQLITE_OK ;

      PD_CHECK( NULL != _stmt, SDB_SYS, error, PDERROR,
                "Failed to bind integer, statement is not opened" ) ;

      dbRC = sqlite3_bind_int( _stmt, position, value ) ;
      DB_RC_CHECK( dbRC, SQLITE_OK, rc, error, PDERROR,
                   "Failed to bind integer at [%u], rc: %d, err: %s",
                   position, dbRC, sqlite3_errmsg( _database ) ) ;

   done:
      PD_TRACE_EXITRC( SDB__UTILSQLITESTMT_BINDINT32AT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSQLITESTMT_BINDINT64AT, "_utilSQLiteStatement::bindINT64At" )
   INT32 _utilSQLiteStatement::bindINT64At( UINT32 position, INT64 value )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSQLITESTMT_BINDINT64AT ) ;

      INT32 dbRC = SQLITE_OK ;

      PD_CHECK( NULL != _stmt, SDB_SYS, error, PDERROR,
                "Failed to bind long, statement is not opened" ) ;

      dbRC = sqlite3_bind_int64( _stmt, position, value ) ;
      DB_RC_CHECK( dbRC, SQLITE_OK, rc, error, PDERROR,
                   "Failed to bind long at [%u], rc: %d, err: %s",
                   position, dbRC, sqlite3_errmsg( _database ) ) ;

   done:
      PD_TRACE_EXITRC( SDB__UTILSQLITESTMT_BINDINT64AT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSQLITESTMT_BINDTEXTAT, "_utilSQLiteStatement::bindTEXTAt" )
   INT32 _utilSQLiteStatement::bindTEXTAt( UINT32 position, const CHAR *value )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSQLITESTMT_BINDTEXTAT ) ;

      INT32 dbRC = SQLITE_OK ;

      SDB_ASSERT( NULL != value, "value is invalid" ) ;

      PD_CHECK( NULL != _stmt, SDB_SYS, error, PDERROR,
                "Failed to bind long, statement is not opened" ) ;

      dbRC = sqlite3_bind_text( _stmt, position, value, ossStrlen( value ),
                                NULL ) ;
      DB_RC_CHECK( dbRC, SQLITE_OK, rc, error, PDERROR,
                   "Failed to bind text [%u], rc: %d, err: %s",
                   position, dbRC, sqlite3_errmsg( _database ) ) ;

   done:
      PD_TRACE_EXITRC( SDB__UTILSQLITESTMT_BINDTEXTAT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _utilSQLiteContext implement
    */
   _utilSQLiteContext::_utilSQLiteContext()
   : _utilSQLiteStmtBase(),
     _hitEnd( FALSE ),
     _columnCount( 0 )
   {
   }

   _utilSQLiteContext::~_utilSQLiteContext()
   {
      closeContext() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSQLITECTX_OPENCTX, "_utilSQLiteContext::openContext" )
   INT32 _utilSQLiteContext::openContext( sqlite3 *database,
                                         sqlite3_stmt *stmt )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSQLITECTX_OPENCTX ) ;

      INT32 tmpCount = 0 ;

      rc = _open( database, stmt ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open statement, rc: %d", rc ) ;

      tmpCount = sqlite3_column_count( stmt ) ;
      PD_CHECK( tmpCount > 0, SDB_SYS, error, PDERROR,
                "Failed to open context, prepared statement returns no "
                "data" ) ;

      _hitEnd = FALSE ;
      _columnCount = (UINT32)tmpCount ;

   done:
      PD_TRACE_EXITRC( SDB__UTILSQLITECTX_OPENCTX, rc ) ;
      return rc ;

   error:
      closeContext() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSQLITECTX_CLOSECTX, "_utilSQLiteContext::closeContext" )
   void _utilSQLiteContext::closeContext()
   {
      PD_TRACE_ENTRY( SDB__UTILSQLITECTX_CLOSECTX ) ;

      _hitEnd = FALSE ;
      _columnCount = 0 ;
      _columnMap.clear() ;

      _close() ;

      PD_TRACE_EXIT( SDB__UTILSQLITECTX_CLOSECTX ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSQLITECTX_MOVENEXT, "_utilSQLiteContext::moveNext" )
   INT32 _utilSQLiteContext::moveNext()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSQLITECTX_MOVENEXT ) ;

      INT32 dbRC = SQLITE_OK ;

      PD_CHECK( !_hitEnd, SDB_DMS_EOC, error, PDDEBUG, "Failed to move next, "
                "already finished fetch" ) ;
      PD_CHECK( NULL != _stmt, SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to move next, context is not opened" ) ;

      dbRC = sqlite3_step( _stmt ) ;
      if ( SQLITE_DONE == dbRC )
      {
         PD_LOG( PDDEBUG, "Done for context" ) ;
         _hitEnd = TRUE ;
         rc = SDB_DMS_EOC ;
         goto done ;
      }
      DB_RC_CHECK( dbRC, SQLITE_ROW, rc, error, PDERROR,
                   "Failed to move next row, rc: %d", dbRC ) ;

   done:
      PD_TRACE_EXITRC( SDB__UTILSQLITECTX_MOVENEXT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSQLITECTX_GETCOLNAME, "_utilSQLiteContext::getColumnName" )
   INT32 _utilSQLiteContext::getColumnName( UINT32 position, const CHAR *&name )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSQLITECTX_GETCOLNAME ) ;

      PD_CHECK( NULL != _stmt, SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to get column name, context is not opened" ) ;
      PD_CHECK( position < _columnCount, SDB_OUT_OF_BOUND, error, PDERROR,
                "Failed to get column name, position is out of bound" ) ;

      name = sqlite3_column_name( _stmt, position ) ;

   done:
      PD_TRACE_EXITRC( SDB__UTILSQLITECTX_GETCOLNAME, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSQLITECTX_GETFLOAT64AT, "_utilSQLiteContext::getFLOAT64At" )
   INT32 _utilSQLiteContext::getFLOAT64At( UINT32 position, FLOAT64 &value )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSQLITECTX_GETFLOAT64AT ) ;

      PD_CHECK( NULL != _stmt, SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to get float, context is not opened" ) ;
      PD_CHECK( position < _columnCount, SDB_OUT_OF_BOUND, error, PDERROR,
                "Failed to get float, position is out of bound" ) ;

      value = sqlite3_column_double( _stmt, position ) ;

   done:
      PD_TRACE_EXITRC( SDB__UTILSQLITECTX_GETFLOAT64AT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSQLITECTX_GETINT32AT, "_utilSQLiteContext::getINT32At" )
   INT32 _utilSQLiteContext::getINT32At( UINT32 position, INT32 &value )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSQLITECTX_GETINT32AT ) ;

      PD_CHECK( NULL != _stmt, SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to get integer, context is not opened" ) ;
      PD_CHECK( position < _columnCount, SDB_OUT_OF_BOUND, error, PDERROR,
                "Failed to get integer, position is out of bound" ) ;

      value = sqlite3_column_int( _stmt, position ) ;

   done:
      PD_TRACE_EXITRC( SDB__UTILSQLITECTX_GETINT32AT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSQLITECTX_GETINT64AT, "_utilSQLiteContext::getINT64At" )
   INT32 _utilSQLiteContext::getINT64At( UINT32 position, INT64 &value )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSQLITECTX_GETINT64AT ) ;

      PD_CHECK( NULL != _stmt, SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to get long, context is not opened" ) ;
      PD_CHECK( position < _columnCount, SDB_OUT_OF_BOUND, error, PDERROR,
                "Failed to get long, position is out of bound" ) ;

      value = sqlite3_column_int64( _stmt, position ) ;

   done:
      PD_TRACE_EXITRC( SDB__UTILSQLITECTX_GETINT64AT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSQLITECTX_GETTEXTAT, "_utilSQLiteContext::getTEXTAt" )
   INT32 _utilSQLiteContext::getTEXTAt( UINT32 position, const CHAR *&value )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSQLITECTX_GETTEXTAT ) ;

      PD_CHECK( NULL != _stmt, SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to get text, context is not opened" ) ;
      PD_CHECK( position < _columnCount, SDB_OUT_OF_BOUND, error, PDERROR,
                "Failed to get text, position is out of bound" ) ;

      value = (const CHAR *)( sqlite3_column_text( _stmt, position ) ) ;

   done:
      PD_TRACE_EXITRC( SDB__UTILSQLITECTX_GETTEXTAT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSQLITECTX_GETFLOAT64_NAME, "_utilSQLiteContext::getFLOAT64" )
   INT32 _utilSQLiteContext::getFLOAT64( const CHAR *name, FLOAT64 &value )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSQLITECTX_GETFLOAT64_NAME ) ;

      UINT32 position = 0 ;

      PD_CHECK( NULL != _stmt, SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to get float, context is not opened" ) ;

      rc = _getColumnPosition( name, position ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to find column [%s], rc: %d",
                   name, rc ) ;

      value = sqlite3_column_double( _stmt, position ) ;

   done:
      PD_TRACE_EXITRC( SDB__UTILSQLITECTX_GETFLOAT64_NAME, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSQLITECTX_GETINT32_NAME, "_utilSQLiteContext::getINT32" )
   INT32 _utilSQLiteContext::getINT32( const CHAR *name, INT32 &value )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSQLITECTX_GETINT32_NAME ) ;

      UINT32 position = 0 ;

      PD_CHECK( NULL != _stmt, SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to get integer, context is not opened" ) ;

      rc = _getColumnPosition( name, position ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to find column [%s], rc: %d",
                   name, rc ) ;

      value = sqlite3_column_int( _stmt, position ) ;

   done:
      PD_TRACE_EXITRC( SDB__UTILSQLITECTX_GETINT32_NAME, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSQLITECTX_GETINT64_NAME, "_utilSQLiteContext::getINT64" )
   INT32 _utilSQLiteContext::getINT64( const CHAR *name, INT64 &value )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSQLITECTX_GETINT64_NAME ) ;

      UINT32 position = 0 ;

      PD_CHECK( NULL != _stmt, SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to get long, context is not opened" ) ;

      rc = _getColumnPosition( name, position ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to find column [%s], rc: %d",
                   name, rc ) ;

      value = sqlite3_column_int64( _stmt, position ) ;

   done:
      PD_TRACE_EXITRC( SDB__UTILSQLITECTX_GETINT64_NAME, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSQLITECTX_GETTEXT_NAME, "_utilSQLiteContext::getTEXT" )
   INT32 _utilSQLiteContext::getTEXT( const CHAR *name, const CHAR *&value )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSQLITECTX_GETTEXT_NAME ) ;

      UINT32 position = 0 ;

      PD_CHECK( NULL != _stmt, SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to get text, context is not opened" ) ;

      rc = _getColumnPosition( name, position ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to find column [%s], rc: %d",
                   name, rc ) ;

      value = (const CHAR *)( sqlite3_column_text( _stmt, position ) ) ;

   done:
      PD_TRACE_EXITRC( SDB__UTILSQLITECTX_GETTEXT_NAME, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSQLITECTX__GETCOLPOS, "_utilSQLiteContext::_getColumnPosition" )
   INT32 _utilSQLiteContext::_getColumnPosition( const CHAR *name,
                                                UINT32 &position )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSQLITECTX__GETCOLPOS ) ;

      SDB_ASSERT( NULL != name, "name is invalid" ) ;

      UTIL_SQLITE_COLUMN_MAP::iterator iter ;

      if ( !_isColumnMapped() )
      {
         rc = _mapColumns() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to map columns, rc: %d", rc ) ;
      }

      // convert CHAR * to ossPoolString requires memory allocation
      // if memory allocation failed, iterate elements in map one by one
      try
      {
         iter = _columnMap.find( name ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDWARNING, "Failed to find name [%s] in column map, "
                 "occur exception: %s", name, e.what() ) ;

         iter = _columnMap.begin() ;
         while ( iter != _columnMap.end() )
         {
            if ( 0 == ossStrcmp( iter->first.c_str(), name ) )
            {
               break ;
            }
            ++ iter ;
         }
      }

      // check if we found the column
      PD_CHECK( iter != _columnMap.end(), SDB_SYS, error, PDERROR,
                "Failed to get position by column [%s], it is not found",
                name ) ;

      position = iter->second ;

   done:
      PD_TRACE_EXITRC( SDB__UTILSQLITECTX__GETCOLPOS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSQLITECTX__MAPCOLS, "_utilSQLiteContext::_mapColumns" )
   INT32 _utilSQLiteContext::_mapColumns()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSQLITECTX__MAPCOLS ) ;

      PD_CHECK( NULL != _stmt, SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to map columns, context is not opened" ) ;
      PD_CHECK( _columnCount > 0, SDB_SYS, error, PDERROR,
                "Failed to map columns, column set is empty" ) ;

      _columnMap.clear() ;
      for ( UINT32 position = 0 ; position < _columnCount ; ++ position )
      {
         const CHAR *name = NULL ;

         // get name from meta data
         name = sqlite3_column_name( _stmt, position ) ;
         PD_CHECK( NULL != name, SDB_SYS, error, PDERROR,
                   "Failed to get name of column [%u] from statement",
                   position ) ;

         // save to map
         try
         {
            _columnMap.insert( make_pair( name, position ) ) ;
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to save column, occur exception: %s",
                    e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXIT( SDB__UTILSQLITECTX__MAPCOLS ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _utilSQLiteDB implement
    */
   _utilSQLiteDB::_utilSQLiteDB()
   : _database( NULL )
   {
      _databasePath[ 0 ] = '\0' ;
   }

   _utilSQLiteDB::~_utilSQLiteDB()
   {
      _closeDatabase() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSQLITEDB_INITIALIZE, "_utilSQLiteDB::initialize" )
   INT32 _utilSQLiteDB::initialize( const CHAR *databasePath )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSQLITEDB_INITIALIZE ) ;

      SDB_ASSERT( NULL != databasePath, "database path is invalid" ) ;

      rc = _openDatabase( databasePath ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open database [%s], rc: %d",
                   databasePath, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__UTILSQLITEDB_INITIALIZE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSQLITEDB_FINALIZE, "_utilSQLiteDB::finalize" )
   void _utilSQLiteDB::finalize()
   {
      PD_TRACE_ENTRY( SDB__UTILSQLITEDB_FINALIZE ) ;

      _closeDatabase() ;

      PD_TRACE_EXIT( SDB__UTILSQLITEDB_FINALIZE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSQLITEDB_EXECCMD, "_utilSQLiteDB::execCommand" )
   INT32 _utilSQLiteDB::execCommand( const CHAR *command )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSQLITEDB_EXECCMD ) ;

      INT32 dbRC = SQLITE_OK ;

      PD_CHECK( NULL != _database, SDB_SYS, error, PDERROR,
                "Failed to execute command, database is not opened" ) ;

      PD_LOG( PDDEBUG, "Execute command: %s", command ) ;

      dbRC = sqlite3_exec( _database, command, NULL, NULL, NULL ) ;
      DB_RC_CHECK( dbRC, SQLITE_OK, rc, error, PDERROR,
                   "Failed to execute command [%s], rc: %d, err: %s",
                   command, dbRC, sqlite3_errmsg( _database ) ) ;

   done:
      PD_TRACE_EXITRC( SDB__UTILSQLITEDB_EXECCMD, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSQLITEDB_PREPARECMD, "_utilSQLiteDB::prepareCommand" )
   INT32 _utilSQLiteDB::prepareCommand( const CHAR *command,
                                       utilSQLiteStatement &statement )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSQLITEDB_PREPARECMD ) ;

      INT32 dbRC = SQLITE_OK ;
      sqlite3_stmt *stmt = NULL ;

      PD_CHECK( NULL != _database, SDB_SYS, error, PDERROR,
                      "Failed to execute command, database is not opened" ) ;

      PD_LOG( PDDEBUG, "Execute command: %s", command ) ;

      dbRC = sqlite3_prepare_v2( _database, command, ossStrlen( command ),
                                 &stmt, NULL ) ;
      DB_RC_CHECK( dbRC, SQLITE_OK, rc, error, PDERROR,
                   "Failed to prepare command [%s], rc: %d, err: %s",
                   command, dbRC, sqlite3_errmsg( _database ) ) ;

      rc = statement.openStatement( _database, stmt ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open statement for command [%s], "
                   "rc: %d", command, rc ) ;

      // take owned by statement
      stmt = NULL ;

   done:
      if ( NULL != stmt )
      {
         sqlite3_finalize( stmt ) ;
      }
      PD_TRACE_EXITRC( SDB__UTILSQLITEDB_PREPARECMD, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSQLITEDB_EXECQUERY, "_utilSQLiteDB::execQuery" )
   INT32 _utilSQLiteDB::execQuery( const CHAR *query,
                                  utilSQLiteContext &context )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSQLITEDB_EXECQUERY ) ;

      INT32 dbRC = SQLITE_OK ;
      sqlite3_stmt *stmt = NULL ;

      PD_CHECK( NULL != _database, SDB_SYS, error, PDERROR,
                "Failed to execute query, database is not opened" ) ;

      PD_LOG( PDDEBUG, "Execute query: %s", query ) ;

      dbRC = sqlite3_prepare_v2( _database, query, ossStrlen( query ), &stmt,
                                 NULL ) ;
      DB_RC_CHECK( dbRC, SQLITE_OK, rc, error, PDERROR,
                   "Failed to prepare query [%s], rc: %d, err: %s",
                   query, dbRC, sqlite3_errmsg( _database ) ) ;

      rc = context.openContext( _database, stmt ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open context for query [%s], "
                   "rc: %d", query, rc ) ;

      // take owned by context
      stmt = NULL ;

   done:
      if ( NULL != stmt )
      {
         sqlite3_finalize( stmt ) ;
      }
      PD_TRACE_EXITRC( SDB__UTILSQLITEDB_EXECQUERY, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSQLITEDB__OPENDB, "_utilSQLiteDB::_openDatabase" )
   INT32 _utilSQLiteDB::_openDatabase( const CHAR *databasePath )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSQLITEDB__OPENDB ) ;

      SDB_ASSERT( NULL != databasePath, "database path is invalid" ) ;

      INT32 dbRC = SQLITE_OK ;

      PD_CHECK( NULL == _database, SDB_SYS, error, PDERROR,
                "Failed to open database, database is already opened" ) ;

      // open database
      dbRC = sqlite3_open( databasePath, &_database ) ;
      DB_RC_CHECK( dbRC, SQLITE_OK, rc, error, PDERROR,
                   "Failed to open database [%s], rc: %d, err: %s",
                   databasePath, dbRC, sqlite3_errmsg( _database ) ) ;

      // set cache size to 100 pages
      dbRC = sqlite3_exec( _database, "PRAGMA cache_size = 100", NULL, NULL,
                           NULL ) ;
      DB_RC_CHECK( dbRC, SQLITE_OK, rc, error, PDERROR,
                   "Failed to set cache size of database [%s], "
                   "rc: %d, err: %s", databasePath, dbRC,
                   sqlite3_errmsg( _database ) ) ;

      // save database path
      ossStrncpy( _databasePath, databasePath, OSS_MAX_PATHSIZE ) ;

   done:
      PD_TRACE_EXITRC( SDB__UTILSQLITEDB__OPENDB, rc ) ;
      return rc ;

   error:
      _closeDatabase() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSQLITEDB__CLOSEDB, "_utilSQLiteDB::_closeDatabase" )
   void _utilSQLiteDB::_closeDatabase()
   {
      PD_TRACE_ENTRY( SDB__UTILSQLITEDB__CLOSEDB ) ;

      if ( NULL != _database )
      {
         _databasePath[ 0 ] = '\0' ;

         sqlite3_close( _database ) ;
         _database = NULL ;
      }

      PD_TRACE_EXIT( SDB__UTILSQLITEDB__CLOSEDB ) ;
   }

}
