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

   Source File Name = recordresulttask.hpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef __RECORDRESULTTASK_HPP__
#define __RECORDRESULTTASK_HPP__

#include "dataopertask.hpp"
#include "task.hpp"
#include <vector>
#include <fstream>
#include <boost/thread/thread.hpp>

class CRecordResultTask:public CTask
{
   private:
         CRecordResultTask ( int No ) ;
         ~CRecordResultTask ( ) ;
         
   public:
         static CRecordResultTask* getInstance ( ) ;
         void addStatResult ( int num, int diffsec, int diffUsec, int No ) ;
         void runMode ( int runMode )
         {
            m_runMode = runMode ;
         }
         virtual void stop ( ) ;

   private:
         virtual bool Init ( ) ;
         virtual void Do ( ) ;

         typedef struct struStat
         {
            int thrNo ;
            long totalnum ;
            int diffSec ;
            int diffUSec ;
          } Stat ;

   private:
         int m_runMode ;
         boost::mutex m_statmutex ;
         std::vector<Stat> m_stats ;

         boost::mutex m_condmutex ;
         boost::condition_variable_any m_cond ;
         std::fstream m_foutResult ;

         static CRecordResultTask* m_instance ;
} ;


#endif
