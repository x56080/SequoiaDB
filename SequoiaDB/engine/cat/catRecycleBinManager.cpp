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

   Source File Name = catRecycleBinManager.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for catalog node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/03/2021  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "catCommon.hpp"
#include "catRecycleBinManager.hpp"
#include "rtn.hpp"
#include "pdTrace.hpp"
#include "catTrace.hpp"
#include "../bson/bson.hpp"

using namespace bson ;

namespace engine
{

   // maximum retry count for dropping oldest recycle item
   #define CAT_RECYCLE_MAX_RETRY ( 3 )

   /*
      _catRecycleBinManager implement
    */
   _catRecycleBinManager::_catRecycleBinManager()
   : _BASE()
   {
   }

   _catRecycleBinManager::~_catRecycleBinManager()
   {
   }



   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR_INIT, "_catRecycleBinManager::init" )
   INT32 _catRecycleBinManager::init( const utilRecycleBinConf &conf )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR_INIT ) ;

      rc = _BASE::init() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init based recycle bin manager, "
                   "rc: %d", rc ) ;

      // set configure in cache
      setConf( conf ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATRECYBINMGR_INIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR_UPDATECONF, "_catRecycleBinManager::updateConf" )
   INT32 _catRecycleBinManager::updateConf( const utilRecycleBinConf &newConf,
                                            pmdEDUCB *cb,
                                            INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR_UPDATECONF ) ;

      rc = catUpdateRecycleBinConf( newConf, cb, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update recycle bin conf, "
                   "rc: %d", rc ) ;

      setConf( newConf ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATRECYBINMGR_UPDATECONF, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
