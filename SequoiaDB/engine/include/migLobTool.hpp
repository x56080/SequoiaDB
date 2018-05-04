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

   Source File Name = migLobTool.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/31/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef MIG_LOBTOOL_HPP_
#define MIG_LOBTOOL_HPP_

#include "oss.hpp"
#include "client.hpp"
#include "ossIO.hpp"
#include "../bson/bson.hpp"
#include "migLobDef.hpp"

namespace lobtool
{
   class _migLobTool : public SDBObject
   {
   public:
      _migLobTool() ;
      virtual ~_migLobTool() ;

   public:
      INT32 exec( const bson::BSONObj &options ) ;

   private:
      INT32 _exportLob( const migOptions &ops ) ;

      INT32 _importLob( const migOptions &ops ) ;

      INT32 _migrate( const migOptions &ops ) ;

   private:
      struct imprtIterator
      {
         UINT32 loadSize ;
         UINT32 start ;
         imprtIterator()
         :loadSize( 0 ),
          start( 0 )
         {

         }

        BOOLEAN empty() const
        {
           return 0 == loadSize || start == loadSize ;
        }

        UINT32 keepSize() const
        {
           return loadSize - start ;
        }
      } ;
   private:
      INT32 _createFile( const CHAR *fullPath ) ;

      INT32 _openFile( const CHAR *fullPath,
                       const migFileHeader *&header ) ;

      INT32 _write( const CHAR *buf, UINT32 size ) ;

      void _initFileHeader( migFileHeader *header ) ;

      INT32 _initDB( const migOptions &ops ) ;

      void _closeDB() ;

      INT32 _append2File( const bson::BSONObj &obj ) ;

      INT32 _truncate( INT64 len ) ;

      INT32 _refreshHeader( UINT64 totalNum ) ;

      INT32 _sendLobFromFile( BOOLEAN ignorefe,
                              imprtIterator &itr,
                              BOOLEAN &skip ) ;

      INT32 _getLobMeta( const imprtIterator &itr,
                         bson::OID &oid, SINT64 &len,
                         SINT32 &objLen ) ;

      INT32 _migrateLob2Dst( const bson::OID &oid,
                             sdbclient::sdbCollection &cl ) ;

   private:
      OSSFILE _file ;
      CHAR *_buf ;
      UINT32 _bufSize ;
      UINT32 _written ;

      sdbclient::sdb* _db ;
      sdbclient::sdbCollection _cl ;
   } ;
   typedef class _migLobTool migLobTool ;
}
#endif

