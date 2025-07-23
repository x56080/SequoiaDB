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

   Source File Name = ossMmap.hpp

   Descriptive Name = Operating System Services Memory Map Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains structure of Memory Map File.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OSSMMAP_HPP_
#define OSSMMAP_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossIO.hpp"
#include "ossRWMutex.hpp"
#include "ossUtil.hpp"
#include <vector>
#include <string>

using namespace std ;

/*
   _ossMmapFile define
*/
class _ossMmapFile : public SDBObject
{
public:
   class _ossMmapSegment : public SDBObject
   {
   public :
      ossValuePtr _ptr ;
      UINT32      _length ;
      UINT64      _offset ;
      UINT64      _lastAccessTick ;
      UINT64      _lastFreeTick ;
#if defined (_WINDOWS)
      HANDLE _maphandle ;
#endif
      _ossMmapSegment ( ossValuePtr ptr, UINT32 length, UINT64 offset )
      {
         _ptr = ptr ;
         _length = length ;
         _offset = offset ;
         _lastAccessTick = 1 ;
         _lastFreeTick = 0 ;
#if defined (_WINDOWS)
         _maphandle = INVALID_HANDLE_VALUE ;
#endif
      }

      _ossMmapSegment ()
      {
         _ptr = 0 ;
         _length = 0 ;
         _offset = 0 ;
         _lastAccessTick = 1 ;
         _lastFreeTick = 0 ;
#if defined (_WINDOWS)
         _maphandle = INVALID_HANDLE_VALUE ;
#endif
      }
   } ;
   typedef _ossMmapSegment ossMmapSegment ;

protected:
   OSSFILE  _file ;
   BOOLEAN  _opened ;
   UINT64   _totalLength ;
   string   _fileName ;

private:
   engine::ossRWMutex         _rwMutex ;

   ossMmapSegment*            _pSegArray ;
   UINT32                     _capacity ;
   UINT32                     _size ;
   ossMmapSegment*            _pTmpArray ;

   mutable volatile UINT64    _lastAccessTick ;
   UINT64                     _lastFreeTick ;

   void  _clearSeg() ;
   INT32 _ensureSpace( UINT32 size ) ;
   UINT64 (*_pGetTickFunc)() ;

public:

   OSS_INLINE UINT32 begin()
   {
      return 0 ;
   }

   OSS_INLINE ossMmapSegment* next( UINT32 &pos )
   {
      if ( pos >= _size )
      {
         return NULL ;
      }
      ++pos ;
      return &_pSegArray[ pos - 1 ] ;
   }

   OSS_INLINE UINT32 segmentSize() const
   {
      return _size ;
   }

   OSS_INLINE UINT64 totalLength() const
   {
      return _totalLength ;
   }

   OSS_INLINE ossValuePtr getSegmentInfo( UINT32 pos,
                                          UINT32 *pLength = NULL,
                                          UINT64 *pOffset = NULL ) const
   {
      ossValuePtr tmpPtr = 0 ;
      tmpPtr = _pSegArray[ pos ]._ptr ;
      if ( pLength )
      {
         *pLength = _pSegArray[ pos ]._length ;
      }
      if ( pOffset )
      {
         *pOffset = _pSegArray[ pos ]._offset ;
      }
      if ( _pGetTickFunc )
      {
         _lastAccessTick = (*_pGetTickFunc)() ;

         if ( 0 == _lastAccessTick )
         {
            _lastAccessTick = 1 ;
         }

         _pSegArray[ pos ]._lastAccessTick = _lastAccessTick ;
      }
      return tmpPtr ;
   }

   OSS_INLINE void setGetTickFunc( UINT64 (*pGetTickFunc)() )
   {
      _pGetTickFunc = pGetTickFunc ;

      if ( _pGetTickFunc )
      {
         _lastAccessTick = (*_pGetTickFunc)() ;

         if ( 0 == _lastAccessTick )
         {
            _lastAccessTick = 1 ;
         }
      }
   }

   OSS_INLINE void setFileFreeTick( UINT64 tick )
   {
      _lastFreeTick = tick ;
   }

   OSS_INLINE UINT64 getFileAccessTick() const
   {
      return _lastAccessTick ;
   }

   OSS_INLINE UINT64 getFileLastFreeTick() const
   {
      return _lastFreeTick ;
   }

   UINT64 getSegmentAccessTick( UINT32 pos ) const ;
   UINT64 getSegmentFreeTick( UINT32 pos ) const ;

public:
   _ossMmapFile ()
   {
      _opened = FALSE ;
      _totalLength = 0 ;
      _fileName.clear() ;

      _pSegArray = NULL ;
      _capacity = 0 ;
      _size = 0 ;
      _pTmpArray = NULL ;
      _lastAccessTick = 1 ;
      _lastFreeTick = 0 ;
      _pGetTickFunc = NULL ;
   }
   virtual ~_ossMmapFile ()
   {
      if ( _opened )
      {
         ossClose ( _file ) ;
         _opened = FALSE ;
      }
      _clearSeg() ;
   }
   INT32 open ( const CHAR *pFilename,
                UINT32 iMode = OSS_READWRITE|OSS_EXCLUSIVE|OSS_CREATE,
                UINT32 iPermission = OSS_RU|OSS_WU|OSS_RG ) ;
   void  close () ;
   INT32 map ( UINT64 offset, UINT32 length, void **pAddress ) ;
   INT32 flushAll ( BOOLEAN sync = FALSE ) ;
   INT32 flush ( UINT32 segmentID, BOOLEAN sync = FALSE ) ;
   /*
      length : -1, means flush offset to end
   */
   INT32 flushBlock ( UINT32 segmentID, UINT32 offset,
                      INT32 length, BOOLEAN sync = FALSE ) ;
   virtual INT32 freeCache ( UINT32 segmentID ) ;
   INT32 unlink () ;
   INT32 size ( UINT64 &fileSize ) ;

} ;
typedef class _ossMmapFile ossMmapFile ;

#endif // OSSMMAP_HPP_
