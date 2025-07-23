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

   Source File Name = task.cpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#include "task.hpp"
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <stdio.h>

CTask::CTask ( int No )
:m_bIsExit ( false ) ,
 m_pThrd ( NULL ) ,
 m_thrNo( No ) 
{
}

CTask::~CTask ( )
{
   delete m_pThrd ;
}

void CTask::run ( ) 
{
   if ( !Init( ) )
   {
      printf( "thread init failed\n" ) ;
      return ;
   }

   while ( !m_bIsExit)
   {
      Do ( ) ;
   }
}

void CTask::start()
{
   boost::function0<void> f = boost::bind( &CTask::run, this ) ;
   m_pThrd = new boost::thread ( f ) ;
}

void CTask::wait ( )
{
   m_pThrd->join ( ) ;
}


