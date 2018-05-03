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

   Source File Name = utilAccessDataLocalIO.cpp

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

_utilAccessDataLocalIO::_utilAccessDataLocalIO()
{
}

_utilAccessDataLocalIO::~_utilAccessDataLocalIO()
{
   ossClose ( _fileIO ) ;
}

INT32 _utilAccessDataLocalIO::initialize( void *pParamet )
{
   INT32 rc = SDB_OK ;
   utilAccessParametLocalIO *temp = NULL ;
   if ( !pParamet )
   {
      rc = SDB_SYS ;
      goto error ;
   }
   temp = (utilAccessParametLocalIO*)pParamet ;

   rc = ossOpen ( temp->pFileName,
                  OSS_READONLY | OSS_SHAREREAD,
                  OSS_DEFAULTFILE,
                  _fileIO ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to open input file %s, rc = %d",
               temp->pFileName, rc ) ;
      goto error ;
   }
done:
   return rc ;
error:
   goto done ;
}

INT32 _utilAccessDataLocalIO::readNextBuffer ( CHAR *pBuffer, UINT32 &size )
{
   INT32 rc = SDB_OK ;
   SINT64 iLenRead = 0 ;
   UINT32 sourceSize = 0 ;
   SINT64 readPos = 0 ;
   sourceSize = size ;
   SDB_ASSERT ( pBuffer, "pBuffer can't be NULL" ) ;
   while ( size > 0 )
   {
      rc = ossRead ( &_fileIO, pBuffer + readPos, size, &iLenRead ) ;
      if ( rc && SDB_INTERRUPT != rc && SDB_EOF != rc )
      {
         PD_LOG ( PDERROR, "Failed to read from file, rc = %d", rc ) ;
         goto error ;
      }
      else if ( rc == SDB_EOF )
      {
         goto done ;
      }
      else
      {
         rc = SDB_OK ;
         size -= iLenRead ;
         readPos += iLenRead ;
      }
   }
done:
   size = sourceSize - size ;
   return rc ;
error:
   goto done ;
}