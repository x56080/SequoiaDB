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

   Source File Name = omagentPluginMgr.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/05/2018  HJW Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OMAGENT_PLUGIN_MGR_HPP__
#define OMAGENT_PLUGIN_MGR_HPP__

#include "omagentDef.hpp"
#include "pmdOptionsMgr.hpp"
#include "omagentTask.hpp"

namespace engine
{
   class _omAgentPluginMgr
   {
   public:
      _omAgentPluginMgr() ;
      virtual ~_omAgentPluginMgr() ;

      virtual INT32 init ( _pmdCfgRecord *pOptions ) ;
      virtual INT32 active () ;
      virtual INT32 deactive () ;
      virtual INT32 fini () ;

      virtual void onConfigChange() ;

   private:
      INT32 _startPlugin() ;
      INT32 _stopPlugin() ;
      INT32 _startTask( _omaTask *pTask ) ;
      BOOLEAN _isGeneral() ;

   private:
      _pmdCfgRecord *_options ;
   } ;
   typedef _omAgentPluginMgr omAgentPluginMgr ;
}

#endif