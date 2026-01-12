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

   Source File Name = dpsLogPage.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declare for data types used in
   SequoiaDB.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DPSLOGPAGE_HPP_
#define DPSLOGPAGE_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossUtil.hpp"
#include "dpsMessageBlock.hpp"
#include "monLatch.hpp"
#include "dpsLogDef.hpp"
#include <string>

using namespace std ;

namespace engine
{

   /*
      _dpsLogPage define
   */
   class _dpsLogPage : public SDBObject
   {
      private:
         monRWMutex           _mtx;
         _dpsMessageBlock     *_mb;
         UINT32               _pageNumber;
         BYTE                 _dirtyFlag ;
         DPS_LSN              _beginLSN ;

      public:
         _dpsLogPage();

         _dpsLogPage( UINT32 size );

         ~_dpsLogPage();

         string toString() const ;

      public:
         OSS_INLINE UINT32 getLastSize() const
         {
            return _mb->idleSize();
         }

         OSS_INLINE UINT32 getBufSize() const
         {
            return _mb->size();
         }

         OSS_INLINE UINT32 getLength() const
         {
            return _mb->length();
         }

         OSS_INLINE void clear()
         {
            _dirtyFlag = 0 ;
            _mb->clear();
         }

         OSS_INLINE void invalidate()
         {
            clear() ;
            _beginLSN.reset() ;
         }

         OSS_INLINE void zeroLastData()
         {
            if ( _mb )
            {
               _mb->invalidateData() ;
               makeDirty() ;
            }
         }

         OSS_INLINE void setBeginLSN ( const DPS_LSN &lsn )
         {
            _beginLSN = lsn  ;
         }

         OSS_INLINE DPS_LSN getBeginLSN ()
         {
            return _beginLSN ;
         }

         OSS_INLINE _dpsMessageBlock *mb()
         {
            return _mb;
         }

         OSS_INLINE void setNumber( UINT32 number )
         {
            _pageNumber = number;
         }

         OSS_INLINE UINT32 getNumber()
         {
            return _pageNumber;
         }

         OSS_INLINE BOOLEAN isDirty() const
         {
            return _dirtyFlag != 0 ? TRUE : FALSE ;
         }

         OSS_INLINE void makeDirty()
         {
            _dirtyFlag = 1 ;
         }

         OSS_INLINE void clearDirty()
         {
            _dirtyFlag = 0 ;
         }

         OSS_INLINE void lockShared()
         {
            _mtx.lock_r();
         }

         OSS_INLINE void unlockShared()
         {
            _mtx.release_r() ;
         }

         OSS_INLINE void lock()
         {
            _mtx.lock_w() ;
         }

         OSS_INLINE void unlock()
         {
            _mtx.release_w() ;
         }

      public:

         INT32 fill( UINT32 offset, const CHAR *src, UINT32 len, BOOLEAN setDirty = TRUE );

         void  truncate( UINT32 offset ) ;

         INT32 reserve( UINT32 len, UINT64 &offset, BOOLEAN setDirty = TRUE );

         INT32 reserve( UINT32 len, BOOLEAN setDirty = TRUE );
   };

   typedef class _dpsLogPage dpsLogPage;
}

#endif // DPSLOGPAGE_HPP_

