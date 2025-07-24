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

   Source File Name = rtnLobPieces.hpp

   Descriptive Name = LOB piece info

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/11/2017  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_LOB_PIECES_HPP_
#define RTN_LOB_PIECES_HPP_ 

#include "oss.hpp"
#include "../bson/bson.h"
#include <vector>
#include <string>

namespace engine
{
   struct _rtnLobPieces
   {
      UINT32 first ;
      UINT32 last ;

      _rtnLobPieces( UINT32 start, UINT32 end )
         : first( start ),
           last( end )
      {
      }

      _rtnLobPieces()
         : first( 0 ),
           last( 0 )
      {
      }

      OSS_INLINE bool contains( UINT32 piece ) const
      {
         return ( piece >= first && piece <= last ) ;
      }

      void operator=( const _rtnLobPieces& pieces )
      {
         first = pieces.first ;
         last = pieces.last ;
      }
   } ;

   class _rtnLobPiecesInfo: public SDBObject
   {
   public:
      _rtnLobPiecesInfo() ;
      ~_rtnLobPiecesInfo() ;

   private:
      // disallow copy and assign
      _rtnLobPiecesInfo( const _rtnLobPiecesInfo& ) ;
      void operator=( const _rtnLobPiecesInfo& ) ;

   public:
      INT32          addPiece( UINT32 piece ) ;
      INT32          addPieces( const _rtnLobPieces& pieces ) ;
      INT32          delPiece( UINT32 piece ) ;
      INT32          delPieces( const _rtnLobPieces& pieces ) ;
      BOOLEAN        hasPiece( UINT32 piece ) const ;
      _rtnLobPieces  getSection( INT32 index ) const ;
      INT32          requiredMem() const ;
      INT32          saveTo( CHAR* buf, INT32 length ) const ;
      INT32          readFrom( const CHAR* buf, INT32 length ) ;
      INT32          mergeFrom( const CHAR* buf, INT32 length ) ;
      INT32          saveTo( bson::BSONArray& array ) const ;
      INT32          readFrom( const bson::BSONArray& array ) ;
      std::string    toString() const ;

   public:
      OSS_INLINE BOOLEAN empty() const { return _sections.empty() ; }
      OSS_INLINE INT32   sectionNum() const { return (INT32)_sections.size() ; }
      OSS_INLINE void    clear() { _sections.clear() ; }

   private:
      INT32 _findPiece( UINT32 piece ) const ;
      INT32 _findNearestLeast( UINT32 piece ) const ;

   private:
      typedef std::vector<_rtnLobPieces> LOB_PIECESINFO_TYPE ;
      LOB_PIECESINFO_TYPE _sections ;
   } ;
}

#endif /* RTN_LOB_PIECES_HPP_ */

