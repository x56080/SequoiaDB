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

   Source File Name = sptClassMetaInfo.hpp

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
#ifndef SPTFUNCINFO_HPP__
#define SPTFUNCINFO_HPP__

#include "core.hpp"
#include "../bson/bson.h"
#include "../spt/sptFuncDef.hpp"

#include <vector>
#include <map>
#include <string>

using namespace bson ;
using std::string ;
using std::vector ;
using std::map ;
using std::multimap ;

namespace engine
{

   #define SPT_CLASS_SEPARATOR       "::"
   #define SPT_GLOBAL_CLASS          "Global"
   
   #define SPT_FUNC_CONSTRUCTOR       0x00000001
   #define SPT_FUNC_INSTANCE          0x00000002
   #define SPT_FUNC_STATIC            0x00000004
   #define SPT_FUNC_INS_STA           (SPT_FUNC_INSTANCE | SPT_FUNC_STATIC) 

   enum SPT_LANG
   {
      SPT_LANG_EN = 1,
      SPT_LANG_CN = 2
   } ;

   struct _sptFuncMetaInfo
   {
      string         funcName ;
      vector<string> syntax ;
      string         desc ;
      INT32          funcType ;
      string         path ;
   } ;
   typedef struct _sptFuncMetaInfo sptFuncMetaInfo ; 

   typedef pair< string, vector<sptFuncMetaInfo> > PAIR_FUNC_META_INFO ;
   typedef map< string, vector<sptFuncMetaInfo> > MAP_FUNC_META_INFO ;
   typedef map< string, vector<sptFuncMetaInfo> >::iterator MAP_FUNC_META_INFO_IT ;

   // extract js class's info and func's info from conf file and troff file
   class _sptClassMetaInfo : public SDBObject
   {
   public:
      _sptClassMetaInfo() ;
      _sptClassMetaInfo( const string &lang ) ;
      ~_sptClassMetaInfo() {}

   public:
      INT32                       queryFuncInfo( const string &fuzzyFuncName, 
                                                 const string &matcher,
                                                 BOOLEAN isInstance,
                                                 vector<string> &output ) ;
      INT32                       getTroffFile( const string &fullName,
                                                string &path) ;
      INT32                       getMetaInfo( const string &className,
                                               INT32 type,
                                               vector<sptFuncMetaInfo> &output ) ;
   private:
      INT32                       _init() ;
      INT32                       _loadTroffFile() ;
      INT32                       _extractTroffInfo() ;
      void                        _mergeMetaInfo() ;
      
   private:
      INT32                       _getFuncName( string &filePath, 
                                                string &funcName ) ;
      INT32                       _getContents( const CHAR *pFileBuff,
                                                const CHAR *pMark1,
                                                const CHAR*pMark2,
                                                CHAR **ppBuff,
                                                INT32 *pBuffSize ) ;
      INT32                       _getFuncSynopsis( const CHAR *pFileBuff, 
                                                    INT32 fileSize, 
                                                    vector<string> &output ) ;
      INT32                       _getFuncDesc( const CHAR *pFileBuff, 
                                                INT32 fileSize, 
                                                string &desc ) ;
   private:
      MAP_FUNC_DEF_INFO           _map_func_def_info ;
      multimap< string, string >  _mapFiles ;
      
   private:
      SPT_LANG                    _lang ;
      BOOLEAN                     _initOK ;
      MAP_FUNC_META_INFO          _map_func_meta_info ;
      vector<string>              _functions ;
   } ;
   typedef class _sptClassMetaInfo sptClassMetaInfo ;

} // namespace
#endif // SPTFUNCINFO_HPP__
