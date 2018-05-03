/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = spdFMP.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of Replication component. This file contains structure for
   replication control block.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SPDFMP_HPP_
#define SPDFMP_HPP_

#include "core.hpp"
#include "ossNPipe.hpp"
#include "../bson/bson.h"

using namespace bson ;

namespace engine
{
   class _pmdEDUCB ;

   class _spdFMP : public SDBObject
   {
   public:
      _spdFMP() ;
      virtual ~_spdFMP() ;

      UINT32 getSeqID() const { return _seqID ; }

   public:
      /// timeout is sec.
      /// user should call getOwned if necessary.
      virtual INT32 read( BSONObj &msg, _pmdEDUCB *cb,
                          BOOLEAN ignoreTimeout = TRUE ) ;

      virtual INT32 write( const BSONObj &msg ) ;

      OSS_INLINE const OSSPID &id() const
      {
         return _id ;
      }

      OSS_INLINE void setDiscarded()
      {
         _discarded = TRUE ;
         return ;
      }

      OSS_INLINE BOOLEAN discarded()const
      {
         return _discarded ;
      }

      INT32 reset( _pmdEDUCB *cb ) ;

      INT32 quit( _pmdEDUCB *cb ) ;

   private:
      INT32 _extendReadBuf() ;

      INT32 _extractMsg( BSONObj &msg, BOOLEAN &extracted ) ;

      OSS_INLINE void _clear()
      {
         _totalRead = 0 ;
         _itr = 0 ;
         _expect = 0 ;
         return ;
      }

   protected:
      UINT32         _seqID ;
      OSSNPIPE       _in ;
      OSSNPIPE       _out ;
      OSSPID         _id ;
      BOOLEAN        _discarded ;
      CHAR           *_readBuf ;
      INT32          _readBufSize ;
      INT32          _totalRead ;
      INT32          _itr ;
      INT32          _expect ;

      friend class _spdFMPMgr ;
   } ;
   typedef class _spdFMP spdFMP ;
}

#endif

