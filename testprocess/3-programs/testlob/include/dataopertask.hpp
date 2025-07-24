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

   Source File Name = dataopertask.hpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef __TEST_DATAOPERTASK_HPP__
#define __TEST_DATAOPERTASK_HPP__

#include "task.hpp"

class IClient ;
class CDataOperTask:public CTask
{
   public:
         CDataOperTask ( int needFinishNum ,
                                   int runMode ,
                                   int interval ,
                                   int No ) ;
         ~CDataOperTask ( ) ;

         void setClient ( IClient* pClient )
         {
             m_pClient = pClient ;
         }

   private:
         bool Init ( ) ;
         void Do ( ) ;
         void AddUp ( int nCnt ) ;

   private:
         int m_needFinishNum ;
         int m_runMode ;
         int m_interval ;
         IClient* m_pClient ;
         timeval m_tb, m_te ;

         static int m_init ;
         
} ;

#endif
