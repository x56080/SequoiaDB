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

   Source File Name = qgmOptiCommand.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/19/2023  ZHY Initial Draft

   Last Changed =

*******************************************************************************/
#include "qgmOptiCommand.hpp"
#include "authAccessControlList.hpp"
#include "boost/exception/diagnostic_information.hpp"

namespace engine
{
   INT32 qgmOptiCommand::_checkPrivileges( ISession *session ) const
   {
      INT32 rc = SDB_OK;
      if ( !session->privilegeCheckEnabled() )
      {
         goto done;
      }

      try
      {
         boost::shared_ptr< const authAccessControlList > acl;
         rc = session->getACL( acl );
         PD_RC_CHECK( rc, PDERROR, "Failed to get ACL" );
         boost::shared_ptr< authResource > r;
         authActionSet actions;
         if ( SQL_GRAMMAR::CRTCS == _commandType )
         {
            r = authResource::forCluster();
            actions.addAction( ACTION_TYPE_createCS );
         }
         else if ( SQL_GRAMMAR::DROPCS == _commandType )
         {
            r = authResource::forCluster();
            actions.addAction( ACTION_TYPE_dropCS );
         }
         else if ( SQL_GRAMMAR::CRTCL == _commandType )
         {
            r = authResource::forCS( _fullName.relegation().toString() );
            actions.addAction( ACTION_TYPE_createCL );
         }
         else if ( SQL_GRAMMAR::DROPCL == _commandType )
         {
            r = authResource::forCS( _fullName.relegation().toString() );
            actions.addAction( ACTION_TYPE_dropCL );
         }
         else if ( SQL_GRAMMAR::CRTINDEX == _commandType )
         {
            r = authResource::forExact( _fullName.relegation().toString(),
                                        _fullName.attr().toString() );
            actions.addAction( ACTION_TYPE_createIndex );
         }
         else if ( SQL_GRAMMAR::DROPINDEX == _commandType )
         {
            r = authResource::forExact( _fullName.relegation().toString(),
                                        _fullName.attr().toString() );
            actions.addAction( ACTION_TYPE_dropIndex );
         }
         else
         {
            // special handle for list command, need pass here
            r.reset();
         }

         if ( r && !actions.empty() && !acl->isAuthorizedForActionsOnResource( *r, actions ) )
         {
            rc = SDB_NO_PRIVILEGES;
            PD_LOG_MSG( PDERROR, "No privilege to execute command: %s", toPoolString().c_str() );
            goto error;
         }

         // special handle for list command
         if ( SQL_GRAMMAR::LISTCS == _commandType )
         {
            r = authResource::forCluster();
            authActionSet actions1;
            actions1.addAction( ACTION_TYPE_list );
            authActionSet actions2;
            actions2.addAction( ACTION_TYPE_listCollectionSpaces );
            if ( !acl->isAuthorizedForActionsOnResource( *r, actions1 ) &&
                 !acl->isAuthorizedForActionsOnResource( *r, actions2 ) )
            {
               rc = SDB_NO_PRIVILEGES;
               PD_LOG_MSG( PDERROR, "No privilege to execute command: %s", toPoolString().c_str() );
               goto error;
            }
         }
         else if ( SQL_GRAMMAR::LISTCL == _commandType )
         {
            r = authResource::forCluster();
            authActionSet actions;
            actions.addAction( ACTION_TYPE_list );
            
            if ( !acl->isAuthorizedForActionsOnResource( *r, actions ) )
            {
               rc = SDB_NO_PRIVILEGES;
               PD_LOG_MSG( PDERROR, "No privilege to execute command: %s", toPoolString().c_str() );
               goto error;
            }
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e );
         PD_RC_CHECK( rc, PDERROR, "Occur exception: %s. rc: %d", e.what(), rc );
         goto error;
      }
      catch ( boost::exception &e )
      {
         rc = SDB_SYS;
         PD_LOG( PDERROR, "Occured exception when check privileges: %s, rc: %d",
                 boost::diagnostic_information( e ).c_str(), rc );
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   ossPoolString qgmOptiCommand::toPoolString() const
   {
      StringBuilder str;
      switch ( _commandType )
      {
      case SQL_GRAMMAR::CRTCS :
         str << "create collectionspace " << _fullName.toString();
         break;
      case SQL_GRAMMAR::DROPCS :
         str << "drop collectionspace " << _fullName.toString();
         break;
      case SQL_GRAMMAR::CRTCL :
         str << "create collection " << _fullName.toString();
         break;
      case SQL_GRAMMAR::DROPCL :
         str << "drop collection " << _fullName.toString();
         break;
      case SQL_GRAMMAR::CRTINDEX :
         str << "create index on collection " << _fullName.toString();
         break;
      case SQL_GRAMMAR::DROPINDEX :
         str << "drop index on collection " << _fullName.toString();
         break;
      case SQL_GRAMMAR::LISTCS :
         str << "list collectionspaces";
         break;
      case SQL_GRAMMAR::LISTCL :
         str << "list collections";
         break;
      default :
         str << "UNKNOWN";
         break;
      }
      return str.poolStr();
   }
} // namespace engine