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

   Source File Name = optAPM.hpp

   Descriptive Name = Optimizer Access Plan Manager Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Access
   Plan Manager, which is pooling access plans that has been used.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft
          01/07/2017  HGM Move from rtnAPM.hpp

   Last Changed =

*******************************************************************************/
#ifndef OPTAPM_HPP__
#define OPTAPM_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "optAccessPlan.hpp"
#include "optAccessPlanRuntime.hpp"
#include "optPlanMarker.hpp"
#include "utilHashTable.hpp"
#include "dmsEventHandler.hpp"
#include "rtnCollectionInfo.hpp"

using namespace std ;

namespace engine
{

   #define OPT_PLAN_CACHE_DFT_LOCK_NUM    ( 64 )
   #define OPT_PLAN_CACHE_ACT_HIGH_PERC   ( 0.80 )
   #define OPT_PLAN_CACHE_ACT_LOW_PERC    ( 0.50 )
   #define OPT_PLAN_CACHE_AVG_BUCKET_SIZE ( 3 )

   class _optCachedPlanMonitor ;
   typedef class _optCachedPlanMonitor optCachedPlanMonitor ;

   /*
      _optAccessPlanCache define
    */
   class _optAccessPlanCache : public _utilHashTable< optAccessPlanKey,
                                                      optAccessPlan,
                                                      OPT_PLAN_CACHE_DFT_LOCK_NUM >
   {
      public :
         _optAccessPlanCache () ;

         ~_optAccessPlanCache () ;

         BOOLEAN initialize ( UINT32 bucketNum,
                              optCachedPlanMonitor *pMonitor ) ;

         void deinitialize () ;

         BOOLEAN addPlan ( optAccessPlan *pPlan ) ;

         void removeCachedPlan ( optAccessPlan *pPlan, INT32 lockType = -1 ) ;

         void resetCachedPlanActivity( optAccessPlan *pPlan,
                                       INT32 lockType = -1 ) ;
<<<<<<< HEAD

         void invalidateSUPlans ( dmsCachedPlanMgr *pCachedPlanMgr,
                                  UINT32 suLID ) ;
=======
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2

         void invalidateSUPlans ( utilCSUniqueID csUID ) ;

         void invalidateCLPlans ( utilCLUniqueID clUID ) ;

         void invalidateAllPlans () ;

         void invalidateCLPlans ( const CHAR *pCLFullName ) ;

         void invalidateSUPlans ( const CHAR *pCSName ) ;

         UINT32 getCachedPlanCount () const ;

         INT32 getCachedPlanList ( vector<BSONObj> &cachedPlanList ) ;

         void enableCaching () ;

         void disableCaching () ;

      protected :
         virtual void _afterAddItem ( UINT32 bucketID, optAccessPlan *pPlan ) ;

         virtual void _afterGetItem ( UINT32 bucketID, optAccessPlan *pPlan ) ;

         virtual void _afterRemoveItem ( UINT32 bucketID, optAccessPlan *pPlan ) ;

      protected :
         optCachedPlanMonitor *_pMonitor ;
   } ;

   typedef class _optAccessPlanCache optAccessPlanCache ;

   /*
      _optCachedPlanActivity define
    */
   class _optCachedPlanActivity : public SDBObject
   {
      public :
         _optCachedPlanActivity () ;

         ~_optCachedPlanActivity () ;

         void clear () ;

         void setPlan ( optAccessPlan *pPlan, UINT64 timestamp ) ;

         OSS_INLINE optAccessPlan *getPlan ()
         {
            return _pPlan ;
         }

         OSS_INLINE UINT64 getLastAccessTime () const
         {
            return _lastAccessTime ;
         }

         OSS_INLINE void setLastAccessTime ( UINT64 lastAccessTime )
         {
            _lastAccessTime = lastAccessTime ;
         }

         OSS_INLINE UINT64 getAccessCount () const
         {
            return _accessCount ;
         }

         OSS_INLINE void incAccessCount ()
         {
            _accessCount ++ ;
         }

         OSS_INLINE UINT64 getPeriodAccessCount () const
         {
            return _periodAccessCount ;
         }

         OSS_INLINE void incPeriodAccessCount ()
         {
            _periodAccessCount ++ ;
         }

         OSS_INLINE void decPeriodAccessCount ( UINT64 count )
         {
            if ( _periodAccessCount > count )
            {
               _periodAccessCount -= count ;
            }
            else
            {
               _periodAccessCount = 0 ;
            }
         }

         OSS_INLINE BOOLEAN isEmpty () const
         {
            return ( NULL == _pPlan ) ;
         }

         void setQueryActivity ( const optQueryActivity &queryActivity,
                                 const rtnParamList &parameters ) ;

         void toBSON ( BSONObjBuilder &builder ) ;

      protected:
         void _clearPlan()
         {
            if ( NULL != _pPlan )
            {
               _pPlan->release() ;
               _pPlan = NULL ;
            }
         }

         void _setPlan( optAccessPlan *pPlan )
         {
            _clearPlan() ;
            if ( NULL != pPlan )
            {
               pPlan->incRefCount() ;
               _pPlan = pPlan ;
            }
         }

      protected :
         optAccessPlan *   _pPlan ;
         UINT64            _lastAccessTime ;
         UINT64            _periodAccessCount ;
         UINT64            _accessCount ;
         ossTickDelta      _totalQueryTimeTick ;
         optQueryActivity  _maxQueryActivity ;
         optQueryActivity  _minQueryActivity ;

         monSpinXLatch     _latch ;
   } ;

   typedef class _optCachedPlanActivity optCachedPlanActivity ;

   /*
      _optCachedPlanMonitor define
    */
   class _optCachedPlanMonitor : public SDBObject
   {
      public :
         _optCachedPlanMonitor () ;

         ~_optCachedPlanMonitor () ;

         BOOLEAN initialize ( optAccessPlanCache *pPlanCache ) ;

         void deinitialize () ;

         OSS_INLINE BOOLEAN isInitialized () const
         {
            return ( NULL != _pActivities ) ;
         }

         BOOLEAN setActivity ( optAccessPlan *pPlan ) ;

         OSS_INLINE void setCachedPlanActivity ( optAccessPlan *pPlan )
         {
            INT32 activityID = pPlan->getActivityID() ;
            if ( OPT_INVALID_ACT_ID != activityID )
            {
               optCachedPlanActivity &activity = _pActivities[ activityID ] ;
               activity.setLastAccessTime( _accessTimestamp.inc() ) ;
               activity.incPeriodAccessCount() ;
            }
         }

         OSS_INLINE optCachedPlanActivity *getActivity ( INT32 activityID )
         {
            if ( OPT_INVALID_ACT_ID != activityID )
            {
               return &( _pActivities[ activityID ] ) ;
            }
            return NULL ;
         }

         OSS_INLINE void resetActivity ( INT32 activityID )
         {
            if ( OPT_INVALID_ACT_ID != activityID )
            {
               _pActivities[ activityID ].setPlan( NULL, 0 ) ;
               UINT64 freeActivityIndex = _freeIndexEnd.inc() % _activityNum ;
               _pFreeActivityIDs[ freeActivityIndex ] = activityID ;
               _cachedPlanCount.dec() ;
            }
         }

         OSS_INLINE UINT32 getCachedPlanCount () const
         {
            return _cachedPlanCount.peek() ;
         }

         OSS_INLINE monRWMutex *getClearLock ()
         {
            return &_clearLock ;
         }

         OSS_INLINE ossEvent *getClearEvent ()
         {
            return &_clearEvent ;
         }

         void signalPlanClearJob () ;

         void clearCachedPlans () ;

         void clearExpiredCachedPlans () ;

      protected :
         INT32 _allocateActivity ( optAccessPlan *pPlan ) ;

      protected :
         // Begin to the free index, where to get free activities
         ossAtomic64 _freeIndexBegin ;

         // End to the free index, where to return free activities
         ossAtomic64 _freeIndexEnd ;

         // Free index
         UINT32 *_pFreeActivityIDs ;

         // Thread flag to clearing procedure
         ossAtomic32 _clearThread ;

         // Mutex to protect clearing procedure
         monRWMutex _clearLock ;

         // Clear event to signal clear job
         ossEvent _clearEvent ;

         // Number of activities ( The capacity of plan cache )
         UINT32 _activityNum ;

         // High water mark to clear cached plans
         UINT32 _highWaterMark ;

         // Low water mark to stop clear cached plans
         UINT32 _lowWaterMark ;

         // Clock index to scan the activity table
         UINT32 _clockIndex ;

         // Activity table
         optCachedPlanActivity *_pActivities ;

         // Total number of cached plans
         ossAtomic32 _cachedPlanCount ;

         // Access timestamp for cached plans
         ossAtomic64 _accessTimestamp ;

         // Last timestamp to finished clearing procedure
         UINT64 _lastClearTimestamp ;

         // Pointer to plan cache
         optAccessPlanCache *_pPlanCache ;
   } ;

   /*
      _optAccessPlanManager define
    */
   class _optAccessPlanManager : public SDBObject,
                                 public _optAccessPlanConfigHolder,
                                 public _mthMatchConfigHolder
   {
      public :
         _optAccessPlanManager () ;

         ~_optAccessPlanManager () ;

         INT32 init ( UINT32 bucketNum,
                      OPT_PLAN_CACHE_LEVEL cacheLevel,
                      UINT32 sortBufferSize,
                      INT32 optCostThreshold,
                      BOOLEAN enableMixCmp,
<<<<<<< HEAD
                      INT32 planCacheMainCLThreshold ) ;
=======
                      BOOLEAN activateClearJob = TRUE ) ;
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2

         INT32 reinit ( UINT32 bucketNum,
                        OPT_PLAN_CACHE_LEVEL cacheLevel,
                        UINT32 sortBufferSize,
                        INT32 optCostThreshold,
                        BOOLEAN enableMixCmp,
                        INT32 planCacheMainCLThreshold ) ;

         INT32 fini () ;

         OSS_INLINE OPT_PLAN_CACHE_LEVEL getCacheLevel () const
         {
            return _cacheLevel ;
         }

         OSS_INLINE BOOLEAN isInitialized () const
         {
            return _planCache.isInitialized() ;
         }

         OSS_INLINE optAccessPlanCache *getPlanCache ()
         {
            return &_planCache ;
         }

         OSS_INLINE optCachedPlanMonitor *getPlanMonitor ()
         {
            return &_monitor ;
         }

         // Try to get access plan from cache, if could not get access plan
         // from cache, create one
<<<<<<< HEAD
         INT32 getAccessPlan ( const rtnQueryOptions &options,
                               dmsStorageUnit *su,
                               dmsMBContext *mbContext,
                               optAccessPlanRuntime &planRuntime,
                               const rtnExplainOptions *expOptions = NULL ) ;
=======
         INT32 getAccessPlan( IExecutor *executor,
                              const rtnQueryOptions &options,
                              const rtnCollectionInfo &info,
                              optAccessPlanRuntime &planRuntime,
                              const rtnExplainOptions *expOptions = NULL ) ;
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2

         // Create access plan directly without caching
         INT32 getTempAccessPlan( IExecutor *executor,
                                  const rtnQueryOptions &options,
                                  const rtnCollectionInfo &info,
                                  optAccessPlanRuntime &planRuntime );

         void invalidateCLPlans ( const CHAR *pCLFullName ) ;

         void invalidateCLPlans ( utilCLUniqueID clUID ) ;

         void invalidateSUPlans ( const CHAR * pCSName ) ;

         void invalidateAllPlans () ;

         void setQueryActivity ( INT32 activityID,
                                 const optQueryActivity &queryActivity,
                                 const rtnParamList &parameters ) ;

         // acquire access plan ID
         INT64 acquireAccessPlanID()
<<<<<<< HEAD
         {
            return _accessPlanIdGenerator.inc() ;
         }

      public :
         // For _IDmsEventHandler
         virtual INT32 onCreateCS ( IDmsEventHolder *pEventHolder,
                                    IDmsSUCacheHolder *pCacheHolder,
                                    pmdEDUCB *cb,
                                    SDB_DPSCB *dpsCB ) ;

         virtual INT32 onLoadCS ( IDmsEventHolder *pEventHolder,
                                  IDmsSUCacheHolder *pCacheHolder,
                                  pmdEDUCB *cb,
                                  SDB_DPSCB *dpsCB ) ;

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

         virtual INT32 onRebuildIndex ( IDmsEventHolder *pEventHolder,
                                        IDmsSUCacheHolder *pCacheHolder,
                                        const dmsEventCLItem &clItem,
                                        const dmsEventIdxItem &idxItem,
                                        pmdEDUCB *cb,
                                        SDB_DPSCB *dpsCB ) ;

         virtual INT32 onDropIndex ( IDmsEventHolder *pEventHolder,
                                     IDmsSUCacheHolder *pCacheHolder,
                                     const dmsEventCLItem &clItem,
                                     const dmsEventIdxItem &idxItem,
                                     pmdEDUCB *cb,
                                     SDB_DPSCB *dpsCB ) ;

         virtual INT32 onClearSUCaches ( IDmsEventHolder *pEventHolder,
                                         IDmsSUCacheHolder *pCacheHolder ) ;

         virtual INT32 onClearCLCaches ( IDmsEventHolder *pEventHolder,
                                         IDmsSUCacheHolder *pCacheHolder,
                                         const dmsEventCLItem &clItem ) ;

         virtual INT32 onChangeSUCaches ( IDmsEventHolder *pEventHolder,
                                          IDmsSUCacheHolder *pCacheHolder ) ;

         OSS_INLINE virtual UINT32 getMask () const
=======
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
         {
            return _accessPlanIdGenerator.inc() ;
         }

         OSS_INLINE virtual const CHAR *getName() const
         {
            return "access plan manager" ;
         }

      protected :
<<<<<<< HEAD
         INT32 _getCLAccessPlan ( const rtnQueryOptions &options,
                                  dmsStorageUnit *su,
                                  dmsMBContext *mbContext,
                                  optAccessPlanRuntime &planRuntime,
                                  const rtnExplainOptions *expOptions ) ;

         INT32 _getCLAccessPlan ( const rtnQueryOptions &options,
                                  OPT_PLAN_CACHE_LEVEL cacheLevel,
                                  dmsStorageUnit *su,
                                  dmsMBContext *mbContext,
                                  optAccessPlanRuntime &planRuntime,
                                  const rtnExplainOptions *expOptions ) ;

         INT32 _getMainCLAccessPlan ( const rtnQueryOptions &options,
                                      dmsStorageUnit *su,
                                      dmsMBContext *mbContext,
=======
         INT32 _getCLAccessPlan ( IExecutor *executor,
                                  const rtnQueryOptions &options,
                                  const rtnCollectionInfo &info,
                                  optAccessPlanRuntime &planRuntime,
                                  const rtnExplainOptions *expOptions ) ;

         INT32 _getCLAccessPlan ( IExecutor *executor,
                                  const rtnQueryOptions &options,
                                  OPT_PLAN_CACHE_LEVEL cacheLevel,
                                  const rtnCollectionInfo &info,
                                  optAccessPlanRuntime &planRuntime,
                                  const rtnExplainOptions *expOptions ) ;

         INT32 _getMainCLAccessPlan ( IExecutor *executor,
                                      const rtnQueryOptions &options,
                                      const rtnCollectionInfo &info,
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
                                      optAccessPlanRuntime &planRuntime ) ;

         INT32 _prepareAccessPlanKey ( optAccessPlanKey &planKey,
                                       optAccessPlanHelper &planHelper,
                                       optAccessPlanRuntime &planRuntime ) ;

         INT32 _createAccessPlan ( optAccessPlanKey &planKey,
                                   optAccessPlanRuntime &planRuntime,
                                   optAccessPlanHelper &planHelper,
                                   optGeneralAccessPlan **ppPlan,
                                   BOOLEAN needCache ) ;

         INT32 _getCachedAccessPlan ( const optAccessPlanKey &planKey,
                                      optAccessPlan **ppPlan ) ;

         BOOLEAN _cacheAccessPlan ( optAccessPlan *pPlan ) ;

         // Helpers for parameterized plans
         INT32 _validateParamPlan ( const rtnCollectionInfo &info,
                                    optAccessPlanKey &planKey,
                                    optAccessPlanRuntime &planRuntime,
                                    optAccessPlanHelper &planHelper,
                                    optParamAccessPlan *plan ) ;

         // Helpers for main-collection plans
         INT32 _createMainCLPlan ( optAccessPlanKey &planKey,
                                   const rtnQueryOptions &subOptions,
                                   optAccessPlanRuntime &planRuntime,
                                   optAccessPlanHelper &planHelper,
                                   optMainCLAccessPlan **ppPlan ) ;

         INT32 _validateMainCLPlan ( optMainCLAccessPlan *mainPlan,
                                     const rtnQueryOptions &subOptions,
                                     optAccessPlanRuntime &planRuntime,
                                     optAccessPlanHelper &planHelper ) ;

         INT32 _bindMainCLPlan ( optMainCLAccessPlan *mainPlan,
                                 const rtnQueryOptions &subOptions,
                                 optAccessPlanRuntime &planRuntime,
                                 optAccessPlanHelper &planHelper ) ;
         
         // Helpers for clear background job
         INT32 _startClearJob () ;
         void  _stopClearJob () ;

      protected :
         monSpinXLatch           _reinitLatch ;
         optAccessPlanCache      _planCache ;
         optCachedPlanMonitor    _monitor ;
         optPlanMarker           _marker ;
         EDUID                   _clearJobEduID ;

         ossAtomicSigned64       _accessPlanIdGenerator ;

         // Configured options
         OPT_PLAN_CACHE_LEVEL    _cacheLevel ;
   } ;

   typedef class _optAccessPlanManager optAccessPlanManager ;
}

#endif //OPTAPM_HPP__
