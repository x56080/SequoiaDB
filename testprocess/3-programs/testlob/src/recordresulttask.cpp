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

   Source File Name = recordresulttask.cpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#include "recordresulttask.hpp"
#include "common.h"
#include <unistd.h>

CRecordResultTask* CRecordResultTask::m_instance ;
using std::fstream ;
using std::endl ;

CRecordResultTask::CRecordResultTask ( int No )
:CTask ( No )
{
}

CRecordResultTask::~CRecordResultTask ( )
{
   if ( m_foutResult.is_open( ) )
   {
      m_foutResult.close ( ) ;
   }
}

CRecordResultTask* CRecordResultTask::getInstance ( )
{
   if ( NULL == m_instance )
   {
      m_instance = new CRecordResultTask ( 0 ) ;
   }
   return m_instance ;
}

void CRecordResultTask::addStatResult (int num, int diffsec, int diffUsec, int No)
{
   Stat stat ;
   stat.thrNo = No ;
   stat.totalnum = num ;
   stat.diffSec = diffsec ;
   stat.diffUSec = diffUsec ;

   {
      boost::lock_guard<boost::mutex> lock( m_statmutex ) ;
      m_stats.push_back ( stat ) ;
   }

   boost::unique_lock<boost::mutex> lock2 ( m_condmutex ) ;
   m_cond.notify_all ( ) ;
   return ;
}

void CRecordResultTask::stop ( ) 
{
   if ( m_stats.size( ) )
   {
      sleep ( 5 ) ;
   }
   CTask::stop ( ) ;
   boost::unique_lock<boost::mutex> lock2 ( m_condmutex ) ;
   m_cond.notify_all ( ) ;
}

bool CRecordResultTask::Init ( ) 
{
   m_foutResult.open ( "./result.log", std::fstream::in | std::fstream::out | std::fstream::trunc ) ;

   if ( !m_foutResult.is_open( ) ) 
   {
      std::cout << "open result.log failed!" << endl ;
      return false ;
   }

   return true ;

}

void CRecordResultTask::Do ( ) 
{
   do
   {
      boost::unique_lock<boost::mutex> lock ( m_condmutex ) ;
      while ( !isStop( ) && m_stats.size( ) == 0 )
      {
         m_cond.wait ( m_condmutex ) ;
      }

      std::vector<Stat> stats ;
      {
         boost::lock_guard<boost::mutex> lock( m_statmutex ) ;
         stats = m_stats ;
         m_stats.clear ( ) ;
      }

      for ( std::vector<Stat>::size_type i = 0; i < stats.size ( ) ; ++i )
      {
         m_foutResult << stats[i].thrNo << "," ;
         std::streamsize width = m_foutResult.width ( 9 ) ;
         m_foutResult.fill ( '0' ) ;
         m_foutResult << stats[i].totalnum << "," ;
         m_foutResult << stats[i].diffSec << "." ;
         m_foutResult.width( 6 ) ;
         m_foutResult << stats[i].diffUSec << endl ;
         m_foutResult.width ( width ) ;
      }

      if ( isStop ( ) )
      {
         break ;
      }
         
   }while ( true ) ;
}


