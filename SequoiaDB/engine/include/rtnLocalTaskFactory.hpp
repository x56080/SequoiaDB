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

   Source File Name = rtnLocalTaskFactory.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/27/2020  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_LOCALTASK_FACTORY_HPP__
#define RTN_LOCALTASK_FACTORY_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "utilPooledAutoPtr.hpp"
#include "../bson/bson.h"
#include <map>

using namespace std ;
using namespace bson ;

namespace engine
{

   /*
      RTN_LOCAL_TASK_TYPE define
   */
   enum RTN_LOCAL_TASK_TYPE
   {
      RTN_LOCAL_TASK_MIN               = 0,
      RTN_LOCAL_TASK_RENAMECS          = 1,
      RTN_LOCAL_TASK_RENAMECL          = 2,
      RTN_LOCAL_TASK_RECYCLECS         = 3,
      RTN_LOCAL_TASK_RECYCLECL         = 4,
      RTN_LOCAL_TASK_RETURNCS          = 5,
      RTN_LOCAL_TASK_RETURNCL          = 6,

      RTN_LOCAL_TASK_MAX
   } ;

   const CHAR* rtnLocalTaskType2Str( RTN_LOCAL_TASK_TYPE type ) ;

   #define RTN_LT_INVALID_TASKID                      (0)
   /*
      Task Field Define
   */
   #define RTN_LT_FIELD_TASKTYPE                      "Type"
   #define FTN_LT_FIELD_TASKTYPE_DESP                 "TypeDesp"

   /*
      _rtnLocalTaskBase define
   */
   class _rtnLocalTaskBase : public SDBObject
   {
      friend class _rtnLocalTaskMgr ;
      friend class _rtnLTFactory ;
      public:
         _rtnLocalTaskBase():_taskID(RTN_LT_INVALID_TASKID) {}
         virtual ~_rtnLocalTaskBase() {}

      public:
         INT32             toBson( BSONObj &obj ) const ;
         ossPoolString     toPrintString() const ;
         UINT64            getTaskID() const { return _taskID ; }
         BOOLEAN           isTaskValid() const
         {
            return _taskID != RTN_LT_INVALID_TASKID ? TRUE : FALSE ;
         }

      public:
         virtual RTN_LOCAL_TASK_TYPE getTaskType() const = 0 ;

         virtual INT32     initFromBson( const BSONObj &obj ) = 0 ;
         virtual BOOLEAN   muteXOn ( const _rtnLocalTaskBase *pOther ) const = 0 ;

      protected:
         virtual INT32     _toBson( BSONObjBuilder &builder ) const = 0 ;

      private:
         void              _setTaskID( UINT64 taskID ) { _taskID = taskID ; }

      private:
         UINT64            _taskID ;

   } ;
   typedef _rtnLocalTaskBase rtnLocalTaskBase ;
   typedef utilSharePtr<rtnLocalTaskBase>    rtnLocalTaskPtr ;

   /*
      Common define
   */
   typedef _rtnLocalTaskBase* (*RTN_NEW_LOCALTASK)() ;

   #define RTN_DECLARE_LT_AUTO_REGISTER() \
      public: \
         static _rtnLocalTaskBase *newThis ()

   #define RTN_IMPLEMENT_LT_AUTO_REGISTER(theClass) \
      _rtnLocalTaskBase *theClass::newThis () \
      { \
         return SDB_OSS_NEW theClass() ;\
      } \
      _rtnLTAssit theClass##Assit ( theClass::newThis )


   /*
      _rtnLTFactory define
   */
   class _rtnLTFactory : public SDBObject
   {
      typedef map< INT32, RTN_NEW_LOCALTASK >      MAP_LT ;
      typedef MAP_LT::iterator                     MAP_LT_IT ;

      friend class _rtnLTAssit ;

      public:
         _rtnLTFactory() ;
         ~_rtnLTFactory() ;

      public:
         INT32          create( INT32 taskType, rtnLocalTaskPtr &ptr ) ;

      protected:
         INT32          _register( INT32 taskType,
                                   RTN_NEW_LOCALTASK pFunc ) ;

      private:
         MAP_LT         _mapTask ;

   } ;
   typedef _rtnLTFactory rtnLTFactory ;

   rtnLTFactory* rtnGetLTFactory() ;

   /*
      _rtnLTAssit define
   */
   class _rtnLTAssit
   {
      public:
         _rtnLTAssit( RTN_NEW_LOCALTASK pFunc ) ;
         ~_rtnLTAssit() ;
   } ;
   typedef _rtnLTAssit rtnLTAssit ;

}

#endif // RTN_LOCALTASK_FACTORY_HPP__
