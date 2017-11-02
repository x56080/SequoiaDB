/*******************************************************************************


   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = rtnAPM.hpp

   Descriptive Name = RunTime Access Plan Manager Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Access
   Plan Manager, which is pooling access plans that has been used.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTNAPM_HPP__
#define RTNAPM_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "optAccessPlan.hpp"
#include "ossLatch.hpp"
#include <map>

using namespace std ;

namespace engine
{
   class _dmsStorageUnit ;
   class _rtnAccessPlanManager ;

   /*
      _rtnAPContext define
   */
   struct _rtnAPContext
   {
      INT32          _lastListID ;

      _rtnAPContext()
      {
         _lastListID = 0 ;
      }
   } ;
   typedef _rtnAPContext rtnAPContext ;

   /*
      RTN_APL_SIZE size
   */
   #define RTN_APL_SIZE             ( 5 )

   /*
      _rtnAccessPlanList define
      one access plan list is for same hash result for one collection
   */
   class _rtnAccessPlanList : public SDBObject
   {
      friend class _rtnAccessPlanSet ;
   private :
      ossSpinSLatch           _mutex ;
      vector<optAccessPlan*>  _plans ;
      _rtnAccessPlanManager   *_apm ;
      INT32                   _lastID ;
      ossAtomic32             _emptyAPNum ;
      ossAtomic32             *_pSetEmptyAPNum ;
      ossAtomic32             *_pTotalEmptyAPNum ;

   public :
      explicit _rtnAccessPlanList ( _rtnAccessPlanManager *apm,
                                    ossAtomic32 *pSetEmptyAPNum,
                                    ossAtomic32 *pTotalEmptyAPNum )
      :_emptyAPNum( 0 )
      {
         _apm = apm ;
         _lastID = 0 ;
         _pSetEmptyAPNum = pSetEmptyAPNum ;
         _pTotalEmptyAPNum = pTotalEmptyAPNum ;
      }
      ~_rtnAccessPlanList()
      {
         SDB_ASSERT( _emptyAPNum.peek() >= 0 &&
                     _emptyAPNum.peek() <= _plans.size(),
                     "0 <= _emptyAPNum <= _plans.size()" ) ;

         vector<optAccessPlan *>::iterator it ;
         for ( it = _plans.begin(); it != _plans.end(); ++it )
         {
            SDB_OSS_DEL (*it) ;
         }
         _plans.clear() ;
      }

      optAccessPlan* getPlan ( const BSONObj &query,
                               const BSONObj &orderBy,
                               const BSONObj &hint,
                               rtnAPContext &context ) ;

      BOOLEAN  addPlan ( optAccessPlan *plan,
                         const rtnAPContext &context,
                         SINT32 &incSize ) ;

      void     releasePlan ( optAccessPlan *plan ) ;
      void     invalidate ( UINT32 &cleanNum ) ;

      INT32 size()
      {
         ossScopedLock _lock ( &_mutex, SHARED ) ;
         return _plans.size() ;
      }

      BOOLEAN  hasEmptyAP()
      {
         return _emptyAPNum.compare( 0 ) ? FALSE : TRUE ;
      }

      void clear ( UINT32 &cleanNum ) ;

   protected:
      UINT32      _clearFast( INT32 expectNum = 1 ) ;

   } ;
   typedef _rtnAccessPlanList rtnAccessPlanList ;

   //#define RTN_APS_SIZE             ( 500 )
   #define RTN_APS_DFT_OCCUPY_PCT   ( 0.75f )

   /*
      _rtnAccessPlanSet define
   */
   class _rtnAccessPlanSet : public SDBObject
   {
      friend class _rtnAccessPlanManager ;

      typedef map<UINT32, rtnAccessPlanList*>      MAP_CODE_2_LIST ;
      typedef MAP_CODE_2_LIST::iterator            MAP_CODE_2_LIST_IT ;

   private :
      ossSpinSLatch           _mutex ;
      ossAtomic32             _totalNum ;
      ossAtomic32             _emptyAPNum ;
      ossAtomic32             *_pTotalEmptyAPNum ;
      MAP_CODE_2_LIST         _planLists ;
      _dmsStorageUnit         *_su ;
      _rtnAccessPlanManager   *_apm ;
      CHAR _collectionName[ DMS_COLLECTION_NAME_SZ+1 ] ;

   public :
      explicit _rtnAccessPlanSet( _dmsStorageUnit *su,
                                  const CHAR *collectionName,
                                  _rtnAccessPlanManager *apm,
                                  ossAtomic32 *pTotalEmptyAPNum )
      :_totalNum( 0 ), _emptyAPNum( 0 )
      {
         ossMemset ( _collectionName, 0, sizeof(_collectionName) ) ;
         ossStrncpy ( _collectionName, collectionName,
                      sizeof(_collectionName) ) ;
         _pTotalEmptyAPNum = pTotalEmptyAPNum ;
         _su = su ;
         _apm = apm ;
      }
      ~_rtnAccessPlanSet()
      {
         SDB_ASSERT( _emptyAPNum.peek() >= 0 &&
                     _emptyAPNum.peek() <= _totalNum.peek(),
                     "0 <= _emptyAPNum <= _totalNum" ) ;

         MAP_CODE_2_LIST_IT it ;
         for ( it = _planLists.begin(); it != _planLists.end(); ++it )
         {
            SDB_OSS_DEL (*it).second ;
         }
         _planLists.clear() ;
      }

      BOOLEAN addPlan ( optAccessPlan *plan,
                        const rtnAPContext &context,
                        SINT32 &incSize ) ;

      optAccessPlan* getPlan ( const BSONObj &query,
                               const BSONObj &orderBy,
                               const BSONObj &hint,
                               rtnAPContext &context ) ;

      void  invalidate ( UINT32 &cleanNum ) ;
      void  releasePlan ( optAccessPlan *plan ) ;
      void  clear ( UINT32 &cleanNum, BOOLEAN full = TRUE ) ;

      INT32 size() { return _totalNum.peek() ; }
      const CHAR* getName() const { return _collectionName ; }

      BOOLEAN  hasEmptyAP()
      {
         return _emptyAPNum.compare( 0 ) ? FALSE : TRUE ;
      }

   protected:
      UINT32   _clearFast( INT32 expectNum = 1 ) ;

   } ;
   typedef class _rtnAccessPlanSet rtnAccessPlanSet ;

   #define RTN_APM_DFT_OCCUPY_PCT   ( 0.75f )

   /*
      _rtnAccessPlanManager define
      one access plan manager may have one or more access plan set,
      access plan manager is per collection space
   */
   class _rtnAccessPlanManager : public SDBObject
   {
      // C version of string map, std::string is too slow
      struct cmp_str
      {
         bool operator() (const char *a, const char *b)
         {
            return std::strcmp(a,b)<0 ;
         }
      } ;
      typedef map<const CHAR*, _rtnAccessPlanSet*, cmp_str>    PLAN_SETS ;
#ifdef _WINDOWS
      typedef PLAN_SETS::iterator                              PLAN_SETS_IT ;
#else
      typedef map<const CHAR*, _rtnAccessPlanSet*>::iterator   PLAN_SETS_IT ;
#endif // _WINDOWS

   private :
      ossSpinSLatch        _mutex ;
      ossAtomic32          _totalNum ;
      ossAtomic32          _emptyAPNum ;
      _dmsStorageUnit      *_su ;
      PLAN_SETS            _planSets ;
      UINT32               _bucketsNum ;

   public :
      explicit _rtnAccessPlanManager( _dmsStorageUnit *su ) ;
      ~_rtnAccessPlanManager() ;

      void  invalidatePlans ( const CHAR *collectionName ) ;
      INT32 getPlan ( const BSONObj &query,
                      const BSONObj &orderBy,
                      const BSONObj &hint,
                      const CHAR *collectionName,
                      optAccessPlan **out ) ;
      void  releasePlan ( optAccessPlan *plan ) ;
      INT32 size()
      {
         return _totalNum.peek() ;
      }
      void clear ( BOOLEAN full = TRUE ) ;

      BOOLEAN  hasEmptyAP()
      {
         return _emptyAPNum.compare( 0 ) ? FALSE : TRUE ;
      }

   protected:
      BOOLEAN  addPlan ( optAccessPlan *plan,
                         const rtnAPContext &context,
                         SINT32 &incSize ) ;

      UINT32   _clearFast( INT32 expectNum ) ;

   } ;
   typedef class _rtnAccessPlanManager rtnAccessPlanManager ;

}

#endif //RTNAPM_HPP__

