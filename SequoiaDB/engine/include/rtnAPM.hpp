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
      _rtnEliminateList define
   */
   template< typename T >
   class _rtnEliminateList : public SDBObject
   {
      public:
         _rtnEliminateList()
         {
            _header = NULL ;
            _tail = NULL ;
         }
         ~_rtnEliminateList()
         {
         }

         void     insert( T *node, BOOLEAN needLock )
         {
            if ( needLock )
            {
               _lock.get() ;
            }

            if ( node->getNext() || node->getPrev() )
            {
               remove( node, FALSE ) ;
            }

            if ( !_header || !_tail )
            {
               _header = node ;
               _tail = node ;
            }
            else
            {
               node->setNext( _header ) ;
               _header->setPrev( node ) ;
               _header = node ;
            }

            if ( needLock )
            {
               _lock.release() ;
            }
         }
         void     remove( T *node, BOOLEAN needLock )
         {
            if ( needLock )
            {
               _lock.get() ;
            }

            if ( node->getNext() )
            {
               node->getNext()->setPrev( node->getPrev() ) ;
            }
            if ( node->getPrev() )
            {
               node->getPrev()->setNext( node->getNext() ) ;
            }
            if ( _header == node )
            {
               _header = node->getNext() ;
            }
            if ( _tail == node )
            {
               _tail = node->getPrev() ;
            }
            node->setNext( NULL ) ;
            node->setPrev( NULL ) ;

            if ( needLock )
            {
               _lock.release() ;
            }
         }
         void     reHeader( T *node, BOOLEAN needLock )
         {
            if ( needLock )
            {
               _lock.get() ;
            }

            remove( node, FALSE ) ;
            insert( node, FALSE ) ;

            if ( needLock )
            {
               _lock.release() ;
            }
         }

         T*       header() { return _header ; }
         T*       next( T *node ) { return node->getNext() ; }
         T*       tail() { return _tail ; }
         T*       prev( T *node ) { return node->getPrev() ; }

         void     lock()
         {
            _lock.get() ;
         }
         void     release()
         {
            _lock.release() ;
         }

      private:
         T              *_header ;
         T              *_tail ;
         ossSpinXLatch  _lock ;
   } ;

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

      _rtnAccessPlanList      *_next ;
      _rtnAccessPlanList      *_prev ;

      UINT32                  _hash ;

   public :
      explicit _rtnAccessPlanList ( _rtnAccessPlanManager *apm,
                                    ossAtomic32 *pSetEmptyAPNum,
                                    ossAtomic32 *pTotalEmptyAPNum,
                                    UINT32 hash )
      :_emptyAPNum( 0 )
      {
         _apm = apm ;
         _lastID = 0 ;
         _pSetEmptyAPNum = pSetEmptyAPNum ;
         _pTotalEmptyAPNum = pTotalEmptyAPNum ;

         _next = NULL ;
         _prev = NULL ;
         _hash = hash ;
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

      _rtnAccessPlanList* getNext() { return _next ; }
      _rtnAccessPlanList* getPrev() { return _prev ; }
      void  setNext( _rtnAccessPlanList *next ) { _next = next ; }
      void  setPrev( _rtnAccessPlanList *prev ) { _prev = prev ; }

      UINT32 hash() const { return _hash ; }

   protected:
      UINT32      _clearFast( INT32 expectNum = 1 ) ;

   } ;
   typedef _rtnAccessPlanList rtnAccessPlanList ;

   #define RTN_APS_DFT_OCCUPY_PCT   ( 0.75f )

   /*
      _rtnAccessPlanSet define
   */
   class _rtnAccessPlanSet : public SDBObject
   {
      friend class _rtnAccessPlanManager ;

      typedef map<UINT32, rtnAccessPlanList*>      MAP_CODE_2_LIST ;
      typedef MAP_CODE_2_LIST::iterator            MAP_CODE_2_LIST_IT ;

      typedef _rtnEliminateList< rtnAccessPlanList >  ACCESS_LIST ;

   private :
      ossSpinSLatch           _mutex ;
      ossAtomic32             _totalNum ;
      ossAtomic32             _emptyAPNum ;
      ossAtomic32             *_pTotalEmptyAPNum ;
      MAP_CODE_2_LIST         _planLists ;
      _dmsStorageUnit         *_su ;
      _rtnAccessPlanManager   *_apm ;
      ACCESS_LIST             _accessList ;
      CHAR _collectionName[ DMS_COLLECTION_NAME_SZ+1 ] ;

      _rtnAccessPlanSet       *_next ;
      _rtnAccessPlanSet       *_prev ;

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

         _next = NULL ;
         _prev = NULL ;
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

      _rtnAccessPlanSet* getNext() { return _next ; }
      _rtnAccessPlanSet* getPrev() { return _prev ; }
      void  setNext( _rtnAccessPlanSet *next ) { _next = next ; }
      void  setPrev( _rtnAccessPlanSet *prev ) { _prev = prev ; }

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

      typedef _rtnEliminateList< _rtnAccessPlanSet >           ACCESS_LIST ;

   private :
      ossSpinSLatch        _mutex ;
      ossAtomic32          _totalNum ;
      ossAtomic32          _emptyAPNum ;
      _dmsStorageUnit      *_su ;
      PLAN_SETS            _planSets ;
      UINT32               _bucketsNum ;
      ACCESS_LIST          _accessList ;

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

