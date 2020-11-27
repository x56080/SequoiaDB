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

