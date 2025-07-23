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

   Source File Name = omToolCmdBase.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/09/2019  HJW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "omToolCmdBase.hpp"

namespace omTool
{
   omToolCmdBase::omToolCmdBase()
   {
      _options = NULL ;
   }

   omToolCmdBase::~omToolCmdBase()
   {
   }

   void omToolCmdBase::setOptions( omToolOptions *options )
   {
      _options = options ;
   }

   void omToolCmdBase::_setErrorMsg( const CHAR *pMsg )
   {
      _errMsg = pMsg ;
   }

   void omToolCmdBase::_setErrorMsg( const string &msg )
   {
      _errMsg = msg.c_str() ;
   }

   _omToolCmdAssit::_omToolCmdAssit( OMTOOL_NEW_FUNC pFunc )
   {
      if ( pFunc )
      {
         omToolCmdBase *pCommand = (*pFunc)() ;
         if ( pCommand )
         {
            getOmToolCmdBuilder()->_register ( pCommand->name(), pFunc ) ;
            SDB_OSS_DEL pCommand ;
            pCommand = NULL ;
         }
      }
   }

   _omToolCmdAssit::~_omToolCmdAssit()
   {
   }

   _omToolCmdBuilder::_omToolCmdBuilder ()
   {
   }

   _omToolCmdBuilder::~_omToolCmdBuilder ()
   {
   }

   omToolCmdBase* _omToolCmdBuilder::create( const CHAR *command )
   {
      OMTOOL_NEW_FUNC pFunc = _find ( command ) ;

      if ( pFunc )
      {
         return (*pFunc)() ;
      }

      return NULL ;
   }

   void _omToolCmdBuilder::release( omToolCmdBase *&pCommand )
   {
      if ( pCommand )
      {
         SDB_OSS_DEL pCommand ;
         pCommand = NULL ;
      }
   }

   INT32 _omToolCmdBuilder::_register ( const CHAR *name, OMTOOL_NEW_FUNC pFunc )
   {
      INT32 rc = SDB_OK ;
      pair< MAP_CMD_IT, BOOLEAN > ret ;

      if ( ossStrlen( name ) == 0 )
      {
         goto done ;
      }

      ret = _cmdMap.insert( pair<const CHAR*, OMTOOL_NEW_FUNC>(name, pFunc) ) ;
      if ( FALSE == ret.second )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   OMTOOL_NEW_FUNC _omToolCmdBuilder::_find( const CHAR *name )
   {
      if ( name )
      {
         MAP_CMD_IT it = _cmdMap.find( name ) ;
         if ( it != _cmdMap.end() )
         {
            return it->second ;
         }
      }
      return NULL ;
   }

   _omToolCmdBuilder* getOmToolCmdBuilder()
   {
      static _omToolCmdBuilder cmdBuilder ;
      return &cmdBuilder ;
   }
}