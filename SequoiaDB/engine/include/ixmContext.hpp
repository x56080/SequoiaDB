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

   Source File Name = ixmContext.hpp

   Descriptive Name = Index Management Context

   When/how to use: this program may be used on binary and text-formatted
   versions of index management component. This file contains structure for
   index extent and its methods. The B Tree Insert/Delete/Update methods are
   also defined in this file.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/10/2019  JT  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IXMCONTEXT_HPP_
#define IXMCONTEXT_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "dms.hpp"
#include "dpsTransLockMgr.hpp"
using namespace bson ;

namespace engine
{
   class _pmdEDUCB ;

   class _ixmLockInfo : public SDBObject
   {
   public :
      dmsExtentID         page ;     // index page id/number
      INT8                lockMode ;
   public :
      OSS_INLINE void reset()
      {
         page       = DMS_INVALID_EXTENT ;
         lockMode   = DPS_TRANSLOCK_MAX ;
      }

      _ixmLockInfo() { reset() ; }
      _ixmLockInfo( const _ixmLockInfo &rhs ) { *this = rhs ; }

      _ixmLockInfo( dmsExtentID pageNumber )
      {
         reset();
         page = pageNumber ;
      }

      OSS_INLINE BOOLEAN isValid()
      {
         return ( DMS_INVALID_EXTENT != page ) ? TRUE : FALSE ;
      }

      OSS_INLINE void setPage( const dmsExtentID pageNumber )
      {
         reset() ;
         page = pageNumber ;
      }

      OSS_INLINE _ixmLockInfo& operator= ( const _ixmLockInfo &rhs )
      {
         page       = rhs.page ;
         lockMode   = rhs.lockMode ;
         return *this ;
      }
   } ;

   class _ixmContext : public SDBObject
   {
   public :
      _ixmContext ( _pmdEDUCB * eduCB,
                    UINT32      logicalCSID ) ;

      ~_ixmContext() ;

      OSS_INLINE ixmIndexLockManager * getLockMgr() { return _pIndexLockMgr ; }

      OSS_INLINE UINT32 logicalCSID() { return _logicalCSID ; }

      OSS_INLINE _pmdEDUCB * eduCB() { return _eduCB ; }

      BOOLEAN isLocking( const dmsExtentID idxPage ) ;

      BOOLEAN getLockHeldInfo( _ixmLockInfo & lockInfo ) ;

   private :
      _pmdEDUCB             * _eduCB ;
      ixmIndexLockManager   * _pIndexLockMgr ;
      UINT32                  _logicalCSID ;
   } ;

   // try to lock an index page
   INT32 ixmTryLock( _ixmContext *pContext, dmsExtentID idxPage, INT8 mode ) ;

   // lock an index page
   INT32 ixmLock( _ixmContext *pContext, dmsExtentID idxPage, INT8 mode ) ;

   // release an index page lock
   void ixmUnlock( _ixmContext * pContext,
                   dmsExtentID idxPage,
                   BOOLEAN bForceRelease = FALSE ) ;

   // force release locks on all index pages
   void ixmUnlockAll( _ixmContext * pContext ) ;

   // count and print( optional ) all index locks
   // this function is mainly for debugging
   UINT32 ixmCountAllIndexLocks( _ixmContext * pContext,
                                 BOOLEAN       bPrintLog = FALSE,
                                 CHAR        * memoStr   = NULL ) ;
}

#endif //IXMCONTEXT_HPP_

