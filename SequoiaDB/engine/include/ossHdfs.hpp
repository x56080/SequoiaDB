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

   Source File Name = ossHdfs.hpp

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
#ifndef OSSHDFS_HPP_
#define OSSHDFS_HPP_

#define OSS_HDFS_RDONLY 1
#define OSS_HDFS_WRONLY 2

class _ossHdfs
{
private:
   void* _pHdfsFile ;
   void* _pHdfsFS ;
public:
   void* pFunctionAddress ;
public:
   int ossHdfsConnect( const char *hostName,
                       unsigned short port,
                       const char *user ) ;
   int ossHdfsDisconnect() ;
   int ossHdfsOpenFile( const char *path, int flags, int bufferSize,
                        short replication, int blocksize ) ;
   int ossHdfsCloseFile() ;
   int ossHdfsRead( char *buffer, int size, int &readSize ) ;
   _ossHdfs() ;
   ~_ossHdfs() ;
} ;
typedef class _ossHdfs ossHdfs ;

#endif