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

   Source File Name = dmsStorageJob.hpp

   Descriptive Name = Data Management Service Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/10/2013  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMS_STORAGE_JOB_HPP__
#define DMS_STORAGE_JOB_HPP__

#include "rtnBackgroundJobBase.hpp"
#include "monDMS.hpp"
#include "ossLatch.hpp"
#include "ossEvent.hpp"

namespace engine
{

   class _dmsStorageBase ;
   class _dmsStorageUnit ;
   class _dmsMBContext ;
   class _dmsPageMap ;

   #define DMS_EXTEND_JOB_NAME_LEN           ( 200 )
   /*
      _dmsExtendSegmentJob define
   */
   class _dmsExtendSegmentJob : public _rtnBaseJob
   {
      public:
         _dmsExtendSegmentJob ( _dmsStorageBase *pSUBase ) ;
         virtual ~_dmsExtendSegmentJob () ;

      public:
         virtual RTN_JOB_TYPE type () const ;
         virtual const CHAR* name () const ;
         virtual BOOLEAN muteXOn ( const _rtnBaseJob *pOther ) ;
         virtual INT32 doit () ;

         virtual BOOLEAN reuseEDU() const { return TRUE ; }

      private:
         _dmsStorageBase            *_pSUBase ;
         CHAR                       _name[ DMS_EXTEND_JOB_NAME_LEN + 1 ] ;

   } ;
   typedef _dmsExtendSegmentJob  dmsExtendSegmentJob ;

   /*
      _dmsPageMappingDispatcher define
   */
   class _dmsPageMappingDispatcher : public SDBObject
   {
      public:
         _dmsPageMappingDispatcher() ;
         ~_dmsPageMappingDispatcher() ;

         INT32       active() ;

         BOOLEAN     dispatchItem( monCSName &item ) ;
         void        endDispatch() ;
         UINT32      prepare() ;

         ossEvent*   getEmptyEvent() ;
         ossEvent*   getNtyEvent() ;

         void        exitJob( BOOLEAN isControl ) ;

      protected:
         void        _checkAndStartJob( BOOLEAN needLock ) ;

      private:
         MON_CSNAME_VEC       _vecCSName ;
         ossSpinXLatch        _latch ;
         ossEvent             _ntyEvent ;
         ossAutoEvent         _emptyEvent ;

         BOOLEAN              _startCtrlJob ;
         UINT32               _curAgent ;
         UINT32               _idleAgent ;
   } ;
   typedef _dmsPageMappingDispatcher dmsPageMappingDispatcher ;

   /*
      _dmsPageMappingJob define
   */
   class _dmsPageMappingJob : public _rtnBaseJob
   {
      public:
         _dmsPageMappingJob( dmsPageMappingDispatcher *pDispatcher,
                             INT32 timeout = -1 ) ;
         virtual ~_dmsPageMappingJob() ;

         BOOLEAN isControlJob() const ;

      public:
         virtual RTN_JOB_TYPE type () const ;
         virtual const CHAR* name () const ;
         virtual BOOLEAN muteXOn ( const _rtnBaseJob *pOther ) ;
         virtual INT32 doit () ;

      protected:
         void           _doUnit( const monCSName *pItem ) ;
         void           _doACollection( _dmsStorageUnit *su,
                                        _dmsMBContext *mbContext,
                                        _dmsPageMap *pPageMap ) ;

      private:
         dmsPageMappingDispatcher  *_pDispatcher ;
         INT32                      _timeout ;

   } ;
   typedef _dmsPageMappingJob dmsPageMappingJob ;

   /*
      Function define
   */
   INT32 startExtendSegmentJob ( EDUID *pEDUID, _dmsStorageBase *pSUBase ) ;

   INT32 dmsStartMappingJob( EDUID *pEDUID,
                             dmsPageMappingDispatcher *pDispatcher,
                             INT32 timeout ) ;

}

#endif //DMS_STORAGE_JOB_HPP__

