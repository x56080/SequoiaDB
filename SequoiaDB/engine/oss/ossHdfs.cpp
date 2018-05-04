/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = ossHdfs.cpp

   Descriptive Name =

   When/how to use: parse Data util

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/30/2013  JW  Initial Draft

   Last Changed =

******************************************************************************/

#include "../util/hdfs.h"
#include "ossHdfs.hpp"

//typedef int ( OSS_MODULE_FUNCTION ) () ;
//typedef OSS_MODULE_FUNCTION *OSS_MODULE_PFUNCTION ;

#define OSS_HDFS_CONNECT    (hdfsFS(*)(const char*,unsigned short,const char*))
#define OSS_HDFS_DISCONNECT (int(*)(hdfsFS))
#define OSS_HDFS_OPENFILE   (hdfsFile(*)(hdfsFS,const char*,int,int,short,int))
#define OSS_HDFS_CLOSEFILE  (int(*)(hdfsFS,hdfsFile))
#define OSS_HDFS_READ       (int(*)(hdfsFS,hdfsFile,void*,int))


int _ossHdfs::ossHdfsConnect( const char *hostName,
                              unsigned short port,
                              const char *user )
{
   int rc = 0 ;
   if ( !pFunctionAddress )
   {
      rc = -1 ;
      goto error ;
   }
   _pHdfsFS = (OSS_HDFS_CONNECT(pFunctionAddress))( hostName, port, user ) ;
   if ( !_pHdfsFS )
   {
      rc = -1 ;
      goto error ;
   }
done:
   return rc ;
error:
   goto done ;
}

int _ossHdfs::ossHdfsDisconnect()
{
   int rc = 0 ;
   if ( !pFunctionAddress || !_pHdfsFS )
   {
      rc = -1 ;
      goto error ;
   }
   rc = (OSS_HDFS_DISCONNECT(pFunctionAddress))( _pHdfsFS ) ;
   if ( -1 == rc )
   {
      goto error ;
   }
   _pHdfsFS = NULL ;
done:
   return rc ;
error:
   goto done ;
}

int _ossHdfs::ossHdfsOpenFile( const char *path,
                               int flags,
                               int bufferSize,
                               short replication,
                               int blocksize )
{
   int rc = 0 ;
   if ( !pFunctionAddress || !_pHdfsFS )
   {
      rc = -1 ;
      goto error ;
   }
   if ( OSS_HDFS_RDONLY == flags )
   {
      flags = O_RDONLY ;
   }
   else if ( OSS_HDFS_WRONLY == flags )
   {
      flags = O_WRONLY ;
   }
   
   _pHdfsFile = (OSS_HDFS_OPENFILE(pFunctionAddress))( _pHdfsFS,
                                                       path,
                                                       flags,
                                                       bufferSize,
                                                       replication,
                                                       blocksize ) ;
   if ( !_pHdfsFile )
   {
      rc = -1 ;
      goto error ;
   }
done:
   return rc ;
error:
   goto done ;
}

int _ossHdfs::ossHdfsCloseFile()
{
   int rc = 0 ;
   if ( !pFunctionAddress || !_pHdfsFS || !_pHdfsFile )
   {
      rc = -1 ;
      goto error ;
   }
   rc = (OSS_HDFS_CLOSEFILE(pFunctionAddress))( (hdfsFS)_pHdfsFS,
                                                (hdfsFile)_pHdfsFile ) ;
   if ( -1 == rc )
   {
      goto error ;
   }
   _pHdfsFile = NULL ;
done:
   return rc ;
error:
   goto done ;
}
   
int _ossHdfs::ossHdfsRead( char *buffer, int size, int &readSize )
{
   int rc = 0 ;
   if ( !pFunctionAddress || !_pHdfsFS || !_pHdfsFile )
   {
      rc = -1 ;
      goto error ;
   }
   readSize = (OSS_HDFS_READ(pFunctionAddress))( (hdfsFS)_pHdfsFS,
                                                 (hdfsFile)_pHdfsFile,
                                                 buffer,
                                                 size ) ;
   if ( 0 > readSize )
   {
      rc = -1 ;
      goto error ;
   }
   else if ( 0 == readSize )
   {
      rc = 0 ;
   }
   else
   {
      rc = 1 ;
   }
done:
   return rc ;
error:
   goto done ;
}

_ossHdfs::_ossHdfs() : _pHdfsFile(NULL),
                       _pHdfsFS(NULL),
                       pFunctionAddress(NULL)
{
}

_ossHdfs::~_ossHdfs()
{
}
