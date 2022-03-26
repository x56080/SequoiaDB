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

   Source File Name = coordCMDEventHandler.hpp

   Descriptive Name = Coord Command Event Handler

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/27/2017  XJH Init
   Last Changed =

*******************************************************************************/

#ifndef COORD_CMD_EVENT_HANDLER_HPP__
#define COORD_CMD_EVENT_HANDLER_HPP__

#include "coordCommandBase.hpp"
#include "coordFactory.hpp"

using namespace bson ;

namespace engine
{

   /*
      _coordCMDArguments define
   */
   class _coordCMDArguments : public SDBObject
   {
      public :
         _coordCMDArguments () { _pBuf = NULL ; }

         virtual ~_coordCMDArguments () {}

         /* A copy of the query object */
         BSONObj _boQuery ;

         /* Name of the catalog target to be updated */
         string _targetName ;

         /* ignore error return codes */
         SET_RC _ignoreRCList ;

         /* retry when error returned */
         SET_RC _retryRCList ;

         /* the return context buf pointer */
         rtnContextBuf *_pBuf ;

         CoordGroupList _groupList ;
   } ;
   typedef _coordCMDArguments coordCMDArguments ;

   /*
      _coordCMDEventHandler implement
    */
   class _coordCMDEventHandler
   {
   public:
      _coordCMDEventHandler() {}
      virtual ~_coordCMDEventHandler() {}

      virtual const CHAR *getName() const = 0 ;

      virtual INT32 parseCatReturn( coordCMDArguments *pArgs,
                                    const std::vector<bson::BSONObj> &cataObjs )
      {
         return SDB_OK ;
      }

      virtual INT32 parseCatP2Return( coordCMDArguments *pArgs,
                                      const std::vector<bson::BSONObj> &cataObjs )
      {
         return SDB_OK ;
      }

      virtual BOOLEAN needRewriteDataMsg()
      {
         return FALSE ;
      }

      virtual INT32 rewriteDataMsg( bson::BSONObjBuilder &queryBuilder,
                                    bson::BSONObjBuilder &hintBuilder )
      {
         return SDB_OK ;
      }

      virtual INT32 onBeginEvent( coordResource *pResource,
                                  coordCMDArguments *pArgs,
                                  pmdEDUCB *cb )
      {
         return SDB_OK ;
      }

      virtual INT32 onDataP1Event( SDB_EVENT_OCCUR_TYPE type,
                                   coordResource *pResource,
                                   coordCMDArguments *pArgs,
                                   pmdEDUCB *cb )
      {
         return SDB_OK ;
      }

      virtual INT32 onDataP2Event( SDB_EVENT_OCCUR_TYPE type,
                                   coordResource *pResource,
                                   coordCMDArguments *pArgs,
                                   pmdEDUCB *cb )
      {
         return SDB_OK ;
      }

      virtual INT32 onCommitEvent( coordResource *pResource,
                                   coordCMDArguments *pArgs,
                                   pmdEDUCB *cb )
      {
         return SDB_OK ;
      }

      virtual INT32 onRollbackEvent( coordResource *pResource,
                                     coordCMDArguments *pArgs,
                                     pmdEDUCB *cb )
      {
         return SDB_OK ;
      }
   } ;

   typedef class _coordCMDEventHandler coordCMDEventHandler ;
   typedef ossPoolList< coordCMDEventHandler * > COORD_CMD_EVENT_HANDLER_LIST ;
   typedef COORD_CMD_EVENT_HANDLER_LIST::iterator COORD_CMD_EVENT_HANDLER_LIST_IT ;

   typedef ossPoolList< ossPoolString >   COORD_GLOBIDXCL_NAME_LIST ;
   typedef COORD_GLOBIDXCL_NAME_LIST::iterator
                                          COORD_GLOBIDXCL_NAME_LIST_IT ;
   typedef COORD_GLOBIDXCL_NAME_LIST::const_iterator
                                          COORD_GLOBIDXCL_NAME_LIST_CIT ;

   /*
      _coordDataCMDHelper define
    */
   class _coordDataCMDHelper
   {
   public:
      _coordDataCMDHelper() {}
      ~_coordDataCMDHelper() {}

      INT32 dropCL( coordResource *resource,
                    const CHAR *clName,
                    BOOLEAN skipRecycleBin,
                    BOOLEAN ignoreLock,
                    pmdEDUCB *cb ) ;
      INT32 truncateCL( coordResource *resource,
                        const CHAR *clName,
                        BOOLEAN skipRecycleBin,
                        BOOLEAN ignoreLock,
                        pmdEDUCB *cb ) ;
      INT32 alterCL( coordResource *resource,
                     const CHAR *clName,
                     const bson::BSONObj &options,
                     pmdEDUCB *cb ) ;
      INT32 dropCS( coordResource *resource,
                    const CHAR *csName,
                    BOOLEAN skipRecycleBin,
                    BOOLEAN ignoreLock,
                    pmdEDUCB *cb ) ;
   } ;
   typedef class _coordDataCMDHelper coordDataCMDHelper ;

   /*
      _coordCMDGlobIdxHandler define
    */
   class _coordCMDGlobIdxHandler : public _coordCMDEventHandler
   {
   public:
      _coordCMDGlobIdxHandler() {}
      virtual ~_coordCMDGlobIdxHandler() {}

      virtual INT32 parseCatReturn( coordCMDArguments *pArgs,
                                    const std::vector<bson::BSONObj> &cataObjs ) ;

      virtual INT32 onBeginEvent( coordResource *pResource,
                                  coordCMDArguments *pArgs,
                                  pmdEDUCB *cb ) ;

      virtual INT32 onDataP1Event( SDB_EVENT_OCCUR_TYPE type,
                                   coordResource *pResource,
                                   coordCMDArguments *pArgs,
                                   pmdEDUCB *cb ) ;

   protected:
      INT32 _repairCheckGlobIdxCLs( coordResource *resource,
                                    BOOLEAN enableRepairCheck,
                                    pmdEDUCB *cb ) ;

   protected:
      COORD_GLOBIDXCL_NAME_LIST _globalIndexes ;
   } ;

   typedef class _coordCMDGlobIdxHandler coordCMDGlobIdxHandler ;

   /*
      _coordDropGlobIdxHandler define
    */
   class _coordDropGlobIdxHandler : public _coordCMDGlobIdxHandler
   {
   public:
      _coordDropGlobIdxHandler() {}
      virtual ~_coordDropGlobIdxHandler() {}

      virtual const CHAR *getName() const
      {
         return "drop global index" ;
      }

      virtual INT32 onDataP2Event( SDB_EVENT_OCCUR_TYPE type,
                                   coordResource *pResource,
                                   coordCMDArguments *pArgs,
                                   pmdEDUCB *cb ) ;

   protected:
      INT32 _dropGlobIdxCLs( coordResource *resource,
                             pmdEDUCB *cb ) ;
   } ;

   typedef class _coordDropGlobIdxHandler coordDropGlobIdxHandler ;

   /*
      _coordTruncGlobIdxHandler define
    */
   class _coordTruncGlobIdxHandler : public _coordCMDGlobIdxHandler
   {
   public:
      _coordTruncGlobIdxHandler() {}
      virtual ~_coordTruncGlobIdxHandler() {}

      virtual const CHAR *getName() const
      {
         return "truncate global index" ;
      }

      virtual INT32 onDataP2Event( SDB_EVENT_OCCUR_TYPE type,
                                   coordResource *resource,
                                   coordCMDArguments *arguments,
                                   pmdEDUCB *cb ) ;

   protected:
      INT32 _truncGlobIdxCLs( coordResource *resource,
                              pmdEDUCB *cb ) ;
   } ;

   typedef class _coordTruncGlobIdxHandler coordTruncGlobIdxHandler ;

   /*
      _coordCMDRecycleHandler define
    */
   class _coordCMDRecycleHandler : public _coordCMDEventHandler
   {
   public:
      _coordCMDRecycleHandler() {}
      virtual ~_coordCMDRecycleHandler() {}

      virtual const CHAR *getName() const
      {
         return "recycle" ;
      }

      virtual INT32 parseCatReturn( coordCMDArguments *pArgs,
                                    const std::vector<bson::BSONObj> &cataObjs ) ;

      virtual BOOLEAN needRewriteDataMsg()
      {
         return _recycleOptions.isEmpty() ? FALSE : TRUE ;
      }

      virtual INT32 rewriteDataMsg( bson::BSONObjBuilder &queryBuilder,
                                    bson::BSONObjBuilder &hintBuilder ) ;

      virtual INT32 onBeginEvent( coordResource *resource,
                                   coordCMDArguments *arguments,
                                   pmdEDUCB *cb ) ;

      virtual INT32 onDataP1Event( SDB_EVENT_OCCUR_TYPE type,
                                   coordResource *resource,
                                   coordCMDArguments *arguments,
                                   pmdEDUCB *cb ) ;

   protected:
      INT32 _dropRecycleItem( coordResource *resource,
                              const CHAR *recycleName,
                              BOOLEAN ignoreIfNotExists,
                              BOOLEAN isRecursive,
                              BOOLEAN isEnforced,
                              BOOLEAN ignoreLock,
                              pmdEDUCB *cb ) ;

      INT32 _dropRecycleItems( coordResource *resource,
                               BOOLEAN ignoreIfNotExists,
                               BOOLEAN isRecursive,
                               BOOLEAN isEnforced,
                               BOOLEAN ignoreLock,
                               pmdEDUCB *cb ) ;

   protected:
      bson::BSONObj            _recycleOptions ;
      UTIL_RECY_ITEM_NAME_LIST _droppingItems ;
   } ;

   typedef class _coordCMDRecycleHandler coordCMDRecycleHandler ;

}

#endif // COORD_CMD_EVENT_HANDLER_HPP__
