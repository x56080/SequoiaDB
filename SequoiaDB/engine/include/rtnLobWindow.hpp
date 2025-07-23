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

   Source File Name = rtnLobWindow.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/31/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_LOBWINDOW_HPP_
#define RTN_LOBWINDOW_HPP_

#include "dmsLobDef.hpp"
#include "rtnLobTuple.hpp"

namespace engine
{
   class _rtnLobWindow : public SDBObject
   {
   public:
      _rtnLobWindow() ;
      virtual ~_rtnLobWindow() ;

   public:
      INT32 init( INT32 pageSize, BOOLEAN mergeMeta, BOOLEAN metaPageDataCached ) ;

      BOOLEAN continuous( INT64 offset ) const ;

      UINT32 getLobSequence() const ;

      INT32 prepare4Write( INT64 offset, UINT32 len, const CHAR *data ) ;

      BOOLEAN getNextWriteSequences( RTN_LOB_TUPLES &tuples ) ;

      void cacheLastDataOrClearCache() ;

      BOOLEAN getCachedData( _rtnLobTuple &tuple ) ;

      BOOLEAN getMetaPageData( _rtnLobTuple &tuple ) ;

      INT32 prepare4Read( INT64 lobLen,
                          INT64 offset,
                          UINT32 len,
                          RTN_LOB_TUPLES &tuples ) ;
   private:
      BOOLEAN _getNextWriteSequence( _rtnLobTuple &tuple ) ;
      BOOLEAN _hasData() const ;
      UINT32  _getCurDataPageSize() const ;

   private:
      INT32          _pageSize ;
      UINT32         _logarithmic ;
      BOOLEAN        _mergeMeta ;
      BOOLEAN        _metaPageDataCached ;

      INT64          _curOffset ;
      CHAR*          _pool ;
      INT32          _cachedSz ;
      INT32          _metaSize ;

      /// reuse rtnLobPiece to keep write data.
      _rtnLobTuple   _writeData ;
   } ;
   typedef class _rtnLobWindow rtnLobWindow ;
}

#endif

