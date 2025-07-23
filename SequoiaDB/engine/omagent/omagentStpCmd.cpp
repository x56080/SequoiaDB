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

   Source File Name = omagentStpCmd.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/01/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#include "omagentStpCmd.hpp"
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
      _omaCreateStpCMD implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _omaCreateStpCMD )

   _omaCreateStpCMD::_omaCreateStpCMD()
   {
   }

   _omaCreateStpCMD::~_omaCreateStpCMD()
   {
   }

   INT32 _omaCreateStpCMD::init( const CHAR *information )
   {
      INT32 rc = SDB_OK ;

      PD_CHECK( !sdbGetOMAgentOptions()->isStandAlone(),
                SDB_PERM, error, PDERROR,
                "Failed to create STP node, omagent is standalone mode" ) ;

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

   INT32 _omaCreateStpCMD::doit( BSONObj &retObject )
   {
      INT32 rc = SDB_OK ;

      BSONObj config ;

      try
      {
         BSONElement element = _config.getField( "$optionObj" ) ;
         if ( Object == element.type() )
         {
            config = element.embeddedObject() ;
            PD_LOG( PDEVENT, "Got STP node config %s",
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

      rc = sdbGetOMAgentMgr()->getNodeMgr()->addStpNode( config ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add STP node, rc: %d", rc ) ;

   done:
      return rc ;

   error:
      goto done ;
   }

   /*
      _omaRemoveStpCMD implement
    */
   IMPLEMENT_OACMD_AUTO_REGISTER( _omaRemoveStpCMD )

   _omaRemoveStpCMD::_omaRemoveStpCMD()
   {
   }

   _omaRemoveStpCMD::~_omaRemoveStpCMD()
   {
   }

   INT32 _omaRemoveStpCMD::init( const CHAR *information )
   {
      return SDB_OK ;
   }

   INT32 _omaRemoveStpCMD::doit( BSONObj &retObject )
   {
      INT32 rc = SDB_OK ;

      rc = sdbGetOMAgentMgr()->getNodeMgr()->removeStpNode() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to remove STP node, rc: %d", rc ) ;

   done:
      return rc ;

   error:
      goto done ;
   }

   /*
      _omaStartStpCMD implement
    */
   IMPLEMENT_OACMD_AUTO_REGISTER( _omaStartStpCMD )

   _omaStartStpCMD::_omaStartStpCMD()
   {
   }

   _omaStartStpCMD::~_omaStartStpCMD()
   {
   }

   INT32 _omaStartStpCMD::init( const CHAR *information )
   {
      return SDB_OK ;
   }

   INT32 _omaStartStpCMD::doit( BSONObj &retObject )
   {
      INT32 rc = SDB_OK ;

      rc = sdbGetOMAgentMgr()->getNodeMgr()->startStpNode() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to start STP node, rc: %d", rc ) ;

   done:
      return rc ;

   error:
      goto done ;
   }

   /*
      _omaStopStpCMD implement
    */
   IMPLEMENT_OACMD_AUTO_REGISTER( _omaStopStpCMD )

   _omaStopStpCMD::_omaStopStpCMD()
   {
   }

   _omaStopStpCMD::~_omaStopStpCMD()
   {
   }

   INT32 _omaStopStpCMD::init( const CHAR *information )
   {
      return SDB_OK ;
   }

   INT32 _omaStopStpCMD::doit( BSONObj &retObject )
   {
      INT32 rc = SDB_OK ;

      rc = sdbGetOMAgentMgr()->getNodeMgr()->stopStpNode() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to stop STP node, rc: %d", rc ) ;

   done:
      return rc ;

   error:
      goto done ;
   }

   /*
      _omaGetStpCMD implement
    */
   IMPLEMENT_OACMD_AUTO_REGISTER( _omaGetStpCMD )

   _omaGetStpCMD::_omaGetStpCMD()
   {
   }

   _omaGetStpCMD::~_omaGetStpCMD()
   {
   }

   INT32 _omaGetStpCMD::init( const CHAR *infromation )
   {
      return SDB_OK ;
   }

   INT32 _omaGetStpCMD::doit( BSONObj &retObject )
   {
      INT32 rc = SDB_OK ;

      const CHAR *configPath = sdbGetOMAgentOptions()->getCfgPath() ;
      string serviceName ;

      rc = omGetStpFromConfig( configPath, serviceName ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get service name from "
                   "STP config path [%s], rc: %d", configPath, rc ) ;

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
