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
*******************************************************************************/

#include "core.hpp"
#include <stdio.h>
#include "../client/client.hpp"
#include <iostream>

using namespace std ;

string toJson( const bson &b )
{
   string json ;
   INT32 len = bson_sprint_length( &b ) + 1;
   CHAR *buf = (CHAR *)malloc(len) ;
   if ( NULL != buf )
   {
      if ( bsonToJson( buf, len, &b, FALSE ) )
      {
         json = string( buf ) ;
      }
      free( buf ) ;
   }
   return json ;
}

INT32 main()
{
   bson obj ;
   bson_init( &obj ) ;
   UINT64 _id = 0 ;
   CHAR text[1000] = {'1'} ;
   memset( text, '1', 1000 ) ;
   text[999] = '\0' ;
   bson_append_long( &obj, "_id", _id ) ;
   bson_append_long( &obj, "index1", _id ) ;
   bson_append_long( &obj, "index2", _id ) ;
   bson_append_string( &obj, "text", text ) ;
   bson_finish( &obj ) ;
   cout << bson_size( &obj ) << endl ;
   cout << toJson( obj ) << endl ;
   _id++ ;
   *((UINT64 *)(obj.data+8)) = _id ;
   cout << toJson( obj ) << endl ;
   bson_destroy( &obj ) ;

   return 0 ;
}
