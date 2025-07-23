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

   Source File Name = catCMDBase.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains base class of catalog
   command class.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who      Description
   ====== =========== ======== ==============================================
          2019/10/02  Ting YU  Initial Draft

   Last Changed =

*******************************************************************************/
#include "catCMDBase.hpp"
#include "catTrace.hpp"

using namespace bson ;

namespace engine
{
   _catCmdBuilder::_catCmdBuilder ()
   {
   }

   _catCmdBuilder::~_catCmdBuilder ()
   {
      _mapCommand.clear() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATCMDBUILDER__REGISTER, "_catCmdBuilder::_register" )
   INT32 _catCmdBuilder::_register ( const CHAR *name, CAT_CMD_NEW_FUNC pFunc )
   {
      INT32 rc = SDB_OK ;

      if ( !name || !(*name) || !pFunc )
      {
         SDB_ASSERT( FALSE, "Invalid parameters" ) ;
         rc = SDB_SYS ;
      }
      else
      {
         MAP_COMMAND_IT it = _mapCommand.find( name ) ;
         if ( it != _mapCommand.end() )
         {
            SDB_ASSERT( FALSE, "Command already exist" ) ;
            rc = SDB_SYS ;
         }
         else
         {
            try
            {
               _mapCommand.insert( MAP_COMMAND::value_type( name, pFunc ) ) ;
            }
            catch( std::exception &e )
            {
               rc = ossException2RC( &e ) ;
            }
         }
      }
      return rc ;
   }

   INT32 _catCmdBuilder::create ( const CHAR *name,
                                  _catCMDBase *&pCommand )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL == pCommand, "pCommand must be NULL" ) ;

      if ( !name || !(*name) )
      {
         SDB_ASSERT( FALSE, "Invalid parameters" ) ;
         rc = SDB_SYS ;
      }
      else
      {
         MAP_COMMAND_IT it = _mapCommand.find( name ) ;
         if ( it != _mapCommand.end() )
         {
            CAT_CMD_NEW_FUNC &pFunc = it->second ;
            pCommand = (*pFunc)() ;
            if ( !pCommand )
            {
               rc = SDB_OOM ;
            }
         }
         else
         {
            rc = SDB_INVALIDARG ;
         }
      }
      return rc ;
   }

   void _catCmdBuilder::release ( _catCMDBase *&pCommand )
   {
      if ( pCommand )
      {
         SDB_OSS_DEL pCommand ;
         pCommand = NULL ;
      }
   }

   _catCmdBuilder *getCatCmdBuilder ()
   {
      static _catCmdBuilder cmdBuilder ;
      return &cmdBuilder ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATCMDASSIT__CATCMDASSIT, "_catCmdAssit::_catCmdAssit" )
   _catCmdAssit::_catCmdAssit ( CAT_CMD_NEW_FUNC pFunc )
   {
      PD_TRACE_ENTRY ( SDB__CATCMDASSIT__CATCMDASSIT ) ;
      if ( pFunc )
      {
         _catCMDBase *pCommand = (*pFunc)() ;
         if ( pCommand )
         {
            getCatCmdBuilder()->_register ( pCommand->name(), pFunc ) ;
            SDB_OSS_DEL pCommand ;
            pCommand = NULL ;
         }
      }
      PD_TRACE_EXIT ( SDB__CATCMDASSIT__CATCMDASSIT ) ;
   }

}
