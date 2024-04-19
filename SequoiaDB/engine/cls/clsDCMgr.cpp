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

   Source File Name = clsDCMgr.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/02/2015  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#include "clsDCMgr.hpp"
#include "msgDef.h"
#include "msgMessage.hpp"
#include "msgCatalog.hpp"
#include "catDef.hpp"
#include "../bson/bson.h"

using namespace bson ;

namespace engine
{

   /*
      _clsDCBaseInfo implement
   */
   _clsDCBaseInfo::_clsDCBaseInfo()
   {
      _reset() ;
   }

   _clsDCBaseInfo::~_clsDCBaseInfo()
   {
   }

   void _clsDCBaseInfo::_reset()
   {
      _clusterName = "" ;
      _businessName = "" ;
      _address = "" ;
      _activated = TRUE ;
      _readonly = FALSE ;
      _hasCsUniqueHWM = FALSE ;
      _csUniqueHWM = 0 ;
      _catVersion = CATALOG_VERSION_CUR ;

      _orgObj = BSONObj() ;

      _recycleBinConf.reset() ;
   }

   INT32 _clsDCBaseInfo::lock_r( INT32 millisec )
   {
      return _rwMutex.lock_r( millisec ) ;
   }

   INT32 _clsDCBaseInfo::release_r()
   {
      return _rwMutex.release_r() ;
   }

   INT32 _clsDCBaseInfo::lock_w( INT32 millisec )
   {
      return _rwMutex.lock_w( millisec ) ;
   }

   INT32 _clsDCBaseInfo::release_w()
   {
      return _rwMutex.release_w() ;
   }

   INT32 _clsDCBaseInfo::updateFromBSON( BSONObj &obj, BOOLEAN checkGroups )
   {
      INT32 rc = SDB_OK ;

      //reset
      _reset() ;

      // begin to update
      _orgObj = obj ;

      BSONObj subObj ;
      BSONElement subEle ;
      BSONElement e = obj.getField( FIELD_NAME_DATACENTER ) ;
      if ( Object == e.type() )
      {
         subObj = e.embeddedObject() ;
         subEle = subObj.getField( FIELD_NAME_CLUSTERNAME ) ;
         _clusterName = subEle.valuestrsafe() ;

         subEle = subObj.getField( FIELD_NAME_BUSINESSNAME ) ;
         _businessName = subEle.valuestrsafe() ;

         subEle = subObj.getField( FIELD_NAME_ADDRESS ) ;
         _address = subEle.valuestrsafe() ;
      }
      else if ( !e.eoo() )
      {
         goto error ;
      }

      e = obj.getField( FIELD_NAME_ACTIVATED ) ;
      if ( Bool == e.type() )
      {
         _activated = e.Bool() ? TRUE : FALSE ;
      }
      else if ( !e.eoo() )
      {
         goto error ;
      }

      e = obj.getField( FIELD_NAME_READONLY ) ;
      if ( Bool == e.type() )
      {
         _readonly = e.Bool() ? TRUE : FALSE ;
      }
      else if ( !e.eoo() )
      {
         goto error ;
      }

      e = obj.getField( FIELD_NAME_CSUNIQUEHWM ) ;
      if ( e.eoo() )
      {
         _hasCsUniqueHWM = FALSE ;
      }
      else
      {
         if ( e.isNumber() )
         {
            _hasCsUniqueHWM = TRUE ;
            _csUniqueHWM = (utilCSUniqueID)e.numberInt() ;
         }
         else
         {
            goto error ;
         }
      }

      e = obj.getField( FIELD_NAME_CAT_VERSION ) ;
      if ( e.eoo() )
      {
         _catVersion = CATALOG_VERSION_V0 ;
      }
      else if ( e.isNumber() )
      {
         _catVersion = (UINT32)( e.numberInt() ) ;
      }
      else
      {
         PD_LOG( PDERROR, "Failed to parse field [%s]",
                 FIELD_NAME_CAT_VERSION ) ;
         goto error ;
      }

      rc = _recycleBinConf.fromBSON( obj ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update recycle info from BSON, "
                   "rc: %d", rc ) ;

   done:
      return rc ;
   error:
      PD_LOG( PDERROR, "Update base info failed, obj[%s] is invalid",
              obj.toString().c_str() ) ;
      if ( SDB_OK == rc )
      {
         rc = SDB_INVALIDARG ;
      }
      goto done ;
   }

   void _clsDCBaseInfo::setClusterName( const string &name )
   {
      _clusterName = name ;
   }

   void _clsDCBaseInfo::setBusinessName( const string &name )
   {
      _businessName = name ;
   }

   void _clsDCBaseInfo::setAddress( const string &addr )
   {
      _address = addr ;
   }

   void _clsDCBaseInfo::setAcitvated( BOOLEAN activated )
   {
      _activated = activated ;
   }

   void _clsDCBaseInfo::setReadonly( BOOLEAN readonly )
   {
      _readonly = readonly ;
   }

   const CHAR* _clsDCBaseInfo::getClusterName() const
   {
      return _clusterName.c_str() ;
   }

   const CHAR* _clsDCBaseInfo::getBusinessName() const
   {
      return _businessName.c_str() ;
   }

   const CHAR* _clsDCBaseInfo::getAddress() const
   {
      return _address.c_str() ;
   }

   /*
      _clsDCMgr implement
   */
   _clsDCMgr::_clsDCMgr ()
   {
   }

   _clsDCMgr::~_clsDCMgr()
   {
   }

   INT32 _clsDCMgr::initialize()
   {
      return SDB_OK ;
   }

   INT32 _clsDCMgr::updateDCBaseInfo( BSONObj &obj )
   {
      BOOLEAN hasLock = FALSE ;
      INT32 rc = SDB_OK ;

      rc = _baseInfo.lock_w() ;
      PD_RC_CHECK( rc, PDERROR, "Lock dc base info write failed, rc: %d",
                   rc ) ;
      hasLock = TRUE ;

      rc = _baseInfo.updateFromBSON( obj, FALSE ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      if ( hasLock )
      {
         _baseInfo.release_w() ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsDCMgr::updateDCBaseInfo( MsgOpReply* pRes )
   {
      INT32 rc = SDB_OK ;
      INT32 flag = 0 ;
      INT64 contextID = 0 ;
      INT32 startFrom = 0 ;
      INT32 numReturned = 0 ;
      vector< BSONObj > objList ;
      BSONObj objInfo ;

      rc = msgExtractReply( (CHAR *)pRes, &flag, &contextID, &startFrom,
                            &numReturned, objList ) ;
      PD_RC_CHECK( rc, PDERROR, "Extract query reply msg failed, rc: %d",
                   rc ) ;

      rc = flag ;
      if ( SDB_DMS_EOC == rc )
      {
         rc = SDB_OK ;
      }
      else if ( rc )
      {
         PD_LOG( PDWARNING, "Recieve failed dc base info query reply, "
                 "flag: %d", rc ) ;
         goto error ;
      }

      for ( UINT32 i = 0 ; i < objList.size() ; ++i )
      {
         objInfo = objList[ i ] ;
         break ;
      }
      rc = updateDCBaseInfo( objInfo ) ;
      PD_RC_CHECK( rc, PDERROR, "Update dc base info failed, rc: %d", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   utilRecycleBinConf _clsDCBaseInfo::getRecycleBinConf()
   {
      ossScopedRWLock _lock( &_rwMutex, SHARED ) ;
      return _recycleBinConf ;
   }

   void _clsDCBaseInfo::setRecycleBinConf( const utilRecycleBinConf &conf )
   {
      ossScopedRWLock _lock( &_rwMutex, EXCLUSIVE ) ;
      _recycleBinConf = conf ;
   }

}

