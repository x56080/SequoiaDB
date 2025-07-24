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

   Source File Name = omagentRemoteUsrFile.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/03/2016  WJM Initial Draft

   Last Changed =

*******************************************************************************/
#include "omagentRemoteUsrFile.hpp"
#include "omagentMgr.hpp"
#include "omagentDef.hpp"
#include "ossCmdRunner.hpp"
#include "ossPrimitiveFileOp.hpp"
#include <boost/algorithm/string.hpp>
#include "../bson/lib/md5.hpp"
#if defined(_LINUX)
#include <sys/stat.h>
#endif
using namespace bson ;
#define SPT_READ_LEN 1024
#define SPT_MD5_READ_LEN 1024

namespace engine
{
   /*
      _remoteFileOpen implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteFileOpen )

   _remoteFileOpen::_remoteFileOpen()
   {
   }

   _remoteFileOpen::~_remoteFileOpen()
   {
   }

   const CHAR* _remoteFileOpen::name()
   {
      return OMA_REMOTE_FILE_OPEN ;
   }

   INT32 _remoteFileOpen::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      UINT32 permission = OSS_RWXU ;
      string filename ;
      INT32 mode ;
      OSSFILE file ;

      // get argument
      if ( FALSE == _valueObj.hasField( "filename" ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "filename must be config" ) ;
         goto error ;
      }
      if ( String != _valueObj.getField( "filename" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "filename must be string" ) ;
         goto error ;
      }
      filename = _valueObj.getStringField( "filename" ) ;

      if ( TRUE == _optionObj.hasField( "mode" ) )
      {
         if( NumberInt != _optionObj.getField( "mode" ).type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "mode must be INT32" );
            goto error ;
         }
         mode = _optionObj.getIntField( "mode" ) ;

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
      rc = ossOpen( filename.c_str(),
                    OSS_READWRITE | OSS_CREATE,
                    permission,
                    file ) ;

      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "failed to open file:%s, rc:%d",
                     filename.c_str(), rc ) ;
         goto error ;
      }
   done:
      if ( file.isOpened() )
      {
         ossClose( file ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteFileRead implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteFileRead )

   _remoteFileRead::_remoteFileRead()
   {
   }

   _remoteFileRead::~_remoteFileRead()
   {
   }

   INT32 _remoteFileRead::init( const CHAR* pInfomation )
   {
      INT32 rc = SDB_OK ;

      rc = _remoteExec::init( pInfomation ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get argument, rc: %d", rc ) ;

      if ( FALSE == _valueObj.hasField( "filename" ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "filename must be config" ) ;
         goto error;
      }
      if ( String != _valueObj.getField( "filename" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "filename must be string" ) ;
         goto error ;
      }
      _filename = _valueObj.getStringField( "filename" ) ;

      if ( FALSE == _valueObj.hasField( "location" ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "location must be config" ) ;
         goto error ;
      }
      if ( NumberInt != _valueObj.getField( "location" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "location must be int" ) ;
         goto error ;
      }
      _location = _valueObj.getIntField( "location" ) ;


      if ( FALSE == _valueObj.hasField( "size" ) )
      {
         _size = SPT_READ_LEN ;
      }
      else if ( NumberInt != _valueObj.getField( "size" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "size must be int" ) ;
         goto error ;
      }
      else
      {
         _size = _valueObj.getIntField( "size" ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   const CHAR* _remoteFileRead::name()
   {
      return OMA_REMOTE_FILE_READ ;
   }

   INT32 _remoteFileRead::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      OSSFILE file ;
      CHAR stackBuf[SPT_READ_LEN + 1] = { 0 } ;
      CHAR *buf = NULL ;
      SINT64 read = 0 ;

      // open file
      rc = ossOpen( _filename.c_str(),
                    OSS_READWRITE | OSS_CREATE,
                    OSS_RWXU,
                    file ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "failed to open file:%s, rc:%d",
                     _filename.c_str(), rc ) ;
         goto error ;
      }

      rc = ossSeek( &file, _location, OSS_SEEK_SET ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to seek:%d", rc ) ;
         goto error ;
      }

      // use static array or alloc buffer
      if ( SPT_READ_LEN < _size )
      {
         buf = ( CHAR * )SDB_OSS_MALLOC( _size + 1 ) ;
         if ( NULL == buf )
         {
            PD_LOG_MSG( PDERROR, "failed to allocate mem." ) ;
            rc = SDB_OOM ;
            goto error ;
         }
      }
      else
      {
         buf = ( CHAR * )stackBuf ;
      }

      // read content
      rc = ossReadN( &file, _size, buf, read ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to read file:%d", rc ) ;
         goto error ;
      }
      buf[read] = '\0' ;

      retObj = BSON( "readStr" << buf ) ;
   done:
      if ( SPT_READ_LEN < _size && NULL != buf )
      {
         SDB_OSS_FREE( buf ) ;
      }

      if ( file.isOpened() )
      {
         ossClose( file ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteFileWrite implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteFileWrite )

   _remoteFileWrite::_remoteFileWrite()
   {
   }

   _remoteFileWrite::~_remoteFileWrite()
   {
   }

   INT32 _remoteFileWrite::init( const CHAR * pInfomation )
   {
      INT32 rc = SDB_OK ;

      rc = _remoteExec::init( pInfomation ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get argument, rc: %d", rc ) ;

      if ( FALSE == _valueObj.hasField( "filename" ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "filename must be config" ) ;
         goto error;
      }
      if ( String != _valueObj.getField( "filename" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "filename must be string" ) ;
         goto error ;
      }
      _filename = _valueObj.getStringField( "filename" ) ;

      if ( FALSE == _valueObj.hasField( "location" ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "location must be config" ) ;
         goto error ;
      }
      if ( NumberInt != _valueObj.getField( "location" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "location must be int" ) ;
         goto error ;
      }
      _location = _valueObj.getIntField( "location" ) ;
      _content = _valueObj.getStringField( "content" ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   const CHAR* _remoteFileWrite::name()
   {
      return OMA_REMOTE_FILE_WRITE ;
   }

   INT32 _remoteFileWrite::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      OSSFILE file ;

      // open file
      rc = ossOpen( _filename.c_str(),
                    OSS_READWRITE | OSS_CREATE,
                    OSS_RWXU,
                    file ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "failed to open file:%s, rc:%d",
                     _filename.c_str(), rc ) ;
         goto error ;
      }

      rc = ossSeek( &file, _location, OSS_SEEK_SET ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to seek:%d", rc ) ;
         goto error ;
      }

      // write content
      rc = ossWriteN( &file, _content.c_str(), _content.size() ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to write to file:%d", rc ) ;
         goto error ;
      }
   done:
      if ( file.isOpened() )
      {
         ossClose( file ) ;
      }

      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteFileRemove implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteFileRemove )

   _remoteFileRemove::_remoteFileRemove()
   {
   }

   _remoteFileRemove::~_remoteFileRemove()
   {
   }

   const CHAR* _remoteFileRemove::name()
   {
      return OMA_REMOTE_FILE_REMOVE ;
   }

   INT32 _remoteFileRemove::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string filepath ;

      // get pathname
      if ( FALSE == _valueObj.hasField( "filepath" ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "filepath must be config" ) ;
         goto error ;
      }
      if ( String != _valueObj.getField( "filepath" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "filepath must be string" ) ;
         goto error ;
      }
      filepath = _valueObj.getStringField( "filepath" ) ;

      // delete file
      rc = ossDelete( filepath.c_str() ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "failed to remove file:%s, rc:%d",
                 filepath.c_str(), rc ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteFileExist implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteFileExist )

   _remoteFileExist::_remoteFileExist()
   {
   }

   _remoteFileExist::~_remoteFileExist()
   {
   }

   const CHAR* _remoteFileExist::name()
   {
      return OMA_REMOTE_FILE_ISEXIST ;
   }

   INT32 _remoteFileExist::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string filepath ;
      BSONObjBuilder builder ;
      BOOLEAN fileExist = FALSE ;

      if ( FALSE == _valueObj.hasField( "filepath" ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "filepath must be config" ) ;
         goto error ;
      }
      if ( String != _valueObj.getField( "filepath" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "filepath must be string" ) ;
         goto error ;
      }
      filepath = _valueObj.getStringField( "filepath" ) ;

      // try to access pathname
      rc = ossAccess( filepath.c_str() ) ;
      if ( SDB_OK != rc && SDB_FNE != rc )
      {
         PD_LOG_MSG( PDERROR, "access file failed" ) ;
         goto error ;
      }
      else if ( SDB_OK == rc )
      {
         fileExist = TRUE ;
      }

      rc = SDB_OK ;
      builder.appendBool( "isExist", fileExist ) ;
      retObj = builder.obj() ;
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteFileCopy implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteFileCopy )

   _remoteFileCopy::_remoteFileCopy()
   {
   }

   _remoteFileCopy::~_remoteFileCopy()
   {
   }

   const CHAR* _remoteFileCopy::name()
   {
      return OMA_REMOTE_FILE_COPY ;
   }

   INT32 _remoteFileCopy::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string src ;
      string dst ;
      BOOLEAN isReplace = TRUE ;
      UINT32 permission = OSS_DEFAULTFILE ;

      // get argument
      if ( FALSE == _matchObj.hasField( "src" ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "src is required" ) ;
         goto error ;
      }
      if ( String != _matchObj.getField( "src" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "src must be string" ) ;
         goto error;
      }
      src = _matchObj.getStringField( "src" ) ;

      if ( FALSE == _valueObj.hasField( "dst" ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "dst is required" ) ;
         goto error ;
      }
      if ( String != _valueObj.getField( "dst" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "dst must be string" ) ;
         goto error;
      }
      dst = _valueObj.getStringField( "dst" ) ;

      if ( TRUE == _optionObj.hasField( "replace" ) )
      {
         if( Bool != _optionObj.getField( "replace" ).type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "replace must be BOOLEAN" ) ;
            goto error ;
         }
         isReplace = _optionObj.getBoolField( "replace" ) ;
      }

      if ( TRUE == _optionObj.hasField( "mode" ) )
      {
         if ( NumberInt != _optionObj.getField( "mode" ).type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "mode must be INT32" ) ;
            goto error ;
         }
         INT32 mode = 0 ;
         permission = 0 ;
         mode = _optionObj.getIntField( "mode" ) ;

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
            PD_LOG_MSG( PDERROR, "Failed to get src file stat" ) ;
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

      // copy file
      rc = ossFileCopy( src.c_str(), dst.c_str(), permission, isReplace ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "copy file failed" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteFileMove implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteFileMove )

   _remoteFileMove::_remoteFileMove()
   {
   }

   _remoteFileMove::~_remoteFileMove()
   {
   }

   const CHAR* _remoteFileMove::name()
   {
      return OMA_REMOTE_FILE_MOVE ;
   }

   INT32 _remoteFileMove::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string src ;
      string dst ;

      // get argument
      if ( FALSE == _matchObj.hasField( "src" ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "src is required" ) ;
         goto error ;
      }
      if ( String != _matchObj.getField( "src" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "src must be string" ) ;
         goto error ;
      }
      src = _matchObj.getStringField( "src" ) ;

      if ( FALSE == _valueObj.hasField( "dst" ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "dst is required" ) ;
         goto error ;
      }
      if ( String != _valueObj.getField( "dst" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "dst must be string" ) ;
         goto error ;
      }
      dst = _valueObj.getStringField( "dst" ) ;

      // rename path
      rc = ossRenamePath( src.c_str(), dst.c_str() ) ;
      if ( rc )
      {
         PD_LOG_MSG( PDERROR, "rename path failed" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteFileMkdir implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteFileMkdir )

   _remoteFileMkdir::_remoteFileMkdir()
   {
   }

   _remoteFileMkdir::~_remoteFileMkdir()
   {
   }

   const CHAR* _remoteFileMkdir::name()
   {
      return OMA_REMOTE_FILE_MKDIR ;
   }

   INT32 _remoteFileMkdir::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      UINT32 permission = OSS_DEFAULTDIR ;
      string name ;

      // get argument
      if ( FALSE == _valueObj.hasField( "name" ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "name is required" ) ;
         goto error ;
      }
      if ( String != _valueObj.getField( "name" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "name must be string" ) ;
         goto error ;
      }
      name = _valueObj.getStringField( "name" ) ;

      if ( TRUE == _optionObj.hasField( "mode" ) )
      {
         if ( NumberInt != _optionObj.getField( "mode" ).type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "mode must be INT32" ) ;
            goto error ;
         }
         INT32 mode = 0 ;
         mode = _optionObj.getIntField( "mode" ) ;
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

      // make dir
      rc = ossMkdir( name.c_str(), permission ) ;
      if ( SDB_FE == rc )
      {
         rc = SDB_OK ;
      }
      else if ( rc )
      {
         PD_LOG_MSG( PDERROR, "create dir failed" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteFileFind implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteFileFind )

   _remoteFileFind::_remoteFileFind()
   {
   }

   _remoteFileFind::~_remoteFileFind()
   {
   }

   const CHAR* _remoteFileFind::name()
   {
      return OMA_REMOTE_FILE_FIND ;
   }

   INT32 _remoteFileFind::doit( BSONObj &retObj )
   {
      INT32              rc = SDB_OK ;
      string             findType = "n" ;
      string             mode ;
      UINT32             exitCode = 0 ;
      string             rootDir ;
      _ossCmdRunner      runner ;
      string             value ;
      string             pathname ;
      string             outStr ;
      BSONObjBuilder     builder ;
      stringstream       cmd ;

      // get argument
      if ( TRUE == _optionObj.isEmpty() )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "optionObj must be config") ;
         goto error ;
      }
      if ( TRUE == _optionObj.hasField( "value" ) )
      {
         if ( String != _optionObj.getField( "value" ).type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "value must be string" ) ;
            goto error ;
         }
         value = _optionObj.getStringField( "value" ) ;
      }

      if ( TRUE == _optionObj.hasField( "mode" ) )
      {
         if ( String != _optionObj.getField( "mode" ).type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "mode must be string" ) ;
            goto error ;
         }
         findType = _optionObj.getStringField( "mode" ) ;
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
            PD_LOG_MSG( PDERROR, "value shouldn't contain '/'" ) ;
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
         PD_LOG_MSG( PDERROR, "mode must be required type" ) ;
         goto error ;
      }

      if ( TRUE == _optionObj.hasField( "pathname" ) )
      {
         if ( String != _optionObj.getField( "pathname" ).type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "pathname must be string" ) ;
            goto error ;
         }
         pathname = _optionObj.getStringField( "pathname" ) ;
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
         stringstream ss ;
         ss << "failed to exec cmd " << cmd.str() << ",rc: "
            << rc
            << ",exit: "
            << exitCode ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }

      // get result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         stringstream ss ;
         ss << "failed to read msg from cmd \"" << cmd.str() << "\", rc:"
            << rc ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }
      else if ( SDB_OK != exitCode )
      {
         if ( '\n' == outStr[ outStr.size()-1 ] )
         {
            outStr.erase( outStr.size()-1, 1 ) ;
         }
         rc = exitCode ;
         PD_LOG_MSG( PDERROR, outStr.c_str() ) ;
         goto error ;
      }

      // extract result
      rc = _extractFindInfo( outStr.c_str(), builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "Failed to extract find info" ) ;
         goto error ;
      }

      retObj = builder.obj() ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _remoteFileFind::_extractFindInfo( const CHAR* buf,
                                        BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      vector<string> splited ;

      /*
      content format:
         xx/xxx/xxx1
         xx/xxx/xxx2
         xx/xxx/xxx3
      */
      try
      {
         boost::algorithm::split( splited, buf, boost::is_any_of("\r\n") ) ;
      }
      catch( std::exception e )
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

      for( UINT32 index = 0; index != splited.size(); index++ )
      {
         BSONObjBuilder objBuilder ;
         objBuilder.append( "pathname", splited[index] ) ;
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

   /*
      _remoteFileChmod implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteFileChmod )

   _remoteFileChmod::_remoteFileChmod()
   {
   }

   _remoteFileChmod::~_remoteFileChmod()
   {
   }

   const CHAR* _remoteFileChmod::name()
   {
      return OMA_REMOTE_FILE_CHMOD ;
   }

   INT32 _remoteFileChmod::doit( BSONObj &retObj )
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

      // get argument
      if ( FALSE == _matchObj.hasField( "pathname" ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "filename must be config" ) ;
         goto error ;
      }
      if ( String != _matchObj.getField( "pathname" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "filename must be string" ) ;
         goto error ;
      }
      pathname = _matchObj.getStringField( "pathname" ) ;

      if ( FALSE == _valueObj.hasField( "mode" ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "mode must be config" ) ;
         goto error ;
      }
      if ( NumberInt != _valueObj.getField( "mode" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "mode must be INT32" ) ;
         goto error ;
      }
      mode = _valueObj.getIntField( "mode" ) ;

      if ( TRUE == _optionObj.hasField( "recursive" ) )
      {
         if ( Bool != _optionObj.getField( "recursive" ).type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "recursive must be bool" ) ;
            goto error ;
         }
         isRecursive = _optionObj.getBoolField( "recursive" ) ;
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
         stringstream ss ;
         ss << "failed to exec cmd " << cmd.str() << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }

      // read result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "Failed to read result" ) ;
         goto error ;
      }
      else if ( SDB_OK != exitCode )
      {
         rc = exitCode ;
         PD_LOG_MSG( PDERROR, outStr.c_str() ) ;
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

   /*
      _remoteFileChown implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteFileChown )

   _remoteFileChown::_remoteFileChown()
   {
   }

   _remoteFileChown::~_remoteFileChown()
   {
   }

   const CHAR* _remoteFileChown::name()
   {
      return OMA_REMOTE_FILE_CHOWN ;
   }

   INT32 _remoteFileChown::doit( BSONObj &retObj )
   {
#if defined (_LINUX)
      INT32 rc = SDB_OK ;
      UINT32 exitCode    = 0 ;
      BOOLEAN isRecursive = FALSE ;
      string username = "" ;
      string groupname = "" ;
      stringstream       cmd ;
      _ossCmdRunner      runner ;
      string             pathname ;
      BSONObj            optionObj ;
      string             outStr ;

      // get argument
      if ( FALSE == _matchObj.hasField( "filename" ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "filename must be config" ) ;
         goto error ;
      }
      if ( String != _matchObj.getField( "filename" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "filename must be string" ) ;
         goto error ;
      }
      pathname = _matchObj.getStringField( "filename" ) ;

      if ( TRUE == _valueObj.isEmpty() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "optionObj must be config" ) ;
         goto error ;
      }
      if ( FALSE == _valueObj.hasField( "username" ) &&
           FALSE == _valueObj.hasField( "groupname" ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "username or groupname must be config" ) ;
         goto error ;
      }
      if ( TRUE == _valueObj.hasField( "username" ) )
      {
         if ( String != _valueObj.getField( "username" ).type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "username must be string" ) ;
            goto error ;
         }
         username = _valueObj.getStringField( "username" ) ;
      }
      if ( TRUE == _valueObj.hasField( "groupname" ) )
      {
         if ( String != _valueObj.getField( "groupname" ).type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "groupname must be string" ) ;
            goto error ;
         }
         groupname = _valueObj.getStringField( "groupname" ) ;
      }

      if ( TRUE == _optionObj.hasField( "recursive" ) )
      {
         if ( Bool != _optionObj.getField( "recursive" ).type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "recursive must be bool" ) ;
            goto error ;
         }
         isRecursive = _optionObj.getBoolField( "recursive" ) ;
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
         stringstream ss ;
         ss << "failed to exec cmd " << cmd.str() << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }

      // read result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "Failed to read result" ) ;
         goto error ;
      }
      else if ( SDB_OK != exitCode )
      {
         rc = exitCode ;
         PD_LOG_MSG( PDERROR, outStr.c_str() ) ;
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

   /*
      _remoteFileChgrp implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteFileChgrp )

   _remoteFileChgrp::_remoteFileChgrp()
   {
   }

   _remoteFileChgrp::~_remoteFileChgrp()
   {
   }

   const CHAR* _remoteFileChgrp::name()
   {
      return OMA_REMOTE_FILE_CHGRP ;
   }

   INT32 _remoteFileChgrp::doit( BSONObj &retObj )
   {
#if defined (_LINUX)
      INT32 rc = SDB_OK ;
      UINT32 exitCode    = 0 ;
      BOOLEAN isRecursive = FALSE ;
      stringstream       cmd ;
      _ossCmdRunner      runner ;
      string             pathname ;
      string             groupname ;
      string             outStr ;

      // get argument
      if ( FALSE == _matchObj.hasField( "filename" ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "filename must be config" ) ;
         goto error ;
      }
      if ( String != _matchObj.getField( "filename" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "filename must be string" ) ;
         goto error ;
      }
      pathname = _matchObj.getStringField( "filename" ) ;

      if ( FALSE == _valueObj.hasField( "groupname" ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "groupname must be config" ) ;
         goto error ;
      }
      if ( String != _valueObj.getField( "groupname" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "groupname must be string" ) ;
         goto error ;
      }
      groupname = _valueObj.getStringField( "groupname" ) ;

      if ( TRUE == _optionObj.hasField( "recursive" ) )
      {
         if ( Bool != _optionObj.getField( "recursive" ).type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "recursive must be bool" ) ;
            goto error ;
         }
         isRecursive = _optionObj.getBoolField( "recursive" ) ;
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
         stringstream ss ;
         ss << "failed to exec cmd " << cmd.str() << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }

      // get result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "Failed to read result" ) ;
         goto error ;
      }
      else if ( SDB_OK != exitCode )
      {
         rc = exitCode ;
         PD_LOG_MSG( PDERROR, outStr.c_str() ) ;
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

   /*
      _remoteFileGetUmask implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteFileGetUmask )

   _remoteFileGetUmask::_remoteFileGetUmask()
   {

   }

   _remoteFileGetUmask::~_remoteFileGetUmask()
   {
   }

   const CHAR* _remoteFileGetUmask::name()
   {
      return OMA_REMOTE_FILE_GET_UMASK ;
   }

   INT32 _remoteFileGetUmask::doit( BSONObj &retObj )
   {
#if defined(_LINUX)
      INT32              rc = SDB_OK ;
      UINT32             exitCode = 0 ;
      string             outStr ;
      string  cmd = "umask" ;
      _ossCmdRunner runner ;

      // run cmd
      rc = runner.exec( cmd.c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         stringstream ss ;
         ss << "failed to exec cmd " << cmd << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }

      // get result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         stringstream ss ;
         ss << "failed to read msg from cmd \"" << cmd << "\", rc:"
            << rc ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }

      if( outStr[ outStr.size() - 1 ] == '\n' )
      {
         outStr.erase( outStr.size()-1, 1 ) ;
      }
      retObj = BSON( "mask" << outStr.c_str() ) ;
   done:
      return rc ;
   error:
      goto done ;

#elif defined (_WINDOWS)
      retObj = BSON( "mask" << "" ) ;
      return SDB_OK ;
#endif
   }

   /*
      _remoteFileSetUmask implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteFileSetUmask )

   _remoteFileSetUmask::_remoteFileSetUmask()
   {
   }

   _remoteFileSetUmask::~_remoteFileSetUmask()
   {
   }

   const CHAR* _remoteFileSetUmask::name()
   {
      return OMA_REMOTE_FILE_SET_UMASK ;
   }

   INT32 _remoteFileSetUmask::doit( BSONObj &retObj )
   {
#if defined(_LINUX)
      INT32              rc = SDB_OK ;
      stringstream       cmd ;
      _ossCmdRunner      runner ;
      string             outStr ;
      INT32              userMask ;
      INT32              mask ;

      // get argument
      if ( FALSE == _valueObj.hasField( "mask" ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "mask must be config" ) ;
         goto error ;
      }
      if ( NumberInt != _valueObj.getField( "mask" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "mask must be INT32" ) ;
         goto error ;
      }
      mask = _valueObj.getIntField( "mask" ) ;

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

   /*
      _remoteFileList implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteFileList )

   _remoteFileList::_remoteFileList()
   {
   }

   _remoteFileList::~_remoteFileList()
   {
   }

   const CHAR* _remoteFileList::name()
   {
      return OMA_REMOTE_FILE_LIST ;
   }

   INT32 _remoteFileList::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      UINT32 exitCode    = 0 ;
      BOOLEAN showDetail = FALSE ;
      BSONObjBuilder     builder ;
      stringstream       cmd ;
      _ossCmdRunner      runner ;
      string             outStr ;
      string             pathname ;
      BSONObj            optionObj ;

      // get argument and build cmd
#if defined (_LINUX)
      cmd << "ls -A -l" ;
#elif defined (_WINDOWS)
      cmd << "cmd /C dir /-C /A" ;
#endif
      if ( TRUE == _optionObj.hasField( "pathname" ) )
      {
         if ( String != _optionObj.getField( "pathname" ).type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "pathname must be string" ) ;
            goto error ;
         }
         cmd << " " << _optionObj.getStringField( "pathname" ) ;
      }
      showDetail = optionObj.getBoolField( "detail" ) ;

      // run cmd
      rc = runner.exec( cmd.str().c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         stringstream ss ;
         ss << "failed to exec cmd " << cmd.str() << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }

      // get result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         stringstream ss ;
         ss << "failed to read msg from cmd \"" << cmd.str() << "\", rc:"
            << rc ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }
      else if ( SDB_OK != exitCode )
      {
         rc = exitCode ;
         PD_LOG_MSG( PDERROR, outStr.c_str() ) ;
         goto error ;
      }

      // extract result
      rc = _extractListInfo( outStr.c_str(), builder, showDetail ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "Failed to extract list info" ) ;
         goto error ;
      }

      retObj = builder.obj() ;
   done:
      return rc ;
   error:
      goto done ;
   }

#if defined (_LINUX)
   INT32 _remoteFileList::_extractListInfo( const CHAR* buf,
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
                  columns[8] += " " + columns[index] ;
               }
            }
            fileObjBuilder.append( "name", columns[8] ) ;
            fileObjBuilder.append( "size", columns[4] ) ;
            fileObjBuilder.append( "mode", columns[0] ) ;
            fileObjBuilder.append( "user", columns[2] ) ;
            fileObjBuilder.append( "group", columns[3] ) ;
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
                  columns[8] += " " + columns[index] ;
               }
            }
            fileObjBuilder.append( "name", columns[8] ) ;
            fileObjBuilder.append( "mode", columns[0] ) ;
            fileObjBuilder.append( "user", columns[2] ) ;
            fileVec.push_back( fileObjBuilder.obj() ) ;
         }
      }

      // merge vector< BSONObj> into BSONObj
      for( UINT32 index = 0; index < fileVec.size(); index++ )
      {
         try
         {
            builder.append( boost::lexical_cast<string>( index ).c_str(),
                            fileVec[index] ) ;
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
   INT32 _remoteFileList::_extractListInfo( const CHAR* buf,
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

   /*
      _remoteFileGetPathType implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteFileGetPathType )

   _remoteFileGetPathType::_remoteFileGetPathType()
   {
   }

   _remoteFileGetPathType::~_remoteFileGetPathType()
   {
   }

   const CHAR* _remoteFileGetPathType::name()
   {
      return OMA_REMOTE_FILE_GET_PATH_TYPE ;
   }

   INT32 _remoteFileGetPathType::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string pathname ;
      string pathType ;
      CHAR   realPath[ OSS_MAX_PATHSIZE + 1] = { '\0' } ;
      SDB_OSS_FILETYPE type ;

      // get argument
      if ( FALSE == _matchObj.hasField( "pathname" ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "pathname must be config" ) ;
         goto error ;
      }
      if ( String != _matchObj.getField( "pathname" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "pathname must be string" ) ;
         goto error ;
      }
      pathname = _matchObj.getStringField( "pathname" ) ;

      // build real path
      if ( NULL == ossGetRealPath( pathname.c_str(),
                                   realPath,
                                   OSS_MAX_PATHSIZE + 1 ) )
      {
         rc = SDB_SYS ;
         PD_LOG_MSG( PDERROR, "Failed to build real path" ) ;
         goto error ;
      }

      // get path type
      rc = ossGetPathType( realPath, &type ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "Failed to get path type" ) ;
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

      retObj = BSON( "pathType" << pathType ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteFileIsEmptyDir implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteFileIsEmptyDir )

   _remoteFileIsEmptyDir::_remoteFileIsEmptyDir()
   {
   }

   _remoteFileIsEmptyDir::~_remoteFileIsEmptyDir()
   {
   }

   const CHAR* _remoteFileIsEmptyDir::name()
   {
      return OMA_REMOTE_FILE_IS_EMPTYDIR ;
   }

   INT32 _remoteFileIsEmptyDir::doit( BSONObj &retObj )
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
      if ( FALSE == _matchObj.hasField( "pathname" ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "pathname must be config" ) ;
         goto error ;
      }
      if ( String != _matchObj.getField( "pathname" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "pathname must be string" ) ;
         goto error ;
      }
      pathname = _matchObj.getStringField( "pathname" ) ;

      rc = ossAccess( pathname.c_str() ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "pathname not exist" ) ;
         goto error ;
      }

      // check path type
      rc = ossGetPathType( pathname.c_str(), &type ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "Failed to get path type" ) ;
         goto error ;
      }
      if ( SDB_OSS_DIR != type )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "pathname must be dir" ) ;
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
         stringstream ss ;
         ss << "failed to exec cmd " << cmd.str() << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }

      // get result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         stringstream ss ;
         ss << "failed to read msg from cmd \"" << cmd.str() << "\", rc:"
            << rc ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
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
      retObj = BSON( "isEmpty" << isEmpty ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteFileStat implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteFileStat )

   _remoteFileStat::_remoteFileStat()
   {
   }

   _remoteFileStat::~_remoteFileStat()
   {
   }

   const CHAR* _remoteFileStat::name()
   {
      return OMA_REMOTE_FILE_STAT ;
   }

#if defined (_LINUX)
   INT32 _remoteFileStat::doit( BSONObj &retObj )
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
      if ( FALSE == _matchObj.hasField( "filename" ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "filename must be config" ) ;
         goto error ;
      }
      if ( String != _matchObj.getField( "filename" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "filename must be string" ) ;
         goto error ;
      }
      pathname = _matchObj.getStringField( "filename" ) ;

      // set result format
      cmd << "stat -c\"%n|%s|%U|%G|%x|%y|%z|%A\" " << pathname ;

      // run command
      rc = runner.exec( cmd.str().c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         stringstream ss ;
         ss << "failed to exec cmd " << cmd.str() << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         PD_LOG_MSG( PDERROR, "%s", ss.str().c_str() ) ;
         goto error ;
      }

      // get result
      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         stringstream ss ;
         ss << "failed to read msg from cmd \"" << cmd.str() << "\", rc:"
            << rc ;
         PD_LOG_MSG( PDERROR, "%s", ss.str().c_str() ) ;
         goto error ;
      }
      else if ( SDB_OK != exitCode )
      {
         rc = exitCode ;
         PD_LOG_MSG( PDERROR, outStr.c_str() ) ;
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
         boost::algorithm::split( splited, outStr, boost::is_any_of("\r\n") ) ;
      }
      catch( std::exception )
      {
         rc = SDB_SYS ;
         PD_LOG_MSG( PDERROR, "Failed to split result" ) ;
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
            PD_LOG_MSG( PDERROR, "Failed to split result" ) ;
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
         retObj = builder.obj() ;
         goto done ;
      }

      rc = SDB_SYS ;
      PD_LOG_MSG( PDERROR, "Failed to build result" ) ;
      goto error ;
   done:
      return rc ;
   error:
      goto done ;
   }

#elif defined (_WINDOWS)
   INT32 _remoteFileStat::doit( BSONObj &retObj )
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
      if ( FALSE == _matchObj.hasField( "filename" ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "filename must be config" ) ;
         goto error ;
      }
      if ( String != _matchObj.getField( "filename" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "filename must be string" ) ;
         goto error ;
      }
      pathname = _matchObj.getStringField( "filename" ) ;

      if ( NULL == ossGetRealPath( pathname.c_str(),
                                   realPath,
                                   OSS_MAX_PATHSIZE + 1 ) )
      {
         rc = SDB_SYS ;
         PD_LOG_MSG( PDERROR, "Failed to build real path" ) ;
         goto error ;
      }

      // get file type
      rc = ossGetPathType( realPath, &ossFileType ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "Failed to get file type" ) ;
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
         PD_LOG_MSG( PDERROR, "Failed to get file size" ) ;
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
      retObj = builder.obj() ;
   done:
      return rc ;
   error:
      goto done ;

   }
#endif
   /*
      _remoteFileMd5 implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteFileMd5 )

   _remoteFileMd5::_remoteFileMd5()
   {
   }

   _remoteFileMd5::~_remoteFileMd5()
   {
   }

   const CHAR* _remoteFileMd5::name()
   {
      return OMA_REMOTE_FILE_MD5 ;
   }

   INT32 _remoteFileMd5::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      SINT64 bufSize = SPT_MD5_READ_LEN ;
      SINT64 hasRead = 0 ;
      CHAR readBuf[SPT_MD5_READ_LEN + 1] = { 0 } ;
      OSSFILE file ;
      string filename ;
      stringstream ss ;
      BOOLEAN isOpen = FALSE ;
      md5_state_t st ;
      md5_init( &st ) ;
      md5::md5digest digest ;
      string code ;

      // check, we need 1 argument filename
      if ( FALSE == _matchObj.hasField( "filename" ) )
      {
         rc = SDB_INVALIDARG  ;
         ss << "filename must be config" ;
         goto error ;
      }
      if ( String != _matchObj.getField( "filename" ).type() )
      {
         rc = SDB_INVALIDARG ;
         ss << "filename must be string" ;
         goto error ;
      }
      filename = _matchObj.getStringField( "filename" ) ;

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
      retObj = BSON( "md5" << code.c_str() ) ;
   done:
      if ( TRUE == isOpen )
         ossClose( file ) ;
      return rc ;
   error:
      PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
      goto done ;
   }


   /*
      _remoteFileGetContentSize implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteFileGetContentSize )

   _remoteFileGetContentSize::_remoteFileGetContentSize()
   {
   }

   _remoteFileGetContentSize::~_remoteFileGetContentSize()
   {
   }

   const CHAR* _remoteFileGetContentSize::name()
   {
      return OMA_REMOTE_FILE_GET_CONTENT_SIZE ;
   }

   INT32 _remoteFileGetContentSize::doit( BSONObj &retObj )
   {
      INT32 rc                       = SDB_OK ;
      INT32 size                     = 0 ;
      ossPrimitiveFileOp             op ;
      ossPrimitiveFileOp::offsetType offset ;
      string                         name ;

      // get filename
      if ( FALSE == _matchObj.hasField( "name" ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "name must be config" ) ;
         goto error ;
      }
      if ( String != _matchObj.getField( "name" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "name must be string" ) ;
         goto error ;
      }
      name = _matchObj.getStringField( "name" ) ;

      // open file
      rc = op.Open ( name.c_str() , OSS_PRIMITIVE_FILE_OP_READ_ONLY ) ;
      if ( rc != SDB_OK )
      {
         PD_LOG_MSG( PDERROR, "Can't open file: %s", name.c_str() ) ;
         goto error ;
      }

      // get filesize
      rc = op.getSize ( &offset ) ;
      if ( rc != SDB_OK )
      {
         PD_LOG_MSG( PDERROR, "Failed to get file's size" ) ;
         goto error ;
      }
      size = offset.offset ;
      retObj = BSON( "size" << size ) ;
   done:
      op.Close() ;
      return rc ;
   error:
      goto done ;
   }
}
