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

   Source File Name = taskmng.hpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef __TASKMNG_HPP__
#define __TASKMNG_HPP__

#include "task.hpp"

#include <vector>
#include <string>

using namespace std ;

class Task ;
class IClient ;
class CTaskMng
{
   public:
         static CTaskMng& getInstance ( ) ;
         void setRunPara (int runMode ,
                                   int totalNum ,
                                   int thrNum ,
                                   int interval ,
                                   const char* connectStr ,
                                   const char* dbName ) ;
         bool Init ( ) ;
         void run ( ) ;
         void wait ( ) ;
         
   private:
         CTaskMng ( ) ;
         ~CTaskMng ( ) ;

         int GetFullPath ( char* pPath ) ;
         typedef IClient* (*CreateFun) ( const char* pSvrAddr ,
                                         const char* pDbName ,
                                         int startID ) ;
         IClient* GetClient ( int startID ) ;

   private:
         int m_runMode ;
         int m_totalNum ;
         int m_thrNum ;
         int m_interval ;
         const char* m_connectStr ;
         const char* m_dbName ;

         static void *m_handle ;
         static CreateFun m_createFun ;
         std::vector<CTask* > m_tasks ; 
} ;

#endif
