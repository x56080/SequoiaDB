/*******************************************************************************


   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = dpsDump.cpp

   Descriptive Name = Data Protection Service Log Formatter

   When/how to use: this program may be used on binary and text-formatted
   versions of data protection component. This file contains code to format log
   files.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          8/06/2015  LZ  Initial Draft

   Last Changed =

*******************************************************************************/
#include "sdbDpsDump.hpp"
#include "pmdOptionsMgr.hpp"
#include "dpsLogRecord.hpp"
#include "dpsLogFile.hpp"
#include "dpsDump.hpp"
#include "ossVer.hpp"
#include "utilStr.hpp"
#include "dpsLogRecordDef.hpp"
#include "ossPath.hpp"
#include "utilCommon.hpp"

#include <boost/filesystem.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

namespace fs = boost::filesystem ;
using namespace engine;

#ifndef R_OK
#define R_OK   4
#endif //R_OK

#define REPLOG_NAME_PREFIX "sequoiadbLog"
#define SEP_CHAR_DOT "."
#define LOG_FILE_NAME "dpsDumpLog.log"
OSSFILE logFile ;
const static CHAR *logFMT = "Level: %s"OSS_NEWLINE"Function: %-32s"OSS_NEWLINE
                            "File: %s"OSS_NEWLINE"Line: %d"OSS_NEWLINE
                            "Message:"OSS_NEWLINE"%s"OSS_NEWLINE""OSS_NEWLINE;

void writeLog( BOOLEAN console, const CHAR *type, const CHAR *func,
               const CHAR *file, const int line, const CHAR *fmt, ... )
{

   INT32 rc = SDB_OK ;
   CHAR msg[4096] = { 0 } ;
   CHAR wContent[4096] = { 0 } ;
   va_list ap ;

   va_start( ap, fmt );
   vsnprintf( msg, 4096, fmt, ap ) ;
   va_end( ap ) ;

   ossSnprintf( wContent, 4096, logFMT, type, func, file, line, msg ) ;
   if ( console )
   {
      std::cout << type << ": " << msg << std::endl ;
   }

   {
      INT64 toWrite = ossStrlen( wContent ) ; ;
      INT64 writePos  = 0 ;
      INT64 writeSize = 0 ;
      while( toWrite > 0 )
      {
         rc = ossWrite( &logFile, wContent + writePos, toWrite, &writeSize ) ;
         if ( SDB_OK != rc && SDB_INTERRUPT != rc )
         {
            std::cout << "Failed to log to file" << std::endl ;
            goto done ;
         }

         rc = SDB_OK ;
         toWrite -= writeSize ;
         writePos += writeSize ;
      }
   }

done:
   return;
}

#define __LOG_WRAPPER( out, LEVEL, fmt, ... )                                  \
do                                                                             \
{                                                                              \
   writeLog( out, LEVEL, __FUNCTION__, __FILE__, __LINE__, fmt, __VA_ARGS__ ) ;\
} while ( FALSE )


#define LogError( fmt, ... )                    \
   __LOG_WRAPPER( TRUE, "Error", fmt, __VA_ARGS__ )

#define LogEvent( fmt, ... )                    \
   __LOG_WRAPPER( FALSE, "Event", fmt, __VA_ARGS__ )

INT32 writeToFile( OSSFILE &file, const CHAR* pContent, BOOLEAN console )
{
   INT32 rc        = SDB_OK ;
   ///< write buffer
   if ( console )
   {
      std::cout << pContent ;
   }
   else
   {
      INT64 len       = ossStrlen( pContent ) ;
      INT64 writePos  = 0 ;
      INT64 writeSize = 0 ;
      while( writePos < len )
      {
         rc = ossWrite( &file, pContent + writePos, len, &writeSize ) ;
         if( rc && SDB_INTERRUPT != rc )
         {
            LogError( "Failed to write data to file, data: %s", pContent ) ;
            goto error ;
         }
         rc = SDB_OK ;
         writePos += writeSize ;
      }
   }

done:
   return rc ;
error:
   goto done ;
}

/// filter factory implementation
_dpsFilterFactory::_dpsFilterFactory()
{
}

_dpsFilterFactory::~_dpsFilterFactory()
{
   std::list< dpsDumpFilter * >::iterator it = _filterList.begin() ;
   for ( ; it != _filterList.end() ; ++it )
   {
      release( *it ) ;
   }

   _filterList.clear() ;
}

_dpsFilterFactory* _dpsFilterFactory::getInstance()
{
   static _dpsFilterFactory factory ;
   return &factory ;
}

dpsDumpFilter* _dpsFilterFactory::createFilter( int type )
{
   dpsDumpFilter *filter = NULL ;

   switch( type )
   {
   case SDB_LOG_FILTER_TYPE :
      {
         filter = SDB_OSS_NEW dpsTypeFilter() ;
         break ;
      }
   case SDB_LOG_FILTER_NAME :
      {
         filter = SDB_OSS_NEW dpsNameFilter() ;
         break ;
      }
   case SDB_LOG_FILTER_LSN  :
      {
         filter = SDB_OSS_NEW dpsLsnFilter() ;
         break ;
      }
    case SDB_LOG_FILTER_META :
      {
          filter = SDB_OSS_NEW dpsMetaFilter() ;
          break ;
      }
   case SDB_LOG_FILTER_NONE :
      {
         filter = SDB_OSS_NEW dpsNoneFilter() ;
         break ;
      }
   case SDB_LOG_FILTER_LAST :
      {
         filter = SDB_OSS_NEW dpsLastFilter() ;
         break ;
      }

   default:
      LogError("unknown filter type: %d", type) ;
      goto error ;
   }

   if( NULL == filter )
   {
      LogError("Unable to allocate filter, type: %d", type) ;
      goto error ;
   }
   _filterList.push_back( filter ) ;

done:
   return filter ;
error:
   goto done ;
}

void _dpsFilterFactory::release( dpsDumpFilter *filter )
{
   if( NULL != filter )
   {
      SDB_OSS_DEL( filter ) ;
      filter = NULL ;
   }
}

/////////////////////////////////////////////////////////////////////
// this block is used to add filter's implementation of interface doFilter()
// and other interface was declared in macro DECLARE_FILTER(...)

////////////////////////////////////////////////////////////////////
/// for dpsTypeFilter
BOOLEAN _dpsTypeFilter::match( dpsDumper *dumper, CHAR *pRecord )
{
   BOOLEAN rc = FALSE ;
   dpsLogRecordHeader *pHeader =(dpsLogRecordHeader*)( pRecord ) ;
   if( pHeader->_type == dumper->opType )
   {
      rc = dpsDumpFilter::match( dumper, pRecord ) ;
      goto done ;
   }

done:
   return rc ;
}

INT32 _dpsTypeFilter::doFilte( dpsDumper *dumper, OSSFILE &out,
                               const CHAR *logFilePath )
{
   INT32 rc = SDB_OK ;

   rc = dumper->filte( this, out, logFilePath ) ;
   if( rc )
   {
      //PD_LOG( "!parse log file: [%s] error, rc = %d", logFilePath, rc ) ;
      goto error ;
   }

done:
   return rc;

error:
   goto done;
}

////////////////////////////////////////////////////////////////////
///< for _dpsNameFilter
BOOLEAN _dpsNameFilter::match( dpsDumper *dumper, CHAR *pRecord )
{
   BOOLEAN rc = FALSE ;
   dpsLogRecord record ;
   record.load( pRecord ) ;
   dpsLogRecord::iterator itr = record.find( DPS_LOG_PUBLIC_FULLNAME ) ;

   if( 0 == ossStrncmp( dumper->name, "", sizeof( dumper->name ) ) )
   {
      rc = dpsDumpFilter::match( dumper, pRecord ) ;
      goto done ;
   }

   if( itr.valid() )
   {
      if( NULL != ossStrstr( itr.value(), dumper->name ) )
      {
         rc = dpsDumpFilter::match( dumper, pRecord ) ;
         goto done ;
      }
   }

done:
   return rc ;
}

INT32 _dpsNameFilter::doFilte( dpsDumper *dumper, OSSFILE &out,
                               const CHAR *logFilePath )
{
   INT32 rc = SDB_OK ;

   rc = dumper->filte( this, out, logFilePath ) ;
   if( rc )
   {
      //PD_LOG( "!parse log file: [%s] error, rc = %d", filename, rc ) ;
      goto error ;
   }

done:
   return rc;

error:
   goto done;
}

////////////////////////////////////////////////////////////////////
///< for _dpsMetaFilter
BOOLEAN _dpsMetaFilter::match( dpsDumper *dumper, CHAR *pRecord )
{
   return dpsDumpFilter::match( dumper, pRecord ) ;
}

INT32 _dpsMetaFilter::doFilte( dpsDumper *dumper, OSSFILE &out,
                               const CHAR *logFilePath )
{
   return SDB_OK ;
}

/*
////////////////////////////////////////////////////////////////////
///< for _dpsMetaFilter
BOOLEAN _dpsMetaFilter::match( const dpsCmdData *data, CHAR *pRecord )
{
   return FALSE ;
}

INT32 _dpsMetaFilter::doFilte( const dpsCmdData *data, OSSFILE &out,
                              const CHAR *logFilePath )
{
   INT32 rc             = SDB_OK ;
   UINT64 len           = 0 ;
   UINT64 outBufferSize = BLOCK_SIZE ;
   CHAR *pOutBuffer     = NULL ;
   BOOLEAN start        = FALSE ;
   dpsMetaData metaData ;
   if( dpsLogFilter::isDir( data->srcPath ) )
   {
      INT32 const MAX_FILE_COUNT =
         _dpsLogFilter::getFileCount( data->srcPath ) ;
      if( 0 == MAX_FILE_COUNT )
      {
         printf( "Cannot find any Log files\nPlease check"
            " and input the correct log file path\n" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      printf("Analysis Log File Data begin...\n\n" ) ;
      start = TRUE ;
      for( INT32 idx = 0 ; idx < MAX_FILE_COUNT ; ++idx )
      {
         // src log file ;
         fs::path fileDir( data->srcPath ) ;
         const CHAR *filepath = fileDir.string().c_str() ;
         CHAR shortName[ 30 ] = { 0 } ;
         CHAR filename[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;

         ossSnprintf( shortName, sizeof( shortName ) - 1, "sequoiadbLog.%d",
            idx ) ;
         utilBuildFullPath( filepath, shortName, OSS_MAX_PATHSIZE, filename ) ;

         if( !dpsLogFilter::isFileExisted( filename ) )
         {
            printf( "Warning: file:[%s] is missing\n", filename ) ;
            continue ;
         }

         rc = metaFilte( metaData, out, filename, idx ) ;
         if( rc )
         {
            printf( "!parse log file: [%s] error, rc = %d\n", filename, rc ) ;
            continue ;
         }
      }
      metaData.fileCount = metaData.metaList.size() ;

retry:
      pOutBuffer = ( CHAR * )SDB_OSS_REALLOC( pOutBuffer , outBufferSize + 1 ) ;
      if( NULL == pOutBuffer )
      {
         printf( "Failed to allocate %lld bytes, LINE:%d, FILE:%s\n",
            outBufferSize + 1, __LINE__, __FILE__ ) ;
         ossMemset( pOutBuffer, 0, outBufferSize + 1 ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      if( 0 < metaData.fileCount )
      {
         len = analysisMetaData( metaData, pOutBuffer, outBufferSize ) ;
         if( len >= outBufferSize )
         {
            outBufferSize += BLOCK_SIZE ;
            goto retry ;
         }

         if( data->output )
         {
            printf( "%s", pOutBuffer ) ;
         }
         else
         {
            rc = writeToFile( out, pOutBuffer ) ;
            if( rc )
            {
               goto error ;
            }
         }
      }
   }
   else
   {
      printf( "meta info need assigned a path of dir\n" ) ;
      rc = SDB_INVALIDPATH ;
      goto error ;
   }

done:
   if( start )
   {
      printf("\nAnalysis Log File Data end...\n" ) ;
   }
   return rc;
error:
   goto done;
}
*/

////////////////////////////////////////////////////////////////////
///< for _dpsLsnFilter
BOOLEAN _dpsLsnFilter::match( dpsDumper *dumper, CHAR *pRecord )
{
   return dpsDumpFilter::match( dumper, pRecord ) ;
}

INT32 _dpsLsnFilter::doFilte( dpsDumper *dumper, OSSFILE &out,
                              const CHAR *logFilePath )
{
   INT32 rc = SDB_OK ;

   rc = dumper->filte( this, out, logFilePath ) ;
   if( rc )
   {
      //PD_LOG( "!parse log file: [%s] error, rc = %d", logFilePath, rc ) ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

////////////////////////////////////////////////////////////////////
///< for _dpsNoneFilter
BOOLEAN _dpsNoneFilter::match( dpsDumper *dumper, CHAR *pRecord )
{
   return dpsDumpFilter::match( dumper, pRecord ) ;
}

INT32 _dpsNoneFilter::doFilte( dpsDumper *dumper, OSSFILE &out,
                               const CHAR *logFilePath )
{
   INT32 rc = SDB_OK ;

   rc = dumper->filte( this, out, logFilePath ) ;
   if( rc )
   {
      //PD_LOG( "!parse log file: [%s] error, rc = %d", logFilePath, rc ) ;
      goto error ;
   }

done:
   return rc;

error:
   goto done;
}

////////////////////////////////////////////////////////////////////
///< for lastFilter
BOOLEAN _dpsLastFilter::match( dpsDumper *dumper, CHAR *pRecord )
{
   return dpsDumpFilter::match( dumper, pRecord ) ;
}

INT32 _dpsLastFilter::doFilte( dpsDumper *dumper, OSSFILE &out,
                               const CHAR *logFilePath )
{
   INT32 rc = SDB_OK ;

   rc = dumper->filte( this, out, logFilePath ) ;
   if( rc )
   {
      goto error ;
   }

done:
   return rc ;

error:
   goto done ;
}



_dpsDumper::_dpsDumper() : _metaContent(NULL), _filter(NULL)
{

}

_dpsDumper::~_dpsDumper()
{

}

INT32 _dpsDumper::initialize( INT32 argc, CHAR** argv,
                              po::options_description &desc,
                              po::variables_map &vm )
{
   INT32 rc            = SDB_OK ;

   DPS_FILTER_ADD_OPTIONS_BEGIN( desc )
      FILTER_OPTIONS
   DPS_FILTER_ADD_OPTIONS_END

   rc = utilReadCommandLine( argc, argv, desc, vm ) ;
   if ( SDB_OK != rc )
   {
      std::cout << "Failed to parse command line" << std::endl ;
      goto error ;
   }

   if( !_validCheck( vm ) )
   {
      std::cout << "Invalid arguments" << std::endl ;
      displayArgs( desc ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if( vm.count( DPS_DUMP_HELP ) )
   {
      displayArgs( desc ) ;
      rc = SDB_PMD_HELP_ONLY ;
      goto done ;
   }

   if( vm.count( DPS_DUMP_VER ) )
   {
      ossPrintVersion( "SequoiaDB version" ) ;
      rc = SDB_PMD_VERSION_ONLY ;
      goto done ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 _dpsDumper::prepare( po::variables_map &vm )
{
   INT32 rc = SDB_OK ;
   rc = engine::pmdCfgRecord::init( NULL, &vm ) ;
   if( rc )
   {
      std::cout << "Invalid arguments" << std::endl ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 _dpsDumper::process( const po::options_description &desc,
                           const po::variables_map &vm )
{
   INT32 rc            = SDB_OK ;
   dpsDumpFilter *nextFilter = NULL ;

   if( vm.count( DPS_DUMP_OUTPUT ) )
   {
      consolePrint = FALSE ;
   }
   else
   {
      consolePrint = TRUE ;
   }

   ///< we should deal with lsn filter first
   if( vm.count( DPS_DUMP_LSN ) && vm.count( DPS_DUMP_LAST ) )
   {
      std::cout << "--lsn cannot be used with --last!!" << std::endl ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if( vm.count( DPS_DUMP_LSN ) )
   {
      _filter = dpsFilterFactory::getInstance()
                ->createFilter( SDB_LOG_FILTER_LSN ) ;
      CHECK_FILTER( _filter ) ;
      const CHAR *pLsn = vm[ DPS_DUMP_LSN ].as<std::string>().c_str() ;
      try
      {
         lsn = boost::lexical_cast< UINT64 >( pLsn ) ;
      }
      catch( boost::bad_lexical_cast& e )
      {
         std::cout << "Unable to cast lsn to UINT64" << std::endl ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
   }
   else if( vm.count( DPS_DUMP_LAST ) )
   {
      _filter = dpsFilterFactory::getInstance()
                ->createFilter( SDB_LOG_FILTER_LAST ) ;
      CHECK_FILTER( _filter ) ;
   }

   if( vm.count( DPS_DUMP_TYPE ) )
   {
      nextFilter = dpsFilterFactory::getInstance()
                   ->createFilter( SDB_LOG_FILTER_TYPE ) ;
      CHECK_FILTER( nextFilter ) ;
      ASSIGNED_FILTER( _filter, nextFilter ) ;
   }

   if( vm.count( DPS_DUMP_NAME ) )
   {
      nextFilter = dpsFilterFactory::getInstance()
                   ->createFilter( SDB_LOG_FILTER_NAME ) ;
      CHECK_FILTER( nextFilter ) ;
      ASSIGNED_FILTER( _filter, nextFilter ) ;
   }

   if( vm.count( DPS_DUMP_META ) )
   {
      if( NULL != _filter )
      {
         std::cout << "meta command must be used alone!" << std::endl ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      _filter = dpsFilterFactory::getInstance()
                ->createFilter( SDB_LOG_FILTER_META ) ;
      CHECK_FILTER( _filter ) ;
   }

   if( NULL == _filter )
   {
      _filter = dpsFilterFactory::getInstance()
                ->createFilter( SDB_LOG_FILTER_NONE ) ;
      CHECK_FILTER( _filter ) ;
   }

   rc = dump();
   if ( SDB_OK != rc )
   {
      if ( DPS_LOG_REACH_HEAD == rc )
      {
         std::cout << "over the valid log lsn" << std::endl ;
         goto done ;
      }
      else
      {
         std::cout << "error occurs when processing, rc = " << rc << std::endl;
         goto error;
      }
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 _dpsDumper::dump()
{
   INT32   rc      = SDB_OK ;
   BOOLEAN fOpened = FALSE ;
   OSSFILE fileFrom, fileTo ;
   CHAR dstFile[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
   dpsCmdData data ;
   BOOLEAN _isDir = FALSE ;

   rc = isDir( dstPath, _isDir ) ;
   // if SDB_FNE == rc, then treat dstPath as file-type, not dir-type
   if ( SDB_FNE == rc )
   {
      _isDir = FALSE ;
      rc = SDB_OK ;
   }
   else if( SDB_PERM == rc )
   {
      LogError( "Permission error: %s", dstPath ) ;
      goto error ;
   }
   else if( SDB_OK != rc )
   {
      LogError( "Failed to check whether %s is dir, rc = %d",
                dstPath, rc ) ;
      goto error ;
   }

   if( _isDir )
   {
      INT32 len = ossStrlen( dstPath ) ;
      if ( OSS_FILE_SEP_CHAR == dstPath[ len - 1 ] )
      {
         ossSnprintf( dstFile, OSS_MAX_PATHSIZE, "%s%s",
                      dstPath, "tmpLog.log" ) ;
      }
      else
      {
         ossSnprintf( dstFile, OSS_MAX_PATHSIZE, "%s"OSS_FILE_SEP"%s",
                      dstPath, "tmpLog.log" ) ;
      }
   }
   else
   {
      ossSnprintf( dstFile, OSS_MAX_PATHSIZE, "%s", dstPath ) ;
   }

   if( !consolePrint )
   {
      rc = ossOpen( dstFile, OSS_REPLACE | OSS_READWRITE,
                    OSS_RU | OSS_WU | OSS_RG, fileTo ) ;
      if( rc )
      {
         LogError("Unable to open file: %s", dstFile ) ;
         goto error ;
      }
      fOpened = TRUE ;
   }

   // analysis meta info whatever "-m" is assigned
   {
      rc = _analysisMeta();
      if ( SDB_OK != rc )
      {
         goto error;
      }
   }
   rc = _writeTo( fileTo, _metaContent, consolePrint ) ;
   if( rc )
   {
      goto error ;
   }

   /// < meta need do specially
   if ( SDB_LOG_FILTER_META == _filter->getType() )
   {
      goto done;
   }
   ///< parse meta info end

   rc = isDir( srcPath, _isDir ) ;
   if( SDB_FNE == rc || SDB_PERM == rc )
   {
      LogError( "Permission error or path not exist: %s", srcPath ) ;
      goto error ;
   }
   if( SDB_OK != rc )
   {
      LogError( "Failed to get check whether %s is dir, rc = %d",
                srcPath, rc ) ;
      goto error ;
   }
   if( _isDir )
   {
      if( SDB_LOG_FILTER_LAST == _filter->getType() )
      {
         LogError( "a file path is needed when using --last/-e, "
                   "current: %s is a directory", srcPath );
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      INT32 fileCount = 0 ;
      rc = getFileCount( srcPath, fileCount ) ;
      if( SDB_OK != rc )
      {
         LogError( "Failed to get file-count in dir: %s", srcPath ) ;
         goto error ;
      }
      if( 0 == fileCount )
      {
         LogError( "Cannot find any dpsLogFile in path: %s", srcPath ) ;
         rc = SDB_FNE ;
         goto error ;
      }

      if( 0 >= fileCount )
      {
         LogError( "Cannot find any Log files from: %s, "
                   "check input path again", srcPath) ;
         rc = SDB_INVALIDPATH ;
         goto error ;
      }

      for( INT32 idx = 0 ; idx < fileCount ; ++idx )
      {
         // src log file ;
         fs::path fileDir( srcPath ) ;
         const CHAR *filepath = fileDir.string().c_str() ;
         CHAR shortName[ 30 ] = { 0 } ;
         CHAR filename[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;

         ossSnprintf( shortName, sizeof( shortName ) - 1,
                      REPLOG_NAME_PREFIX SEP_CHAR_DOT"%d", idx ) ;
         utilBuildFullPath( filepath, shortName,
                            OSS_MAX_PATHSIZE, filename ) ;

         if( !isFileExisted( filename ) )
         {
            rc = SDB_INVALIDPATH ;
            goto error ;
         }

         rc = _filter->doFilte( this, fileTo, filename ) ;
         if( ( rc && SDB_DPS_CORRUPTED_LOG == rc )
             || idx != fileCount - 1 )
         {
            rc = SDB_OK ;
            continue ;
         }
      }
   }
   else
   {
      if( !isFileExisted( srcPath ) )
      {
         rc = SDB_INVALIDPATH ;
         goto error ;
      }

      rc = _filter->doFilte( this, fileTo, srcPath ) ;
      if( rc && SDB_DPS_CORRUPTED_LOG != rc )
      {
         goto error ;
      }
      rc = SDB_OK ;
   }

done:
   if( fOpened )
   {
      ossClose( fileTo ) ;
   }
   return rc ;

error:
   goto done ;
}


INT32 _dpsDumper::parseMeta( CHAR *buffer )
{
   return SDB_OK ;
}

INT32 _dpsDumper::doDataExchange( engine::pmdCfgExchange *pEx )
{
   resetResult() ;
   CHAR path[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;

   rdxString( pEx, DPS_DUMP_SOURCE, path,
              OSS_MAX_PATHSIZE, FALSE, FALSE, "./" ) ;
   if ( NULL == ossGetRealPath(path, srcPath, OSS_MAX_PATHSIZE))
   {
      LogEvent( "Failed to get real path of source: %s", srcPath ) ;
      ossStrncpy( srcPath, "./", OSS_MAX_PATHSIZE ) ;
   }

   rdxString( pEx, DPS_DUMP_OUTPUT, path,
              OSS_MAX_PATHSIZE, FALSE, FALSE, "./" ) ;
   if ( NULL == ossGetRealPath(path, dstPath, OSS_MAX_PATHSIZE))
   {
      LogEvent( "Failed to get real path of destination: %s", dstPath ) ;
      ossStrncpy( dstPath, "./", OSS_MAX_PATHSIZE ) ;
   }

   rdxString( pEx, DPS_DUMP_NAME, name,
              OSS_MAX_PATHSIZE, FALSE, FALSE, "" ) ;

   rdxUShort( pEx, DPS_DUMP_TYPE, opType,
              FALSE, TRUE, (UINT16)PDWARNING ) ;

   rdxInt( pEx, DPS_DUMP_LSN_AHEAD, lsnAhead, FALSE, TRUE, 20 ) ;

   rdxInt( pEx, DPS_DUMP_LSN_BACK, lsnBack, FALSE, TRUE, 20 ) ;

   rdxInt( pEx, DPS_DUMP_LAST, lastCount, FALSE, TRUE, 0 ) ;

   return getResult() ;
}

INT32 _dpsDumper::postLoaded( engine::PMD_CFG_STEP step )
{
   return SDB_OK ;
}

INT32 _dpsDumper::preSaving()
{
   return SDB_OK ;
}

BOOLEAN _dpsDumper::_validCheck( const po::variables_map &vm )
{
   BOOLEAN valid = FALSE ;

   if(   vm.count( DPS_DUMP_HELP )
      || vm.count( DPS_DUMP_VER )
      || vm.count( DPS_DUMP_TYPE )
      || vm.count( DPS_DUMP_NAME )
      || vm.count( DPS_DUMP_META )
      || vm.count( DPS_DUMP_LSN )
      || vm.count( DPS_DUMP_SOURCE )
      || vm.count( DPS_DUMP_OUTPUT )
      || vm.count( DPS_DUMP_LAST ) )
   {
      valid = TRUE ;
   }

   return valid ;
}

INT32 _dpsDumper::_analysisMeta()
{
   INT32 rc = SDB_OK ;
   UINT32 begin    = DPS_INVALID_LOG_FILE_ID ;
   UINT32 work     = 0 ;
   UINT32 idx      = 0 ;
   UINT32 beginIdx = 0 ;
   INT32 fileCount = 0;
   BOOLEAN _isDir  = FALSE ;

   CHAR  dirPath[ OSS_MAX_PATHSIZE + 1 ] = { 0 };

   rc = isDir( srcPath, _isDir ) ;
   if( SDB_FNE == rc || SDB_PERM == rc )
   {
      LogError( "Permission error or path not exist: %s", srcPath ) ;
      goto error ;
   }
   if( SDB_OK != rc )
   {
      LogError( "Failed to get check whether %s is dir, rc = %d",
                srcPath, rc ) ;
      goto error ;
   }

   if( !_isDir )
   {
      CHAR filename[ OSS_MAX_PATHSIZE * 2 ] = { 0 } ;
      dpsFileMeta meta ;
      ossMemcpy( filename, srcPath, OSS_MAX_PATHSIZE ) ;
      CHAR *pos = ossStrrchr( filename, '.' ) ;
      UINT32 index = 0 ;
      if ( NULL == pos )
      {
         LogError( "Invalid file, file:[%s]", filename ) ;
         goto error ;
      }
      index = ossAtoi( pos + 1 ) ;
      if( !isFileExisted( filename ) )
      {
         LogError( "Permission error or file not exist, file:[%s]", filename ) ;
         goto error ;
      }

      rc = _metaFilte( filename, index, meta ) ;
      if( rc && DPS_LOG_FILE_INVALID != rc  )
      {
         LogError( "Failed to parse meta data of file:[%s], rc = %d",
                   filename, rc ) ;
         goto error ;
      }
      rc = SDB_OK ;
      _meta.metaList.push_back( meta ) ;
      _meta.fileBegin = _meta.fileWork = 0 ;
      _meta.fileCount = 1 ;
   }
   else
   {
      INT32 index = 0 ;
      ossMemcpy( dirPath, srcPath, OSS_MAX_PATHSIZE ) ;
      rc = getFileCount( dirPath, fileCount ) ;
      if( SDB_OK != rc )
      {
         LogError( "Failed to get file-count in dir: %s", dirPath ) ;
         goto error ;
      }
      if( 0 == fileCount )
      {
         LogError( "Cannot find any dpsLogFile in path: %s", dirPath ) ;
         rc = SDB_FNE ;
         goto error ;
      }

      {
         fs::path fileDir( dirPath ) ;
         for ( index = 0; index < fileCount; ++index )
         {
            const CHAR *filepath = fileDir.string().c_str() ;
            CHAR filename[ OSS_MAX_PATHSIZE * 2 ] = { 0 } ;
            CHAR tempName[ 30 ] = { 0 };
            ossSnprintf( tempName, OSS_MAX_PATHSIZE,
                         REPLOG_NAME_PREFIX SEP_CHAR_DOT"%d", index ) ;
            utilBuildFullPath( filepath, tempName, OSS_MAX_PATHSIZE, filename ) ;

            if( !isFileExisted( filename ) )
            {
               LogError( "Permission error or file not exist, file:[%s]", filename ) ;
               continue ;
            }

            dpsFileMeta meta;
            rc = _metaFilte( filename, index, meta ) ;
            if( rc && DPS_LOG_FILE_INVALID != rc )
            {
               LogError( "Failed to parse meta data of file:[%s], rc = %d",
                         filename, rc ) ;
               continue ;
            }
            rc = SDB_OK ;
            _meta.metaList.push_back( meta ) ;
         }
      }

      _meta.fileCount = _meta.metaList.size() ;
      // find begin file
      idx = 0 ;
      while( idx < _meta.fileCount )
      {
         const dpsFileMeta &meta = _meta.metaList[ idx ] ;
         if( DPS_INVALID_LOG_FILE_ID == meta.logID )
         {
            ++idx ;
            continue ;
         }

         if( DPS_INVALID_LOG_FILE_ID == begin
            || ( meta.logID < begin &&
                 begin - meta.logID < DPS_INVALID_LOG_FILE_ID / 2 )
            || ( meta.logID > begin &&
                 meta.logID - begin > DPS_INVALID_LOG_FILE_ID / 2 ) )
         {
            _meta.fileBegin = meta.index ;
            begin = meta.logID;
            beginIdx = idx ;
         }
         ++idx ;
      }

      // find work file
      idx = 0 ;
      work = beginIdx ;
      while( 0 == _meta.metaList[ work ].restSize &&
             idx < _meta.metaList.size() )
      {
         _meta.fileWork = work ;
         ++work ;
         if( work > _meta.fileCount )
         {
            work = 0;
         }
         ++idx ;
      }
      if( DPS_INVALID_LOG_FILE_ID != _meta.metaList[ work ].logID )
      {
         _meta.fileWork = work ;
      }
   }

   if( 0 < _meta.fileCount )
   {
      UINT64 validLen = 0 ;
      UINT64 bufferSize = 4096 ;

      if( _isDir )
      {
         rc = sortFiles( _meta ) ;
         if( SDB_OK != rc )
         {
            LogError( "Failed to sort files, rc: %d", rc ) ;
            goto error ;
         }
      }
retry:
      _metaContent = ( CHAR * )SDB_OSS_REALLOC( _metaContent , bufferSize + 1 ) ;
      if( NULL == _metaContent )
      {
         LogError( "Failed to allocate %lld bytes", bufferSize + 1 ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      validLen = _dumpMeta( _meta, _metaContent, bufferSize ) ;
      if( validLen >= bufferSize )
      {
         bufferSize += validLen ;
         goto retry ;
      }
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 _dpsDumper::getFileCount( const CHAR *path, INT32 &fileCount /*out*/ )
{
   INT32 rc = SDB_OK ;
   const CHAR *filter = REPLOG_NAME_PREFIX SEP_CHAR_DOT "*" ;
   multimap< string, string > mapFiles ;
   fileCount = 0 ;

   rc = ossEnumFiles( path, mapFiles, filter ) ;
   fileCount = mapFiles.size() ;

   return rc ;
}

INT32 _dpsDumper::isDir( const CHAR *path, BOOLEAN &dir )
{
   INT32 rc = SDB_OK ;
   SDB_OSS_FILETYPE fileType = SDB_OSS_UNK ;

   rc = ossAccess( path, R_OK ) ;
   if ( SDB_OK != rc )
   {
      goto error;
   }

   rc = ossGetPathType( path, &fileType ) ;
   if ( SDB_OK != rc )
   {
      goto error;
   }

   dir = FALSE ;
   if( SDB_OSS_DIR == fileType )
   {
      dir =  TRUE ;
   }
done:
   return rc ;
error:
   goto done ;
}

BOOLEAN _dpsDumper::isFileExisted( const CHAR *path )
{
   INT32 rc = SDB_OK ;
   OSSFILE file ;
   rc = ossOpen( path, OSS_READONLY, OSS_RU, file ) ;
   if( rc )
   {
      if( SDB_PERM == rc )
      {
         LogError( "Permission error, file:[%s]", path ) ;
      }
      else
      {
         LogError( "File: [%s] is not existed", path ) ;
      }
      goto done ;
   }
   ossClose( file ) ;

done:
   return SDB_OK == rc ;
}

INT32 _dpsDumper::sortFiles( dpsMetaData &meta )
{
   std::vector<dpsFileMeta> tmp = meta.metaList ;
   meta.metaList.clear() ;
   UINT32 begin = meta.fileBegin ;
   UINT32 work = meta.fileWork ;
   UINT32 idx = begin ;
   /* 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19
    * begin from -->|                     |
    *                         work-here-->|------------
    * ----------->|<--end here
   */
   // push index from work file
   while ( idx < tmp.size())
   {
      meta.metaList.push_back(tmp[idx]) ;
      ++idx ;
   }

   // push index back from
   idx = 0 ;
   while (idx < begin)
   {
      meta.metaList.push_back(tmp[idx]) ;
      ++idx ;
   }

   meta.fileBegin = 0;
   meta.fileWork = ( ( work + meta.fileCount ) - begin ) % 20 ;

   return SDB_OK ;
}

INT32 _dpsDumper::_metaFilte( const CHAR *filename, INT32 index,
                              dpsFileMeta &meta )
{
   SDB_ASSERT( filename, "filename cannot be NULL ") ;

   INT32 rc = SDB_OK  ;

   OSSFILE in ;
   CHAR pRecordHead[ sizeof( dpsLogRecordHeader ) + 1 ] = { 0 } ;
   CHAR pLogHead[ DPS_LOG_HEAD_LEN + 1 ] = { 0 } ;
   BOOLEAN opened             = FALSE ;
   INT64 fileSize             = 0 ;
   INT64 offset               = 0 ;
   dpsLogHeader *logHeader    = NULL ;
   dpsLogRecordHeader *header = NULL ;
   INT64 len                  = 0 ;
   INT64 totalRecordSize      = 0 ;
   UINT64 preLsn              = 0 ;

   LogEvent( "Parse file:[ %s ] begin", filename ) ;
   rc = ossOpen( filename, OSS_DEFAULT | OSS_READONLY,
                 OSS_RU | OSS_RG, in ) ;
   if( rc )
   {
      LogError( "Unable to open file[%s], rc = %d", filename, rc ) ;
      goto error ;
   }
   opened = TRUE ;

   rc = _checkLogFile( in, fileSize, filename );
   if( rc )
   {
      goto error ;
   }
   SDB_ASSERT( fileSize > 0, "fileSize must be gt 0" ) ;

   rc = _readLogHead( in, offset, fileSize, NULL, 0, pLogHead, len ) ;
   if( rc && DPS_LOG_FILE_INVALID != rc )
   {
      LogEvent("Read a invlid log file: %s", filename) ;
      goto error;
   }
   // start format log head
   totalRecordSize = fileSize - DPS_LOG_HEAD_LEN ;
   logHeader = (dpsLogHeader*)pLogHead ;

   // init meta data
   meta.index     = index ;
   meta.logID     = logHeader->_logID ;
   meta.firstLSN  = logHeader->_firstLSN.offset ;
   meta.lastLSN   = logHeader->_firstLSN.offset ;

   if( DPS_LOG_INVALID_LSN != logHeader->_firstLSN.offset )
   {
      while ( offset < fileSize )
      {
         rc = _readRecordHead( in, offset, fileSize, pRecordHead ) ;
         if( rc && SDB_DPS_CORRUPTED_LOG != rc )
         {
            goto error ;
         }

         header = ( dpsLogRecordHeader * )pRecordHead ;
         if( SDB_DPS_CORRUPTED_LOG == rc )
         {
            meta.expectLSN = logHeader->_firstLSN.offset +
                             offset - DPS_LOG_HEAD_LEN ;
            meta.lastLSN = preLsn ;
            meta.validSize = offset - DPS_LOG_HEAD_LEN ;
            meta.restSize = totalRecordSize - meta.validSize ;

            LogEvent( "Record was corrupted with length: %d",
                      header->_length ) ;
            rc = SDB_OK ;
            break ;
         }
         if( LOG_TYPE_DUMMY == header->_type )
         {
            // reach end of file
            meta.expectLSN = header->_lsn + header->_length ;
            meta.lastLSN = header->_lsn ;
            meta.validSize = header->_lsn % totalRecordSize ;
            meta.restSize = totalRecordSize -
                            ( meta.validSize + header->_length ) ;
            break;
         }

         // hit invalid lsn
         if( preLsn > header->_lsn )
         {
            // current lsn will to be overwrite by next record
            meta.expectLSN = logHeader->_firstLSN.offset +
                             offset - DPS_LOG_HEAD_LEN ;
            meta.lastLSN = preLsn ;
            meta.validSize = offset - DPS_LOG_HEAD_LEN ;
            meta.restSize = totalRecordSize - meta.validSize ;
            break ;
         }

         preLsn = header->_lsn ;
         offset += header->_length ;
      }
   }
   else
   {
      meta.validSize = 0 ;
      meta.restSize = totalRecordSize ;
   }

done:
   if( opened )
      ossClose( in ) ;
   LogEvent("Parse file:[ %s ] end", filename ) ;

   return rc ;

error:
   goto done ;
}


INT64 _dpsDumper::_dumpMeta( const dpsMetaData& meta,
                             CHAR* pBuffer, const UINT64 bufferSize )
{
   SDB_ASSERT( pBuffer, "pOutBuffer cannot be NULL " ) ;
   UINT64 len = 0 ;
   len += ossSnprintf( pBuffer + len, bufferSize - len,
                       "======================================="OSS_NEWLINE
                       ) ;
   len += ossSnprintf( pBuffer + len, bufferSize - len,
                       "    Log Files in total: %d"OSS_NEWLINE,
                       meta.fileCount ) ;
   len += ossSnprintf( pBuffer + len, bufferSize - len,
                       "    LogFile begin     : sequoiadbLog.%d"OSS_NEWLINE,
                       meta.metaList[ 0 ].index ) ;
   len += ossSnprintf( pBuffer + len, bufferSize - len,
                       "    LogFile work      : sequoiadbLog.%d"OSS_NEWLINE,
                       meta.metaList[ meta.fileWork ].index ) ;
   len += ossSnprintf( pBuffer + len, bufferSize - len,
                       "        begin Lsn     : 0x%08lx"OSS_NEWLINE,
                       meta.metaList[ 0 ].firstLSN ) ;
   len += ossSnprintf( pBuffer + len, bufferSize - len,
                       "        current Lsn   : 0x%08lx"OSS_NEWLINE,
                       meta.metaList[ meta.fileWork ].lastLSN ) ;
   len += ossSnprintf( pBuffer + len, bufferSize - len,
                       "        expect Lsn    : 0x%08lx"OSS_NEWLINE,
                       ( meta.metaList[ meta.fileWork ].expectLSN ) ) ;
   len += ossSnprintf( pBuffer + len, bufferSize - len,
                       "======================================="OSS_NEWLINE
                       ) ;
   // foreach file meta
   for( UINT32 idx = 0; idx < meta.metaList.size(); ++idx )
   {
      const dpsFileMeta& fMeta = meta.metaList[ idx ] ;
      len += ossSnprintf( pBuffer + len, bufferSize - len,
                          OSS_NEWLINE"Log File Name: sequoiadbLog.%d"OSS_NEWLINE,
                          fMeta.index ) ;
      len += ossSnprintf( pBuffer + len, bufferSize - len,
                          "Logic ID     : %d"OSS_NEWLINE, fMeta.logID ) ;
      len += ossSnprintf( pBuffer + len, bufferSize - len,
                          "First LSN    : 0x%08lx"OSS_NEWLINE, fMeta.firstLSN ) ;
      len += ossSnprintf( pBuffer + len, bufferSize - len,
                          "Last  LSN    : 0x%08lx"OSS_NEWLINE, fMeta.lastLSN ) ;
      len += ossSnprintf( pBuffer + len, bufferSize - len,
                          "Valid Size   : %lld bytes"OSS_NEWLINE, fMeta.validSize ) ;
      len += ossSnprintf( pBuffer + len, bufferSize - len,
                          "Rest Size    : %lld bytes"OSS_NEWLINE, fMeta.restSize ) ;
   }

   return len ;
}

INT32 _dpsDumper::_checkLogFile( OSSFILE& file, INT64& size, const CHAR *filename )
{
   SDB_ASSERT( filename, "filename cannot null" ) ;

   INT32 rc = SDB_OK ;
   // calculate file size
   rc = ossGetFileSize( &file, &size ) ;
   if( rc )
   {
      LogError( "Failed to get file size: %s, rc = %d", filename, rc ) ;
      goto error ;
   }
   // make sure the size valid
   if( size < DPS_LOG_HEAD_LEN )
   {
      LogError( "Log file %s is %lld bytes, "
                "which is smaller than log file head",
                filename, size ) ;
      rc = SDB_DPS_CORRUPTED_LOG ;
      goto error ;
   }

   //log file must be multiple of page size
   if(( size - DPS_LOG_HEAD_LEN ) % DPS_DEFAULT_PAGE_SIZE != 0 )
   {
      LogError( "Log file %s is %lld bytes, "
                "which is not aligned with page size",
                filename, size ) ;
      rc = SDB_DPS_CORRUPTED_LOG ;
      goto error ;
   }

done:
   return rc;
error:
   goto done;
}

INT32 _dpsDumper::_readRecordHead( OSSFILE& in, const INT64 offset,
                                 const INT64 fileSize, CHAR *pRecordHeadBuffer )
{
   SDB_ASSERT( offset < fileSize, "offset out of range " ) ;
   SDB_ASSERT( pRecordHeadBuffer, "OutBuffer cannot be NULL " ) ;

   INT32 rc       = SDB_OK ;
   INT64 readPos  = 0 ;
   INT64 restLen  = sizeof( dpsLogRecordHeader ) ;
   INT64 fileRead = 0 ;
   dpsLogRecordHeader *header = NULL ;

   while( restLen > 0 )
   {
      rc = ossSeekAndRead( &in, offset, pRecordHeadBuffer + readPos,
                           restLen, &fileRead ) ;
      if( rc && SDB_INTERRUPT != rc)
      {
         LogError( "Failed to read from file, expect %lld bytes, "
                   "actual read %lld bytes, rc = %d",
                   restLen, fileRead, rc ) ;
         rc = SDB_OOM ;
         goto error ;
      }
      rc = SDB_OK ;
      restLen -= fileRead ;
      readPos += fileRead ;
   }

   header = ( dpsLogRecordHeader * )pRecordHeadBuffer ;
   if ( header->_length < sizeof( dpsLogRecordHeader) ||
        header->_length > DPS_RECORD_MAX_LEN )
   {
      rc = SDB_DPS_CORRUPTED_LOG ;
      goto error ;
   }

   if ( LOG_TYPE_DUMMY == header->_type )
   {
      LogEvent( "Reach the Dummy record head, offset:%lld", offset ) ;
      goto done ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 _dpsDumper::_readRecord( OSSFILE& in, const INT64 offset,
                               const INT64 fileSize, const INT64 readLen,
                               CHAR *pOutBuffer )
{
   INT32 rc = SDB_OK ;

   SDB_ASSERT( offset < fileSize, "offset out of range " ) ;
   SDB_ASSERT( readLen > 0, "readLen lt 0!!" ) ;
   INT64 restLen  = 0 ;
   INT64 readPos  = 0 ;
   INT64 fileRead = 0;

   restLen = readLen ;
   while( restLen > 0 )
   {
      rc = ossSeekAndRead( &in, offset, pOutBuffer + readPos,
                           restLen, &fileRead ) ;
      if( rc && SDB_INTERRUPT != rc)
      {
         LogError( "Failed to read from file, expect %lld bytes, "
                   "actual read %lld bytes, rc = %d",
                   restLen, fileRead, rc ) ;
         goto error ;
      }
      rc = SDB_OK ;
      restLen -= fileRead ;
      readPos += fileRead ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 _dpsDumper::_readLogHead( OSSFILE &in, INT64& offset, const INT64 fileSize,
                                CHAR *pOutBuffer, const INT64 bufferSize,
                                CHAR *pOutHeader, INT64& outLen )
{
   SDB_ASSERT( fileSize > DPS_LOG_HEAD_LEN,
               "fileSize must gt than DPS_LOG_HEAD_LEN" ) ;

   INT32 rc       = SDB_OK ;
   INT64 readPos  = 0 ;
   INT64 fileRead = 0 ;
   INT64 restLen  = DPS_LOG_HEAD_LEN ;
   CHAR  pBuffer[ DPS_LOG_HEAD_LEN + 1 ] = { 0 } ;
   dpsLogHeader *header = NULL ;
   INT64 len      = 0 ;

   while( restLen > 0 )
   {
      rc = ossRead( &in, pBuffer + readPos, restLen, &fileRead ) ;
      if( rc && SDB_INTERRUPT != rc)
      {
         LogError( "Failed to read from file, expect %lld bytes,  "
                   "actual read %lld bytes, rc = %d",
                   fileSize, fileRead, rc ) ;
         goto error ;
      }
      rc = SDB_OK ;
      restLen -= fileRead ;
      readPos += fileRead ;
   }

   header =( _dpsLogHeader* )pBuffer ;
   if( DPS_INVALID_LOG_FILE_ID != header->_logID )
   {
      UINT64 beginOffset = header->_firstLSN.offset ;
      beginOffset = beginOffset %( fileSize - DPS_LOG_HEAD_LEN ) ;
      offset += beginOffset ;
   }
   else
   {
      rc = DPS_LOG_FILE_INVALID ;
   }
   // modify offset
   offset += DPS_LOG_HEAD_LEN ;

   if( pOutBuffer )
   {
      len = engine::dpsDump::dumpLogFileHead( pBuffer, DPS_LOG_HEAD_LEN,
                                              pOutBuffer, bufferSize,
                                              DPS_DMP_OPT_HEX |
                                              DPS_DMP_OPT_HEX_WITH_ASCII |
                                              DPS_DMP_OPT_FORMATTED ) ;
   }

   if ( pOutHeader )
   {
      ossMemcpy( pOutHeader, pBuffer, DPS_LOG_HEAD_LEN );
   }

done:
   outLen = len ;
   return rc ;
error:
   goto done ;
}

INT32 _dpsDumper::_writeTo( OSSFILE &fd, const CHAR* pContent, BOOLEAN console )
{
   INT32 rc        = SDB_OK ;

   rc = writeToFile( fd, pContent, console) ;
   if( SDB_OK != rc )
   {
      goto error ;
   }
   // write a enter into file
   rc = writeToFile( fd, OSS_NEWLINE, console ) ;
   if( SDB_OK != rc )
   {
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 _dpsDumper::filte( dpsDumpFilter *filter,
                         OSSFILE &out, const CHAR *filename )
{
   SDB_ASSERT( filter, "filter is NULL" ) ;
   SDB_ASSERT( filename, "filename cannot be NULL" ) ;

   INT32 rc = SDB_OK ;

   OSSFILE in ;
   CHAR pLogHead[ sizeof( dpsLogHeader ) + 1 ] = { 0 } ;
   CHAR pRecordHead[ sizeof( dpsLogRecordHeader ) + 1 ] = { 0 } ;
   CHAR *pRecordBuffer = NULL ;
   INT64 recordLength  = 0 ;
   INT32 ahead         = lsnAhead ;
   INT32 back          = lsnBack ;
   INT32 totalCount    = NONE_LSN_FILTER ; //ahead + back ;

   BOOLEAN opened      = FALSE ;
   INT64 fileSize      = 0 ;
   INT64 offset        = 0 ;
   CHAR *pOutBuffer    = NULL ; ///< buffer for formatted log
   INT64 outBufferSize = 0 ;
   INT64 len           = 0 ;
   BOOLEAN printLogHead= FALSE ;

   CHAR parseBegin[ BLOCK_SIZE ] = { 0 } ;
   len  = ossSnprintf( parseBegin, BLOCK_SIZE, OSS_NEWLINE""OSS_NEWLINE ) ;
   len += ossSnprintf( parseBegin + len, BLOCK_SIZE - len,
                       "parse file : [%s]"OSS_NEWLINE, filename ) ;

   rc = ossOpen( filename, OSS_DEFAULT | OSS_READONLY,
                 OSS_RU | OSS_WU | OSS_RG, in ) ;
   if( rc )
   {
      LogError( "Unable to open file: %s. rc = %d", filename, rc ) ;
      goto error ;
   }

   opened = TRUE ;

   rc = _checkLogFile( in, fileSize, filename ) ;
   if( rc )
   {
      goto error ;
   }
   SDB_ASSERT( fileSize > 0, "fileSize must be gt 0" ) ;
   len = DPS_LOG_HEAD_LEN * LOG_BUFFER_FORMAT_MULTIPLIER ;

retry_head:
   // format log head
   if( len > outBufferSize )
   {
      CHAR *pOrgBuff = pOutBuffer ;
      pOutBuffer =(CHAR*)SDB_OSS_REALLOC ( pOutBuffer, len + 1 ) ;
      if( !pOutBuffer )
      {
         LogError( "Failed to allocate memory for %lld bytes", len + 1 ) ;
         pOutBuffer = pOrgBuff ;
         rc = SDB_OOM ;
         goto error ;
      }
      outBufferSize = len ;
      ossMemset( pOutBuffer, 0, len + 1 ) ;
   }

   rc = _readLogHead( in, offset, fileSize,  pOutBuffer, outBufferSize,
                      pLogHead, len ) ;
   if( rc && DPS_LOG_FILE_INVALID != rc )
   {
      goto error ;
   }
   rc = SDB_OK ;
   if( len >= outBufferSize )
   {
      len += BLOCK_SIZE ;
      goto retry_head ;
   }

   // lsn must be done specially
   if( SDB_LOG_FILTER_LSN == filter->getType() )
   {
      dpsLogHeader *logHeader = ( dpsLogHeader * )pLogHead ;
      if(   ( logHeader->_firstLSN.offset > lsn )
         || (   lsn >= logHeader->_firstLSN.offset
              + fileSize - DPS_LOG_HEAD_LEN ) )
      {
         goto done ;
      }

      offset = DPS_LOG_HEAD_LEN + (lsn % ( fileSize - DPS_LOG_HEAD_LEN )) ;
      // seek to the log by lsn assigned
      {
         rc = _seekToLsnMatched( in, offset, fileSize, ahead ) ;

         if( rc && DPS_LOG_REACH_HEAD != rc )
         {
            LogError( "hit a invalid or corrupted log: %lld in file : [%s] "
                      "is invalid", lsn, filename ) ;
            goto error ;
         }
         totalCount = ahead + back + 1 ;
         if( DPS_LOG_REACH_HEAD == rc )
         {
            offset = DPS_LOG_HEAD_LEN ;
         }
      }
   }
   else if( SDB_LOG_FILTER_LAST == filter->getType() )
   {
      INT32 recordCount = lastCount - 1 ;
      offset = DPS_LOG_HEAD_LEN ;
      _seekToEnd( in, offset, fileSize ) ;
      rc = _seekToLsnMatched( in, offset, fileSize, recordCount ) ;
      if( rc && DPS_LOG_REACH_HEAD != rc )
      {
         //printf( "File was corrupted.\n" ) ;
         goto error ;
      }
      totalCount = recordCount + 1 ;
      if( DPS_LOG_REACH_HEAD == rc )
      {
         offset = DPS_LOG_HEAD_LEN ;
      }
   }

   // then dump each record
   while(   offset < fileSize &&
          ( NONE_LSN_FILTER  == totalCount || totalCount > 0 ) )
   {
      rc = _readRecordHead( in, offset, fileSize, pRecordHead ) ;
      if( rc && SDB_DPS_CORRUPTED_LOG != rc )
      {
         goto error ;
      }

      dpsLogRecordHeader *header = ( dpsLogRecordHeader *)pRecordHead ;
      if( SDB_DPS_CORRUPTED_LOG == rc && header->_length == 0 )
      {
         goto error ;
      }

      if( header->_length > recordLength )
      {
         pRecordBuffer = ( CHAR * )SDB_OSS_REALLOC ( pRecordBuffer,
                                                     header->_length + 1 );
         if( !pRecordBuffer )
         {
            rc = SDB_OOM;
            LogError( "Failed to allocate %d bytes", header->_length + 1 ) ;
            goto error ;
         }
         recordLength = header->_length ;
      }
      ossMemset( pRecordBuffer, 0, recordLength ) ;
      rc = _readRecord( in, offset, fileSize, header->_length, pRecordBuffer ) ;
      if( rc )
      {
         goto error;
      }

      // filte name
      if( !filter->match( this, pRecordBuffer ) )
      {
         offset += header->_length ;
         if( SDB_LOG_FILTER_LSN == filter->getType() )
         {
            --totalCount ;
         }
         continue ;
      }

      // find first record, then print log head
      if( !printLogHead )
      {
         printLogHead = TRUE ;
         rc = _writeTo( out, parseBegin, consolePrint ) ;
         if( rc )
         {
            goto error ;
         }
         // write to file
         rc = _writeTo( out, pOutBuffer, consolePrint ) ;
         if( rc )
         {
            goto error ;
         }
      }

      // dump log
      dpsLogRecord record ;
      record.load( pRecordBuffer ) ;
      len = recordLength * LOG_BUFFER_FORMAT_MULTIPLIER ;

retry_record:
      if( len > outBufferSize )
      {
         CHAR *pOrgBuff = pOutBuffer ;
         pOutBuffer =(CHAR*)SDB_OSS_REALLOC( pOutBuffer, len + 1 ) ;
         if( !pOutBuffer )
         {
            LogError( "Failed to allocate memory for %lld bytes", len + 1 ) ;
            pOutBuffer = pOrgBuff ;
            rc = SDB_OOM ;
            goto error ;
         }
         outBufferSize = len ;
         ossMemset( pOutBuffer, 0, len + 1 ) ;
      }

      len = record.dump( pOutBuffer, outBufferSize,
                         DPS_DMP_OPT_HEX | DPS_DMP_OPT_HEX_WITH_ASCII |
                         DPS_DMP_OPT_FORMATTED ) ;
      if( len >= outBufferSize )
      {
         len += BLOCK_SIZE ;
         goto retry_record ;
      }


      // write dump data
      rc = _writeTo( out, pOutBuffer, consolePrint ) ;
      if( rc )
      {
         goto error ;
      }

      offset += header->_length ;
      if(    ( SDB_LOG_FILTER_LSN  == filter->getType() )
          || ( SDB_LOG_FILTER_LAST == filter->getType() ) )
      {
         --totalCount ;
      }
   }

done:
   if( pRecordBuffer )
      SDB_OSS_FREE( pRecordBuffer ) ;
   if( pOutBuffer )
      SDB_OSS_FREE( pOutBuffer ) ;
   if( opened )
      ossClose( in ) ;
   return rc ;

error:
   goto done ;
}

INT32 _dpsDumper::_seekToLsnMatched( OSSFILE &in, INT64 &offset,
                                     const INT64 fileSize, INT32 &prevCount )
{
   SDB_ASSERT( offset >= DPS_LOG_HEAD_LEN, "offset lt DPS_LOG_HEAD_LEN" ) ;
   INT32 rc     = SDB_OK ;
   INT64 newOff = 0 ;
   INT32 count  = prevCount ;

   CHAR pRecordHead[ sizeof( dpsLogRecordHeader ) + 1 ] = { 0 } ;
   dpsLogRecordHeader *header = NULL ;

   if( offset > fileSize || offset < DPS_LOG_HEAD_LEN )
   {
      LogError( "wrong LSN position: %lld", offset ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if( count < 0 )
   {
      LogError( "pre-count must be gt 0, current is: %d", prevCount ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   prevCount = 0 ;
   while( offset < fileSize && prevCount < count )
   {
      rc = _readRecordHead( in, offset, fileSize, pRecordHead ) ;
      // ignore the return value
      header = ( dpsLogRecordHeader * )pRecordHead ;
      if ( header->_length < sizeof( dpsLogRecordHeader ) ||
           header->_length > DPS_RECORD_MAX_LEN )
      {
         rc = SDB_DPS_CORRUPTED_LOG ;
         goto error ;
      }

      if ( LOG_TYPE_DUMMY == header->_type )
      {
         LogError( "wrong lsn: %lld input", lsn ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      newOff = header->_lsn % ( fileSize - DPS_LOG_HEAD_LEN ) ;
      if( 0 == newOff || DPS_LOG_INVALID_LSN ==  header->_preLsn )
      {
         rc = DPS_LOG_REACH_HEAD;
         goto done ;
      }
      offset = DPS_LOG_HEAD_LEN +
               header->_preLsn % ( fileSize - DPS_LOG_HEAD_LEN ) ;
      ++prevCount ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 _dpsDumper::_seekToEnd( OSSFILE &in, INT64 &offset, const INT64 fileSize )
{
   SDB_ASSERT( offset >= DPS_LOG_HEAD_LEN, "offset lt DPS_LOG_HEAD_LEN" ) ;
   INT32 rc    = SDB_OK ;
   INT64 prevOffset = offset ;

   CHAR pRecordHead[ sizeof( dpsLogRecordHeader ) + 1 ] = { 0 } ;
   dpsLogRecordHeader *header = NULL;

   if( offset > fileSize || offset < DPS_LOG_HEAD_LEN )
   {
      LogError( "Wrong LSN position: %lld", offset ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   while( offset < fileSize )
   {
      rc = _readRecordHead( in, offset, fileSize, pRecordHead ) ;
      // ignore the return value
      header = ( dpsLogRecordHeader * )pRecordHead ;
      if ( header->_length < sizeof( dpsLogRecordHeader ) ||
           header->_length > DPS_RECORD_MAX_LEN )
      {
         rc = SDB_DPS_CORRUPTED_LOG ;
         goto error ;
      }

      if ( LOG_TYPE_DUMMY == header->_type )
      {
         break ;
      }

      prevOffset = offset ;
      offset += header->_length ;
   }

done:
   offset = prevOffset ;
   return rc ;
error:
   goto done ;
}


//////////////////////////////////////////////////////////////////////////
// main entry
INT32 main( INT32 argc, CHAR **argv )
{
   INT32 rc = SDB_OK ;
   po::options_description desc( "Command options" ) ;
   po::variables_map vm ;
   dpsDumper dumper ;
   BOOLEAN logOpened = FALSE ;

   rc = ossOpen( LOG_FILE_NAME, OSS_REPLACE|OSS_READWRITE,
                 OSS_RU|OSS_WU|OSS_RG|OSS_RO, logFile ) ;
   if ( SDB_OK != rc )
   {
      std::cout << "Failed to open the file "LOG_FILE_NAME ;
      if ( SDB_PERM == rc )
      {
         cout << " : permission error" ;
      }
      cout << endl ;
      goto error ;
   }
   logOpened = TRUE ;

   rc = dumper.initialize( argc, argv, desc, vm ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   rc = dumper.prepare( vm );
   if ( SDB_OK != rc )
   {
      dumper.displayArgs( desc );
      goto error;
   }

   rc = dumper.process( desc, vm ) ;
   if ( SDB_OK != rc )
   {
      goto error;
   }

done:
   if ( logOpened )
   {
      ossClose( logFile ) ;
   }
   if ( SDB_PMD_HELP_ONLY == rc || SDB_PMD_VERSION_ONLY == rc )
   {
      rc = SDB_OK ;
   }
   return engine::utilRC2ShellRC( rc ) ;
error :
   goto done  ;
}
