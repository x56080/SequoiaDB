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

   Source File Name = baseCommand.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/01/2015  LZ  Initial Draft

   Last Changed =

*******************************************************************************/
#include "baseCommand.hpp"

baseCommand::baseCommand( const CHAR *name, const CHAR *secondName )
{
   commandMgr *mgr = commandMgr::instance() ;
   if ( NULL != mgr )
   {
      mgr->addCommand( name, this ) ;
      _name = name ;
      if ( NULL != secondName )
      {
         mgr->addCommand( secondName, this ) ;
      }
   }
}

commandMgr* commandMgr::instance()
{
   static commandMgr mgr ;
   return &mgr ;
}
