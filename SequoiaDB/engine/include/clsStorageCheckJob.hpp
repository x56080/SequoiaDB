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

   Source File Name = clsStorageCheckJob.hpp

   Descriptive Name = Storage Checking Job Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/

#ifndef CLS_STORAGE_CHECK_JOB_HPP__
#define CLS_STORAGE_CHECK_JOB_HPP__

#include "rtnBackgroundJobBase.hpp"
#include "dmsEventHandler.hpp"

namespace engine
{
   // One hour interval
   #define STORAGE_CHECK_UNIT_INTERVAL ( OSS_ONE_SEC * 60 * 60 )

   /*
      CLS_CHK_CATA_STATUS define
   */
   enum CLS_CHK_CATA_STATUS
   {
      CLS_CHK_CATA_NONE = 0,
      CLS_CHK_CATA_SUC,
      CLS_CHK_CATA_FAILED
   } ;

   /*
      clsStorageEventHandler define
   */
   class _clsStorageEventHandler : public _IDmsEventHandler, public SDBObject
   {
      public:
         _clsStorageEventHandler() ;
         virtual ~_clsStorageEventHandler() ;

      public:
         virtual INT32 onDropCL ( SDB_EVENT_OCCUR_TYPE type,
                                  IDmsEventHolder *pEventHolder,
                                  IDmsSUCacheHolder *pCacheHolder,
                                  const dmsEventCLItem &clItem,
                                  dmsDropCLOptions *options,
                                  pmdEDUCB *cb,
                                  SDB_DPSCB *dpsCB ) ;

         virtual UINT32 getMask () const { return DMS_EVENT_MASK_CLS ; }

         virtual const CHAR *getName() const { return "cluster storage event handler" ; }

   } ;
   typedef _clsStorageEventHandler clsStorageEventHandler ;

   /*
    *  _clsStorageCheckJob define
    */
   class _clsStorageCheckJob : public _rtnBaseJob
   {
      public:
      _clsStorageCheckJob () ;
      virtual ~_clsStorageCheckJob () ;

   public:
      virtual RTN_JOB_TYPE type () const { return RTN_JOB_CLS_STORAGE_CHECK ; }

      virtual const CHAR* name () const { return "DmsCheck" ; }

      virtual BOOLEAN muteXOn ( const _rtnBaseJob *pOther ) { return FALSE ; }

      virtual INT32 doit () ;

   protected:
      INT32       _checkCSItem( monCSName csName,
                                BOOLEAN checkUniqueID = TRUE,
                                UINT64 noAccessTime = 0,
                                INT32 *pCheckCataStatus = NULL ) ;

      INT32       _checkEmptyCSItem( const monCSName &csName,
                                     BOOLEAN checkUniqueID = TRUE,
                                     UINT64 noAccessTime = 0 ) ;

      INT32       _checkCLItem( const CHAR* clName,
                                utilCLUniqueID clUniqueID,
                                INT32 *pCheckCataStatus = NULL ) ;

      void        _checkCS() ;

      void        _checkEmptyCS() ;

      void        _checkNotifyItem() ;

      INT32       _checkCLExist( const CHAR *clName,
                                 utilCLUniqueID clUniqueID,
                                 BOOLEAN &exist ) ;

   private:
      UINT64      _lastCheckCSTick ;
      UINT64      _lastCheckEmptyCSTick ;

   } ;

   typedef _clsStorageCheckJob clsStorageCheckJob ;

   INT32 startStorageCheckJob ( EDUID *pEDUID ) ;

}

#endif //CLS_STORAGE_CHECK_JOB_HPP__

