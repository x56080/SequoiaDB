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

   Source File Name = dmsCompress.hpp

   Descriptive Name =

   When/how to use: str util

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          20/06/2014  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMSCOMPRESS_HPP__
#define DMSCOMPRESS_HPP__

#include "core.hpp"
#include "ossRWMutex.hpp"
#include "utilCompressor.hpp"
#include "../bson/bson.h"

using namespace bson ;

namespace engine
{
   class _pmdEDUCB ;

   /*
    * One collection has one compressor entry, in which the compressor and
    * dictionary addresses are stored.
    * Compressor and dictionary values:
    * (1) No compression is configured for the collection, both are NULL.
    * (2) Dictionary compression is configured, both are valid.
    * (3) Otherwise( snappy ), compressor is valid while dictionary is NULL.
    */
   class _dmsCompressorEntry : public SDBObject
   {
      friend class _dmsCompressorGuard ;
   public:
      _dmsCompressorEntry() ;

      void setCompressor( utilCompressor *compressor ) ;
      void setDictionary( const utilDictHandle dictionary ) ;
      void setFlags ( UINT8 flags ) ;

      OSS_INLINE utilCompressor* getCompressor() { return _compressor ; }
      OSS_INLINE const utilDictHandle getDictionary() { return _dictionary ; }
      OSS_INLINE const utilDictHandle getDictionary( utilCompressor * compressor )
      {
         return ( NULL != compressor && compressor->needDictionay() ) ?
                _dictionary : UTIL_INVALID_DICT ;
      }
      OSS_INLINE UINT8 getFlags () const { return _flags ; }

      /*
       * Whether the compressor is ready. Only then it's true the compression/
       * decompression can be done.
       */
      OSS_INLINE BOOLEAN ready() { return ( NULL != _compressor ) ; }

      OSS_INLINE UTIL_COMPRESSOR_TYPE getCompressorType () const
      {
         return NULL == _compressor ? UTIL_COMPRESSOR_INVALID :
                                      _compressor->getType() ;
      }

      void reset() ;

   private:
      utilCompressor *_compressor ;    /* Global compressor address */
      utilDictHandle _dictionary ;     /* For dictionary compression */
      UINT8          _flags ;
      ossRWMutex _lock ;
   } ;
   typedef _dmsCompressorEntry dmsCompressorEntry ;

   /* Concurrence protection for collection compressor entry. */
   class _dmsCompressorGuard
   {
   public:
      _dmsCompressorGuard( _dmsCompressorEntry *compEntry, OSS_LATCH_MODE mode )
         : _lock( &compEntry->_lock),
           _mode( mode )
      {
         if ( SHARED == _mode )
         {
            _lock->lock_r() ;
         }
         else if ( EXCLUSIVE == _mode )
         {
            _lock->lock_w() ;
         }
      }

      ~_dmsCompressorGuard()
      {
         release() ;
      }

      void release()
      {
         if ( SHARED == _mode )
         {
            _lock->release_r() ;
         }
         else if ( EXCLUSIVE == _mode )
         {
            _lock->release_w() ;
         }
         _mode = -1 ;
      }

   private:
      ossRWMutex *_lock ;
      INT32 _mode ;
   } ;
   typedef _dmsCompressorGuard dmsCompressorGuard ;

   /*
      ppData: output data pointer, not need release
   */
   INT32 dmsCompress ( _pmdEDUCB *cb, _dmsCompressorEntry *compressorEntry,
                       const CHAR *pInputData, INT32 inputSize,
                       const CHAR **ppData, INT32 *pDataSize,
                       UINT8 &ratio ) ;

   INT32 dmsCompress ( _pmdEDUCB *cb, _dmsCompressorEntry *compressorEntry,
                       const BSONObj &obj, const CHAR* pOIDPtr, INT32 oidLen,
                       const CHAR **ppData, INT32 *pDataSize, UINT8 &ratio ) ;

   /*
      ppData: output data pointer, not need release
   */
   INT32 dmsUncompress ( _pmdEDUCB *cb, _dmsCompressorEntry *compressorEntry,
                         UINT8 compressType, const CHAR *pInputData, INT32 inputSize,
                         const CHAR **ppData, INT32 *pDataSize ) ;
}

#endif // DMSCOMPRESS_HPP__

