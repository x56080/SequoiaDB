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

   Source File Name = rtnRecycleBinManager.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for recycle bin manager.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/03/2021  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTN_RECYCLE_BIN_MANAGER_HPP__
#define RTN_RECYCLE_BIN_MANAGER_HPP__

#include "oss.hpp"
#include "ossLatch.hpp"
#include "rtn.hpp"
#include "utilRecycleBinConf.hpp"
#include "utilRecycleItem.hpp"

namespace engine
{

   /*
      _rtnRecycleBinManager define
    */
   class _rtnRecycleBinManager : public SDBObject
   {
   public:
      _rtnRecycleBinManager() ;
      virtual ~_rtnRecycleBinManager() ;

      INT32 init() ;

      // set config of recycle bin
      OSS_INLINE void setConf( const utilRecycleBinConf &conf )
      {
         ossScopedLock _lock( &_confLatch, EXCLUSIVE ) ;
         _conf = conf ;
         _isConfValid = TRUE ;
      }

      // get config of recycle bin
      OSS_INLINE utilRecycleBinConf getConf()
      {
         ossScopedLock _lock( &_confLatch, SHARED ) ;
         return _conf ;
      }

      // set configure is invalid
      OSS_INLINE void setConfInvalid()
      {
         ossScopedLock _lock( &_confLatch, EXCLUSIVE ) ;
         _isConfValid = FALSE  ;
      }

      // get if configure is valid
      OSS_INLINE BOOLEAN isConfValid()
      {
         ossScopedLock _lock( &_confLatch, SHARED ) ;
         return _isConfValid ;
      }

      // get recycle item by recycle name
      INT32 getItem( const CHAR *recycleName,
                     pmdEDUCB *cb,
                     utilRecycleItem &item ) ;
      // get recycle item by origin ID
      INT32 getItem( utilGlobalID originID,
                     pmdEDUCB *cb,
                     utilRecycleItem &item ) ;
      // get recycle items by given matcher and order
      INT32 getItems( const bson::BSONObj &matcher,
                      const bson::BSONObj &orderBy,
                      const bson::BSONObj &hint,
                      INT64 numToReturn,
                      pmdEDUCB *cb,
                      UTIL_RECY_ITEM_LIST &itemList )
      {
         return _getItems( matcher, orderBy, hint, numToReturn, cb, itemList ) ;
      }

      // get recycle items in context by given matcher and order
      INT32 getItems( const bson::BSONObj &matcher,
                      const bson::BSONObj &orderBy,
                      const bson::BSONObj &hint,
                      INT64 numToReturn,
                      pmdEDUCB *cb,
                      INT64 &contextID )
      {
         return _getItems( matcher, orderBy, hint, numToReturn, cb, contextID ) ;
      }

      // count all recycle items
      OSS_INLINE INT32 countAllItems( pmdEDUCB *cb,
                                      INT64 &count )
      {
         bson::BSONObj dummy ;
         return _countItems( dummy, cb, count ) ;
      }

      // count recycle items by matcher
      OSS_INLINE INT32 countItems( const bson::BSONObj &matcher,
                                   pmdEDUCB *cb,
                                   INT64 &count )
      {
         return _countItems( matcher, cb, count ) ;
      }

   protected:
      // get collection to save recycle items
      virtual const CHAR *_getRecyItemCL() const = 0 ;

      // get recycle item in BSON by recycle name
      INT32 _getItemObject( const CHAR *recycleName,
                            pmdEDUCB *cb,
                            bson::BSONObj &object ) ;
      // get recycle item in BSON by origin ID
      INT32 _getItemObject( utilGlobalID originID,
                            pmdEDUCB *cb,
                            bson::BSONObj &object ) ;
      // get recycle item in BSON by matcher
      INT32 _getItemObject( const bson::BSONObj &matcher,
                            pmdEDUCB *cb,
                            bson::BSONObj &object ) ;

      // get recycle items in context by matcher and order
      INT32 _getItems( const bson::BSONObj &matcher,
                       const bson::BSONObj &orderBy,
                       const bson::BSONObj &hint,
                       INT64 numToReturn,
                       pmdEDUCB *cb,
                       INT64 &contextID ) ;
      // get recycle items in list by matcher and order
      INT32 _getItems( const bson::BSONObj &matcher,
                       const bson::BSONObj &orderBy,
                       const bson::BSONObj &hint,
                       INT64 numToReturn,
                       pmdEDUCB *cb,
                       UTIL_RECY_ITEM_LIST &itemList ) ;

      // count recycle items by matcher
      INT32 _countItems( const bson::BSONObj &matcher,
                         pmdEDUCB *cb,
                         INT64 &count ) ;

   protected:
      // latch to protect configuration
      ossSpinSLatch        _confLatch ;
      // cache of recycle bin information
      utilRecycleBinConf   _conf ;
      // indicates if configure is valid
      BOOLEAN              _isConfValid ;

      SDB_RTNCB * _rtnCB ;
      SDB_DMSCB * _dmsCB ;
      SDB_DPSCB * _dpsCB ;
   } ;

   typedef class _rtnRecycleBinManager rtnRecycleBinManager ;

}

#endif // RTN_RECYCLE_BIN_MANAGER_HPP__
