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

   Source File Name = rtnBackgroundJob.hpp

   Descriptive Name = Data Management Service Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/06/2013  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_BACKGROUND_JOB_HPP_
#define RTN_BACKGROUND_JOB_HPP_

#include "rtnBackgroundJobBase.hpp"
#include "dms.hpp"
#include "dpsLogWrapper.hpp"
#include "dmsCB.hpp"
#include "dmsIdxTaskStatus.hpp"
#include <string>

#include "../bson/bsonobj.h"

using namespace bson ;

namespace engine
{
   /*
      _rtnIndexJob define
   */
   class _rtnIndexJob : public _rtnBaseJob
   {
      public:
         _rtnIndexJob ( RTN_JOB_TYPE type,
                        const CHAR *pCLName,
                        const BSONObj &indexObj, SDB_DPSCB *dpsCB,
                        UINT64 offset, BOOLEAN isRollBack ) ;

         virtual ~_rtnIndexJob() ;

         INT32 init () ;
         const CHAR* getIndexName () const ;
         const CHAR* getCollectionName() const ;

         static INT32 checkIndexExist( const CHAR *pCLName,
                                       const CHAR *pIdxName,
                                       BOOLEAN &hasExist ) ;

      public:
         virtual RTN_JOB_TYPE type () const ;
         virtual const CHAR* name () const ;
         virtual BOOLEAN muteXOn ( const _rtnBaseJob *pOther ) ;
         virtual INT32 doit () ;

      protected:
         RTN_JOB_TYPE            _type ;
         CHAR                    _clFullName[DMS_COLLECTION_FULL_NAME_SZ + 1] ;
         std::string             _indexName ;
         std::string             _jobName ;
         BSONObj                 _indexObj ;
         BSONElement             _indexEle ;
         BOOLEAN                 _hasAddUnique ;
         UINT32                  _csLID ;
         UINT32                  _clLID ;
         SDB_DPSCB*              _dpsCB ;
         SDB_DMSCB*              _dmsCB ;
         UINT64                  _lsn ;
         BOOLEAN                 _isRollback ;
         BOOLEAN                 _regCLJob ;
   };
   typedef _rtnIndexJob rtnIndexJob ;

   /*
      _rtnIndexJobHolder define
    */
   class _rtnIndexJobHolder : public utilPooledObject
   {
   public:
      _rtnIndexJobHolder() ;
      ~_rtnIndexJobHolder() ;

      // register collection index job
      INT32 regCLJob( const CHAR *collection ) ;

      // unregister collection index job
      void  unregCLJob( const CHAR *collection ) ;

      // has collection job
      BOOLEAN hasCLJob( const CHAR *collection ) ;

      // clear job holder
      void fini() ;

   protected:
      void _unregCLJob( const ossPoolString &collection ) ;
      void _unregCLJobIter( const CHAR *collection ) ;
      BOOLEAN _hasCLJob( const ossPoolString &collection ) ;
      BOOLEAN _hasCLJobIter( const CHAR *collection ) ;

   protected:
      typedef ossPoolMap< ossPoolString, UINT32 > CL_JOB_MAP ;
      ossSpinSLatch  _mapLatch ;
      CL_JOB_MAP     _clJobs ;
   } ;

   typedef class _rtnIndexJobHolder rtnIndexJobHolder ;

   rtnIndexJobHolder *rtnGetIndexJobHolder() ;

   /*
      _rtnCleanupIdxStatusJob define
   */
   class _rtnCleanupIdxStatusJob : public _utilLightJob
   {
      public:
         _rtnCleanupIdxStatusJob() {}
         virtual ~_rtnCleanupIdxStatusJob() {}

      public:
         virtual const CHAR* name() const ;
         virtual INT32 doit( IExecutor *pExe, UTIL_LJOB_DO_RESULT &result,
                             UINT64 &sleepTime ) ;
   };
   typedef _rtnCleanupIdxStatusJob rtnCleanupIdxStatusJob ;

   INT32 rtnStartCleanupIdxStatusJob() ;

   /*
      _rtnLoadJob define
   */
   class _rtnLoadJob : public _rtnBaseJob
   {
      public:
         _rtnLoadJob() {}
         virtual ~_rtnLoadJob() {}

      public:
         virtual RTN_JOB_TYPE type () const ;
         virtual const CHAR* name () const ;
         virtual BOOLEAN muteXOn ( const _rtnBaseJob *pOther ) ;
         virtual INT32 doit () ;
   };
   typedef _rtnLoadJob rtnLoadJob ;

   INT32 rtnStartLoadJob() ;

   typedef void (*RTN_ON_REBUILD_DONE_FUNC)( INT32 rc ) ;
   /*
      _rtnRebuildJob define
   */
   class _rtnRebuildJob : public _rtnBaseJob
   {
      public:
         _rtnRebuildJob() ;
         virtual ~_rtnRebuildJob() ;
      public:
         virtual RTN_JOB_TYPE type () const ;
         virtual const CHAR* name () const ;
         virtual BOOLEAN muteXOn ( const _rtnBaseJob *pOther ) ;
         virtual INT32 doit () ;

         void    setInfo( RTN_ON_REBUILD_DONE_FUNC pFunc = NULL ) ;

     private:
         RTN_ON_REBUILD_DONE_FUNC   _pFunc ;
   } ;
   typedef _rtnRebuildJob rtnRebuildJob ;

   INT32    rtnStartRebuildJob( RTN_ON_REBUILD_DONE_FUNC pFunc = NULL ) ;

}

#endif //RTN_BACKGROUND_JOB_HPP_

