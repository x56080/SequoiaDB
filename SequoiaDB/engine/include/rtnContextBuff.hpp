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

   Source File Name = rtnContextBuff.hpp

   Descriptive Name = RunTime Context Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Runtime
   Context.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTNCONTEXTBUFF_HPP_
#define RTNCONTEXTBUFF_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "pd.hpp"
#include "ossMem.hpp"
#include "ossLatch.hpp"
#include "ossRWMutex.hpp"
#include "ossAtomic.hpp"
#include "../bson/bsonobj.h"

using namespace bson ;

namespace engine
{

   // default buffer size should be at least DMS_PAGE_SIZE64K
   #define RTN_DFT_BUFFERSIZE                DMS_PAGE_SIZE_MAX
   #define RTN_RESULTBUFFER_SIZE_MAX         DMS_SEGMENT_SZ

   #define RTN_BUFF_TO_REAL_PTR( buff )      ((CHAR*)buff - 8 )
   #define RTN_REAL_PTR_TO_BUFF( ptr )       ((CHAR*)ptr + 8 )
   #define RTN_BUFF_TO_PTR_SIZE( size )      ( size + 8 )
   #define RTN_GET_REFERENCE( buff )         (INT32*)((CHAR*)buff - 4 )
   #define RTN_GET_CONTEXT_FLAG( buff )      (INT32*)((CHAR*)buff - 8 )

   /*
      _rtnObjBuff define
   */
   class _rtnObjBuff : public SDBObject
   {
      public:
         _rtnObjBuff( const CHAR *pBuff, INT32 buffLen, INT32 recordNum )
         {
            _pBuff      = pBuff ;
            _buffSize   = buffLen ;
            _recordNum  = recordNum ;
            _curOffset  = 0 ;
            _owned      = FALSE ;
         }

         _rtnObjBuff( const _rtnObjBuff &right ) ;
         virtual ~_rtnObjBuff() ;

         /*
            ensure buff is owned
         */
         virtual INT32        getOwned() ;
         virtual void         release() ;

         _rtnObjBuff&         operator=( const _rtnObjBuff &right ) ;

         const CHAR* data () const { return _pBuff ; }
         const CHAR* front() const { return _pBuff + _curOffset; }
         INT32       offset() const { return _curOffset ; }
         INT32       size () const { return _buffSize ; }
         INT32       recordNum () const { return _recordNum ; }
         BOOLEAN     eof () const { return _curOffset >= _buffSize ; }
         void        resetItr () { _curOffset = 0 ; }
         INT32       truncate ( UINT32 num ) ;
         INT32       pop() ;
         INT32       pushFront( const BSONObj &obj ) ;

         OSS_INLINE   INT32  nextObj ( BSONObj &obj ) ;

      protected:
         const CHAR           *_pBuff ;
         INT32                _buffSize ;
         INT32                _recordNum ;
         INT32                _curOffset ;
         BOOLEAN              _owned ;
   } ;
   typedef _rtnObjBuff rtnObjBuff ;

   /*
      _rtnObjBuff OSS_INLINE functions
   */
   OSS_INLINE INT32 _rtnObjBuff::nextObj( BSONObj &obj )
   {
      if ( eof() )
      {
         return SDB_DMS_EOC ;
      }
      try
      {
         BSONObj objTemp( &_pBuff[_curOffset] ) ;
         obj = objTemp ;
         _curOffset += ossAlign4( (UINT32)obj.objsize() ) ;

         if ( _recordNum > 0 )
         {
            --_recordNum ;
         }
         else
         {
            SDB_ASSERT( FALSE, "_recordNum is invalid" ) ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Failed to create bson object: %s", e.what() ) ;
         return SDB_SYS ;
      }
      return SDB_OK ;
   }

   /*
      _rtnContextBuf define
   */
   class _rtnContextBuf : public rtnObjBuff
   {
      friend class _rtnContextBase ;
      friend class _rtnContextStoreBuf ;

      private:
         void  _reference( INT32 *pCounter, ossRWMutex *pMutex ) ;

      public:
         _rtnContextBuf() ;
         _rtnContextBuf( const _rtnContextBuf &right ) ;
         _rtnContextBuf( const CHAR *pBuff, INT32 buffLen, INT32 recordNum ) ;
         _rtnContextBuf( const BSONObj &obj ) ;
         virtual ~_rtnContextBuf () ;

         virtual INT32        getOwned() ;

         _rtnContextBuf&      operator=( const _rtnContextBuf &right ) ;

         void        release () ;

         INT64       getStartFrom() const { return _startFrom ; }

         void        setStartFrom(INT32 startFrom) { _startFrom = startFrom; }

      private:
         INT32                *_pBuffCounter ;
         ossRWMutex           *_pBuffLock ;
         BOOLEAN              _released ;
         CHAR                 *_pOrgBuff ;
         INT64                _startFrom ;

         BSONObj              _object ;

   } ;
   typedef _rtnContextBuf rtnContextBuf ;

}

#endif //RTNCONTEXTBUFF_HPP_

