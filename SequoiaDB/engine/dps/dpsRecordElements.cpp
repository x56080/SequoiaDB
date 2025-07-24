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

   Source File Name = dpsRecordElements.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "dpsRecordElements.hpp"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
#include "dpsLogRecord.hpp"

#include <utility>

namespace engine
{
   _dpsRecordElements::_dpsRecordElements( utilUniqueBuffer &&buf,
                                           UINT32 size,
                                           INT32 num ) noexcept :
   _size( size ),
   _elementNum( num ),
   _buf( buf.get() ),
   _owner( std::move(buf) )
   {

   }

   _dpsRecordElements::_dpsRecordElements( const CHAR *buf,
                                           UINT32 size,
                                           INT32 num ) noexcept :
   _size( size ),
   _elementNum( num ),
   _buf( buf )
   {

   }

   _dpsRecordElements::_dpsRecordElements( _dpsRecordElements &&o ) noexcept :
   _size( o._size ),
   _elementNum( o._elementNum ),
   _buf( o._buf ),
   _owner( std::move(o._owner) )
   {
      o.reset() ;
   }

   _dpsRecordElements &_dpsRecordElements::operator=( _dpsRecordElements &&o ) noexcept
   {
      _size = o._size ;
      _elementNum = o._elementNum ;
      _buf = o._buf ;
      _owner = std::move( o._owner ) ;
      o.reset() ;
      return *this ;
   }

   BOOLEAN _dpsRecordElements::isValid() const
   {
      return nullptr != _buf && sizeof(dpsRecordEle) <= _size ;
   }

   INT32 _dpsRecordElements::getOwned()
   {
      INT32 rc = SDB_OK ;
      if ( isValid() && !isOwned() )
      {
         utilUniqueBuffer b = utilUniqueBuffer::allocate( _size ) ;
         if ( OSS_UNLIKELY(!b) )
         {
            PD_LOG( PDERROR, "failed to allocate mem." ) ;
            rc = SDB_OOM ;
            goto error ;  
         }

         ossMemcpy( b.get(), _buf, _size ) ;
         _owner = std::move( b ) ;
         _buf = _owner.get() ;
      }
   done:
      return rc ;
   error:
      goto done ; 
   }

   UINT32 _dpsRecordElements::getElementNum() const
   {
      if ( 0 <= _elementNum )
      {
         return static_cast<UINT32>(_elementNum) ;
      }
      else if ( isValid() )
      {
         UINT32 num = 0 ;
         iterator itr = begin() ;
         while ( itr.isValid() )
         {
            ++num ;
            itr.next() ;
         }
         _elementNum = static_cast<INT32>( num ) ;
         return num ;
      }
      else
      {
         return 0 ;
      }
   }
   
   _dpsRecordElements::iterator _dpsRecordElements::begin() const
   {
      return isValid() ? iterator( _buf, _size ) : iterator() ;
   }

   _dpsRecordElements::iterator _dpsRecordElements::seek( DPS_TAG tag ) const
   {
      SDB_ASSERT( DPS_INVALID_TAG != tag, "can not be invalid" ) ;

      if ( isValid() )
      {
         iterator itr( _buf, _size ) ;
         while ( itr.isValid() )
         {
            if ( tag == itr.getTag() )
            {
               return itr ;
            }
            else
            {
               itr.next() ;
            }
         }
      }
      
      return iterator() ;
   }

   BOOLEAN _dpsRecordElements::contains( DPS_TAG tag ) const
   {
      SDB_ASSERT( DPS_INVALID_TAG != tag, "can not be invalid" ) ;
      if ( isValid() )
      {
         iterator itr( _buf, _size ) ;
         while ( itr.isValid() )
         {
            if ( tag == itr.getTag() )
            {
               return TRUE ;
            }
            else
            {
               itr.next() ;
            }
         }
      }

      return FALSE ;
   }

///////////////////////////////////_dpsRecordElements::iterator
   _dpsRecordElements::iterator::iterator( const CHAR *data, UINT32 size ) noexcept :
   _data( data ),
   _size( size )
   {

   }

   BOOLEAN _dpsRecordElements::iterator::isValid() const
   {
      return nullptr != _data && sizeof(dpsRecordEle) <= _size ;
   }

   DPS_TAG _dpsRecordElements::iterator::getTag() const
   {
      return isValid() ?
             _getElementHeader()->tag : DPS_INVALID_TAG ;
   }

   utilSlice _dpsRecordElements::iterator::getValue() const
   {
      utilSlice s;
      if ( isValid() )
      {
         const _dpsRecordEle *e = _getElementHeader() ;
         s.reset( e->len, _data + sizeof( _dpsRecordEle ) ) ;
      }

      return s ;      
   }

   const _dpsRecordEle *_dpsRecordElements::iterator::_getElementHeader() const
   {
      SDB_ASSERT( isValid(), "can not be invalid" ) ;
      return reinterpret_cast<const dpsRecordEle *>( _data ) ;
   }

   BOOLEAN _dpsRecordElements::iterator::next()
   {
      if ( isValid() )
      {
         const _dpsRecordEle *e = _getElementHeader() ;
         if ( _size >= ((sizeof( _dpsRecordEle ) << 1) + e->len) )
         {
            _data += ( sizeof( _dpsRecordEle ) + e->len ) ;
            _size -= ( sizeof( _dpsRecordEle ) + e->len ) ;
            if ( DPS_INVALID_TAG == _getElementHeader()->tag )
            {
               reset() ;
               return FALSE ;
            }
            else
            {
            #if defined (_DEBUG)
               SDB_ASSERT( _size >= (sizeof(_dpsRecordEle) + _getElementHeader()->len),
                           "invalid element size!" ) ;
            #endif
               return TRUE ;
            }
         }
         else
         {
            reset() ; 
            return FALSE ;
         }
      }
      else
      {
         return FALSE ;
      }
   }
   
} // namespace engine
