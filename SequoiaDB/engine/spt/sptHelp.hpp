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

   Source File Name = sptHelp.hpp

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
#ifndef SPTHELP_HPP__
#define SPTHELP_HPP__

#include "sptClassMetaInfo.hpp"

namespace engine
{
   #define SPT_SYNOPSIS_INDENT           3
   #define SPT_BRIEF_INDENT              30
   
   class _sptHelp : public SDBObject
   {
   private:
      _sptHelp( const _sptHelp& ) ;
      _sptHelp& operator=( const _sptHelp& ) ;

   public:
      _sptHelp() ;
      ~_sptHelp() {}
      
   public:
      static _sptHelp&            getInstance() ;
      static void                 setLanguage( const string &lang ) ;
      INT32                       displayManual( const string &fuzzyFuncName,
                                                 const string &matcher,
                                                 BOOLEAN isInstance ) ;
      INT32                       displayMethod( const string &className,
                                                 BOOLEAN isInstance ) ;
      INT32                       displayGlobalMethod() ;
      
   private:
      INT32                       _displayConstructorMethod( const string &className,
                                          const vector<sptFuncMetaInfo> &input ) ;
      INT32                       _displayStaticMethod( const string &className,
                                          const vector<sptFuncMetaInfo> &input ) ;
      INT32                       _displayInstanceMethod( const string &className,
                                          const vector<sptFuncMetaInfo> &input ) ;
      INT32                       _displayMethod( const vector<sptFuncMetaInfo> &input,
                                                  INT32 synopsisIndent = SPT_SYNOPSIS_INDENT,
                                                  INT32 briefIndent = SPT_BRIEF_INDENT ) ;
      INT32                       _displayEntry( const vector<string> &synopsis, 
                                                 const string &brief,
                                                 INT32 synopsisIndent = SPT_SYNOPSIS_INDENT,
                                                 INT32 briefIndent = SPT_BRIEF_INDENT ) ;
      INT32                       _splitBrief( const string &brief,
                                               vector<string> &vec ) ;
      INT32                       _getSplitPosition( const CHAR *pos, 
                                                     INT32 lineLen, 
                                                     INT32 *offset ) ;
   private:
      static string               _lang ;
      sptClassMetaInfo            _meta ;
   } ;
   typedef class _sptHelp sptHelp ;

} // namespace
#endif // SPTHELP_HPP__