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

   Source File Name = expUtil.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who          Description
   ====== =========== ============ =============================================
          29/07/2016  Lin Yuebang  Initial Draft

   Last Changed =

*******************************************************************************/
#include "expUtil.hpp"
#include "utilCommon.hpp"

namespace exprt
{
   #define EXP_SPACE_STR   " \t"
   
   UINT32 RC2ShellRC(INT32 rc)
   {
      return engine::utilRC2ShellRC(rc);
   }

   void cutStr( const string &str, vector<string>& subs, const string &cutBy )
   {
      string::size_type prev = 0 ;
      string::size_type sep = string::npos ;
      
      sep = str.find( cutBy ) ;
      subs.push_back( string( str, 0, sep ) ) ;
      
      while ( string::npos != sep ) 
      {
         prev = sep + cutBy.size() ;
         sep = str.find( cutBy, prev ) ;
         subs.push_back( string( str, prev, sep-prev ) ) ;
      } 
   }

   void trimBoth( string &str ) 
   {
      trimLeft(str) ;
      trimRight(str) ;
   }
   void trimLeft ( string &str ) 
   {
      if ( !str.empty() )
      {
         string::size_type pos = str.find_first_not_of(EXP_SPACE_STR) ;
         str.erase( 0, pos ) ;
      }
   }
   void trimRight( string &str ) 
   {
      if ( !str.empty() )
      {
         string::size_type pos = str.find_last_not_of(EXP_SPACE_STR) ;
         if ( string::npos == pos ) { str.erase() ; }
         else if ( pos < str.size()-1 ) { str.erase( pos+1 ) ; }
      }
   }
}