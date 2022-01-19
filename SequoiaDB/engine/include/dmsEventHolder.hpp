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

   Source File Name = dmsEventHolder.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SDB_DMS_EVENT_HOLDER_HPP_
#define SDB_DMS_EVENT_HOLDER_HPP_

#include "dmsEventHandler.hpp"

namespace engine
{
   class _dmsStorageUnit;
   typedef class _dmsStorageUnit dmsStorageUnit;

   class _dmsCacheHolder;
   typedef class _dmsCacheHolder dmsCacheHolder;

   class _dmsEventHolder : public _IDmsEventHolder
   {
      public :
         _dmsEventHolder( dmsStorageUnit *su ) ;

         virtual ~_dmsEventHolder () ;

         virtual void regHandler ( _IDmsEventHandler *pHandler ) ;

         virtual void unregHandler ( _IDmsEventHandler *pHandler ) ;

         virtual void unregAllHandlers () ;

         virtual INT32 onCreateCS ( UINT32 mask,
                                    pmdEDUCB *cb,
                                    SDB_DPSCB *dpsCB ) ;

         virtual INT32 onLoadCS ( UINT32 mask,
                                  pmdEDUCB *cb,
                                  SDB_DPSCB *dpsCB ) ;

         virtual INT32 onUnloadCS ( UINT32 mask,
                                    pmdEDUCB *cb,
                                    SDB_DPSCB *dpsCB ) ;

         virtual INT32 onRenameCS ( UINT32 mask,
                                    const CHAR *pOldCSName,
                                    const CHAR *pNewCSName,
                                    pmdEDUCB *cb,
                                    SDB_DPSCB *dpsCB ) ;

         virtual INT32 onDropCS ( UINT32 mask,
                                  pmdEDUCB *cb,
                                  SDB_DPSCB *dpsCB ) ;

         virtual INT32 onCreateCL ( UINT32 mask,
                                    const dmsEventCLItem &clItem,
                                    pmdEDUCB *cb,
                                    SDB_DPSCB *dpsCB ) ;

         virtual INT32 onRenameCL ( UINT32 mask,
                                    const dmsEventCLItem &clItem,
                                    const CHAR *pNewCLName,
                                    pmdEDUCB *cb,
                                    SDB_DPSCB *dpsCB ) ;

         virtual INT32 onTruncateCL ( UINT32 mask,
                                      const dmsEventCLItem &clItem,
                                      UINT32 newCLLID,
                                      pmdEDUCB *cb,
                                      SDB_DPSCB *dpsCB ) ;

         virtual INT32 onDropCL ( UINT32 mask,
                                  const dmsEventCLItem &clItem,
                                  pmdEDUCB *cb,
                                  SDB_DPSCB *dpsCB ) ;

         virtual INT32 onCreateIndex ( UINT32 mask,
                                       const dmsEventCLItem &clItem,
                                       const dmsEventIdxItem &idxItem,
                                       pmdEDUCB *cb,
                                       SDB_DPSCB *dpsCB ) ;

         virtual INT32 onRebuildIndex ( UINT32 mask,
                                        const dmsEventCLItem &clItem,
                                        const dmsEventIdxItem &idxItem,
                                        pmdEDUCB *cb,
                                        SDB_DPSCB *dpsCB ) ;

         virtual INT32 onDropIndex ( UINT32 mask,
                                     const dmsEventCLItem &clItem,
                                     const dmsEventIdxItem &idxItem,
                                     pmdEDUCB *cb,
                                     SDB_DPSCB *dpsCB ) ;

         virtual INT32 onLinkCL ( UINT32 mask,
                                  const dmsEventCLItem &clItem,
                                  const CHAR *pMainCLName,
                                  pmdEDUCB *cb,
                                  SDB_DPSCB *dpsCB ) ;

         virtual INT32 onUnlinkCL ( UINT32 mask,
                                    const dmsEventCLItem &clItem,
                                    const CHAR *pMainCLName,
                                    pmdEDUCB *cb,
                                    SDB_DPSCB *dpsCB ) ;

         virtual INT32 onClearSUCaches ( UINT32 mask ) ;

         virtual INT32 onClearCLCaches ( UINT32 mask,
                                         const dmsEventCLItem &clItem ) ;

         virtual INT32 onChangeSUCaches ( UINT32 mask ) ;

         virtual const CHAR *getCSName () const ;

         virtual UINT32 getSUID () const ;

         virtual UINT32 getSULID () const ;

         OSS_INLINE virtual void setCacheHolder ( dmsCacheHolder *pCacheHolder )
         {
            _pCacheHolder = pCacheHolder ;
         }

      protected :
         typedef ossPoolList<_IDmsEventHandler *> HANDLER_LIST ;

         dmsStorageUnit *        _su ;
         dmsCacheHolder *        _pCacheHolder ;
         HANDLER_LIST            _handlers ;
   } ;

   typedef class _dmsEventHolder dmsEventHolder;
} // namespace engine


#endif//SDB_DMS_EVENT_HOLDER_HPP_