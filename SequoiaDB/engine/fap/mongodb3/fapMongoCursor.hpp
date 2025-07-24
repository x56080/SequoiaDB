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

   Source File Name = fapMongoCursor.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who     Description
   ====== =========== ======= ==============================================
          2020/04/30  Ting YU Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef _SDB_MONGO_CURSOR_HPP_
#define _SDB_MONGO_CURSOR_HPP_

#include "ossUtil.hpp"
#include "utilConcurrentMap.hpp"
#include "fapMongodef.hpp"

using namespace std ;

namespace fap
{

struct mongoCursorInfo
{
   INT64 cursorID ;
   UINT64 EDUID ;
   BOOLEAN needAuth ; // TODO
} ;

class _mongoCursorMgr : public SDBObject
{
   typedef engine::utilConcurrentMap<INT64, mongoCursorInfo, 32> CURSOR_MAP ;

public:
   _mongoCursorMgr() {}
   virtual ~_mongoCursorMgr() {}

   BOOLEAN find( INT64 cursorID, mongoCursorInfo& info ) ;
   INT32 insert( const mongoCursorInfo& info ) ;
   void remove( INT64 cursorID ) ;

private:
   CURSOR_MAP _cursorMap ;
} ;
typedef _mongoCursorMgr mongoCursorMgr ;

_mongoCursorMgr *getMongoCursorMgr() ;

}
#endif
