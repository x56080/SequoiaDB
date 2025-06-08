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

   Source File Name = dmsRecycleRecordJob.hpp

   Descriptive Name = Recycle Record Job Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/

#ifndef DMS_RECYCLE_RECORD_JOB_HPP__
#define DMS_RECYCLE_RECORD_JOB_HPP__

#include "rtnBackgroundJobBase.hpp"
#include "dmsCB.hpp"

namespace engine
{
   /*
    *  _dmsRecycleJobInfo define
    */
   struct _dmsRecycleJobInfo : public SDBObject
   {
      monCLSimple _cl ;
      UINT64 _lastWriteCount ;

      _dmsRecycleJobInfo( const monCLSimple &cl ) :
            _cl( cl ),
            _lastWriteCount( OSS_UINT64_MAX ) {}

      BOOLEAN operator < ( const _dmsRecycleJobInfo &r ) const
      {
         return _cl < r._cl ;
      }
   } ;

   /*
    *  _dmsRecycleRecordJob define
    */
   class _dmsRecycleRecordJob : public _rtnBaseJob
   {
      typedef ossPoolSet< _dmsRecycleJobInfo > CLS_JOB_SET ;

   public:
      _dmsRecycleRecordJob () ;
      virtual ~_dmsRecycleRecordJob () ;

   public:
      virtual RTN_JOB_TYPE type () const { return RTN_JOB_DMS_RECYCLE_RECORD ; }

      virtual const CHAR* name () const { return "RecycleRecord" ; }

      virtual BOOLEAN muteXOn ( const _rtnBaseJob *pOther ) { return FALSE ; }

      virtual INT32 doit () ;

   private:
      void _addJob2Set( CLS_JOB_SET &set, const _dmsRecycleJobInfo &jobInfo ) ;

      void _doRecycleRecordJobs( pmdEDUCB *eduCB,
                                 SDB_DMSCB *pDmsCB,
                                 dpsTransCB *pTransCB,
                                 CLS_JOB_SET &set ) ;

      INT32 _doRecycleRecordJob( pmdEDUCB *eduCB,
                                 SDB_DMSCB *pDmsCB,
                                 dpsTransCB *pTransCB,
                                 const _dmsRecycleJobInfo &jobInfo,
                                 BOOLEAN &hasUserWrite,
                                 UINT64 &lastWriteCount ) ;

      INT32 _deleteFirstDeletingRecord( pmdEDUCB *eduCB,
                                        dpsTransCB *pTransCB,
                                        dmsStorageUnit *su,
                                        dmsMBContext *pContext,
                                        const _dmsRecycleJobInfo &jobInfo ) ;
   } ;

   typedef _dmsRecycleRecordJob dmsRecycleRecordJob ;

   INT32 dmsStartRecycleRecordJob ( EDUID *pEDUID ) ;
}

#endif //DMS_RECYCLE_RECORD_JOB_HPP__