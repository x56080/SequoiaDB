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

   Source File Name = sptUsrFile.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "sptUsrFile.hpp"
#include "pd.hpp"
#include "ossMem.hpp"
#include "ossPrimitiveFileOp.hpp"
#include "ossCmdRunner.hpp"
#include <boost/algorithm/string.hpp>
#include "../bson/lib/md5.hpp"

#if defined(_LINUX)
#include <sys/stat.h>
#include <unistd.h>
#endif

#define SPT_MD5_READ_LEN 1024

using namespace std ;
using namespace bson ;

namespace engine
{
JS_MEMBER_FUNC_DEFINE( _sptUsrFile, read )
JS_MEMBER_FUNC_DEFINE( _sptUsrFile, seek )
JS_MEMBER_FUNC_DEFINE( _sptUsrFile, write )
JS_MEMBER_FUNC_DEFINE( _sptUsrFile, close )
JS_MEMBER_FUNC_DEFINE( _sptUsrFile, getInfo )
JS_MEMBER_FUNC_DEFINE( _sptUsrFile, toString )
JS_MEMBER_FUNC_DEFINE( _sptUsrFile, memberHelp )
JS_CONSTRUCT_FUNC_DEFINE( _sptUsrFile, construct )
JS_DESTRUCT_FUNC_DEFINE( _sptUsrFile, destruct )
JS_STATIC_FUNC_DEFINE( _sptUsrFile, remove )
JS_STATIC_FUNC_DEFINE( _sptUsrFile, exist )
JS_STATIC_FUNC_DEFINE( _sptUsrFile, copy )
JS_STATIC_FUNC_DEFINE( _sptUsrFile, move )
JS_STATIC_FUNC_DEFINE( _sptUsrFile, mkdir )
JS_STATIC_FUNC_DEFINE( _sptUsrFile, getFileObj )
JS_STATIC_FUNC_DEFINE( _sptUsrFile, md5 )
JS_STATIC_FUNC_DEFINE( _sptUsrFile, staticHelp )
JS_STATIC_FUNC_DEFINE( _sptUsrFile, readFile )
JS_STATIC_FUNC_DEFINE( _sptUsrFile, find )
JS_STATIC_FUNC_DEFINE( _sptUsrFile, list )
JS_STATIC_FUNC_DEFINE( _sptUsrFile, chmod )
JS_STATIC_FUNC_DEFINE( _sptUsrFile, chown )
JS_STATIC_FUNC_DEFINE( _sptUsrFile, chgrp )
JS_STATIC_FUNC_DEFINE( _sptUsrFile, getPathType )
JS_STATIC_FUNC_DEFINE( _sptUsrFile, getUmask )
JS_STATIC_FUNC_DEFINE( _sptUsrFile, setUmask )
JS_STATIC_FUNC_DEFINE( _sptUsrFile, isEmptyDir )
JS_STATIC_FUNC_DEFINE( _sptUsrFile, getStat )

JS_BEGIN_MAPPING( _sptUsrFile, "File" )
   JS_ADD_MEMBER_FUNC_WITHATTR( "_read", read, 0 )
   JS_ADD_MEMBER_FUNC_WITHATTR( "_write", write, 0 )
   JS_ADD_MEMBER_FUNC_WITHATTR( "_close", close, 0 )
   JS_ADD_MEMBER_FUNC_WITHATTR( "_seek", seek, 0 )
   JS_ADD_MEMBER_FUNC_WITHATTR( "_getInfo", getInfo, 0 )
   JS_ADD_MEMBER_FUNC_WITHATTR( "_toString", toString, 0 )
   JS_ADD_MEMBER_FUNC( "help", memberHelp )
   JS_ADD_STATIC_FUNC_WITHATTR( "_getFileObj", getFileObj, 0 )
   JS_ADD_STATIC_FUNC_WITHATTR( "_readFile", readFile, 0 )
   JS_ADD_STATIC_FUNC_WITHATTR( "_getPathType", getPathType, 0 )
   JS_ADD_STATIC_FUNC( "remove", remove )
   JS_ADD_STATIC_FUNC( "exist", exist )
   JS_ADD_STATIC_FUNC( "copy", copy )
   JS_ADD_STATIC_FUNC( "move", move )
   JS_ADD_STATIC_FUNC( "mkdir", mkdir )
   JS_ADD_STATIC_FUNC( "setUmask", setUmask )
   JS_ADD_STATIC_FUNC_WITHATTR( "_getUmask", getUmask, 0 )
   JS_ADD_STATIC_FUNC_WITHATTR( "_list", list, 0 )
   JS_ADD_STATIC_FUNC_WITHATTR( "_find", find, 0 )
   JS_ADD_STATIC_FUNC( "chmod", chmod )
   JS_ADD_STATIC_FUNC( "chown", chown )
   JS_ADD_STATIC_FUNC( "chgrp", chgrp )
   JS_ADD_STATIC_FUNC( "isEmptyDir", isEmptyDir )
   JS_ADD_STATIC_FUNC( "stat", getStat )
   JS_ADD_STATIC_FUNC( "md5", md5 )
   JS_ADD_STATIC_FUNC( "help", staticHelp )
   JS_ADD_CONSTRUCT_FUNC( construct )
   JS_ADD_DESTRUCT_FUNC( destruct )
JS_MAPPING_END()

   _sptUsrFile::_sptUsrFile()
   {
   }

   _sptUsrFile::~_sptUsrFile()
   {
      if ( _file.isOpened() )
      {
         ossClose( _file ) ;
      }
   }

   INT32 _sptUsrFile::construct( const _sptArguments &arg,
                                 _sptReturnVal &rval,
                                 bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      UINT32 permission = OSS_RWXU ;

      // get filename
      rc = arg.getString( 0, _filename ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "filename must be config" ) ;
      }
      else if ( rc )
      {
         detail = BSON( SPT_ERR << "filename must be string" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get filename, rc: %d", rc ) ;

      // get mode
      if ( arg.argc() > 1 )
      {
         INT32 mode = 0 ;
         rc = arg.getNative( 1, (void*)&mode, SPT_NATIVE_INT32 ) ;
         if ( rc )
         {
            detail = BSON( SPT_ERR << "mode must be INT32" ) ;
            goto error ;
         }
         permission = 0 ;

         if ( mode & 0x0001 )
         {
            permission |= OSS_XO ;
         }
         if ( mode & 0x0002 )
         {
            permission |= OSS_WO ;
         }
         if ( mode & 0x0004 )
         {
            permission |= OSS_RO ;
         }
         if ( mode & 0x0008 )
         {
            permission |= OSS_XG ;
         }
         if ( mode & 0x0010 )
         {
            permission |= OSS_WG ;
         }
         if ( mode & 0x0020 )
         {
            permission |= OSS_RG ;
         }
         if ( mode & 0x0040 )
         {
            permission |= OSS_XU ;
         }
         if ( mode & 0x0080 )
         {
            permission |= OSS_WU ;
         }
         if ( mode & 0x0100 )
         {
            permission |= OSS_RU ;
         }
      }

      // open file
      rc = ossOpen( _filename.c_str(),
                    OSS_READWRITE | OSS_CREATE,
                    permission,
                    _file ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to open file:%s, rc:%d",
                 _filename.c_str(), rc ) ;
         goto error ;
      }
      rval.addSelfProperty( "_filename" )->setValue( _filename ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFile::getFileObj( const _sptArguments &arg,
                                  _sptReturnVal &rval,
                                  bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;

      // new File Object and set return value
      _sptUsrFile * fileObj = _sptUsrFile::crtInstance() ;
      if ( !fileObj )
      {
         rc = SDB_OOM ;
         detail = BSON( SPT_ERR << "Create object failed" ) ;
      }
      else
      {
         rval.setUsrObjectVal<_sptUsrFile>( fileObj ) ;
      }
      return rc ;
   }

   INT32 _sptUsrFile::destruct()
   {
      if ( _file.isOpened() )
      {
         ossClose( _file ) ;
      }
      return SDB_OK ;
   }

   INT32 _sptUsrFile::read( const _sptArguments &arg,
                            _sptReturnVal &rval,
                            bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
#define SPT_READ_LEN 1024
      SINT64 len = SPT_READ_LEN ;
      CHAR stackBuf[ SPT_READ_LEN + 1 ] = { 0 } ;
      CHAR *buf = NULL ;
      SINT64 read = 0 ;

      if ( !_file.isOpened() )
      {
         PD_LOG( PDERROR, "the file is not opened." ) ;
         detail = BSON( SPT_ERR << "file is not opened" ) ;
         rc = SDB_IO ;
         goto error ;
      }

      //get read length
      rc = arg.getNative( 0, &len, SPT_NATIVE_INT64 ) ;
      if ( rc && SDB_OUT_OF_BOUND != rc )
      {
         detail = BSON( SPT_ERR << "size must be native type" ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get size, rc: %d", rc ) ;
      }

      if ( SPT_READ_LEN < len )
      {
         buf = ( CHAR * )SDB_OSS_MALLOC( len + 1 ) ;
         if ( NULL == buf )
         {
            PD_LOG( PDERROR, "failed to allocate mem." ) ;
            rc = SDB_OOM ;
            goto error ;
         }
      }
      else
      {
         buf = ( CHAR * )stackBuf ;
      }

      rc = ossReadN( &_file, len, buf, read ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to read file:%d", rc ) ;
         goto error ;
      }
      buf[ read ] = '\0' ;

      rval.getReturnVal().setValue( buf ) ;
   done:
      if ( SPT_READ_LEN < len && NULL != buf )
      {
         SDB_OSS_FREE( buf ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFile::write( const _sptArguments &arg,
                             _sptReturnVal &rval,
                             bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string content ;

      if ( !_file.isOpened() )
      {
         PD_LOG( PDERROR, "the file is not opened." ) ;
         detail = BSON( SPT_ERR << "file is not opened" ) ;
         rc = SDB_IO ;
         goto error ;
      }

      // get content
      rc = arg.getString( 0, content ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "content must be config" ) ;
      }
      else if ( rc )
      {
         detail = BSON( SPT_ERR << "content must be string" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get content, rc: %d", rc ) ;

      rc = ossWriteN( &_file, content.c_str(), content.size() ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to write to file:%d", rc ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFile::seek( const _sptArguments &arg,
                            _sptReturnVal &rval,
                            bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      INT32 seekSize = 0 ;
      OSS_SEEK whence ;
      string whenceStr = "b" ;

      if ( !_file.isOpened() )
      {
         PD_LOG( PDERROR, "the file is not opened." ) ;
         detail = BSON( SPT_ERR << "file is not opened" ) ;
         rc = SDB_IO ;
         goto error ;
      }

      rc = arg.getNative( 0, &seekSize, SPT_NATIVE_INT32 ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "offset must be config" ) ;
      }
      else if ( rc )
      {
         detail = BSON( SPT_ERR << "offset must be native type" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get offset, rc: %d", rc ) ;

      rc = arg.getString( 1, whenceStr ) ;
      if ( rc && SDB_OUT_OF_BOUND != rc )
      {
         detail = BSON( SPT_ERR << "where must be string(b/c/e)" ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get where, rc: %d", rc ) ;
      }

      if ( "b" == whenceStr )
      {
         whence = OSS_SEEK_SET ;
      }
      else if ( "c" == whenceStr )
      {
         whence = OSS_SEEK_CUR ;
      }
      else if ( "e" == whenceStr )
      {
         whence = OSS_SEEK_END ;
      }
      else
      {
         detail = BSON( SPT_ERR << "where must be string(b/c/e)" ) ;
         PD_LOG( PDERROR, "invalid arg whence:%s", whenceStr.c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = ossSeek( &_file, seekSize, whence ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to seek:%d", rc ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFile::toString( const _sptArguments & arg,
                                _sptReturnVal & rval,
                                bson::BSONObj & detail )
   {
      rval.getReturnVal().setValue( _filename ) ;
      return SDB_OK ;
   }

   INT32 _sptUsrFile::getInfo( const _sptArguments &arg,
                               _sptReturnVal &rval,
                               bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj remoteInfo ;
      BSONObjBuilder builder ;

      // get information about Object
      rc = arg.getBsonobj( 0, remoteInfo ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "remoteInfo must be config" ) ;
         goto error ;
      }
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "remoteInfo must be obj" ) ;
         goto error ;
      }

      // if it is not a remote obj, append local filename
      if ( FALSE == remoteInfo.getBoolField( "isRemote" ) )
      {
         builder.append( "filename", _filename ) ;
      }

      // build result
      builder.append( "type", "File" ) ;
      builder.appendElements( remoteInfo ) ;
      rval.getReturnVal().setValue( builder.obj() ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFile::memberHelp( const _sptArguments & arg,
                                  _sptReturnVal & rval,
                                  BSONObj & detail )
   {
      stringstream ss ;
      ss << "File functions:" << endl
         << "   read( [size] )" << endl
         << "   write( content )" << endl
         << "   seek( offset, [where] ) " << endl
         << "   close()" << endl
         << "   remove( filepath )" << endl
         << "   exist( filepath )" << endl
         << "   copy( src, dst, [replace], [mode] )" << endl
         << "   move( src, dst )" << endl
         << "   mkdir( name, [mode] )" << endl
         << "   find( optionObj, [filterObj] )" << endl
         << "   chmod( filename, mode, [recursive] )" << endl
         << "   chown( filename, optionObj, [recursive] )" << endl
         << "   chgrp( filename, groupname, [recursive] )" << endl
         << "   setUmask( umask )" << endl
         << "   getUmask( base )" << endl
         << "   list( [optionObj], [filterObj] )" << endl
         << "   isFile( pathname )" << endl
         << "   isDir( pathname )" << endl
         << "   isEmptyDir( dirName )" << endl
         << "   stat( filename )" << endl
         << "   md5( filename )" << endl ;
      rval.getReturnVal().setValue( ss.str() ) ;
      return SDB_OK ;
   }

   INT32 _sptUsrFile::remove( const _sptArguments &arg,
                              _sptReturnVal &rval,
                              bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string fullPath ;

      rc = arg.getString( 0, fullPath ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "filepath must be config" ) ;
      }
      else if ( rc )
      {
         detail = BSON( SPT_ERR << "filepath must be string" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get filepath, rc: %d", rc ) ;

      // delete file
      rc = ossDelete( fullPath.c_str() ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to remove file:%s, rc:%d",
                 fullPath.c_str(), rc ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFile::exist( const _sptArguments & arg,
                             _sptReturnVal & rval,
                             BSONObj & detail )
   {
      INT32 rc = SDB_OK ;
      string fullPath ;
      BOOLEAN fileExist = FALSE ;

      rc = arg.getString( 0, fullPath ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "filepath must be config" ) ;
      }
      else if ( rc )
      {
         detail = BSON( SPT_ERR << "filepath must be string" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get filepath, rc: %d", rc ) ;

      // try to access file
      rc = ossAccess( fullPath.c_str() ) ;
      if ( SDB_OK != rc && SDB_FNE != rc )
      {
         detail = BSON( SPT_ERR << "access file failed" ) ;
         goto error ;
      }
      else if ( SDB_OK == rc )
      {
         fileExist = TRUE ;
      }
      rc = SDB_OK ;
      rval.getReturnVal().setValue( fileExist ? true : false ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFile::copy( const _sptArguments & arg,
                            _sptReturnVal & rval,
                            BSONObj & detail )
   {
      INT32 rc = SDB_OK ;
      string src ;
      string dst ;
      INT32 isReplace = TRUE ;
      UINT32 permission = OSS_DEFAULTFILE ;

      rc = arg.getString( 0, src ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "src is required" ) ;
         goto error ;
      }
      else if ( rc )
      {
         detail = BSON( SPT_ERR << "src must be string" ) ;
         goto error ;
      }

      rc = arg.getString( 1, dst ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "dst is required" ) ;
         goto error ;
      }
      else if ( rc )
      {
         detail = BSON( SPT_ERR << "dst must be string" ) ;
         goto error ;
      }

      if ( arg.argc() > 2 )
      {
         rc = arg.getNative( 2, (void*)&isReplace, SPT_NATIVE_INT32 ) ;
         if ( rc )
         {
            detail = BSON( SPT_ERR << "replace must be BOOLEAN" ) ;
            goto error ;
         }
      }

      if ( arg.argc() > 3 )
      {
         INT32 mode = 0 ;
         rc = arg.getNative( 3, (void*)&mode, SPT_NATIVE_INT32 ) ;
         if ( rc )
         {
            detail = BSON( SPT_ERR << "mode must be INT32" ) ;
            goto error ;
         }
         permission = 0 ;

         if ( mode & 0x0001 )
         {
            permission |= OSS_XO ;
         }
         if ( mode & 0x0002 )
         {
            permission |= OSS_WO ;
         }
         if ( mode & 0x0004 )
         {
            permission |= OSS_RO ;
         }
         if ( mode & 0x0008 )
         {
            permission |= OSS_XG ;
         }
         if ( mode & 0x0010 )
         {
            permission |= OSS_WG ;
         }
         if ( mode & 0x0020 )
         {
            permission |= OSS_RG ;
         }
         if ( mode & 0x0040 )
         {
            permission |= OSS_XU ;
         }
         if ( mode & 0x0080 )
         {
            permission |= OSS_WU ;
         }
         if ( mode & 0x0100 )
         {
            permission |= OSS_RU ;
         }
      }
#if defined (_LINUX)
      else
      {
         struct stat fileStat ;
         mode_t fileMode ;
         permission = 0 ;
         if ( stat( src.c_str(), &fileStat ) )
         {
            detail = BSON( SPT_ERR << "Failed to get src file stat" ) ;
            rc = SDB_SYS ;
         }
         fileMode = fileStat.st_mode ;
         if ( fileMode & S_IRUSR )
         {
            permission |= OSS_RU ;
         }
         if ( fileMode & S_IWUSR )
         {
            permission |= OSS_WU ;
         }
         if ( fileMode & S_IXUSR )
         {
            permission |= OSS_XU ;
         }
         if ( fileMode & S_IRGRP )
         {
            permission |= OSS_RG ;
         }
         if ( fileMode & S_IWGRP )
         {
            permission |= OSS_WG ;
         }
         if ( fileMode & S_IXGRP )
         {
            permission |= OSS_XG ;
         }
         if ( fileMode & S_IROTH )
         {
            permission |= OSS_RO ;
         }
         if ( fileMode & S_IWOTH )
         {
            permission |= OSS_WO ;
         }
         if ( fileMode & S_IXOTH )
         {
            permission |= OSS_XO ;
         }
      }
#endif

      rc = ossFileCopy( src.c_str(), dst.c_str(), permission, isReplace ) ;
      if ( rc )
      {
         detail = BSON( SPT_ERR << "copy file failed" ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFile::move( const _sptArguments & arg,
                            _sptReturnVal & rval,
                            BSONObj & detail )
   {
      INT32 rc = SDB_OK ;
      string src ;
      string dst ;

      rc = arg.getString( 0, src ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "src is required" ) ;
         goto error ;
      }
      else if ( rc )
      {
         detail = BSON( SPT_ERR << "src must be string" ) ;
         goto error ;
      }

      rc = arg.getString( 1, dst ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "dst is required" ) ;
         goto error ;
      }
      else if ( rc )
      {
         detail = BSON( SPT_ERR << "dst must be string" ) ;
         goto error ;
      }

      rc = ossRenamePath( src.c_str(), dst.c_str() ) ;
      if ( rc )
      {
         detail = BSON( SPT_ERR << "rename path failed" ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFile::mkdir( const _sptArguments & arg,
                             _sptReturnVal & rval,
                             BSONObj & detail )
   {
      INT32 rc = SDB_OK ;
      string name ;
      UINT32 permission = OSS_DEFAULTDIR ;

      rc = arg.getString( 0, name ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "name is required" ) ;
         goto error ;
      }
      else if ( rc )
      {
         detail = BSON( SPT_ERR << "name must be string" ) ;
         goto error ;
      }

      if ( arg.argc() > 1 )
      {
         INT32 mode = 0 ;
         rc = arg.getNative( 1, (void*)&mode, SPT_NATIVE_INT32 ) ;
         if ( rc )
         {
            detail = BSON( SPT_ERR << "mode must be INT32" ) ;
            goto error ;
         }
         permission = 0 ;

         if ( mode & 0x0001 )
         {
            permission |= OSS_XO ;
         }
         if ( mode & 0x0002 )
         {
            permission |= OSS_WO ;
         }
         if ( mode & 0x0004 )
         {
            permission |= OSS_RO ;
         }
         if ( mode & 0x0008 )
         {
            permission |= OSS_XG ;
         }
         if ( mode & 0x0010 )
         {
            permission |= OSS_WG ;
         }
         if ( mode & 0x0020 )
         {
            permission |= OSS_RG ;
         }
         if ( mode & 0x0040 )
         {
            permission |= OSS_XU ;
         }
         if ( mode & 0x0080 )
         {
            permission |= OSS_WU ;
         }
         if ( mode & 0x0100 )
         {
            permission |= OSS_RU ;
         }
      }

      rc = ossMkdir( name.c_str(), permission ) ;
      if ( SDB_FE == rc )
      {
         rc = SDB_OK ;
      }
      else if ( rc )
      {
         detail = BSON( SPT_ERR << "create dir failed" ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFile::close( const _sptArguments &arg,
                             _sptReturnVal &rval,
                             bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      if ( _file.isOpened() )
      {
         rc = ossClose( _file ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to close file:%d", rc ) ;
            goto error ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   // Read all the contents of the file
   INT32 _sptUsrFile::readFile( const _sptArguments & arg,
                                _sptReturnVal & rval,
                                BSONObj & detail )
   {
      INT32 rc = SDB_OK ;
      CHAR* buf = NULL ;
      ossPrimitiveFileOp op ;
      string name ;
      INT32 readLen = 1024 ;
      INT32 bufLen = readLen + 1 ;

      rc = arg.getString( 0, name ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "name must be config" ) ;
         goto error ;
      }
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "name must be string" ) ;
         goto error ;
      }

      // open file
      rc = op.Open( name.c_str() , OSS_PRIMITIVE_FILE_OP_READ_ONLY ) ;
      if ( rc != SDB_OK )
      {
         PD_LOG( PDERROR, "Can't open file: %s", name.c_str() ) ;
         goto error ;
      }

      // malloc buff
      buf = (CHAR*) SDB_OSS_MALLOC( bufLen ) ;
      if ( NULL == buf )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Failed to malloc buff" ) ;
         detail = BSON( SPT_ERR << "Failed to malloc buff" ) ;
         goto error ;
      }

      {
         INT32 readByte = 0 ;
         INT32 hasRead = 0 ;
         INT32 increaseLen = 1024 ;
         CHAR *curPos = buf ;
         BOOLEAN finishRead = FALSE ;
         while( !finishRead )
         {
            rc = op.Read( readLen , curPos , &readByte ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to read file" ) ;
               detail = BSON( SPT_ERR << "Failed to read file" ) ;
               goto error ;
            }
            hasRead += readByte ;

            // mem not enough, need to realloc, newBuffSize = 2*oldBuffSize
            if ( readByte == readLen )
            {
               bufLen += increaseLen ;
               buf = (CHAR*) SDB_OSS_REALLOC( buf, bufLen ) ;
               if ( NULL == buf )
               {
                  rc = SDB_OOM ;
                  PD_LOG( PDERROR, "Failed to realloc buff" ) ;
                  detail = BSON( SPT_ERR << "Failed to realloc buff" ) ;
                  goto error ;
               }
               curPos = buf + hasRead ;
               readLen = increaseLen ;
               increaseLen *= 2 ;
            }
            else
            {
               finishRead = TRUE ;
            }

         }
         buf[ hasRead ] = '\0' ;
      }
      rval.getReturnVal().setValue( buf ) ;

   done:
      op.Close() ;
      SDB_OSS_FREE( buf ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFile::find( const _sptArguments &arg,
                            _sptReturnVal &rval,
                            BSONObj &detail )
   {
      INT32 rc           = SDB_OK ;
      string findType    = "n" ;
      UINT32 exitCode    = 0 ;
      string             mode ;
      string             rootDir ;
      _ossCmdRunner      runner ;
      string             value ;
      string             pathname ;
      string             outStr ;
      BSONObj            optionObj ;
      BSONObjBuilder     builder ;
      stringstream       cmd ;

      rc = arg.getBsonobj( 0, optionObj ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "optionObj must be config" ) ;
         goto error ;
      }
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "optionObj must be BSONObj" ) ;
         goto error ;
      }

      if ( TRUE == optionObj.hasField( "value" ) )
      {
         if ( String != optionObj.getField( "value" ).type() )
         {
            rc = SDB_INVALIDARG ;
            detail = BSON( SPT_ERR << "value must be string" ) ;
            goto error ;
         }
         value = optionObj.getStringField( "value" ) ;
      }

      if ( TRUE == optionObj.hasField( "mode" ) )
      {
         if ( String != optionObj.getField( "mode" ).type() )
         {
            rc = SDB_INVALIDARG ;
            detail = BSON( SPT_ERR << "mode must be string" ) ;
            goto error ;
         }
         findType = optionObj.getStringField( "mode" ) ;
      }

      /* get the way to find file:
         -name:   filename
         -user:   user uname
         -group:  group gname
         -perm:   permission
      */
      if ( "n" == findType )
      {
         // can't contain path, only filename
         if ( string::npos != value.find( "/", 0 ) )
         {
            rc = SDB_INVALIDARG ;
            detail = BSON( SPT_ERR << "value shouldn't contain '/'" ) ;
            goto error ;
         }
         mode = " -name" ;
      }
      else if ( "u" == findType )
      {
         mode = " -user" ;
      }
      else if ( "p" == findType )
      {
         mode = " -perm" ;
      }
      else if ( "g" == findType )
      {
         mode = " -group" ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "mode must be required type" ) ;
         goto error ;
      }

      if ( TRUE == optionObj.hasField( "pathname" ) )
      {
         if ( String != optionObj.getField( "pathname" ).type() )
         {
            rc = SDB_INVALIDARG ;
            detail = BSON( SPT_ERR << "pathname must be string" ) ;
            goto error ;
         }
         pathname = optionObj.getStringField( "pathname" ) ;

      }

      // build cmd
#if defined (_LINUX)
      cmd << "find" ;

      if ( FALSE == pathname.empty() )
      {
         cmd << " " << pathname ;
      }

      if ( FALSE == value.empty() )
      {
         cmd << mode << " " << value ;
      }
#elif defined (_WINDOWS)
      // windows only support finding file by name
      if ( " -name" != mode )
      {
         goto done ;
      }

      if ( !pathname.empty() &&
           '\\' != pathname[ pathname.size() - 1 ] )
      {
         pathname += "\\" ;
      }
      cmd << "cmd /C dir /b /s "<< pathname << value ;
#endif
      // run cmd
      rc = runner.exec( cmd.str().c_str(), exitCode,
                     FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         stringstream ss ;
         ss << "failed to exec cmd " << cmd.str() << ",rc: "
            << rc
            << ",exit: "
            << exitCode ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }

      // get result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to read msg from cmd runner:%d", rc ) ;
         stringstream ss ;
         ss << "failed to read msg from cmd \"" << cmd.str() << "\", rc:"
            << rc ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }
      else if ( SDB_OK != exitCode )
      {
         if ( '\n' == outStr[ outStr.size() - 1 ] )
         {
            outStr.erase( outStr.size()-1, 1 ) ;
         }

         rc = exitCode ;
         detail = BSON( SPT_ERR << outStr ) ;
         goto error ;
      }

      // extract result
      rc = _extractFindInfo( outStr.c_str(), builder ) ;
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to extract find info" ) ;
         goto error ;
      }

      rval.getReturnVal().setValue( builder.obj() ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFile::_extractFindInfo( const CHAR* buf,
                                        BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      vector<string> splited ;

      /*
      content format:
         xxxxxxxxx1
         xxxxxxxxx2
         xxxxxxxxx3
      */
      try
      {
         boost::algorithm::split( splited, buf,
                                  boost::is_any_of( "\r\n" ) ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                 rc, e.what() ) ;
         goto error ;
      }

      for ( vector<string>::iterator itr = splited.begin();
            itr != splited.end();  )
      {
         if ( itr->empty() )
         {
            itr = splited.erase( itr ) ;
         }
         else
         {
            itr++ ;
         }
      }

      for( UINT32 index = 0; index < splited.size(); index++ )
      {
         BSONObjBuilder objBuilder ;
         objBuilder.append( "pathname", splited[ index ] ) ;
         try
         {
            builder.append( boost::lexical_cast<string>( index ).c_str(),
                            objBuilder.obj() ) ;
         }
         catch( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Fail to build retObj, rc: %d, detail: %s",
                    rc, e.what() ) ;
            goto error ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFile::list( const _sptArguments &arg,
                            _sptReturnVal &rval,
                            BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      UINT32 exitCode    = 0 ;
      BOOLEAN showDetail = FALSE ;

      BSONObjBuilder     builder ;
      stringstream       cmd ;
      _ossCmdRunner      runner ;
      string             outStr ;
      BSONObj            optionObj ;

      // check argument and build cmd
#if defined (_LINUX)
      cmd << "ls -A -l" ;
#elif defined (_WINDOWS)
      cmd << "cmd /C dir /-C /A" ;
#endif
      if ( 1 <= arg.argc() )
      {
         rc = arg.getBsonobj( 0, optionObj ) ;
         if ( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "optionObj must be BSONObj" ) ;
            goto error ;
         }
      }
      showDetail = optionObj.getBoolField( "detail") ;

      if ( TRUE == optionObj.hasField( "pathname" ) )
      {
         if ( String != optionObj.getField( "pathname" ).type() )
         {
            rc = SDB_INVALIDARG ;
            detail = BSON( SPT_ERR << "pathname must be string" ) ;
         }
         cmd << " " << optionObj.getStringField( "pathname" ) ;
      }

      // run cmd
      rc = runner.exec( cmd.str().c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         stringstream ss ;
         ss << "failed to exec cmd " << cmd.str() << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }

      // get result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to read msg from cmd runner:%d", rc ) ;
         stringstream ss ;
         ss << "failed to read msg from cmd \"" << cmd.str() << "\", rc:"
            << rc ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }
      else if ( SDB_OK != exitCode )
      {
         rc = exitCode ;
         detail = BSON( SPT_ERR << outStr ) ;
         goto error ;
      }

      // extract result
      rc = _extractListInfo( outStr.c_str(), builder, showDetail ) ;
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to extract list info" ) ;
         goto error ;
      }

      rval.getReturnVal().setValue( builder.obj() ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

#if defined (_LINUX)
   INT32 _sptUsrFile::_extractListInfo( const CHAR* buf,
                                        BSONObjBuilder &builder,
                                        BOOLEAN showDetail )
   {
      INT32 rc = SDB_OK ;
      vector<string> splited ;
      vector< BSONObj > fileVec ;

      if ( NULL == buf )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "buf can't be null, rc: %d", rc ) ;
         goto error ;
      }

      /*
      content format:
         total 2
         drwxr-xr-x  7 root      root       4096 Oct 11 15:28 20000
         drwxr-xr-x  7 root      root       4096 Oct 11 15:05 30000
      */
      try
      {
         boost::algorithm::split( splited, buf, boost::is_any_of("\r\n") ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                 rc, e.what() ) ;
         goto error ;
      }

      for ( vector<string>::iterator itr = splited.begin();
            itr != splited.end();  )
      {
         if ( itr->empty() )
         {
            itr = splited.erase( itr ) ;
         }
         else
         {
            itr++ ;
         }
      }
      // erase first row: total xx
      splited.erase( splited.begin() ) ;

      if( TRUE == showDetail )
      {
         for ( vector<string>::iterator itrSplit = splited.begin();
            itrSplit != splited.end(); itrSplit++ )
         {
            vector<string> columns ;
            BSONObjBuilder fileObjBuilder ;

            try
            {
               boost::algorithm::split( columns, *itrSplit,
                                        boost::is_any_of(" ") ) ;
            }
            catch( std::exception &e )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                       rc, e.what() ) ;
               goto error ;
            }
            for ( vector<string>::iterator itrCol = columns.begin();
                  itrCol != columns.end();  )
            {
               if ( itrCol->empty() )
               {
                  itrCol = columns.erase( itrCol ) ;
               }
               else
               {
                  itrCol++ ;
               }
            }

            // one line has 9 columns at least. otherwise, discard it
            if ( 9 > columns.size() )
            {
               continue ;
            }
            else
            {
               // exist filename contain ' ', need to merge
               for ( UINT32 index = 9; index < columns.size(); index++ )
               {
                  columns[ 8 ] += " " + columns[ index ] ;
               }
            }
            fileObjBuilder.append( "name", columns[ 8 ] ) ;
            fileObjBuilder.append( "size", columns[ 4 ] ) ;
            fileObjBuilder.append( "mode", columns[ 0 ] ) ;
            fileObjBuilder.append( "user", columns[ 2 ] ) ;
            fileObjBuilder.append( "group", columns[ 3 ] ) ;
            fileObjBuilder.append( "lasttime",
                                   columns[ 5 ] + " " +
                                   columns[ 6 ] + " " +
                                   columns[ 7 ] ) ;
            fileVec.push_back( fileObjBuilder.obj() ) ;
         }
      }
      else
      {
         for ( vector<string>::iterator itrSplit = splited.begin();
            itrSplit != splited.end(); itrSplit++ )
         {
            vector<string> columns ;
            BSONObjBuilder fileObjBuilder ;

            try
            {
               boost::algorithm::split( columns, *itrSplit,
                                        boost::is_any_of(" ") ) ;
            }
            catch( std::exception &e )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                       rc, e.what() ) ;
               goto error ;
            }
            for ( vector<string>::iterator itrCol = columns.begin();
                  itrCol != columns.end();  )
            {
               if ( itrCol->empty() )
               {
                  itrCol = columns.erase( itrCol ) ;
               }
               else
               {
                  itrCol++ ;
               }
            }

            // at least 9 columns
            if ( 9 > columns.size() )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Failed to build result" ) ;
               goto error ;
            }
            else
            {
               // exist filename contain ' ', need to merge
               for ( UINT32 index = 9; index < columns.size(); index++ )
               {
                  columns[ 8 ] += " " + columns[ index ] ;
               }
            }
            fileObjBuilder.append( "name", columns[ 8 ] ) ;
            fileObjBuilder.append( "mode", columns[ 0 ] ) ;
            fileObjBuilder.append( "user", columns[ 2 ] ) ;
            fileVec.push_back( fileObjBuilder.obj() ) ;
         }
      }

      // merge vector< BSONObj> into BSONObj
      for( UINT32 index = 0; index < fileVec.size(); index++ )
      {
         try
         {
            builder.append( boost::lexical_cast<string>( index ).c_str(),
                            fileVec[ index ] ) ;
         }
         catch( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Fail to build retObj, rc: %d, detail: %s",
                    rc, e.what() ) ;
            goto error ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

#elif defined (_WINDOWS)

   INT32 _sptUsrFile::_extractListInfo( const CHAR* buf,
                                        BSONObjBuilder &builder,
                                        BOOLEAN showDetail )
   {
      INT32 rc = SDB_OK ;
      vector<string> splited ;
      vector< BSONObj > fileVec ;

      if ( NULL == buf )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "buf can't be null, rc: %d", rc ) ;
         goto error ;
      }

      /*
       xxxxxxxxx
      xxxxxxxxxxx

      C:\Users\wujiaming\Documents\NetSarang\Xshell\Sessions xxxx

      content format:
         2016/10/11  13:26              3410 xxxxxx
         2016/05/18  08:56              3391 xxxxxxx
              12 xxxxx          37488 xxxx
               2 xxxxx    20122185728 xxxx
      */
      try
      {
         boost::algorithm::split( splited, buf, boost::is_any_of( "\r\n" ) ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                 rc, e.what() ) ;
         goto error ;
      }

      for ( vector<string>::iterator itr = splited.begin();
            itr != splited.end();  )
      {
         if ( itr->empty() )
         {
            itr = splited.erase( itr ) ;
         }
         else
         {
            itr++ ;
         }
      }
      // erase head and tail
      splited.erase( splited.end() - 2, splited.end() ) ;
      splited.erase( splited.begin() , splited.begin() + 5 ) ;

      if( TRUE == showDetail )
      {
         for ( vector<string>::iterator itrSplit = splited.begin();
            itrSplit != splited.end(); itrSplit++ )
         {
            vector<string> columns ;
            BSONObjBuilder fileObjBuilder ;

            try
            {
               boost::algorithm::split( columns, *itrSplit,
                                        boost::is_any_of( " " ) ) ;
            }
            catch( std::exception &e )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                       rc, e.what() ) ;
               goto error ;
            }
            for ( vector<string>::iterator itrCol = columns.begin();
                  itrCol != columns.end();  )
            {
               if ( itrCol->empty() )
               {
                  itrCol = columns.erase( itrCol ) ;
               }
               else
               {
                  itrCol++ ;
               }
            }

            // at least 4 columns
            if ( 4 > columns.size() )
            {
               continue ;
            }
            else
            {
               // exist filename contain ' ', need to merge
               for ( UINT32 index = 4; index < columns.size(); index++ )
               {
                  columns[ 3 ] += " " + columns[ index ] ;
               }
            }
            if ( "<DIV>" == columns[ 2 ] )
            {
               columns[ 2 ] = "" ;
            }
            fileObjBuilder.append( "name", columns[ 3 ] ) ;
            fileObjBuilder.append( "size", columns[ 2 ] ) ;
            fileObjBuilder.append( "mode", "" ) ;
            fileObjBuilder.append( "user", "" ) ;
            fileObjBuilder.append( "group", "" ) ;
            fileObjBuilder.append( "lasttime",
                                   columns[ 0 ] + " " +columns[ 1 ] ) ;
            fileVec.push_back( fileObjBuilder.obj() ) ;
         }
      }
      else
      {
         for ( vector<string>::iterator itrSplit = splited.begin();
            itrSplit != splited.end(); itrSplit++ )
         {
            vector<string> columns ;
            BSONObjBuilder fileObjBuilder ;

            try
            {
               boost::algorithm::split( columns, *itrSplit,
                                        boost::is_any_of(" ") ) ;
            }
            catch( std::exception &e )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                       rc, e.what() ) ;
               goto error ;
            }
            for ( vector<string>::iterator itrCol = columns.begin();
                  itrCol != columns.end();  )
            {
               if ( itrCol->empty() )
               {
                  itrCol = columns.erase( itrCol ) ;
               }
               else
               {
                  itrCol++ ;
               }
            }

            // at least 4 columns
            if ( 4 > columns.size() )
            {
               continue ;
            }
            else
            {
               // exist filename contain ' ', need to merge
               for ( UINT32 index = 4; index < columns.size(); index++ )
               {
                  columns[ 3 ] += " " + columns[ index ] ;
               }
            }
            fileObjBuilder.append( "name", columns[ 3 ] ) ;
            fileObjBuilder.append( "mode", "" ) ;
            fileObjBuilder.append( "user", "" ) ;
            fileVec.push_back( fileObjBuilder.obj() ) ;
         }
      }

      // merge vector< BSONObj> into BSONObj
      for( UINT32 index = 0; index < fileVec.size(); index++ )
      {
         try
         {
            builder.append( boost::lexical_cast<string>( index ).c_str(),
                            fileVec[ index ] ) ;
         }
         catch( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Fail to build retObj, rc: %d, detail: %s",
                    rc, e.what() ) ;
            goto error ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }
#endif

   INT32 _sptUsrFile::chmod( const _sptArguments &arg,
                             _sptReturnVal &rval,
                             BSONObj &detail )
   {
#if defined (_LINUX)
      INT32 rc = SDB_OK ;
      UINT32 exitCode    = 0 ;
      BOOLEAN isRecursive = FALSE ;
      stringstream       cmd ;
      _ossCmdRunner      runner ;
      string             pathname ;
      INT32              mode ;
      string             outStr ;

      // read argument
      rc = arg.getString( 0, pathname ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "filename must be config" ) ;
         goto error ;
      }
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "filename must be string" ) ;
         goto error ;
      }

      rc = arg.getNative( 1, &mode, SPT_NATIVE_INT32 ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "mode must be config" ) ;
         goto error ;
      }
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "mode must be INT32" ) ;
         goto error ;
      }

      if ( 3 <= arg.argc() )
      {
         rc = arg.getNative( 2, &isRecursive, SPT_NATIVE_INT32 ) ;
         if ( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "recursive must be bool" ) ;
            goto error ;
         }
      }

      // build cmd
      cmd << "chmod" ;
      if ( TRUE == isRecursive )
      {
         cmd << " -R" ;
      }
      cmd << " " << oct << mode << " " << pathname ;

      // run cmd
      rc = runner.exec( cmd.str().c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         stringstream ss ;
         ss << "failed to exec cmd " << cmd.str() << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }

      // read result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to read result" ) ;
         goto error ;
      }
      else if ( SDB_OK != exitCode )
      {
         rc = exitCode ;
         detail = BSON( SPT_ERR << outStr ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;

#elif defined (_WINDOWS)
      return SDB_OK ;
#endif
   }

   INT32 _sptUsrFile::chown( const _sptArguments &arg,
                             _sptReturnVal &rval,
                             BSONObj &detail )
   {
#if defined (_LINUX)
      INT32 rc = SDB_OK ;
      UINT32  exitCode    = 0 ;
      BOOLEAN isRecursive = FALSE ;
      string username = "" ;
      string groupname = "" ;
      stringstream cmd ;
      _ossCmdRunner runner ;
      string pathname ;
      BSONObj optionObj ;
      string outStr ;

      // raed arugment
      rc = arg.getString( 0, pathname ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "filename must be config" ) ;
         goto error ;
      }
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "filename must be string" ) ;
         goto error ;
      }

      rc = arg.getBsonobj( 1, optionObj ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "optionObj must be config" ) ;
         goto error ;
      }
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "optionObj must be BSONObj" ) ;
         goto error ;
      }

      if ( FALSE == optionObj.hasField( "username" ) &&
           FALSE == optionObj.hasField( "groupname" ) )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "username or groupname must be config" ) ;
         goto error ;
      }

      if ( TRUE == optionObj.hasField( "username" ) )
      {
         if ( String != optionObj.getField( "username" ).type() )
         {
            rc = SDB_INVALIDARG ;
            detail = BSON( SPT_ERR << "username must be string" ) ;
            goto error ;
         }
         username = optionObj.getStringField( "username" ) ;
      }

      if ( TRUE == optionObj.hasField( "groupname" ) )
      {
         if ( String != optionObj.getField( "groupname" ).type() )
         {
            rc = SDB_INVALIDARG ;
            detail = BSON( SPT_ERR << "groupname must be string" ) ;
            goto error ;
         }
         groupname = optionObj.getStringField( "groupname" ) ;
      }

      if ( 3 <= arg.argc() )
      {
         rc = arg.getNative( 2, &isRecursive, SPT_NATIVE_INT32 ) ;
         if ( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "recursive must be bool" ) ;
            goto error ;
         }
      }

      // build cmd
      cmd << "chown" ;
      if ( TRUE == isRecursive )
      {
         cmd << " -R" ;
      }
      cmd << " " << username << ":" << groupname << " " << pathname ;

      // run cmd
      rc = runner.exec( cmd.str().c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         stringstream ss ;
         ss << "failed to exec cmd " << cmd.str() << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }

      // read result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to read result" ) ;
         goto error ;
      }
      else if ( SDB_OK != exitCode )
      {
         rc = exitCode ;
         detail = BSON( SPT_ERR << outStr ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;

#elif defined (_WINDOWS)
      return SDB_OK ;
#endif
   }

   INT32 _sptUsrFile::chgrp( const _sptArguments &arg,
                             _sptReturnVal &rval,
                             BSONObj &detail )
   {
#if defined (_LINUX)
      INT32 rc = SDB_OK ;
      UINT32 exitCode    = 0 ;
      BOOLEAN isRecursive = FALSE ;
      stringstream cmd ;
      _ossCmdRunner runner ;
      string pathname ;
      string groupname ;
      string outStr ;

      // get argument
      rc = arg.getString( 0, pathname ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "filename must be config" ) ;
         goto error ;
      }
      else if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "filename must be string" ) ;
         goto error ;
      }

      rc = arg.getString( 1, groupname ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "groupname must be config" ) ;
         goto error ;
      }
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "groupname must be string" ) ;
         goto error ;
      }

      if ( 3 <= arg.argc() )
      {
         rc = arg.getNative( 2, &isRecursive, SPT_NATIVE_INT32 ) ;
         if ( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "recursive must be bool" ) ;
            goto error ;
         }
      }

      // build cmd
      cmd << "chgrp" ;
      if ( TRUE == isRecursive )
      {
         cmd << " -R" ;
      }

      cmd << " " << groupname << " " << pathname ;

      // run cmd
      rc = runner.exec( cmd.str().c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         stringstream ss ;
         ss << "failed to exec cmd " << cmd.str() << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }

      // read result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to read result" ) ;
         goto error ;
      }
      else if ( SDB_OK != exitCode )
      {
         rc = exitCode ;
         detail = BSON( SPT_ERR << outStr ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;

#elif defined (_WINDOWS)
      return SDB_OK ;
#endif
   }

   INT32 _sptUsrFile::getUmask( const _sptArguments &arg,
                                _sptReturnVal &rval,
                                BSONObj &detail )
   {
#if defined(_LINUX)
      INT32              rc = SDB_OK ;
      UINT32             exitCode = 0 ;
      string             outStr ;
      string cmd = "umask" ;
      _ossCmdRunner runner ;

      // run cmd
      rc = runner.exec( cmd.c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         stringstream ss ;
         ss << "failed to exec cmd " << cmd << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }

      // get result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to read msg from cmd runner:%d", rc ) ;
         stringstream ss ;
         ss << "failed to read msg from cmd \"" << cmd << "\", rc:"
            << rc ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }

      if( '\n' == outStr[ outStr.size() - 1 ] )
      {
         outStr.erase( outStr.size()-1, 1 ) ;
      }
      rval.getReturnVal().setValue( outStr ) ;
   done:
      return rc ;
   error:
      goto done ;

#elif defined (_WINDOWS)
      rval.getReturnVal().setValue( "" ) ;
      return SDB_OK ;
#endif
   }

   INT32 _sptUsrFile::setUmask( const _sptArguments &arg,
                                _sptReturnVal &rval,
                                BSONObj &detail )
   {
#if defined(_LINUX)
      INT32              rc = SDB_OK ;
      INT32              mask ;
      stringstream       cmd ;
      _ossCmdRunner      runner ;
      string             outStr ;
      INT32              userMask ;

      // get argument
      rc = arg.getNative( 0, (void*)&mask, SPT_NATIVE_INT32 ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "umask must be config" ) ;
         goto error ;
      }
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "umask must be INT32" ) ;
         goto error ;
      }

      userMask = 0 ;
      if ( mask & 0x0001 )
      {
         userMask |= S_IXOTH ;
      }
      if ( mask & 0x0002 )
      {
         userMask |= S_IWOTH ;
      }
      if ( mask & 0x0004 )
      {
         userMask |= S_IROTH ;
      }
      if ( mask & 0x0008 )
      {
         userMask |= S_IXGRP ;
      }
      if ( mask & 0x0010 )
      {
         userMask |= S_IWGRP ;
      }
      if ( mask & 0x0020 )
      {
         userMask |= S_IRGRP ;
      }
      if ( mask & 0x0040 )
      {
         userMask |= S_IXUSR ;
      }
      if ( mask & 0x0080 )
      {
         userMask |= S_IWUSR ;
      }
      if ( mask & 0x0100 )
      {
         userMask |= S_IRUSR ;
      }

      // set mask
      umask( userMask ) ;
   done:
      return rc ;
   error:
      goto done ;

#elif defined (_WINDOWS)
      return SDB_OK ;
#endif
   }

   INT32 _sptUsrFile::getPathType( const _sptArguments &arg,
                                   _sptReturnVal &rval,
                                   BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string pathname ;
      string pathType ;
      CHAR   realPath[ OSS_MAX_PATHSIZE + 1] = { '\0' } ;
      SDB_OSS_FILETYPE type ;

      // get pathname
      rc = arg.getString( 0, pathname ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "pathname must be config" ) ;
         goto error ;
      }
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "pathname must be string" ) ;
         goto error ;
      }

      // build real path
      if ( NULL == ossGetRealPath( pathname.c_str(),
                                   realPath,
                                   OSS_MAX_PATHSIZE + 1 ) )
      {
         rc = SDB_SYS ;
         detail = BSON( SPT_ERR << "Failed to build real path" ) ;
         goto error ;
      }

      // get path type
      rc = ossGetPathType( realPath, &type ) ;
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to get path type" ) ;
         goto error ;
      }

      switch( type )
      {
         case SDB_OSS_FIL:
            pathType = "FIL" ;
            break ;
         case SDB_OSS_DIR:
            pathType = "DIR" ;
            break ;
         case SDB_OSS_SYM:
            pathType = "SYM" ;
            break ;
         case SDB_OSS_DEV:
            pathType = "DEV" ;
            break ;
         case SDB_OSS_PIP:
            pathType = "PIP" ;
            break ;
         case SDB_OSS_SCK:
            pathType = "SCK" ;
            break ;
         default:
            pathType = "UNK" ;
            break ;
      }

      rval.getReturnVal().setValue( pathType ) ;
   done:
      return rc ;
   error:
      goto done ;
   }


   INT32 _sptUsrFile::isEmptyDir( const _sptArguments &arg,
                                  _sptReturnVal &rval,
                                  BSONObj &detail )
   {
      INT32    rc      = SDB_OK ;
      BOOLEAN  isEmpty = FALSE ;
      UINT32             exitCode = 0 ;
      string             pathname ;
      SDB_OSS_FILETYPE   type ;
       stringstream      cmd ;
      _ossCmdRunner      runner ;
      string             outStr ;

      // get pathname
      rc = arg.getString( 0, pathname ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "pathname must be config" ) ;
         goto error ;
      }
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "pathname must be string" ) ;
         goto error ;
      }

      rc = ossAccess( pathname.c_str() ) ;
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "pathname not exist" ) ;
         goto error ;
      }

      // check path type
      rc = ossGetPathType( pathname.c_str(), &type ) ;
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to get path type" ) ;
         goto error ;
      }
      if ( SDB_OSS_DIR != type )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "pathname must be dir" ) ;
         goto error ;
      }

      // list file and get word count
#if defined (_LINUX)
      cmd << "ls -A " << pathname << " | wc -w" ;
#elif defined (_WINDOWS)
      cmd << "cmd /C dir /b " << pathname ;
#endif
      rc = runner.exec( cmd.str().c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         stringstream ss ;
         ss << "failed to exec cmd " << cmd.str() << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }

      // read result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to read msg from cmd runner:%d", rc ) ;
         stringstream ss ;
         ss << "failed to read msg from cmd \"" << cmd.str() << "\", rc:"
            << rc ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }
      if( !outStr.empty() && outStr[ outStr.size() - 1 ] == '\n' )
      {
         outStr.erase( outStr.size()-1, 1 ) ;
      }
#if defined (_LINUX)
      // count will be '0' if list is empty
      if ( "0" == outStr )
      {
         isEmpty = TRUE ;
      }
#elif defined (_WINDOWS)
      // strOut will be empty if list is empty
      if ( outStr.empty() )
      {
         isEmpty = TRUE ;
      }
#endif
      rval.getReturnVal().setValue( isEmpty ? true : false ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

#if defined (_LINUX)
   INT32 _sptUsrFile::getStat( const _sptArguments &arg,
                               _sptReturnVal &rval,
                               BSONObj &detail )
   {
      INT32              rc = SDB_OK ;
      UINT32             exitCode = 0 ;
      string             pathname ;
      stringstream       cmd ;
      _ossCmdRunner      runner ;
      string             outStr ;
      vector<string>     splited ;
      BSONObjBuilder     builder ;
      string             fileType ;

      // get argument
      rc = arg.getString( 0, pathname ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "filename must be config" ) ;
         goto error ;
      }
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "filename must be string" ) ;
         goto error ;
      }

      // set result format
      cmd << "stat -c\"%n|%s|%U|%G|%x|%y|%z|%A\" " << pathname ;

      // run command
      rc = runner.exec( cmd.str().c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         stringstream ss ;
         ss << "failed to exec cmd " << cmd.str() << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }

      // get result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to read msg from cmd runner:%d", rc ) ;
         stringstream ss ;
         ss << "failed to read msg from cmd \"" << cmd.str() << "\", rc:"
            << rc ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }
      else if ( SDB_OK != exitCode )
      {
         rc = exitCode ;
         detail = BSON( SPT_ERR << outStr ) ;
         goto error ;
      }

      /* extract result
      format: xxx|xxx|xxx|xxx|xxx|xxx|xxx|xxx
      explain: separate by '|'
      col[0]: filename   e.g: /home/users/wjm/trunk/bin
      col[1]: size       e.g: 4096
      col[2]: usre name  e.g: root
      col[3]: group name  e.g: root
      col[4]: time of last access  e.g:2016-10-11 15:27:48.839198876 +0800
      col[5]: time of last modification  e.g:2016-10-11 15:27:48.839198876 +0800
      col[6]: time of last change e.g:2016-10-11 15:27:48.839198876 +0800
      col[7]: access rights in human readable form   e.g: drwxrwxrwx
      */
      try
      {
         boost::algorithm::split( splited, outStr, boost::is_any_of( "\r\n" ) ) ;
      }
      catch( std::exception )
      {
         rc = SDB_SYS ;
         detail = BSON( SPT_ERR << "Failed to split result" ) ;
         PD_LOG( PDERROR, "Failed to split result" ) ;
         goto error ;
      }
      for( vector<string>::iterator itr = splited.begin();
           itr != splited.end();  )
      {
         if ( itr->empty() )
         {
            itr = splited.erase( itr ) ;
         }
         else
         {
            itr++ ;
         }
      }

      for( vector<string>::iterator itr = splited.begin();
           itr != splited.end(); itr++ )
      {
         vector<string> token ;
         try
         {
            boost::algorithm::split( token, *itr, boost::is_any_of( "|" ) ) ;
         }
         catch( std::exception )
         {
            rc = SDB_SYS ;
            detail = BSON( SPT_ERR << "Failed to split result" ) ;
            PD_LOG( PDERROR, "Failed to split result" ) ;
            goto error ;
         }
         for( vector<string>::iterator itr = token.begin();
              itr != token.end(); )
         {
            if ( itr->empty() )
            {
               itr = token.erase( itr ) ;
            }
            else
            {
               itr++ ;
            }
         }
         // ignore useless info
         if( 8 != token.size() )
         {
            continue ;
         }

         // get file type
         switch( token[ 7 ][ 0 ] )
         {
            case '-':
               fileType = "regular file" ;
               break ;
            case 'd':
               fileType = "directory" ;
               break ;
            case 'c':
               fileType = "character special file" ;
               break ;
            case 'b':
               fileType = "block special file" ;
               break ;
            case 'l':
               fileType = "symbolic link" ;
               break ;
            case 's':
               fileType = "socket" ;
               break ;
            case 'p':
               fileType = "pipe" ;
               break ;
            default:
               fileType = "unknow" ;
         }
         builder.append( "name", token[ 0 ] ) ;
         builder.append( "size", token[ 1 ] ) ;
         builder.append( "mode", token[ 7 ].substr( 1 ) ) ;
         builder.append( "user", token[ 2 ] ) ;
         builder.append( "group", token[ 3 ] ) ;
         builder.append( "accessTime", token[ 4 ] ) ;
         builder.append( "modifyTime", token[ 5 ] ) ;
         builder.append( "changeTime", token[ 6 ] ) ;
         builder.append( "type", fileType ) ;
         rval.getReturnVal().setValue( builder.obj() ) ;
         goto done ;
      }

      rc = SDB_SYS ;
      detail = BSON( SPT_ERR << "Failed to build result" ) ;
      goto error ;

   done:
      return rc ;
   error:
      goto done ;
   }

#elif defined (_WINDOWS)
   INT32 _sptUsrFile::getStat( const _sptArguments &arg,
                               _sptReturnVal &rval,
                               BSONObj &detail )
   {
      INT32              rc = SDB_OK ;
      string             pathname ;
      string             fileType ;
      SDB_OSS_FILETYPE   ossFileType ;
      CHAR               realPath[ OSS_MAX_PATHSIZE + 1 ] = { '\0' } ;
      BSONObjBuilder     builder ;
      INT64              fileSize ;
      stringstream       fileSizeStr ;

      // get argument
      rc = arg.getString( 0, pathname ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "filename must be config" ) ;
         goto error ;
      }
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "filename must be string" ) ;
         goto error ;
      }

      if ( NULL == ossGetRealPath( pathname.c_str(),
                                   realPath,
                                   OSS_MAX_PATHSIZE + 1 ) )
      {
         rc = SDB_SYS ;
         detail = BSON( SPT_ERR << "Failed to build real path" ) ;
         goto error ;
      }

      // get file type
      rc = ossGetPathType( realPath, &ossFileType ) ;
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to get file type" ) ;
         goto error ;
      }
      switch( ossFileType )
      {
         case SDB_OSS_FIL:
            fileType = "regular file" ;
            break ;
         case SDB_OSS_DIR:
            fileType = "directory" ;
            break ;
         default:
            fileType = "unknow" ;
      }

      // get file size
      rc = ossGetFileSizeByName( realPath, &fileSize ) ;
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to get file size" ) ;
         goto error ;
      }
      fileSizeStr << fileSize ;

      builder.append( "name", pathname ) ;
      builder.append( "size", fileSizeStr.str() ) ;
      builder.append( "mode", "" ) ;
      builder.append( "user", "" ) ;
      builder.append( "group", "" ) ;
      builder.append( "accessTime", "" ) ;
      builder.append( "modifyTime", "" ) ;
      builder.append( "changeTime", "" ) ;
      builder.append( "type", fileType ) ;
      rval.getReturnVal().setValue( builder.obj() ) ;
   done:
      return rc ;
   error:
      goto done ;
   }
#endif
   INT32 _sptUsrFile::md5( const _sptArguments &arg,
                           _sptReturnVal &rval,
                           BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      SINT64 bufSize = SPT_MD5_READ_LEN ;
      SINT64 hasRead = 0 ;
      CHAR readBuf[ SPT_MD5_READ_LEN + 1 ] = { 0 } ;
      OSSFILE file ;
      string filename ;
      stringstream ss ;
      BOOLEAN isOpen = FALSE ;
      md5_state_t st ;
      md5_init( &st ) ;
      md5::md5digest digest ;
      string code ;

      // check, we need 1 argument filename
      if ( 0 == arg.argc() )
      {
         rc = SDB_OUT_OF_BOUND ;
         ss << "filename must be config" ;
         goto error ;
      }
      rc = arg.getString( 0,  filename ) ;
      if ( SDB_OK != rc )
      {
         rc = SDB_INVALIDARG ;
         ss << "filename must be string" ;
         goto error ;
      }

      // open the file
      rc = ossOpen( filename.c_str(), OSS_READONLY | OSS_SHAREREAD,
                    OSS_DEFAULTFILE, file ) ;
      if ( rc )
      {
         ss << "open file[" << filename.c_str() << "] failed: " << rc ;
         goto error ;
      }
      isOpen = TRUE ;

      // read file and get md5
      while ( TRUE )
      {
         rc = ossReadN( &file, bufSize, readBuf, hasRead ) ;
         if ( SDB_EOF == rc || 0 == hasRead )
         {
            rc = SDB_OK ;
            break ;
         }
         else if ( rc )
         {
            ss << "Read file[" << filename.c_str() << "] failed, rc: " << rc ;
            goto error ;
         }
         md5_append( &st, (const md5_byte_t *)readBuf, hasRead ) ;
      }
      md5_finish( &st, digest ) ;
      code = md5::digestToString( digest ) ;
      rval.getReturnVal().setValue( code ) ;
   done:
      if ( TRUE == isOpen )
         ossClose( file ) ;
      return rc ;
   error:
      detail = BSON( SPT_ERR << ss.str() ) ;
      goto done ;
   }

   INT32 _sptUsrFile::staticHelp( const _sptArguments & arg,
                                  _sptReturnVal & rval,
                                  BSONObj & detail )
   {
      stringstream ss ;
      ss << "Methods to access:" << endl
         << " var file = new File( filename, [mode] )" << endl
         << " var file = remoteObj.getFile()" << endl
         << " var file = remoteObj.getFile( filename, [mode] )" << endl
         << "File functions:" << endl
         << "   read( [size] )" << endl
         << "   write( content )" << endl
         << "   seek( offset, [where] ) " << endl
         << "   close()" << endl
         << " File.remove( filepath )" << endl
         << " File.exist( filepath )" << endl
         << " File.copy( src, dst, [replace], [mode] )" << endl
         << " File.move( src, dst )" << endl
         << " File.mkdir( name, [mode] )" << endl
         << " File.find( optionObj, [filterObj] )" << endl
#if defined (_LINUX)
         << " File.chmod( filename, mode, [recursive] )" << endl
         << " File.chown( filename, optionObj, [recursive] )" << endl
         << " File.chgrp( filename, groupname, [recursive] )" << endl
         << " File.setUmask( umask )" << endl
         << " File.getUmask( base )" << endl
#endif
         << " File.list( [optionObj], [filterObj] )" << endl
         << " File.isFile( pathname )" << endl
         << " File.isDir( pathname )" << endl
         << " File.isEmptyDir( dirName )" << endl
         << " File.stat( filename )" << endl
         << " File.md5( filename )" << endl ;
      rval.getReturnVal().setValue( ss.str() ) ;
      return SDB_OK ;
   }

}

