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

   Source File Name = expCL.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who          Description
   ====== =========== ============ =============================================
          29/07/2016  Lin Yuebang  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef EXP_CL_HPP_
#define EXP_CL_HPP_

#include "oss.hpp"
#include "../client/bson/bson.h"
#include "../client/client.h"
#include <vector>
#include <string>
#include <set>

namespace exprt
{
   using namespace std ;

   #define EXPCL_FIELDS_SEP_CHAR    ':'
   #define EXPCL_FIELDS_SEP_STR     ":"

   struct expCL : public SDBObject
   {
      string csName ;
      string clName ;
      string fields ;
      string select ;
      string filter ;
      string sort ;
      INT64  skip ;
      INT64  limit ;

      expCL( const string &csName_,
             const string &clName_,
             const string &fields_ = "",
             const string &select_ = "",
             const string &filter_ = "",
             const string &sort_ = "" ) :
             csName(csName_), clName(clName_), fields(fields_),
             select(select_), filter(filter_), sort(sort_), skip(0), limit(-1)
      {
      }

      expCL() : csName(""), clName(""), fields(""), select(""),
                filter(""), sort(""), skip(0), limit(-1)
      {
      }

      inline string fullName() const
      {
         string name = csName ;
         name += "." ;
         name += clName ;
         return name ;
      }

      // format of rawStr should be :
      // "<csName>.<clName> <field-list>"
      // "<csName>.<clName>" must be specified while "<field-list>" may not be
      INT32 parseCLFields( const string &rawStr ) ;

      expCL &swap( expCL &other )
      {
         csName.swap(other.csName) ;
         clName.swap(other.clName) ;
         fields.swap(other.fields) ;
         filter.swap(other.filter) ;
         sort.swap(other.sort) ;
         return *this ;
      }
   } ;

   bool operator<( const expCL &CL1, const expCL &CL2 ) ;

   class expOptions ;
   class expCLSet : public SDBObject
   {
   public :
      typedef vector<expCL>::iterator iterator ;
      typedef vector<expCL>::const_iterator const_iterator ;

   public :
      explicit expCLSet( expOptions &options ) :
         _options(options), _includeAll(FALSE)
      {
      }
      INT32 parse( sdbConnectionHandle hConn ) ;
      inline iterator       begin()       { return _collections.begin() ; }
      inline const_iterator begin() const { return _collections.begin() ; }
      inline iterator       end()         { return _collections.end() ; }
      inline const_iterator end()   const { return _collections.end() ; }

   private :
      INT32 _completeCLListFields( sdbConnectionHandle hConn ) ;
      INT32 _parseRawFileds( set<expCL> &clFields ) ;
      INT32 _generateCLList( sdbConnectionHandle hConn,
                             const set<string> &includeCS,
                             const set<expCL> &includeCollection,
                             const set<string> &excludeCS,
                             const set<expCL> &excludeCollection ) ;
      // should be call after parsing
      INT32 _parsePost() ;
   private :
      expOptions     &_options ;
      vector<expCL>   _collections ;
      BOOLEAN         _includeAll ;
   } ;

}
#endif
