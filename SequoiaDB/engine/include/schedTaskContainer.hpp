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

   Source File Name = schedTaskContainer.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/19/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SCHED_TASK_CONTAINER_HPP__
#define SCHED_TASK_CONTAINER_HPP__

#include "schedTaskQue.hpp"
#include "ossLatch.hpp"
#include <boost/shared_ptr.hpp>

namespace engine
{

   /*
      _schedTaskContanier define
   */
   class _schedTaskContanier : public SDBObject
   {
      public:
         _schedTaskContanier() ;
         ~_schedTaskContanier() ;

         schedTaskQueBase*    getTaskQue() ;

         INT32    init( const string &name, SCHED_TASK_QUE_TYPE queType ) ;
         void     fini() ;

         void     holdIn() ;
         void     holdOut() ;
         BOOLEAN  hasHold() ;

         const string&  getName() const { return _name ; }

      public:

         void     push( const pmdEDUEvent &event, INT64 userData, UINT64 time ) ;
         BOOLEAN  pop( pmdEDUEvent &event, UINT64 &time, INT64 millisec ) ;

         void     setNice( INT32 nice ) ;

         void     updateFixedRatio( UINT64 totalWeight ) ;
         void     updateAdjustRatio( UINT32 total,
                                     FLOAT64 needProcessNum,
                                     UINT32 hasProcessedNum ) ;

         UINT64   getWeightValue() const ;
         FLOAT64  calcCount( UINT32 total ) ;

      protected:
         UINT64   _nice2Weight( INT32 nice ) const ;

      private:
         schedTaskQueBase        *_pTaskQue ;
         ossAtomic32             _holdNum ;

         string                  _name ;
         INT32                   _nice ;
         UINT64                  _weightValue ;
         FLOAT64                 _fixedRatio ;
         FLOAT64                 _adjustRatio ;

   } ;
   typedef _schedTaskContanier schedTaskContanier ;
   typedef boost::shared_ptr<schedTaskContanier>   schedTaskContanierPtr ;

   typedef map<string, schedTaskContanierPtr>      MAP_CONTAINERPTR ;

   /*
      _schedTaskContanierMgr define
   */
   class _schedTaskContanierMgr : public SDBObject
   {
      public:
         _schedTaskContanierMgr() ;
         ~_schedTaskContanierMgr() ;

         INT32    init( SCHED_TASK_QUE_TYPE queType ) ;
         void     fini() ;

         INT32    addContanier( const string &name,
                                INT32 nice,
                                schedTaskContanierPtr &ptr ) ;
         BOOLEAN  isContainerExist( const string &name ) ;
         INT32    delContanier( const string &name ) ;

         schedTaskContanierPtr   getContanier( const string &name,
                                               BOOLEAN withHold ) ;

         void     resumeIterator() ;
         void     pauseIterator( schedTaskContanierPtr ptr ) ;
         BOOLEAN  nextIterator( schedTaskContanierPtr &ptr ) ;

      protected:

         void     _flushWeight() ;
         void     _updateIterator( schedTaskContanierPtr &ptr ) ;

      private:
         SCHED_TASK_QUE_TYPE              _queType ;
         MAP_CONTAINERPTR                 _mapContanier ;
         schedTaskContanierPtr            _defaultPtr ;

         schedTaskContanierPtr            _lastPtr ;
         schedTaskContanierPtr            _curPtr ;

         ossSpinXLatch                    _latch ;

   } ;
   typedef _schedTaskContanierMgr schedTaskContanierMgr ;

}

#endif // SCHED_TASK_CONTAINER_HPP__
