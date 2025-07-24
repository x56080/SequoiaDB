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

   Source File Name = sptFuncDef.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          6/4/2017    TZB  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SPT_FUNC_DEF_HPP__
#define SPT_FUNC_DEF_HPP__

#include "core.hpp"
#include "../bson/bson.h"
#include "sptObjDesc.hpp"
#include "sptSPScope.hpp"


#include <vector>
#include <map>
#include <string>

using namespace bson ;

namespace engine
{
   #define SPT_FUNC_CONSTRUCTOR       0x00000001
   #define SPT_FUNC_INSTANCE          0x00000002
   #define SPT_FUNC_STATIC            0x00000004
   #define SPT_FUNC_INS_STA           (SPT_FUNC_INSTANCE | SPT_FUNC_STATIC)

   struct _sptFuncDefInfo
   {
      string funcName ;
      INT32  funcType ;
   } ;
   typedef _sptFuncDefInfo sptFuncDefInfo ;

   typedef pair< string, vector<sptFuncDefInfo> > PAIR_FUNC_DEF_INFO ;
   typedef map< string, vector<sptFuncDefInfo> > MAP_FUNC_DEF_INFO ;
   typedef map< string, vector<sptFuncDefInfo> >::iterator MAP_FUNC_DEF_INFO_IT ;
   
   class _sptFuncDef : public SDBObject
   {
   private:
      _sptFuncDef( const _sptFuncDef& ) ;
      _sptFuncDef& operator=( const _sptFuncDef& ) ;

   public:
      _sptFuncDef() ;
      ~_sptFuncDef() {}

   public:
      static _sptFuncDef&         getInstance() ;
      
   public:
      const MAP_FUNC_DEF_INFO&    getFuncDefInfo() ;
      
   private:
      INT32                       _init() ;
      INT32                       _loadFuncInfo( string &className, 
                                       const set< string > & setFunc,
                                       const set< string > & setStaticFunc ) ;
      INT32                       _insert( const string &className, 
                                           const string &funcName,
                                           INT32 type ) ;
   private:
      MAP_FUNC_DEF_INFO           _map_func_def ;
   } ;
   typedef class _sptFuncDef sptFuncDef ;

} // namespace
#endif // SPT_FUNC_DEF_HPP__
