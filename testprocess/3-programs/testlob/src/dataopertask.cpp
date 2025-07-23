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

   Source File Name = dataopertask.cpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#include "common.h"
#include "iclient.h"
#include "recordresulttask.hpp"
#include "dataopertask.hpp"

int CDataOperTask::m_init ;

CDataOperTask::CDataOperTask(int needFinishNum, int runMode, int interval, int No)
:CTask( No ) ,
  m_needFinishNum ( needFinishNum ) ,
  m_runMode ( runMode ) ,
  m_interval ( interval ) ,
  m_pClient ( NULL )
{
}

CDataOperTask::~CDataOperTask ( ) 
{
   if ( NULL != m_pClient )
   {
      m_pClient->Release ( ) ;
   }
}

bool CDataOperTask::Init ( )
{
   assert ( NULL != m_pClient ) ;
   int rc = m_pClient->Connect ( ) ;

    if ( 0 != rc )
   {
      return false ;
   }
    else
   {
      return true ;
   }
}

void CDataOperTask::AddUp ( int nCnt )
{
   int diffSec, diffUSec ;
   if ( nCnt % m_interval == 0 )
   {
      gettimeofday( &m_te, NULL ) ;
      diffSec = m_te.tv_sec - m_tb.tv_sec ;
      diffUSec = m_te.tv_usec - m_tb.tv_usec ;
      if ( diffUSec < 0 )
      {
         diffUSec += 1000000 ;
         diffSec -= 1 ;
      }
      CRecordResultTask::getInstance( )->addStatResult( nCnt, diffSec, diffUSec, getNo( ) ) ;
   }
}

void CDataOperTask::Do ( )
{
   gettimeofday ( &m_tb, NULL ) ;
   int nCnt = 0 ;
   do
   {
      int rc ;
      switch ( m_runMode ) 
      {
         case MODE_PUT:
               {
                 rc = m_pClient->Putfile( nCnt ) ;
                 if ( 0 == rc )
                   nCnt++ ;
               }
               break ;
         case MODE_GET:
               {
                 rc = m_pClient->Getfile( nCnt ) ;
                  if ( 0 == rc )
                     nCnt++ ;
               } 
               break ;
          case MODE_MIX:
               {
                  rc = m_pClient->Mix( nCnt ) ;
                  if ( 0 == rc )
                     nCnt++ ;
               }
               break ;
      }
      
      AddUp ( nCnt ) ;

      if ( nCnt >= m_needFinishNum )
      {
         stop( ) ;
         break ;
      }
   }while ( true ) ;
}


