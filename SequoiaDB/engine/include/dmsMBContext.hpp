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

   Source File Name = dmsMBContext.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SDB_DMS_MB_CONTEXT_HPP_
#define SDB_DMS_MB_CONTEXT_HPP_

#include "sdbInterface.hpp"
#include "utilPooledObject.hpp"
#include "monCB.hpp"
#include "dms.hpp"
#include "monLatch.hpp"
#include "dmsMetaBlock.hpp"

namespace engine
{
   /*
      _dmsContext define
   */
   class _dmsContext : public _IContext, public _utilPooledObject
   {
      public:
         _dmsContext () {}
         virtual ~_dmsContext () {}

      public:
         virtual string toString () const = 0 ;
         virtual UINT16 mbID() const = 0 ;

   };
   typedef _dmsContext  dmsContext ;

   class _dmsMBContext : public _dmsContext
   {
      friend class _dmsStorageDataCommon ;
      private:
         _dmsMBContext() ;
         virtual ~_dmsMBContext() ;
         void _reset () ;

      public:
         virtual string toString () const ;
         virtual INT32  pause () ;
         virtual INT32  resume () ;

         void setSubContext( _IContext *subContext ) ;

         OSS_INLINE INT32   mbLock( INT32 lockType ) ;
         OSS_INLINE INT32   mbTryLock( INT32 lockType ) ;
         OSS_INLINE INT32   mbUnlock() ;
         OSS_INLINE BOOLEAN isMBLock( INT32 lockType ) const ;
         OSS_INLINE BOOLEAN isMBLock() const ;
         OSS_INLINE BOOLEAN canResume() const ;

         virtual     UINT16 mbID () const { return _mbID ; }
         OSS_INLINE  dmsMB* mb () { return _mb ; }
         OSS_INLINE  dmsMBStatInfo* mbStat() { return _mbStat ; }
         OSS_INLINE  UINT32 clLID () const { return _clLID ; }
         OSS_INLINE  UINT32 startLID() const { return _startLID ; }
         OSS_INLINE  INT32  mbLockType() const { return _mbLockType ; }

      private:
         OSS_INLINE INT32   _mbLock( INT32 lockType, BOOLEAN isTry ) ;
      private:
         dmsMB             *_mb ;
         dmsMBStatInfo     *_mbStat ;
         monSpinSLatch     *_latch ;
         UINT32            _clLID ;
         UINT32            _startLID ;
         UINT16            _mbID ;
         INT32             _mbLockType ;
         INT32             _resumeType ;
         _IContext         *_pSubContext ;
   };
   /*
      _dmsMBContext OSS_INLINE functions
   */
   OSS_INLINE INT32 _dmsMBContext::_mbLock( INT32 lockType,
                                            BOOLEAN isTry )
   {
      INT32 rc = SDB_OK ;
      if ( SHARED != lockType && EXCLUSIVE != lockType )
      {
         return SDB_INVALIDARG ;
      }
      if ( _mbLockType == lockType )
      {
         return SDB_OK ;
      }
      // already lock(type not same), need to unlock
      if ( -1 != _mbLockType && SDB_OK != ( rc = pause() ) )
      {
         return rc ;
      }

      // check before lock
      if ( !DMS_IS_MB_INUSE(_mb->_flag) )
      {
         return SDB_DMS_NOTEXIST ;
      }
      if ( _clLID != _mb->_logicalID )
      {
         if ( _startLID == _mbStat->_startLID &&
              DMS_MB_STATINFO_IS_TRUNCATED( _mbStat->_flag ) )
         {
            return SDB_DMS_TRUNCATED ;
         }
         else
         {
            return SDB_DMS_NOTEXIST ;
         }
      }

      if ( isTry )
      {
         BOOLEAN hasLock = FALSE ;
         hasLock = ( SHARED == lockType ) ?
                   _latch->try_get_shared() : _latch->try_get() ;
         if ( !hasLock )
         {
            return SDB_TIMEOUT ;
         }
      }
      else
      {
         ossLatch( _latch, (OSS_LATCH_MODE)lockType ) ;
      }

      // check after lock
      if ( !DMS_IS_MB_INUSE(_mb->_flag) )
      {
         ossUnlatch( _latch, (OSS_LATCH_MODE)lockType ) ;
         return SDB_DMS_NOTEXIST ;
      }
      if ( _clLID != _mb->_logicalID )
      {
         if ( _startLID == _mbStat->_startLID &&
              DMS_MB_STATINFO_IS_TRUNCATED( _mbStat->_flag ) )
         {
            ossUnlatch( _latch, (OSS_LATCH_MODE)lockType ) ;
            return SDB_DMS_TRUNCATED ;
         }
         else
         {
            ossUnlatch( _latch, (OSS_LATCH_MODE)lockType ) ;
            return SDB_DMS_NOTEXIST ;
         }
      }

      _mbLockType = lockType ;
      _resumeType = -1 ;
      return SDB_OK ;
   }
   OSS_INLINE INT32 _dmsMBContext::mbLock( INT32 lockType )
   {
      return _mbLock( lockType, FALSE ) ;
   }
   OSS_INLINE INT32 _dmsMBContext::mbTryLock( INT32 lockType )
   {
      return _mbLock( lockType, TRUE ) ;
   }
   OSS_INLINE INT32 _dmsMBContext::mbUnlock()
   {
      if ( SHARED == _mbLockType || EXCLUSIVE == _mbLockType )
      {
         ossUnlatch( _latch, (OSS_LATCH_MODE)_mbLockType ) ;
         _resumeType = _mbLockType ;
         _mbLockType = -1 ;
      }
      return SDB_OK ;
   }
   OSS_INLINE BOOLEAN _dmsMBContext::isMBLock( INT32 lockType ) const
   {
      return lockType == _mbLockType ? TRUE : FALSE ;
   }
   OSS_INLINE BOOLEAN _dmsMBContext::isMBLock() const
   {
      if ( SHARED == _mbLockType || EXCLUSIVE == _mbLockType )
      {
         return TRUE ;
      }
      return FALSE ;
   }
   OSS_INLINE BOOLEAN _dmsMBContext::canResume() const
   {
      if ( SHARED == _resumeType || EXCLUSIVE == _resumeType )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   typedef class _dmsMBContext dmsMBContext;


   class _dmsMBContextSubScope : public SDBObject
   {
   public:
      _dmsMBContextSubScope( _dmsMBContext* mbContext, _IContext *subContext ) ;
      ~_dmsMBContextSubScope() ;

   private:
      _dmsMBContext *_mbContext ;
   } ;
} // namespace engine


#endif//SDB_DMS_MB_CONTEXT_BASE_HPP_
