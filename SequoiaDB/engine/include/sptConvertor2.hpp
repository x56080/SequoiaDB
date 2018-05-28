/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = sptConvertor2.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of Script component. This file contains structures for javascript
   engine wrapper

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/13/2013  YW Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SPTCONVERTOR2_HPP_
#define SPTCONVERTOR2_HPP_

#include "core.hpp"
#include "jsapi.h"
#include "../bson/bson.hpp"
#include <string>

class sptConvertor2
{
public:
   sptConvertor2( JSContext *cx )
   :_cx( cx )
   {
   }

   ~sptConvertor2()
   {
      _cx = NULL ;
   }

public:
   INT32 toBson( JSObject *obj , bson::BSONObj &bsobj ) ;

   static INT32 toString( JSContext *cx,
                          const jsval &val,
                          std::string &str ) ;

private:
   JSContext *_cx ;
} ;

#endif // SPTCONVERTOR2_HPP_

