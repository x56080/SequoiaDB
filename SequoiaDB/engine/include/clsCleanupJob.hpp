/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = clsCleanupJob.hpp

   Descriptive Name = Data Management Service Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          20/03/2013  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CLS_CLEANUP_JOB_HPP_
#define CLS_CLEANUP_JOB_HPP_

#include "rtnBackgroundJob.hpp"
#include "dmsLobDef.hpp"

using namespace bson ;

namespace engine
{
   enum CLS_CLEANUP_TYPE
   {
      CLS_CLEANUP_BY_RANGE          = 0,
      CLS_CLEANUP_BY_CATAINFO,
      CLS_CLEANUP_BY_SHARDINGINDEX
   };

   class _clsCleanupJob : public _rtnBaseJob
   {
      public:
         _clsCleanupJob( const std::string &clFullName,
                         const BSONObj &splitKeyObj,
                         const BSONObj &splitEndKeyObj,
                         BOOLEAN hasShardingIndex,
                         BOOLEAN isHashSharding,
                         SDB_DPSCB *dpsCB ) ;
         virtual ~_clsCleanupJob() ;

      public:
         virtual RTN_JOB_TYPE type () const ;
         virtual const CHAR* name () const ;
         virtual BOOLEAN muteXOn ( const _rtnBaseJob *pOther ) ;
         virtual INT32 doit () ;

      protected:
         INT32   _cleanByTBSCan ( INT32 w, CLS_CLEANUP_TYPE cleanType ) ;
         INT32   _cleanBySplitKeyObj ( INT32 w ) ;
         INT32   _cleanLobData( INT32 w ) ;

         INT32   _filterDel ( const CHAR* buff, INT32 buffSize,
                              CLS_CLEANUP_TYPE cleanType,
                              UINT32 groupID ) ;

         CLS_CLEANUP_TYPE _cleanupType () const ;

         void  _makeName () ;

      private:
         INT32 _filterDel( const _dmsLobInfoOnPage &page,
                           BOOLEAN &need2Remove ) ;

      protected:
         std::string          _clFullName ;
         BSONObj              _splitKeyObj ;
         BSONObj              _splitEndKeyObj ;
         BOOLEAN              _hasShardingIndex ;
         BOOLEAN              _isHashSharding ;

         std::string          _name ;

         SDB_DPSCB            *_dpsCB ;
         SDB_DMSCB            *_dmsCB ;

   };
   typedef class _clsCleanupJob clsCleanupJob ;

   INT32 startCleanupJob ( const std::string &clFullName,
                           const BSONObj &splitKeyObj,
                           const BSONObj &splitEndKeyObj,
                           BOOLEAN hasShardingIndex,
                           BOOLEAN isHashSharding,
                           SDB_DPSCB *dpsCB,
                           EDUID *pEDUID = NULL,
                           BOOLEAN returnResult = FALSE ) ;

}

#endif //CLS_CLEANUP_JOB_HPP_

