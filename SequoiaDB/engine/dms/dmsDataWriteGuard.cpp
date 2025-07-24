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

   Source File Name = dmsDataWriteGuard.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#include "dmsDataWriteGuard.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"
#include "pmdEDU.hpp"
#include "dmsStorageDataCommon.hpp"

using namespace std ;

namespace engine
{

   /*
      _dmsDataWriteGuard imeplement
    */
   _dmsDataWriteGuard::_dmsDataWriteGuard()
   : _su( nullptr ),
     _mbID( DMS_INVALID_MBID ),
     _eduCB( nullptr ),
     _isEnabled( FALSE ),
     _isInWrite( FALSE )
   {
   }

   _dmsDataWriteGuard::_dmsDataWriteGuard( dmsStorageDataCommon *su,
                                           dmsMBContext *mbContext,
                                           pmdEDUCB *cb,
                                           BOOLEAN isEnabled )
   : _su( su ),
     _mbStat( mbContext->mbStat() ),
     _mbID( mbContext->mbID() ),
     _eduCB( cb ),
     _isEnabled( isEnabled ),
     _isInWrite( FALSE )
   {
   }

   _dmsDataWriteGuard::~_dmsDataWriteGuard()
   {
      abort() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSDATAWRITEGUARD_BEFOREWRITE, "_dmsDataWriteGuard::beforeWrite" )
   void _dmsDataWriteGuard::beforeWrite()
   {
      PD_TRACE_ENTRY( SDB__DMSDATAWRITEGUARD_BEFOREWRITE ) ;

      if ( _isEnabled && !_isInWrite )
      {
         _su->markDirty( _mbID, DMS_CHG_BEFORE ) ;
         _su->incWritePtrCount( _mbID ) ;
         _isInWrite = TRUE ;
      }

      PD_TRACE_EXIT( SDB__DMSDATAWRITEGUARD_BEFOREWRITE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSDATAWRITEGUARD_AFTERWRITE, "_dmsDataWriteGuard::afterWrite" )
   void _dmsDataWriteGuard::afterWrite()
   {
      PD_TRACE_ENTRY( SDB__DMSDATAWRITEGUARD_AFTERWRITE ) ;

      if ( _isEnabled && _isInWrite )
      {
         _mbStat->_snapshotID.inc() ;
         _su->markDirty( _mbID, DMS_CHG_AFTER ) ;
         _su->decWritePtrCount( _mbID ) ;
         _isInWrite = FALSE ;
      }

      PD_TRACE_EXIT( SDB__DMSDATAWRITEGUARD_AFTERWRITE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSDATAWRITEGUARD_BEGIN, "_dmsDataWriteGuard::begin" )
   INT32 _dmsDataWriteGuard::begin()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSDATAWRITEGUARD_BEGIN ) ;

      beforeWrite() ;

      PD_TRACE_EXITRC( SDB__DMSDATAWRITEGUARD_BEGIN, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSDATAWRITEGUARD_COMMIT, "_dmsDataWriteGuard::commit" )
   INT32 _dmsDataWriteGuard::commit()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSDATAWRITEGUARD_COMMIT ) ;

      afterWrite() ;

      PD_TRACE_EXITRC( SDB__DMSDATAWRITEGUARD_COMMIT, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSDATAWRITEGUARD_ABORT, "_dmsDataWriteGuard::abort" )
   INT32 _dmsDataWriteGuard::abort( BOOLEAN isForced )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSDATAWRITEGUARD_ABORT ) ;

      afterWrite() ;

      PD_TRACE_EXITRC( SDB__DMSDATAWRITEGUARD_ABORT, rc ) ;

      return rc ;
   }

}
