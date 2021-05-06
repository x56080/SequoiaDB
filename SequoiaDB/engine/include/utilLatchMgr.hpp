/******************************************************************************


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

   Source File Name = utilLatchMgr.hpp

   Descriptive Name = latch manager

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declare for data types used in
   SequoiaDB.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/09/2021  JT  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef UTILLATCHMGR_HPP_
#define UTILLATCHMGR_HPP_

#include "ossLatch.hpp"
#include "utilPooledObject.hpp"

namespace engine
{

enum UTL_LATCH_OP_MODE
{
   UTL_LATCH_OP_MODE_TRY = 0,
   UTL_LATCH_OP_MODE_ACQUIRE
} ;


// Latch Request Block Header ( LRB Header )
template < class T >
class LRBHeader : public utilPooledObject
{
public :
   LRBHeader     * nextLRBHdr;   // next LRB Header in the chain
   ossSpinSLatch   latch;
   T               id;           // latch id
   UINT32          refCount;
public :
   LRBHeader( const T & latchId )
   : nextLRBHdr( NULL ),
     id( latchId ),
     refCount( 0 ) {}

   ~LRBHeader() {}
} ;


template < class T >
class LRBHeaderHash : public SDBObject
{
public :
   LRBHeader<T> * lrbHdr  ;      // 1st LRB Header in the chain
   ossSpinXLatch  hashHdrLatch ; // ossSpinXLatch, 48 bytes
public :
   LRBHeaderHash() : lrbHdr(NULL) {}
} ;


template< class T >
class latchManager : public SDBObject
{
protected:
   LRBHeaderHash<T> * _hdrBkt ;
   UINT32             _bktSlotMax ;
   BOOLEAN            _initialized ;

public:
   latchManager()
    : _hdrBkt( NULL ),
      _bktSlotMax( 0 ) ,
      _initialized( FALSE ) { }

   virtual ~latchManager()
   {
      if ( _initialized )
      {
         fini() ;
      }
   }

   INT32 init ( UINT32 bucketSize )
   {
      _bktSlotMax = bucketSize ;
      _hdrBkt = SDB_OSS_NEW LRBHeaderHash<T>[ _bktSlotMax ] ;
      if ( NULL == _hdrBkt )
      {
         PD_LOG( PDERROR,
                 "Failed to allocate memory for latch bucket, bucket size:%d",
                 _bktSlotMax ) ;
         return SDB_OOM ;
      }
      // set initialized flag
      _initialized = TRUE ;
      return SDB_OK ;
   }

   void fini()
   {
      if ( _initialized )
      {
         _initialized = FALSE ;
         if ( _hdrBkt )
         {
            SDB_OSS_DEL [] _hdrBkt ;
            _hdrBkt = NULL ;
         }
      }
   }

   INT32 acquire( const T & latchId, const OSS_LATCH_MODE requestMode )
   {
      return _tryOrAcquire( latchId, requestMode, UTL_LATCH_OP_MODE_ACQUIRE ) ;
   }

   INT32 tryAcquire( const T & latchId, const OSS_LATCH_MODE requestMode )
   {
      return _tryOrAcquire( latchId, requestMode, UTL_LATCH_OP_MODE_TRY ) ;
   }

   void release( const T & latchId, const OSS_LATCH_MODE requestMode )
   {
      return _release( latchId, requestMode ) ;
   }

protected:
   OSS_INLINE UINT32 _getBucketNo( const T & id ) const
   {
      return (UINT32)( id.lockIdHash() % _bktSlotMax ) ;
   }

   BOOLEAN _getLRBHdrByLatchId ( const T & id, LRBHeader<T> * & pLRBHdr )
   {
      BOOLEAN found = FALSE ;
      LRBHeader<T> * pLocal = pLRBHdr;
      while ( NULL != pLocal )
      {
         pLRBHdr = pLocal;
         if ( id == pLocal->id )
         {
            found = TRUE ;
            break ;
         }
         pLocal = pLocal->nextLRBHdr ;
      }
      return found ;
   }

   void _removeFromLRBHdrLst(LRBHeader<T>* & lrbBegin, LRBHeader<T>* lrbDel)
   {
      if ( ( NULL != lrbBegin ) && ( NULL != lrbDel ) )
      {
         LRBHeader<T> *plrbHdr = lrbBegin;

         // if the first one is the one to be removed
         if ( lrbDel == lrbBegin )
         {
            lrbBegin = lrbDel->nextLRBHdr ;
         }
         else
         {
            while ( NULL != plrbHdr )
            {
               if ( lrbDel == plrbHdr->nextLRBHdr )
               {
                  plrbHdr->nextLRBHdr = lrbDel->nextLRBHdr ;
                  break ;
               }
               plrbHdr = plrbHdr->nextLRBHdr ;
            }
         }
         lrbDel->nextLRBHdr = NULL ;
      }
   }

   INT32 _allocNewLRBHdr ( const T & latchId, LRBHeader<T> * & pLRBHdrNew )
   {
      pLRBHdrNew = SDB_OSS_NEW LRBHeader<T>( latchId ) ;
      if ( ! pLRBHdrNew )
      {
         return SDB_OOM ;
      }
      return SDB_OK ;
   }

   OSS_INLINE void _releaseLRBHdr( LRBHeader<T> * hdrLRB )
   {
      if ( hdrLRB )
      {
         SDB_OSS_DEL hdrLRB ;
      }
   }

   INT32 _tryOrAcquire
   (
      const T                & latchId,
      const OSS_LATCH_MODE     requestMode,
      const UTL_LATCH_OP_MODE  opMode
   )
   {
      INT32 rc = SDB_OK;
      LRBHeader<T> *pLRBHdrNew = NULL, *pLRBHdr = NULL ;
      UINT32 bktIdx = _getBucketNo( latchId );

      // latch bucket
      _hdrBkt[ bktIdx ].hashHdrLatch.get() ;

      pLRBHdr = _hdrBkt[ bktIdx ].lrbHdr ;

      // LRBHeader doesn't exist
      if ( ( NULL == pLRBHdr ) || (! _getLRBHdrByLatchId( latchId, pLRBHdr )) )
      {
         rc = _allocNewLRBHdr( latchId, pLRBHdrNew ) ;
         if ( rc )
         {
            _hdrBkt[ bktIdx ].hashHdrLatch.release();
            PD_LOG( PDERROR, "Failed to alloc a LRBHeader (rc=%d)", rc ) ;
            return rc ;
         }
         // add new located LRBHeader in LRB Header list
         if ( NULL == _hdrBkt[ bktIdx ].lrbHdr )
         {
            _hdrBkt[ bktIdx ].lrbHdr = pLRBHdrNew;
         }
         else
         {
            pLRBHdr->nextLRBHdr = pLRBHdrNew ;
         }

         pLRBHdr = pLRBHdrNew ;

         // acquire the latch
         pLRBHdr->refCount ++ ;
         if ( SHARED == requestMode )
         {
            pLRBHdr->latch.get_shared();
         }
         else // if ( EXCLUSIVE == requestMode )
         {
            pLRBHdr->latch.get();
         }

         _hdrBkt[ bktIdx ].hashHdrLatch.release();
         return rc ;
      }
      else // LRBHeader exists
      {
         if ( UTL_LATCH_OP_MODE_ACQUIRE == opMode )
         {
            pLRBHdr->refCount ++ ;
            _hdrBkt[ bktIdx ].hashHdrLatch.release();

            if ( SHARED == requestMode )
            {
               pLRBHdr->latch.get_shared();
            }
            else // if ( EXCLUSIVE == requestMode )
            {
               pLRBHdr->latch.get();
            }
            return rc ;
         }
         else // if ( UTL_LATCH_OP_MODE_TRY == opMode )
         {
            if ( SHARED == requestMode )
            {
               if ( pLRBHdr->latch.try_get_shared() )
               {
                  pLRBHdr->refCount ++ ;
               }
               else
               {
                  rc = SDB_LOCK_FAILED ;
               }
            }
            else // if ( EXCLUSIVE == requestMode )
            {
               if ( pLRBHdr->latch.try_get() )
               {
                  pLRBHdr->refCount ++ ;
               }
               else
               {
                  rc = SDB_LOCK_FAILED ;
               }
            }
            _hdrBkt[ bktIdx ].hashHdrLatch.release();
            return rc ;
         }
      }
      return rc ;
   }

   void _release ( const T & latchId, const OSS_LATCH_MODE requestMode )
   {
      LRBHeader<T> *pLRBHdr = NULL ;
      BOOLEAN releaseLRBHdr = FALSE ;
      UINT32 bktIdx = _getBucketNo( latchId );

      _hdrBkt[ bktIdx ].hashHdrLatch.get() ;
      pLRBHdr = _hdrBkt[ bktIdx ].lrbHdr ;
      if ( ( NULL != pLRBHdr ) && ( _getLRBHdrByLatchId( latchId, pLRBHdr ) ) )
      {
         if ( SHARED == requestMode )
         {
            pLRBHdr->latch.release_shared();
         }
         else // if ( EXCLUSIVE == requestMode )
         {
            pLRBHdr->latch.release();
         }
         pLRBHdr->refCount -- ;
         if ( 0 == pLRBHdr->refCount )
         {
            _removeFromLRBHdrLst( _hdrBkt[ bktIdx ].lrbHdr, pLRBHdr ) ;
            releaseLRBHdr = TRUE;
         }
      }

      _hdrBkt[ bktIdx ].hashHdrLatch.release();
      if ( releaseLRBHdr )
      {
         _releaseLRBHdr( pLRBHdr ) ;
      }
   }

} ;


}  // namespace engine

#endif // UTILLATCHMGR_HPP_

