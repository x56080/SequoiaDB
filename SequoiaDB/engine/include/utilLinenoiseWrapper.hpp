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

   Source File Name = utilLinenoiseWrapper.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/08/2013  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_LINENOISE_WRAPPER_HPP__
#define UTIL_LINENOISE_WRAPPER_HPP__

#include "core.hpp"
#include "oss.hpp"
#include <fstream>
#include <string>
#include <vector>
#include <boost/function.hpp>
#include "../util/linenoise.h"

extern std::string historyFile ;

struct _linenoiseCmd : public SDBObject
{
   std::string    cmdName ;
   UINT32         nameSize ;
   UINT32         leafCnt ;
   BOOLEAN        leaf ;
   _linenoiseCmd  *sub ;
   _linenoiseCmd  *next ;
   _linenoiseCmd  *parent;
};
typedef _linenoiseCmd linenoiseCmd ;

// use for checking if linenoise can continue read next line
typedef BOOLEAN (*canContinueNextLineCallback)( const CHAR * str ) ;

class _linenoiseCmdBuilder : public SDBObject
{
   public:
      _linenoiseCmdBuilder() { _pRoot = NULL ; }
      ~_linenoiseCmdBuilder()
      {
         if ( _pRoot )
         {
            _releaseNode( _pRoot ) ;
            _pRoot = NULL ;
          }
      }

   public:
      INT32    loadCmd( const CHAR *filename ) ;
      INT32    addCmd( const CHAR *cmd ) ;
      INT32    delCmd( const CHAR *cmd ) ;
      UINT32   getCompletions( const CHAR *cmd, CHAR *&fill, CHAR **&vec,
                               UINT32 &maxStrLen,
                               UINT32 maxSize = 50 ) ;
   public:
      INT32    _insert( _linenoiseCmd *node, const CHAR *cmd ) ;
      void     _releaseNode( _linenoiseCmd *node ) ;
      UINT32   _near( const CHAR *str1, const CHAR *str2 ) ;

      _linenoiseCmd *_prefixFind( const CHAR *cmd, UINT32 &sameNum ) ;
      UINT32   _getCompleteions( _linenoiseCmd *node, const std::string &prefix,
                                 BOOLEAN getSub,
                                 UINT32 &maxStrLen,
                                 std::vector<CHAR*> &vec,
                                 UINT32 maxSize ) ;

   private:
      _linenoiseCmd           *_pRoot ;

};
typedef _linenoiseCmdBuilder linenoiseCmdBuilder ;

/// Tool functions

linenoiseCmdBuilder* getLinenoiseCmdBuilder() ;
#define g_lnBuilder (*getLinenoiseCmdBuilder())

void lineComplete( const char *buf, linenoiseCompletions *lc ) ;
void linenoiseHistoryCallback( const char *cmd, HISTORY_CHANGETYPE type ) ;

BOOLEAN canContinueNextLine ( const CHAR * str ) ;

BOOLEAN getNextCommand ( const CHAR *prompt, CHAR ** cmd,
                         BOOLEAN continueEnable = TRUE ) ;

BOOLEAN historyClear ( void ) ;
BOOLEAN historyInit ( void ) ;
void clearInputBuffer( void ) ;
BOOLEAN isStdinEmpty( void ) ;
void setCanContinueNextLineCallback( boost::function< BOOLEAN( const CHAR* ) >  funcObj ) ;
#endif //UTIL_LINENOISE_WRAPPER_HPP__

