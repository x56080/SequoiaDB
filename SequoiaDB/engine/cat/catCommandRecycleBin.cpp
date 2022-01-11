/*******************************************************************************


   Copyright (C) 2011-2021 SequoiaDB Ltd.

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

   Source File Name = catCommandRecycleBin.hpp

   Descriptive Name = Catalogue commands for recycle bin

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains Runtime code for catalog
   commands.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/01/2021  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "catCommandRecycleBin.hpp"
#include "catCommon.hpp"
#include "rtnCB.hpp"
#include "catTrace.hpp"
#include "pdTrace.hpp"

namespace engine
{

   /*
      _catCMDGetRecycleBinDetail implement
    */
   CAT_IMPLEMENT_CMD_AUTO_REGISTER( _catCMDGetRecycleBinDetail )

   _catCMDGetRecycleBinDetail::_catCMDGetRecycleBinDetail()
   : _recycleBinMgr( sdbGetCatalogueCB()->getRecycleBinMgr() )
   {
   }

   _catCMDGetRecycleBinDetail::~_catCMDGetRecycleBinDetail()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATCMDGETRECYCLEBINDETAIL_DOIT, "_catCMDGetRecycleBinDetail::doit" )
   INT32 _catCMDGetRecycleBinDetail::doit( _pmdEDUCB *cb,
                                           rtnContextBuf &ctxBuf,
                                           INT64 &contextID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATCMDGETRECYCLEBINDETAIL_DOIT ) ;

      BSONObj resultObj ;
      const utilRecycleBinConf &info = _recycleBinMgr->getConf() ;

      rc = info.toBSON( resultObj, FALSE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for recycle bin detail, "
                   "rc: %d", rc ) ;

      ctxBuf = rtnContextBuf( resultObj ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATCMDGETRECYCLEBINDETAIL_DOIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _catCMDAlterRecycleBin implement
    */
   CAT_IMPLEMENT_CMD_AUTO_REGISTER( _catCMDAlterRecycleBin )

   _catCMDAlterRecycleBin::_catCMDAlterRecycleBin()
   : _recycleBinMgr( sdbGetCatalogueCB()->getRecycleBinMgr() )
   {
   }

   _catCMDAlterRecycleBin::~_catCMDAlterRecycleBin()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATCMDALTERRECYCLEBIN_INIT, "_catCMDAlterRecycleBin::init" )
   INT32 _catCMDAlterRecycleBin::init( const CHAR *pQuery,
                                       const CHAR *pSelector,
                                       const CHAR *pOrderBy,
                                       const CHAR *pHint,
                                       INT32 flags,
                                       INT64 numToSkip,
                                       INT64 numToReturn )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATCMDALTERRECYCLEBIN_INIT ) ;

      // copy from old conf
      _newConf = _recycleBinMgr->getConf() ;

      try
      {
         BSONObj infoObj( pQuery ) ;
         const CHAR *actionName = NULL ;

         BSONElement element = infoObj.getField( FIELD_NAME_ACTION ) ;
         PD_CHECK( String == element.type(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to get field [%s] from alter options [%s]",
                   FIELD_NAME_ACTION, infoObj.toPoolString().c_str() ) ;
         actionName = element.valuestr() ;

         if ( 0 == ossStrcmp( CMD_VALUE_NAME_RECYCLEBIN_ENABLE,
                              actionName ) )
         {
            _newConf.setEnable( TRUE ) ;
         }
         else if ( 0 == ossStrcmp( CMD_VALUE_NAME_RECYCLEBIN_DISABLE,
                                   actionName ) )
         {
            _newConf.setEnable( FALSE ) ;
         }
         else if ( 0 == ossStrcmp( CMD_VALUE_NAME_RECYCLEBIN_SETATTR,
                                   actionName ) )
         {
            BSONObj options ;

            element = infoObj.getField( FIELD_NAME_OPTIONS ) ;
            PD_CHECK( Object == element.type(), SDB_INVALIDARG, error, PDERROR,
                      "Failed to get field [%s] from alter options [%s]",
                      FIELD_NAME_ACTION, infoObj.toPoolString().c_str() ) ;
            options = element.embeddedObject() ;

            rc = _newConf.updateOptions( options ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to update options, rc: %d", rc ) ;
         }
         else
         {
            PD_LOG( PDERROR, "Failed to alter recycle bin by "
                    "unknown action [%s]", actionName ) ;
         }
      }
      catch ( exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception happened: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATCMDALTERRECYCLEBIN_INIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATCMDALTERRECYCLEBIN_DOIT, "_catCMDAlterRecycleBin::doit" )
   INT32 _catCMDAlterRecycleBin::doit( _pmdEDUCB *cb,
                                       rtnContextBuf &ctxBuf,
                                       INT64 &contextID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATCMDALTERRECYCLEBIN_DOIT ) ;

      sdbCatalogueCB *catCB = sdbGetCatalogueCB() ;
      INT16 w = catCB->majoritySize() ;

      rc = _recycleBinMgr->updateConf( _newConf, cb, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update recycle bin conf, "
                   "rc: %d", rc ) ;

      // update catalog cache
      sdbGetCatalogueCB()->getCatDCMgr()->updateDCCache() ;

   done:
      PD_TRACE_EXITRC( SDB__CATCMDALTERRECYCLEBIN_DOIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _catCMDGetRecycleBinCount define
    */
   CAT_IMPLEMENT_CMD_AUTO_REGISTER( _catCMDGetRecycleBinCount )

   _catCMDGetRecycleBinCount::_catCMDGetRecycleBinCount()
   {
   }

   _catCMDGetRecycleBinCount::~_catCMDGetRecycleBinCount()
   {
   }


   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATCMDGETRECYBINCNT_INIT, "_catCMDGetRecycleBinCount::init" )
   INT32 _catCMDGetRecycleBinCount::init( const CHAR *pQuery,
                                          const CHAR *pSelector,
                                          const CHAR *pOrderBy,
                                          const CHAR *pHint,
                                          INT32 flags,
                                          INT64 numToSkip,
                                          INT64 numToReturn )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATCMDGETRECYBINCNT_INIT ) ;

      try
      {
         _queryObj = BSONObj( pQuery ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to get query object, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATCMDGETRECYBINCNT_INIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATCMDGETRECYBINCNT_DOIT, "_catCMDGetRecycleBinCount::doit" )
   INT32 _catCMDGetRecycleBinCount::doit( _pmdEDUCB *cb,
                                          rtnContextBuf &ctxBuf,
                                          INT64 &contextID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATCMDGETRECYBINCNT_DOIT ) ;

      INT64 recycleCount = 0 ;

      rc = sdbGetCatalogueCB()->getRecycleBinMgr()->countItems(
                                             _queryObj, cb, recycleCount ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get recycle item count, rc: %d",
                   rc ) ;

      try
      {
         BSONObj resultObj = BSON( FIELD_NAME_TOTAL << recycleCount ) ;
         ctxBuf = rtnContextBuf( resultObj ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build result, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATCMDGETRECYBINCNT_DOIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
