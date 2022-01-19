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

   Source File Name = dmsEventHolder.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/

#include "dmsEventHolder.hpp"
#include "dmsTrace.hpp"
#include "dmsStorageUnit.hpp"
#include "dmsCacheHolder.hpp"

namespace engine
{
   /*
      _dmsEventHolder implement
    */
   _dmsEventHolder::_dmsEventHolder ( dmsStorageUnit *su )
   {
      SDB_ASSERT( su, "Storage Unit is no valid" ) ;
      _su = su ;
      unregAllHandlers() ;
   }

   _dmsEventHolder::~_dmsEventHolder ()
   {
      unregAllHandlers() ;
   }

   void _dmsEventHolder::regHandler ( _IDmsEventHandler *pHandler )
   {
      if ( !pHandler )
      {
         return ;
      }

      for ( HANDLER_LIST::iterator iter = _handlers.begin() ;
            iter != _handlers.end() ;
            ++ iter )
      {
         if ( *iter == pHandler )
         {
            return ;
         }
      }

      _handlers.push_back( pHandler ) ;
   }

   void _dmsEventHolder::unregHandler ( _IDmsEventHandler *pHandler )
   {
      if ( pHandler )
      {
         return ;
      }

      for ( HANDLER_LIST::iterator iter = _handlers.begin() ;
            iter != _handlers.end() ;
            ++ iter )
      {
         if ( *iter == pHandler )
         {
            _handlers.erase( iter ) ;
            break ;
         }
      }
   }

   void _dmsEventHolder::unregAllHandlers ()
   {
      _handlers.clear() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSEVTHLD_ONCRTCS, "_dmsEventHolder::onCreateCS" )
   INT32 _dmsEventHolder::onCreateCS ( UINT32 mask, pmdEDUCB *cb, SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSEVTHLD_ONCRTCS ) ;

      // Event could not be handled in main thread
      if ( !cb || cb->getType() == EDU_TYPE_MAIN )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      for ( HANDLER_LIST::iterator iter = _handlers.begin() ;
            iter != _handlers.end() ;
            ++ iter )
      {
         _IDmsEventHandler *pHandler = (*iter) ;
         if ( pHandler && ( pHandler->getMask() & mask ) )
         {
            INT32 tmprc = pHandler->onCreateCS( this, _pCacheHolder, cb, dpsCB ) ;
            if ( SDB_OK != tmprc )
            {
               rc = tmprc ;
            }
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__DMSEVTHLD_ONCRTCS, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSEVTHLD_ONLOADCS, "_dmsEventHolder::onLoadCS" )
   INT32 _dmsEventHolder::onLoadCS ( UINT32 mask, pmdEDUCB *cb, SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSEVTHLD_ONLOADCS ) ;

      // Event could not be handled in main thread
      if ( !cb || cb->getType() == EDU_TYPE_MAIN )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      for ( HANDLER_LIST::iterator iter = _handlers.begin() ;
            iter != _handlers.end() ;
            ++ iter )
      {
         _IDmsEventHandler *pHandler = (*iter) ;
         if ( pHandler && ( pHandler->getMask() & mask ) )
         {
            INT32 tmprc = pHandler->onLoadCS( this, _pCacheHolder, cb, dpsCB ) ;
            if ( SDB_OK != tmprc )
            {
               rc = tmprc ;
            }
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__DMSEVTHLD_ONLOADCS, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSEVTHLD_ONUNLOADCS, "_dmsEventHolder::onUnloadCS" )
   INT32 _dmsEventHolder::onUnloadCS ( UINT32 mask, pmdEDUCB *cb, SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSEVTHLD_ONUNLOADCS ) ;

      // Event could not be handled in main thread
      if ( !cb || cb->getType() == EDU_TYPE_MAIN )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      for ( HANDLER_LIST::iterator iter = _handlers.begin() ;
            iter != _handlers.end() ;
            ++ iter )
      {
         _IDmsEventHandler *pHandler = (*iter) ;
         if ( pHandler && ( pHandler->getMask() & mask ) )
         {
            INT32 tmprc = pHandler->onUnloadCS( this, _pCacheHolder, cb, dpsCB ) ;
            if ( SDB_OK != tmprc )
            {
               rc = tmprc ;
            }
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__DMSEVTHLD_ONUNLOADCS, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSEVTHLD_ONRENAMECS, "_dmsEventHolder::onRenameCS" )
   INT32 _dmsEventHolder::onRenameCS ( UINT32 mask, const CHAR *pOldCSName,
                                       const CHAR *pNewCSName, pmdEDUCB *cb,
                                       SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSEVTHLD_ONRENAMECS ) ;

      // Event could not be handled in main thread
      if ( !cb || cb->getType() == EDU_TYPE_MAIN )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      for ( HANDLER_LIST::iterator iter = _handlers.begin() ;
            iter != _handlers.end() ;
            ++ iter )
      {
         _IDmsEventHandler *pHandler = (*iter) ;
         if ( pHandler && ( pHandler->getMask() & mask ) )
         {
            INT32 tmprc = pHandler->onRenameCS( this, _pCacheHolder, pOldCSName,
                                                pNewCSName, cb, dpsCB ) ;
            if ( SDB_OK != tmprc )
            {
               rc = tmprc ;
            }
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__DMSEVTHLD_ONRENAMECS, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSEVTHLD_ONDROPCS, "_dmsEventHolder::onDropCS" )
   INT32 _dmsEventHolder::onDropCS ( UINT32 mask, pmdEDUCB *cb, SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSEVTHLD_ONDROPCS ) ;

      // Event could not be handled in main thread
      if ( !cb || cb->getType() == EDU_TYPE_MAIN )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      for ( HANDLER_LIST::iterator iter = _handlers.begin() ;
            iter != _handlers.end() ;
            ++ iter )
      {
         _IDmsEventHandler *pHandler = (*iter) ;
         if ( pHandler && ( pHandler->getMask() & mask ) )
         {
            INT32 tmprc = pHandler->onDropCS( this, _pCacheHolder,
                                              cb, dpsCB ) ;
            if ( SDB_OK != tmprc )
            {
               rc = tmprc ;
            }
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__DMSEVTHLD_ONDROPCS, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSEVTHLD_ONCRTCL, "_dmsEventHolder::onCreateCL" )
   INT32 _dmsEventHolder::onCreateCL ( UINT32 mask,
                                       const dmsEventCLItem &clItem,
                                       pmdEDUCB *cb,
                                       SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSEVTHLD_ONCRTCL ) ;

      // Event could not be handled in main thread
      if ( !cb || cb->getType() == EDU_TYPE_MAIN )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      for ( HANDLER_LIST::iterator iter = _handlers.begin() ;
            iter != _handlers.end() ;
            ++ iter )
      {
         _IDmsEventHandler *pHandler = (*iter) ;
         if ( pHandler && ( pHandler->getMask() & mask ) )
         {
            INT32 tmprc = pHandler->onCreateCL( this, _pCacheHolder, clItem,
                                                cb, dpsCB ) ;
            if ( SDB_OK != tmprc )
            {
               rc = tmprc ;
            }
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__DMSEVTHLD_ONCRTCL, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSEVTHLD_ONRENAMECL, "_dmsEventHolder::onRenameCL" )
   INT32 _dmsEventHolder::onRenameCL ( UINT32 mask,
                                       const dmsEventCLItem &clItem,
                                       const CHAR *pNewCLName,
                                       pmdEDUCB *cb,
                                       SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSEVTHLD_ONRENAMECL ) ;

      // Event could not be handled in main thread
      if ( !cb || cb->getType() == EDU_TYPE_MAIN )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      for ( HANDLER_LIST::iterator iter = _handlers.begin() ;
            iter != _handlers.end() ;
            ++ iter )
      {
         _IDmsEventHandler *pHandler = (*iter) ;
         if ( pHandler && ( pHandler->getMask() & mask ) )
         {
            INT32 tmprc = pHandler->onRenameCL( this, _pCacheHolder, clItem,
                                                pNewCLName, cb, dpsCB ) ;
            if ( SDB_OK != tmprc )
            {
               rc = tmprc ;
            }
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__DMSEVTHLD_ONRENAMECL, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSEVTHLD_ONTRUNCCL, "_dmsEventHolder::onTruncateCL" )
   INT32 _dmsEventHolder::onTruncateCL ( UINT32 mask,
                                         const dmsEventCLItem &clItem,
                                         UINT32 newCLLID,
                                         pmdEDUCB *cb,
                                         SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSEVTHLD_ONTRUNCCL ) ;

      // Event could not be handled in main thread
      if ( !cb || cb->getType() == EDU_TYPE_MAIN )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      for ( HANDLER_LIST::iterator iter = _handlers.begin() ;
            iter != _handlers.end() ;
            ++ iter )
      {
         _IDmsEventHandler *pHandler = (*iter) ;
         if ( pHandler && ( pHandler->getMask() & mask ) )
         {
            INT32 tmprc = pHandler->onTruncateCL( this, _pCacheHolder, clItem,
                                                  newCLLID, cb, dpsCB ) ;
            if ( SDB_OK != tmprc )
            {
               rc = tmprc ;
            }
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__DMSEVTHLD_ONTRUNCCL, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSEVTHLD_ONDROPCL, "_dmsEventHolder::onDropCL" )
   INT32 _dmsEventHolder::onDropCL ( UINT32 mask,
                                     const dmsEventCLItem &clItem,
                                     pmdEDUCB *cb,
                                     SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSEVTHLD_ONDROPCL ) ;

      // Event could not be handled in main thread
      if ( !cb || cb->getType() == EDU_TYPE_MAIN )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      for ( HANDLER_LIST::iterator iter = _handlers.begin() ;
            iter != _handlers.end() ;
            ++ iter )
      {
         _IDmsEventHandler *pHandler = (*iter) ;
         if ( pHandler && ( pHandler->getMask() & mask ) )
         {
            INT32 tmprc = pHandler->onDropCL( this, _pCacheHolder, clItem,
                                              cb, dpsCB ) ;
            if ( SDB_OK != tmprc )
            {
               rc = tmprc ;
            }
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__DMSEVTHLD_ONDROPCL, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSEVTHLD_ONCRTIDX, "_dmsEventHolder::onCreateIndex" )
   INT32 _dmsEventHolder::onCreateIndex ( UINT32 mask,
                                          const dmsEventCLItem &clItem,
                                          const dmsEventIdxItem &idxItem,
                                          pmdEDUCB *cb,
                                          SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSEVTHLD_ONCRTIDX ) ;

      // Event could not be handled in main thread
      if ( !cb || cb->getType() == EDU_TYPE_MAIN )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      for ( HANDLER_LIST::iterator iter = _handlers.begin() ;
            iter != _handlers.end() ;
            ++ iter )
      {
         _IDmsEventHandler *pHandler = (*iter) ;
         if ( pHandler && ( pHandler->getMask() & mask ) )
         {
            INT32 tmprc = pHandler->onCreateIndex( this, _pCacheHolder, clItem,
                                                   idxItem, cb, dpsCB ) ;
            if ( SDB_OK != tmprc )
            {
               rc = tmprc ;
            }
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__DMSEVTHLD_ONCRTIDX, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSEVTHLD_ONREBUILDIDX, "_dmsEventHolder::onRebuildIndex" )
   INT32 _dmsEventHolder::onRebuildIndex ( UINT32 mask,
                                           const dmsEventCLItem &clItem,
                                           const dmsEventIdxItem &idxItem,
                                           pmdEDUCB *cb,
                                           SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSEVTHLD_ONREBUILDIDX ) ;

      // Event could not be handled in main thread
      if ( !cb || cb->getType() == EDU_TYPE_MAIN )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      for ( HANDLER_LIST::iterator iter = _handlers.begin() ;
            iter != _handlers.end() ;
            ++ iter )
      {
         _IDmsEventHandler *pHandler = (*iter) ;
         if ( pHandler && ( pHandler->getMask() & mask ) )
         {
            INT32 tmprc = pHandler->onRebuildIndex( this, _pCacheHolder,
                                                    clItem, idxItem, cb,
                                                    dpsCB ) ;
            if ( SDB_OK != tmprc )
            {
               rc = tmprc ;
            }
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__DMSEVTHLD_ONREBUILDIDX, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSEVTHLD_ONDROPIDX, "_dmsEventHolder::onDropIndex" )
   INT32 _dmsEventHolder::onDropIndex ( UINT32 mask,
                                        const dmsEventCLItem &clItem,
                                        const dmsEventIdxItem &idxItem,
                                        pmdEDUCB *cb,
                                        SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSEVTHLD_ONDROPIDX ) ;

      // Event could not be handled in main thread
      if ( !cb || cb->getType() == EDU_TYPE_MAIN )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      for ( HANDLER_LIST::iterator iter = _handlers.begin() ;
            iter != _handlers.end() ;
            ++ iter )
      {
         _IDmsEventHandler *pHandler = (*iter) ;
         if ( pHandler && ( pHandler->getMask() & mask ) )
         {
            INT32 tmprc = pHandler->onDropIndex( this, _pCacheHolder, clItem,
                                                 idxItem, cb, dpsCB ) ;
            if ( SDB_OK != tmprc )
            {
               rc = tmprc ;
            }
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__DMSEVTHLD_ONDROPIDX, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSEVTHLD_ONLINKCL, "_dmsEventHolder::onLinkCL" )
   INT32 _dmsEventHolder::onLinkCL ( UINT32 mask,
                                     const dmsEventCLItem &clItem,
                                     const CHAR *pMainCLName,
                                     _pmdEDUCB *cb,
                                     SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSEVTHLD_ONLINKCL ) ;

      // Event could not be handled in main thread
      if ( !cb || cb->getType() == EDU_TYPE_MAIN )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      for ( HANDLER_LIST::iterator iter = _handlers.begin() ;
            iter != _handlers.end() ;
            ++ iter )
      {
         _IDmsEventHandler *pHandler = (*iter) ;
         if ( pHandler && ( pHandler->getMask() & mask ) )
         {
            INT32 tmprc = pHandler->onLinkCL( this, _pCacheHolder, clItem,
                                              pMainCLName, cb, dpsCB ) ;
            if ( SDB_OK != tmprc )
            {
               rc = tmprc ;
            }
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__DMSEVTHLD_ONLINKCL, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSEVTHLD_ONUNLINKCL, "_dmsEventHolder::onUnlinkCL" )
   INT32 _dmsEventHolder::onUnlinkCL ( UINT32 mask,
                                       const dmsEventCLItem &clItem,
                                       const CHAR *pMainCLName,
                                       _pmdEDUCB *cb,
                                       SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSEVTHLD_ONUNLINKCL ) ;

      // Event could not be handled in main thread
      if ( !cb || cb->getType() == EDU_TYPE_MAIN )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      for ( HANDLER_LIST::iterator iter = _handlers.begin() ;
            iter != _handlers.end() ;
            ++ iter )
      {
         _IDmsEventHandler *pHandler = (*iter) ;
         if ( pHandler && ( pHandler->getMask() & mask ) )
         {
            INT32 tmprc = pHandler->onUnlinkCL( this, _pCacheHolder, clItem,
                                                pMainCLName, cb, dpsCB ) ;
            if ( SDB_OK != tmprc )
            {
               rc = tmprc ;
            }
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__DMSEVTHLD_ONUNLINKCL, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSEVTHLD_ONCLRSUCACHES, "_dmsEventHolder::onClearSUCaches" )
   INT32 _dmsEventHolder::onClearSUCaches ( UINT32 mask )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSEVTHLD_ONCLRSUCACHES ) ;

      for ( HANDLER_LIST::iterator iter = _handlers.begin() ;
            iter != _handlers.end() ;
            ++ iter )
      {
         _IDmsEventHandler *pHandler = (*iter) ;
         if ( pHandler && ( pHandler->getMask() & mask ) )
         {
            INT32 tmprc = pHandler->onClearSUCaches( this, _pCacheHolder ) ;
            if ( SDB_OK != tmprc )
            {
               rc = tmprc ;
            }
         }
      }

      PD_TRACE_EXITRC( SDB__DMSEVTHLD_ONCLRSUCACHES, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSEVTHLD_ONCLRCLCACHES, "_dmsEventHolder::onClearCLCaches" )
   INT32 _dmsEventHolder::onClearCLCaches ( UINT32 mask,
                                            const dmsEventCLItem &clItem )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSEVTHLD_ONCLRCLCACHES ) ;

      for ( HANDLER_LIST::iterator iter = _handlers.begin() ;
            iter != _handlers.end() ;
            ++ iter )
      {
         _IDmsEventHandler *pHandler = (*iter) ;
         if ( pHandler && ( pHandler->getMask() & mask ) )
         {
            INT32 tmprc = pHandler->onClearCLCaches( this, _pCacheHolder,
                                                     clItem ) ;
            if ( SDB_OK != tmprc )
            {
               rc = tmprc ;
            }
         }
      }

      PD_TRACE_EXITRC( SDB__DMSEVTHLD_ONCLRCLCACHES, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSEVTHLD_ONCHGSUCACHES, "_dmsEventHolder::onChangeSUCaches" )
   INT32 _dmsEventHolder::onChangeSUCaches ( UINT32 mask )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSEVTHLD_ONCHGSUCACHES ) ;

      for ( HANDLER_LIST::iterator iter = _handlers.begin() ;
            iter != _handlers.end() ;
            ++ iter )
      {
         _IDmsEventHandler *pHandler = (*iter) ;
         if ( pHandler && ( pHandler->getMask() & mask ) )
         {
            INT32 tmprc = pHandler->onChangeSUCaches( this, _pCacheHolder ) ;
            if ( SDB_OK != tmprc )
            {
               rc = tmprc ;
            }
         }
      }

      PD_TRACE_EXITRC( SDB__DMSEVTHLD_ONCHGSUCACHES, rc ) ;

      return rc ;
   }


   const CHAR *_dmsEventHolder::getCSName () const
   {
      return _su->CSName();
   }

   UINT32 _dmsEventHolder::getSUID () const
   {
      return _su->CSID() ;
   }

   UINT32 _dmsEventHolder::getSULID () const
   {
      return _su->LogicalCSID() ;
   }
} // namespace engoine
