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

   Source File Name = taskmng.cpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

#include "taskmng.hpp"
#include "recordresulttask.hpp"
#include "dataopertask.hpp"
#include "common.h"
#include "iclient.h"

void* CTaskMng::m_handle ;
CTaskMng::CreateFun CTaskMng::m_createFun ;

CTaskMng::CTaskMng ( )
:m_runMode ( 0 ) ,
 m_totalNum ( 0 ) ,
 m_thrNum ( 0 ) ,
 m_interval ( 0 ) ,
 m_dbName ( NULL ) ,
 m_connectStr ( NULL ) 
{
}

CTaskMng::~CTaskMng ( )
{
}

CTaskMng& CTaskMng::getInstance ( )
{
   static CTaskMng taskmng ;
   return taskmng ;
}

void CTaskMng::setRunPara ( int runMode ,
                                            int totalNum ,
                                            int thrNum ,
                                            int interval ,
                                            const char* connectStr ,
                                            const char* dbName )
{
   m_runMode = runMode ;
   m_totalNum = totalNum ;
   m_thrNum = thrNum ;
   m_interval = interval ;
   m_connectStr = connectStr ;
   m_dbName = dbName ;
}

int CTaskMng::GetFullPath(char* pPath)
{
   FILE* fp = fopen ( "./config.dat", "r" ) ;
   if ( NULL != fp )
   {
      size_t size = fread ( pPath, sizeof(char), 1024, fp ) ;
      pPath[size] = 0 ;

      char* pEnd = strchr ( pPath, '\n' ) ;
      if ( NULL != pEnd )
      {
         *pEnd = 0 ;
      }

      return 0 ;
   }
   else
   {
      return -1 ;
   }
}

IClient* CTaskMng::GetClient ( int startID )
{
   if ( NULL == m_handle )
   {
      char szPath[1024] = { 0 } ;
      int rc = GetFullPath( szPath ) ;
      assert ( rc == 0 ) ;
      m_handle = dlopen( szPath, RTLD_LAZY ) ;
      if ( NULL != m_handle )
      {
         m_createFun = ( CreateFun ) dlsym ( m_handle, "CreateClient") ;
      }
      else
      {
         printf( "err:%s\n", dlerror ( ) ) ;
      }
   }
   assert ( NULL != m_createFun ) ;
   return m_createFun ( m_connectStr, m_dbName, startID ) ;
}

bool CTaskMng::Init ( )
{
   CRecordResultTask::getInstance() ->runMode ( m_runMode ) ;
   for ( int i = 0 ; i < m_thrNum; ++i )
   {
      int startID = 0 ;
      int needFinishNum = m_totalNum / m_thrNum ;

      startID = i * m_totalNum / m_thrNum ;

      CDataOperTask* pTask = new CDataOperTask ( needFinishNum, m_runMode, m_interval, i + 1 ) ;
      IClient* pClient = GetClient ( startID ) ;
      pTask->setClient(pClient ) ;
      m_tasks.push_back ( pTask ) ;
   }

   return true ;
}

void CTaskMng::run ( )
{
   CRecordResultTask::getInstance( ) -> start ( ) ;
   for ( int i = 0; i < m_thrNum; ++i )
   {
      m_tasks[i] ->start( ) ;
   }
}

void CTaskMng::wait( )
{
   for ( int i = 0; i < m_tasks.size ( ) ; ++i )
   {
      m_tasks[i]->wait ( ) ;
   }

   CRecordResultTask::getInstance( )->stop( ) ;
   CRecordResultTask::getInstance( )->wait( ) ;
}


