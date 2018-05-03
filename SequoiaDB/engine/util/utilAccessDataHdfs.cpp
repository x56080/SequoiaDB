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

   Source File Name = utilAccessDataHdfs.cpp

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

#include "utilAccessData.hpp"

_utilAccessDataHdfs::_utilAccessDataHdfs() : _loadModule(NULL)
{
}

_utilAccessDataHdfs::~_utilAccessDataHdfs()
{
   hdfsUnload() ;
   SAFE_OSS_FREE ( _loadModule ) ;
}

INT32 _utilAccessDataHdfs::initialize( void *pParamet )
{
   INT32 rc = SDB_OK ;
   utilAccessParametHdfs *temp = NULL ;
   if ( !pParamet )
   {
      rc = SDB_SYS ;
      goto error ;
   }
   temp = (utilAccessParametHdfs*)pParamet ;

   _loadModule = SDB_OSS_NEW ossModuleHandle( "libhdfs.so", temp->pPath, 0 ) ;
   if ( !_loadModule )
   {
      rc = SDB_OOM ;
      PD_LOG ( PDERROR, "Failed to malloc memory" ) ;
      goto error ;
   }
   rc = _loadModule->init() ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to init load module,rc = %d", rc ) ;
      goto error ;
   }
   rc = _loadModule->resolveAddress ( "hdfsConnectAsUser", &_function ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to init load module,rc = %d", rc ) ;
      goto error ;
   }
   _pHdfs.pFunctionAddress = (void*)_function ;
   rc = _pHdfs.ossHdfsConnect( temp->pHostName, temp->port, temp->pUser ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to connect to hdfs,rc = %d", rc ) ;
      goto error ;
   }
   rc = _loadModule->resolveAddress ( "hdfsOpenFile", &_function ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to init load module,rc = %d", rc ) ;
      goto error ;
   }
   _pHdfs.pFunctionAddress = (void*)_function ;
   rc = _pHdfs.ossHdfsOpenFile( temp->pFileName, OSS_HDFS_RDONLY, 0, 0, 0 ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to open file,rc = %d", rc ) ;
      goto error ;
   }

   rc = _loadModule->resolveAddress ( "hdfsRead", &_function ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to get read address,rc = %d", rc ) ;
      goto error ;
   }
   _pHdfs.pFunctionAddress = (void*)_function ;
done:
   return rc ;
error:
   goto done ;
}

INT32 _utilAccessDataHdfs::readNextBuffer ( CHAR *pBuffer, UINT32 &size )
{
   INT32 rc = SDB_OK ;
   SDB_ASSERT ( pBuffer, "pBuffer can't be NULL" ) ;
   INT32 iLenRead = 0 ;
   UINT32 sourceSize = 0 ;

   sourceSize = size ;
   while ( size != 0 )
   {
      rc = _pHdfs.ossHdfsRead( pBuffer, (INT32)size, iLenRead ) ;
      if ( 0 > rc )
      {
         rc = SDB_IO ;
         PD_LOG ( PDERROR, "Failed to read from file, rc = %d", rc ) ;
         goto error ;
      }
      else if ( 0 == rc )
      {
         rc = SDB_EOF ;
         goto done ;
      }
      else
      {
         rc = SDB_OK ;
         size -= iLenRead ;
      }
   }
done:
   size = sourceSize - size ;
   return rc ;
error:
   goto done ;
}

INT32 _utilAccessDataHdfs::hdfsUnload()
{
   INT32 rc = SDB_OK ;

   if ( !_loadModule )
   {
      goto done ;
   }

   rc = _loadModule->resolveAddress ( "hdfsCloseFile", &_function ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to get CloseFile address,rc = %d", rc ) ;
      goto error ;
   }
   _pHdfs.pFunctionAddress = (void*)_function ;
   _pHdfs.ossHdfsCloseFile() ;
   
   rc = _loadModule->resolveAddress ( "hdfsDisconnect", &_function ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to get Disconnect address,rc = %d", rc ) ;
      goto error ;
   }
   _pHdfs.pFunctionAddress = (void*)_function ;
   _pHdfs.ossHdfsDisconnect() ;

done:
   return rc ;
error:
   goto done ;
}
