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

   Source File Name = dmsTransContext.cpp

   Descriptive Name = Data Management Service Header

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   dms Reccord ID (RID).

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/14/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#include "dmsTransContext.hpp"
#include "dmsStorageDataCommon.hpp"
#include "rtnIXScanner.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"
// #include "pmd.hpp"    // pmdGetKRCB
#include "pmdEDU.hpp" // _pmdEDUCB
#include "dmsCB.hpp"  // SDB_DMSCB

namespace engine
{

   /*
      _dmsTBTransContext implement
   */
   _dmsTBTransContext::_dmsTBTransContext( _dmsMBContext *pMBContext,
                                           DMS_ACCESS_TYPE accessType,
                                           _pmdEDUCB *cb )
   {
      SDB_ASSERT( pMBContext, "MB Context can't be NULL" ) ;
      SDB_ASSERT( cb, "EDUCB can't be NULL" ) ;
      _pMBContext    = pMBContext ;
      _accessType    = accessType ;
      _pauseFlag     = DMS_TRNCTX_PAUSE_FLG_NONE ;
      _cb            = cb ;
   }

   _dmsTBTransContext::~_dmsTBTransContext()
   {
   }

   INT32 _dmsTBTransContext::_checkAccess()
   {
      INT32 rc = SDB_OK ;

      if ( !dmsAccessAndFlagCompatiblity ( _pMBContext->mb()->_flag,
                                           _accessType ) )
      {
         PD_LOG ( PDERROR, "Incompatible collection mode: %d",
                  _pMBContext->mb()->_flag ) ;
         rc = SDB_DMS_INCOMPATIBLE_MODE ;
      }

      return rc ;
   }


   void _dmsTBTransContext::setPauseFlag( UINT32 flag )
   {
      _pauseFlag = flag ;
   }

   void _dmsTBTransContext::resetPauseFlag()
   {
      _pauseFlag = DMS_TRNCTX_PAUSE_FLG_NONE ;
   }

   UINT32 _dmsTBTransContext::getPauseFlag()
   {
      return _pauseFlag ;
   }


   INT32 _dmsTBTransContext::pause()
   {
      INT32 rc = SDB_OK ;

      const UINT32 flag = getPauseFlag() ;

      if ( DMS_TRNCTX_PAUSE_FLG_UNLOCKALLEXT == ( flag & 0xF0 ) )
      {
         SDB_DMSCB *pDMSCB = pmdGetKRCB()->getDMSCB() ;
         pDMSCB->unlockAllExtent( _cb ) ;
      }
    
      if ( ! ( DMS_TRNCTX_PAUSE_FLG_KEEPMBLATCH == ( flag & 0xF000 ) ) )
      {
         rc = _pMBContext->pause() ;
      }
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSTBTRANSCONTEXT_RESUME, "_dmsTBTransContext::resume" )
   INT32 _dmsTBTransContext::resume( UINT8 whom )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSTBTRANSCONTEXT_RESUME ) ;

      if ( OSS_BIT_TEST( whom, ICTX_RESUME_CONTEXT ) )
      {
         rc = _pMBContext->resume( ICTX_RESUME_CONTEXT ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Resume dms mblock[%s] failed, rc: %d",
                    _pMBContext->toString().c_str(), rc ) ;
            goto error ;
         }
      }

      rc = _checkAccess() ;
      if ( rc )
      {
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__DMSTBTRANSCONTEXT_RESUME, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   /*
      _dmsIXTransContext implement
   */
   _dmsIXTransContext::_dmsIXTransContext( _dmsMBContext *pMBContext,
                                           DMS_ACCESS_TYPE accessType,
                                           _rtnIXScanner *pScanner,
                                           _pmdEDUCB * cb )
   :_dmsTBTransContext( pMBContext, accessType, cb )
   {
      SDB_ASSERT( pScanner, "Scanner can't be NULL" ) ;
      _pScanner      = pScanner ;
      _isSame        = TRUE ;
   }

   _dmsIXTransContext::~_dmsIXTransContext()
   {
   }

   INT32 _dmsIXTransContext::pause()
   {
      INT32 rc = SDB_OK ;
      const UINT32 flag = getPauseFlag() ;

      _isSame = TRUE ;

      if ( DMS_TRNCTX_PAUSE_FLG_IXADVONHOLD == ( flag & 0xF ) )
      {
         _pScanner->informAdvanceToCurrentPos() ;
      }

      rc = _pScanner->pauseScan() ;

      if ( DMS_TRNCTX_PAUSE_FLG_UNLOCKALLEXT == ( flag & 0xF0 ) )
      {
         SDB_DMSCB *pDMSCB = pmdGetKRCB()->getDMSCB() ;
         pDMSCB->unlockAllExtent( _cb ) ;
      }

      if ( SDB_OK == rc )
      {
         if ( !( DMS_TRNCTX_PAUSE_FLG_IXMCTXONLY == ( flag & 0xF00 ) ) )
         {
            rc = _dmsTBTransContext::pause() ;
         }
      }
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSIXTRANSCONTEXT_RESUME, "_dmsIXTransContext::resume" )
   INT32 _dmsIXTransContext::resume( UINT8 whom )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSIXTRANSCONTEXT_RESUME ) ;

      /// first resume base
      if ( OSS_BIT_TEST( whom, ICTX_RESUME_CONTEXT ) )
      {
         rc = _dmsTBTransContext::resume( whom ) ;
         if ( rc )
         {
            goto error ;
         }
      }

      /// then resume scanner
      if ( OSS_BIT_TEST( whom, ICTX_RESUME_SCANNER ) )
      {
         rc = _pScanner->resumeScan( &_isSame ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Resume index scanner failed, rc: %d",
                    rc ) ;
            goto error ;
         }
      }
   done:
      PD_TRACE_EXITRC ( SDB__DMSIXTRANSCONTEXT_RESUME, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _dmsIXTransContext::isCursorSame() const
   {
      return _isSame ;
   }

}


