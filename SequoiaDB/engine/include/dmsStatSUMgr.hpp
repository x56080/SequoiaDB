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

   Source File Name = dmsStatSUMgr.hpp

   Descriptive Name = DMS Statistics Storage Unit Management Header

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS Statistics Storage Unit Management.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/
#ifndef DMSSTATSUMGR_HPP__
#define DMSSTATSUMGR_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "ossLatch.hpp"
#include "dms.hpp"
#include "dmsSysSUMgr.hpp"
#include "dmsEventHandler.hpp"
#include "monDMS.hpp"

using namespace std ;

namespace engine
{

   #define DMS_STAT_SPACE_NAME          "SYSSTAT"
   #define DMS_STAT_COLLECTION_CL_NAME  DMS_STAT_SPACE_NAME ".SYSCOLLECTIONSTAT"
   #define DMS_STAT_INDEX_CL_NAME       DMS_STAT_SPACE_NAME ".SYSINDEXSTAT"
   #define DMS_STAT_IDX_IDX_NAME        "STATIDXIDX"
   #define DMS_STAT_CL_IDX_NAME         "STATCLIDX"


   /*
      _dmsStatSUMgr define
   */
   class _dmsStatSUMgr : public _dmsSysSUMgr,
                         public _IDmsEventHandler
   {
      public :
         _dmsStatSUMgr ( _SDB_DMSCB *dmsCB ) ;

         INT32 init () ;

         OSS_INLINE BOOLEAN initialized ()
         {
            return _initialized ;
         }

         INT32 loadAllStats( pmdEDUCB *cb );

         INT32 loadCSStats( const CHAR *csName, pmdEDUCB *cb );

         INT32 loadCLStats( const CHAR *clFullName, pmdEDUCB *cb );

         INT32 updateCollectionStat ( const BSONObj &collectionStat,
                                      pmdEDUCB *cb,
                                      _dpsLogWrapper *dpsCB ) ;

         INT32 updateIndexStat ( const BSONObj &indexStat,
                                 BOOLEAN isValidForEstimate,
                                 pmdEDUCB *cb, 
                                 _dpsLogWrapper *dpsCB ) ;

      public :
         // For _IDmsEventHandler
         virtual INT32 onUnloadCS ( IDmsEventHolder *pEventHolder,
                                    IDmsSUCacheHolder *pCacheHolder,
                                    pmdEDUCB *cb,
                                    SDB_DPSCB *dpsCB ) ;

         virtual INT32 onRenameCS ( IDmsEventHolder *pEventHolder,
                                    IDmsSUCacheHolder *pCacheHolder,
                                    const CHAR *pOldCSName,
                                    const CHAR *pNewCSName,
                                    pmdEDUCB *cb,
                                    SDB_DPSCB *dpsCB ) ;

         virtual INT32 onDropCS ( SDB_EVENT_OCCUR_TYPE type,
                                  IDmsEventHolder *pEventHolder,
                                  IDmsSUCacheHolder *pCacheHolder,
                                  const dmsEventSUItem &suItem,
                                  dmsDropCSOptions *options,
                                  pmdEDUCB *cb,
                                  SDB_DPSCB *dpsCB ) ;

         virtual INT32 onRenameCL ( IDmsEventHolder *pEventHolder,
                                    IDmsSUCacheHolder *pCacheHolder,
                                    const dmsEventCLItem &clItem,
                                    const CHAR *pNewCLName,
                                    pmdEDUCB *cb,
                                    SDB_DPSCB *dpsCB ) ;

         virtual INT32 onTruncateCL ( SDB_EVENT_OCCUR_TYPE type,
                                      IDmsEventHolder *pEventHolder,
                                      IDmsSUCacheHolder *pCacheHolder,
                                      const dmsEventCLItem &clItem,
                                      dmsTruncCLOptions *options,
                                      pmdEDUCB *cb,
                                      SDB_DPSCB *dpsCB ) ;

         virtual INT32 onDropCL ( SDB_EVENT_OCCUR_TYPE type,
                                  IDmsEventHolder *pEventHolder,
                                  IDmsSUCacheHolder *pCacheHolder,
                                  const dmsEventCLItem &clItem,
                                  dmsDropCLOptions *options,
                                  pmdEDUCB *cb,
                                  SDB_DPSCB *dpsCB ) ;

         virtual INT32 onCreateIndex ( IDmsEventHolder *pEventHolder,
                                       IDmsSUCacheHolder *pCacheHolder,
                                       const dmsEventCLItem &clItem,
                                       const dmsEventIdxItem &idxItem,
                                       pmdEDUCB *cb,
                                       SDB_DPSCB *dpsCB ) ;

         virtual INT32 onDropIndex ( IDmsEventHolder *pEventHolder,
                                     IDmsSUCacheHolder *pCacheHolder,
                                     const dmsEventCLItem &clItem,
                                     const dmsEventIdxItem &idxItem,
                                     pmdEDUCB *cb, SDB_DPSCB *dpsCB ) ;

         virtual INT32 onClearSUCaches ( IDmsEventHolder *pEventHolder,
                                         IDmsSUCacheHolder *pCacheHolder ) ;

         virtual INT32 onClearCLCaches ( IDmsEventHolder *pEventHolder,
                                         IDmsSUCacheHolder *pCacheHolder,
                                         const dmsEventCLItem &clItem ) ;

         OSS_INLINE virtual UINT32 getMask () const
         {
            return DMS_EVENT_MASK_STAT ;
         }

         OSS_INLINE virtual const CHAR *getName() const
         {
            return "statistic SU manager" ;
         }

      protected :

         INT32 _ensureStatMetadata ( pmdEDUCB *cb ) ;

         INT32 _addCollectionStat ( const MON_CS_SIM_LIST &monCSList,
                                    const BSONObj &collectionStat,
                                    BOOLEAN ignoreCrtTime ) ;

         INT32 _addIndexStat ( const MON_CS_SIM_LIST &monCSList,
                               const BSONObj &indexStat,
                               BOOLEAN ignoreCrtTime ) ;

         INT32 _addSUCollectionStat ( const monCSSimple *pMonCS,
                                      const monCLSimple *pMonCL,
                                      const BSONObj &collectionStat,
                                      BOOLEAN ignoreCrtTime ) ;

         INT32 _addSUIndexStat ( const monCSSimple *pMonCS,
                                 const monCLSimple *pMonCL,
                                 const monIndex *pMonIX,
                                 const BSONObj &indexStat,
                                 BOOLEAN ignoreCrtTime ) ;

         INT32 _deleteCollectionStat ( const BSONObj &boMatcher, _pmdEDUCB *cb,
                                       SDB_DPSCB *dpsCB ) ;

         INT32 _deleteIndexStat ( const BSONObj &boMatcher, _pmdEDUCB *cb,
                                  SDB_DPSCB *dpsCB ) ;

         INT32 _updateCollectionStat ( const BSONObj &boMatcher,
                                       const BSONObj &boUpdator,
                                       _pmdEDUCB *cb, SDB_DPSCB *dpsCB ) ;

         INT32 _updateIndexStat ( const BSONObj &boMatcher,
                                  const BSONObj &boUpdator,
                                  _pmdEDUCB *cb, SDB_DPSCB *dpsCB ) ;

         INT32 _loadCollectionStats ( const monCSSimple *pMonCS,
                                      const monCLSimple *pMonCL,
                                      const BSONObj &boMatcher,
                                      pmdEDUCB *cb ) ;

         INT32 _loadIndexStats ( const monCSSimple *pMonCS,
                                 const monCLSimple *pMonCL,
                                 const monIndex *pMonIX,
                                 const BSONObj &boMatcher,
                                 pmdEDUCB *cb ) ;

         INT32 _onIndexOperator ( IDmsEventHolder *pEventHolder,
                                  IDmsSUCacheHolder *pCacheHolder,
                                  const dmsEventCLItem &clItem,
                                  const dmsEventIdxItem &idxItem,
                                  pmdEDUCB *cb,
                                  SDB_DPSCB *dpsCB ) ;

      protected :
         BOOLEAN _initialized ;
         BSONObj _tbScanHint ;
         BSONObj _collectionHint ;
         BSONObj _indexHint ;
   } ;

   typedef class _dmsStatSUMgr dmsStatSUMgr ;

}

#endif //DMSSTATSUMGR_HPP__

