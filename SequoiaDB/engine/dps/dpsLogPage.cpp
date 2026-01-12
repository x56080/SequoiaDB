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

   Source File Name = dpsLogPage.cpp

   Descriptive Name = Data Protection Services Log Page

   When/how to use: this program may be used on binary and text-formatted
   versions of DPS component. This file contains code logic for log page
   operations

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "dpsLogPage.hpp"
#include "pdTrace.hpp"
#include "dpsTrace.hpp"

namespace engine
{
   /*
      _dpsLogPage implement
   */
   _dpsLogPage::_dpsLogPage()
   :_mtx( MON_LATCH_DPSLOGPAGE_MTX, RW_SHARDWRITE )
   {
   //   _dpsLogPage( DPS_DEFAULT_PAGE_SIZE );
      // delete in destructor
      _mb = SDB_OSS_NEW _dpsMessageBlock( DPS_DEFAULT_PAGE_SIZE );
      if ( NULL == _mb )
      {
         PD_LOG( PDERROR, "Alocate message block failed" ) ;
      }
      _pageNumber = 0 ;
      _dirtyFlag  = 0 ;
   }

   _dpsLogPage::_dpsLogPage( UINT32 size )
   :_mtx( MON_LATCH_DPSLOGPAGE_MTX )
   {
      // delete in destructor
      _mb = SDB_OSS_NEW _dpsMessageBlock( size );
      if ( NULL == _mb )
      {
         PD_LOG( PDERROR, "Alocate message block failed" ) ;
      }

      _pageNumber = 0 ;
      _dirtyFlag  = 0 ;
   }

   _dpsLogPage::~_dpsLogPage()
   {
      lock();
      if ( _mb )
      {
         SDB_OSS_DEL _mb ;
         _mb = NULL ;
      }
      _mb = NULL;
      unlock();
   }

   string _dpsLogPage::toString() const
   {
      stringstream ss ;
      ss << "PageNumber: " << _pageNumber
         << "DirtyFlag: " << _dirtyFlag
         << ", BeginLSNV" << _beginLSN.version
         << ", BeginLSNO" << _beginLSN.offset
         << ", Length: " << getLength()
         << ", Size: " << getBufSize()
         << ", IdleSize: " << getLastSize() ;
      return ss.str() ;
   }

   // fill up data into log page
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLGPAGE_FILL, "_dpsLogPage::fill" )
   INT32 _dpsLogPage::fill( UINT32 offset, const CHAR *src, UINT32 len, BOOLEAN setDirty )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DPSLGPAGE_FILL );
      if ( !_mb )
      {
         _mb = SDB_OSS_NEW _dpsMessageBlock( DPS_DEFAULT_PAGE_SIZE );
         if ( NULL == _mb )
         {
            PD_LOG( PDERROR, "Alocate message block failed" ) ;
            rc = SDB_OOM ;
            goto error ;
         }
      }
      ossMemcpy ( _mb->offset( offset ), src, len ) ;

      if ( setDirty )
      {
         makeDirty() ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__DPSLGPAGE_FILL, rc );
      return rc ;
   error :
      goto done ;
   }

   void _dpsLogPage::truncate( UINT32 offset )
   {
      if ( _mb && _mb->size() > offset )
      {
         /// set write prt
         _mb->writePtr( offset ) ;
         /// invalidate last data
         _mb->invalidateData() ;
         /// make dirty
         makeDirty() ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLGPAGE_RESERVE, "_dpsLogPage::reserve" )
   INT32 _dpsLogPage::reserve( UINT32 len, BOOLEAN setDirty )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB__DPSLGPAGE_RESERVE ) ;

      if ( !_mb )
      {
         _mb = SDB_OSS_NEW _dpsMessageBlock( DPS_DEFAULT_PAGE_SIZE );
         if ( NULL == _mb )
         {
            PD_LOG( PDERROR, "Alocate message block failed" ) ;
            rc = SDB_OOM ;
            goto error ;
         }
      }

      if ( getLastSize() >= len )
      {
         // change current writing position
         _mb->writePtr( len + _mb->length() ) ;
      }
      else
      {
         SDB_ASSERT( FALSE, "Invalid len" ) ;
         rc = SDB_OSS_UP_TO_LIMIT ;
         PD_LOG( PDERROR, "Last size(%d) is less than len(%d), rc: %d",
                 getLastSize(), len ) ;
         goto error ;
      }

      if ( setDirty )
      {
         makeDirty() ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__DPSLGPAGE_RESERVE, rc );
      return rc ;
   error :
      goto done ;
   }

   // allocate and return the offset
   INT32 _dpsLogPage::reserve( UINT32 len, UINT64 &offset, BOOLEAN setDirty )
   {
      offset = getLength();
      return reserve( len, setDirty ) ;
   }

}

