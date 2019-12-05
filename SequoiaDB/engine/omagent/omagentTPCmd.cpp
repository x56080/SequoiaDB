/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = omagentTPCmd.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/01/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "omagentTPCmd.hpp"
#include "rtnCommandDef.hpp"
#include "omagentUtil.hpp"
#include "omagentMgr.hpp"
#include "pmdOptions.h"
#include "msgDef.h"
#include "pmd.hpp"

using namespace bson ;

namespace engine
{

   /*
      _omaCreateTPCMD implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _omaCreateTPCMD )

   _omaCreateTPCMD::_omaCreateTPCMD()
   {
   }

   _omaCreateTPCMD::~_omaCreateTPCMD()
   {
   }

   INT32 _omaCreateTPCMD::init( const CHAR *information )
   {
      INT32 rc = SDB_OK ;

      PD_CHECK( !sdbGetOMAgentOptions()->isStandAlone(),
                SDB_PERM, error, PDERROR,
                "Failed to create TP node, omagent is standalone mode" ) ;

      try
      {
         _config = BSONObj( information ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to initialize command [%s], "
                 "occurred exception: %s", name(), e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _omaCreateTPCMD::doit( BSONObj &retObject )
   {
      INT32 rc = SDB_OK ;

      BSONObj config ;

      try
      {
         BSONElement element = _config.getField( "$optionObj" ) ;
         if ( Object == element.type() )
         {
            config = element.embeddedObject() ;
            PD_LOG( PDEVENT, "Got TP node config %s",
                    config.toString().c_str() ) ;
         }
         else
         {
            PD_CHECK( EOO == element.type(), SDB_INVALIDARG, error, PDERROR,
                      "Failed to process \"$optionObj\" field [%s] with "
                      "wrong type, expected object or empty",
                      element.toString().c_str() ) ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to execute command [%s], "
                 "occurred exception: %s", name(), e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = sdbGetOMAgentMgr()->getNodeMgr()->addTPNode( config ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add TP node, rc: %d", rc ) ;

   done:
      return rc ;

   error:
      goto done ;
   }

   /*
      _omaRemoveTPCMD implement
    */
   IMPLEMENT_OACMD_AUTO_REGISTER( _omaRemoveTPCMD )

   _omaRemoveTPCMD::_omaRemoveTPCMD()
   {
   }

   _omaRemoveTPCMD::~_omaRemoveTPCMD()
   {
   }

   INT32 _omaRemoveTPCMD::init( const CHAR *information )
   {
      return SDB_OK ;
   }

   INT32 _omaRemoveTPCMD::doit( BSONObj &retObject )
   {
      INT32 rc = SDB_OK ;

      rc = sdbGetOMAgentMgr()->getNodeMgr()->removeTPNode() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to remove TP node, rc: %d", rc ) ;

   done:
      return rc ;

   error:
      goto done ;
   }

   /*
      _omaStartTPCMD implement
    */
   IMPLEMENT_OACMD_AUTO_REGISTER( _omaStartTPCMD )

   _omaStartTPCMD::_omaStartTPCMD()
   {
   }

   _omaStartTPCMD::~_omaStartTPCMD()
   {
   }

   INT32 _omaStartTPCMD::init( const CHAR *information )
   {
      return SDB_OK ;
   }

   INT32 _omaStartTPCMD::doit( BSONObj &retObject )
   {
      INT32 rc = SDB_OK ;

      rc = sdbGetOMAgentMgr()->getNodeMgr()->startTPNode() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to start TP node, rc: %d", rc ) ;

   done:
      return rc ;

   error:
      goto done ;
   }

   /*
      _omaStopTPCMD implement
    */
   IMPLEMENT_OACMD_AUTO_REGISTER( _omaStopTPCMD )

   _omaStopTPCMD::_omaStopTPCMD()
   {
   }

   _omaStopTPCMD::~_omaStopTPCMD()
   {
   }

   INT32 _omaStopTPCMD::init( const CHAR *information )
   {
      return SDB_OK ;
   }

   INT32 _omaStopTPCMD::doit( BSONObj &retObject )
   {
      INT32 rc = SDB_OK ;

      rc = sdbGetOMAgentMgr()->getNodeMgr()->stopTPNode() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to stop TP node, rc: %d", rc ) ;

   done:
      return rc ;

   error:
      goto done ;
   }

   /*
      _omaGetTPCMD implement
    */
   IMPLEMENT_OACMD_AUTO_REGISTER( _omaGetTPCMD )

   _omaGetTPCMD::_omaGetTPCMD()
   {
   }

   _omaGetTPCMD::~_omaGetTPCMD()
   {
   }

   INT32 _omaGetTPCMD::init( const CHAR *infromation )
   {
      return SDB_OK ;
   }

   INT32 _omaGetTPCMD::doit( BSONObj &retObject )
   {
      INT32 rc = SDB_OK ;

      const CHAR *configPath = sdbGetOMAgentOptions()->getCfgPath() ;
      string serviceName ;

      rc = omGetTPFromConfig( configPath, serviceName ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get service name from "
                   "TP config path [%s], rc: %d", configPath, rc ) ;

      try
      {
         BSONObjBuilder builder ;

         builder.append( FIELD_NAME_SERVICE, serviceName ) ;

         retObject = builder.obj() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to execute command [%s], "
                 "occurred exception: %s", name(), e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

}
