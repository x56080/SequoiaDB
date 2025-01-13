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

   Source File Name = utilSUCache.hpp

   Descriptive Name = utility of storage unit cache management header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for SU cache
   management ( including plan cache, statistics cache, etc. )

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/
#ifndef UTILSUCACHE_HPP__
#define UTILSUCACHE_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "ossUtil.hpp"
#include "utilPooledObject.hpp"
#include "utilArray.hpp"

namespace engine
{

   // Same as DMS_MME_SLOTS
   #define UTIL_SU_CACHE_DFT_SIZE ( 4096 )

   // Same as DMS_INVALID_MBID
   #define UTIL_SU_INVALID_UNITID ( 65535 )

   template < UINT16 CACHESIZE, UINT16 staticSize >
   class _utilSUCache ;

   template < UINT16 CACHESIZE >
   class _IUtilSUCacheHolder ;

   #define UTIL_SU_CACHE_UNIT_STATUS_EMPTY   ( 0 )
   #define UTIL_SU_CACHE_UNIT_STATUS_CACHED  ( 1 )

   #define UTIL_SU_CACHE_UNIT_CLSTAT ( 1 )
   #define UTIL_SU_CACHE_UNIT_IXSTAT ( 2 )
   #define UTIL_SU_CACHE_UNIT_CLPLAN ( 3 )

   class _utilSUCacheUnit ;
   typedef class _utilSUCacheUnit utilSUCacheUnit ;

   /*
      _utilSUCacheUnit define
    */
   class _utilSUCacheUnit : public utilPooledObject
   {
      public :
         _utilSUCacheUnit ()
         {
            _unitID = UTIL_SU_INVALID_UNITID ;
            _createTime = 0 ;
         }

         _utilSUCacheUnit ( UINT16 unitID, UINT64 crtTime )
         {
            _unitID = unitID ;
            _createTime = crtTime ;
         }

         virtual ~_utilSUCacheUnit () {}

         OSS_INLINE UINT16 getUnitID () const
         {
            return _unitID ;
         }

         OSS_INLINE UINT64 getCreateTime () const
         {
            return _createTime ;
         }

         OSS_INLINE void setCreateTime ( UINT64 crtTime )
         {
            _createTime = crtTime ;
         }

         virtual UINT8 getUnitType () const = 0 ;

         virtual BOOLEAN addSubUnit ( utilSUCacheUnit *pSubUnit,
                                      BOOLEAN ignoreCrtTime ) = 0 ;

         virtual void clearSubUnits () = 0 ;

      protected :
         OSS_INLINE void _setUnitID ( UINT16 unitID )
         {
            _unitID = unitID ;
         }

      protected :
         // Unit ID to index in cache
         UINT16   _unitID ;

         // CreateTime (timestamp) of the cache unit
         UINT64   _createTime ;
   } ;

   /*
      _utilSUCache define
    */
   template < UINT16 CACHESIZE = UTIL_SU_CACHE_DFT_SIZE, UINT16 staticSize = 16 >
   class _utilSUCache : public SDBObject
   {
      public :
         _utilSUCache ( UINT8 type, UINT8 unitType,
                        _IUtilSUCacheHolder<CACHESIZE> *pHolder = NULL )
         {
            /// for static assert
            UINT32 __testArray[ ( CACHESIZE >= staticSize ? 1 : -1 ) ] = { UTIL_SU_CACHE_UNIT_STATUS_EMPTY } ;

            _type = type ;
            _unitType = unitType ;
            _pHolder = pHolder ;
            _unitsEx = NULL ;
            _unitStatusEx = NULL ;

            ossMemset( _units, 0, sizeof( _units ) ) ;
            setStatus( __testArray[0] ) ;
         }

         virtual ~_utilSUCache ()
         {
            clearCacheUnits() ;
            _release() ;
         }

         OSS_INLINE UINT8 getType () const
         {
            return _type ;
         }

         OSS_INLINE const utilSUCacheUnit *getCacheUnit ( UINT16 unitID ) const
         {
            if ( unitID < CACHESIZE )
            {
               UINT16 realID = unitID ;
               utilSUCacheUnit **pUnitArray = _getUnitPtr( unitID, realID ) ;

               if ( pUnitArray )
               {
                  return pUnitArray[ realID ] ;
               }
            }
            return NULL ;
         }

         OSS_INLINE utilSUCacheUnit *getCacheUnit ( UINT16 unitID )
         {
            if ( unitID < CACHESIZE )
            {
               UINT16 realID = unitID ;
               utilSUCacheUnit **pUnitArray = (utilSUCacheUnit **)_getUnitPtr( unitID, realID ) ;

               if ( pUnitArray )
               {
                  return pUnitArray[ realID ] ;
               }
            }
            return NULL ;
         }

         OSS_INLINE UINT32 getSize () const
         {
            return CACHESIZE ;
         }

         BOOLEAN addCacheUnit ( utilSUCacheUnit *pUnit,
                                BOOLEAN ignoreCrtTime,
                                BOOLEAN needCheck )
         {
            BOOLEAN added = FALSE ;

            UINT16 unitID = UTIL_SU_INVALID_UNITID ;
            UINT16 realID = UTIL_SU_INVALID_UNITID ;
            utilSUCacheUnit **pUnitArray = NULL ;
            UINT8 *pStatusArray = NULL ;
            utilSUCacheUnit *pTmpUnit = NULL ;

            if ( NULL ==  pUnit ||
                 pUnit->getUnitType() != _unitType )
            {
               goto done ;
            }

            if ( needCheck && NULL != _pHolder &&
                 !_pHolder->checkCacheUnit( pUnit ) )
            {
               // Failed to check cache unit
               goto done ;
            }

            unitID = pUnit->getUnitID() ;
            pUnitArray = _insureArrayPtr( unitID, realID, &pStatusArray ) ;
            if ( !pUnitArray )
            {
               // Failed to alloc unit array
               goto done ;
            }

            pTmpUnit = getCacheUnit( unitID ) ;
            if ( NULL != pTmpUnit )
            {
               if ( ignoreCrtTime ||
                    pTmpUnit->getCreateTime() < pUnit->getCreateTime() )
               {
                  SDB_OSS_DEL pTmpUnit ;
                  pUnitArray[ realID ] = pUnit ;
                  pStatusArray[ realID ] = UTIL_SU_CACHE_UNIT_STATUS_CACHED ;
                  added = TRUE ;
               }
            }
            else if ( unitID < CACHESIZE )
            {
               pUnitArray[ realID ] = pUnit ;
               pStatusArray[ realID ] = UTIL_SU_CACHE_UNIT_STATUS_CACHED ;
               added = TRUE ;
            }

         done :
            return added ;
         }

         BOOLEAN addCacheSubUnit ( utilSUCacheUnit *pSubUnit,
                                   BOOLEAN ignoreCrtTime,
                                   BOOLEAN needCheck )
         {
            BOOLEAN added = FALSE ;
            utilSUCacheUnit *pUnit = NULL ;

            if ( NULL ==  pSubUnit )
            {
               goto done ;
            }

            if ( needCheck && NULL != _pHolder &&
                 !_pHolder->checkCacheUnit( pSubUnit ) )
            {
               // Failed to check cache unit
               goto done ;
            }

            pUnit = getCacheUnit( pSubUnit->getUnitID() ) ;
            if ( NULL != pUnit )
            {
               added = pUnit->addSubUnit( pSubUnit, ignoreCrtTime ) ;
            }

         done :
            return added ;
         }

         BOOLEAN removeCacheUnit ( UINT16 unitID, BOOLEAN needDelete )
         {
            BOOLEAN hasDel = FALSE ;

            if ( unitID < CACHESIZE )
            {
               UINT16 realID = unitID ;
               utilSUCacheUnit **pUnitArray = (utilSUCacheUnit**)_getUnitPtr( unitID, realID ) ;

               if ( pUnitArray && pUnitArray[ realID ] )
               {
                  if ( needDelete )
                  {
                     SDB_OSS_DEL pUnitArray[ realID ] ;
                  }
                  pUnitArray[ realID ] = NULL ;
                  hasDel = TRUE ;
               }

               setStatus( unitID, UTIL_SU_CACHE_UNIT_STATUS_EMPTY ) ;
            }

            return hasDel ;
         }

         BOOLEAN clearCacheUnits ()
         {
            BOOLEAN hasDel = FALSE ;

            for ( UINT16 unitID = 0 ; unitID < staticSize ; ++unitID )
            {
               utilSUCacheUnit *pTmpUnit = _units[ unitID ] ;
               if ( pTmpUnit )
               {
                  SDB_OSS_DEL pTmpUnit ;
                  _units[ unitID ] = NULL ;
                  hasDel = TRUE ;
               }
               _unitStatus[ unitID ] = UTIL_SU_CACHE_UNIT_STATUS_EMPTY ;
            }

            if ( _unitsEx )
            {
               for ( UINT16 unitID = staticSize ; unitID < CACHESIZE ; ++unitID )
               {
                  utilSUCacheUnit *pTmpUnit = _unitsEx[ unitID - staticSize ] ;
                  if ( pTmpUnit )
                  {
                     _unitsEx[ unitID - staticSize ] = NULL ;
                     SDB_OSS_DEL pTmpUnit ;
                     hasDel = TRUE ;
                  }
                  _unitStatusEx[ unitID - staticSize ] = UTIL_SU_CACHE_UNIT_STATUS_EMPTY ;
               }
            }

            return hasDel ;
         }

         UINT8 getStatus ( UINT16 unitID ) const
         {
            if ( unitID < CACHESIZE )
            {
               UINT16 realID = unitID ;
               const UINT8 *pStatusArray = _getStatusPtr( unitID, realID ) ;

               if ( pStatusArray )
               {
                  return pStatusArray[ realID ] ;
               }
            }
            return UTIL_SU_CACHE_UNIT_STATUS_EMPTY ;
         }

         void setStatus ( UINT16 unitID, UINT8 status )
         {
            if ( unitID < CACHESIZE )
            {
               UINT16 realID = unitID ;
               UINT8 *pStatusArray = NULL ;

               if ( NULL != _insureArrayPtr( unitID, realID, &pStatusArray ) )
               {
                  pStatusArray[ realID ] = status ;
               }
            }
         }

         void setStatus ( UINT8 status )
         {
            for ( UINT16 unitID = 0 ; unitID < CACHESIZE ; unitID ++ )
            {
               if ( unitID < staticSize )
               {
                  _unitStatus[ unitID ] = status ;
               }
               else if ( _unitStatusEx )
               {
                  _unitStatusEx[ unitID - staticSize ] = status ;
               }
            }
         }

      private:
         utilSUCacheUnit** _getUnitPtr( UINT16 unitID, UINT16 &realID ) const
         {
            if ( unitID < staticSize )
            {
               realID = unitID ;
               return (utilSUCacheUnit**)_units ;
            }
            else
            {
               realID = unitID - staticSize ;
               return _unitsEx ;
            }
         }

         const UINT8* _getStatusPtr( UINT16 unitID, UINT16 &realID ) const
         {
            if ( unitID < staticSize )
            {
               realID = unitID ;
               return _unitStatus ;
            }
            else
            {
               realID = unitID - staticSize ;
               return _unitStatusEx ;
            }
         }

         utilSUCacheUnit** _insureArrayPtr( UINT16 unitID,
                                            UINT16 &realID,
                                            UINT8 **ppStatusArray = NULL )
         {
            if ( unitID < staticSize )
            {
               realID = unitID ;

               if ( ppStatusArray )
               {
                  *ppStatusArray = _unitStatus ;
               }
               return (utilSUCacheUnit**)_units ;
            }
            else
            {
               realID = unitID - staticSize ;

               if ( !_unitsEx || !_unitStatusEx )
               {
                  utilSUCacheUnit **pUnitsEx = NULL ;
                  UINT8 * pUnitStatusEx = NULL ;

                  ossScopedLock lock( &_latch ) ;

                  /// double check
                  if ( !_unitsEx )
                  {
                     /// allocate
                     pUnitsEx = new (std::nothrow) utilSUCacheUnit*[CACHESIZE-staticSize] ;
                     if ( !pUnitsEx )
                     {
                        PD_LOG( PDWARNING, "Allocate cache unit array failed" ) ;
                        return NULL ;
                     }
                     else
                     {
                        /// reset
                        for ( UINT32 i = 0 ; i < CACHESIZE-staticSize ; ++i )
                        {
                           pUnitsEx[ i ] = NULL ;
                        }
                     }
                  }

                  if ( !_unitStatusEx )
                  {
                     /// allocate
                     pUnitStatusEx = new (std::nothrow) UINT8[CACHESIZE-staticSize] ;
                     if ( !pUnitStatusEx )
                     {
                        PD_LOG( PDWARNING, "Allocate cache status array failed" ) ;
                        if ( pUnitsEx )
                        {
                           delete [] pUnitsEx ;
                           pUnitsEx = NULL ;
                        }
                        return NULL ;
                     }
                     else
                     {
                        /// reset
                        for ( UINT32 i = 0 ; i < CACHESIZE-staticSize ; ++i )
                        {
                           pUnitStatusEx[ i ] = UTIL_SU_CACHE_UNIT_STATUS_EMPTY ;
                        }
                     }
                  }

                  /// after reset then assign to _unitsEx
                  if ( pUnitsEx )
                  {
                     _unitsEx = pUnitsEx ;
                  }

                  /// after reset then assign to _unitStatusEx
                  if ( pUnitStatusEx )
                  {
                     _unitStatusEx = pUnitStatusEx ;
                  }
               }

               if ( ppStatusArray )
               {
                  *ppStatusArray = _unitStatusEx ;
               }
               return _unitsEx ;
            }
         }

         void _release()
         {
            if ( _unitsEx )
            {
               delete [] _unitsEx ;
               _unitsEx = NULL ;
            }

            if ( _unitStatusEx )
            {
               delete _unitStatusEx ;
               _unitStatusEx = NULL ;
            }
         }

      private :
         UINT8                            _type ;
         UINT8                            _unitType ;
         utilSUCacheUnit *                _units[ staticSize ] ;
         UINT8                            _unitStatus[ staticSize ] ;
         utilSUCacheUnit **               _unitsEx ;
         UINT8 *                          _unitStatusEx ;
         _IUtilSUCacheHolder<CACHESIZE> * _pHolder ;
         ossSpinXLatch                    _latch ;
   } ;

   /*
      _IUtilSUCacheHolder define
    */
   template < UINT16 CACHESIZE = UTIL_SU_CACHE_DFT_SIZE >
   class _IUtilSUCacheHolder
   {
      public :
         _IUtilSUCacheHolder () {}

         virtual ~_IUtilSUCacheHolder () {}

         virtual const CHAR *getCSName () const = 0 ;

         virtual UINT32 getSUID () const = 0 ;

         virtual UINT32 getSULID () const = 0 ;

         virtual BOOLEAN isSysSU () const = 0 ;

         virtual BOOLEAN checkCacheUnit ( utilSUCacheUnit *pCacheUnit ) = 0 ;

         virtual BOOLEAN createSUCache ( UINT8 type ) = 0 ;

         virtual BOOLEAN deleteSUCache ( UINT8 type ) = 0 ;

         virtual void deleteAllSUCaches () = 0 ;

         virtual _utilSUCache<CACHESIZE> *getSUCache( UINT8 type ) = 0 ;
   } ;

}

#endif //UTILSUCACHE_HPP__

