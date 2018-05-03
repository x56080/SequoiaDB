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

   Source File Name = utilAccessData.hpp

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

#ifndef UTIL_MODE_ACCESS_DATA_HPP_
#define UTIL_MODE_ACCESS_DATA_HPP_

#include "core.hpp"
#include "ossIO.hpp"
#include "ossDynamicLoad.hpp"
#include "ossHdfs.hpp"

class _utilAccessData : public SDBObject
{
public:
   virtual ~_utilAccessData()
   {
   }
   virtual INT32 initialize ( void *pParamet ) = 0 ;
   virtual INT32 readNextBuffer ( CHAR *pBuffer, UINT32 &size ) = 0 ;
} ;
typedef class _utilAccessData utilAccessData ;


/*              This is model                                      
                local IO
*/

struct utilAccessParametLocalIO
{
   const CHAR *pFileName ;
   utilAccessParametLocalIO() : pFileName(NULL)
   {
   }
} ;

class _utilAccessDataLocalIO : public _utilAccessData
{
private:
   OSSFILE _fileIO ;
public:
   _utilAccessDataLocalIO() ;
   virtual ~_utilAccessDataLocalIO() ;
   virtual INT32 initialize ( void *pParamet ) ;
   virtual INT32 readNextBuffer ( CHAR *pBuffer, UINT32 &size ) ;
} ;
typedef class _utilAccessDataLocalIO utilAccessDataLocalIO ;


/*              hadoop                                      
                hdfs
*/

struct utilAccessParametHdfs
{
   const CHAR *pFileName ;
   const CHAR *pPath ;
   const CHAR *pHostName ;
   const CHAR *pUser ;
   UINT16 port ;
   utilAccessParametHdfs() : pFileName(NULL),
                             pPath(NULL),
                             pHostName(NULL),
                             pUser(NULL),
                             port(0)
   {
   }
} ;

class _utilAccessDataHdfs : public _utilAccessData
{
private:
   ossModuleHandle *_loadModule ;
   OSS_MODULE_PFUNCTION _function ;
   ossHdfs _pHdfs ;
private:
   INT32 hdfsUnload() ;
public:
   _utilAccessDataHdfs() ;
   virtual ~_utilAccessDataHdfs() ;
   virtual INT32 initialize ( void *pParamet ) ;
   virtual INT32 readNextBuffer ( CHAR *pBuffer, UINT32 &size ) ;
} ;
typedef class _utilAccessDataHdfs utilAccessDataHdfs ;

#endif