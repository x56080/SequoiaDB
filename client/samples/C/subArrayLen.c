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

   
*******************************************************************************/
#include <stdio.h>
#include "common.h"

#define FIELD1 "field1"
#define FIELD2 "field2"

INT32 buildOneRecord ( bson* obj )
{
   INT32 rc = SDB_OK ;
   bson_init ( obj ) ;
   bson_append_int ( obj, FIELD1, 100 ) ;
   bson_append_start_array ( obj, FIELD2 ) ;
   bson_append_string ( obj, "0", "a" ) ;
   bson_append_string ( obj, "1", "b" ) ;
   bson_append_string ( obj, "2", "c" ) ;
   bson_append_finish_array ( obj ) ;
   rc = bson_finish ( obj ) ;
   CHECK_RC ( rc, "Failed to build bson" ) ;

done :
   return rc ;
error :
   goto done ;
}

INT32 main ( INT32 argc, CHAR **argv )
{
   INT32 rc        = SDB_OK ;
   const char *key = NULL ;
   int count       = 0 ;
   bson obj ;
   bson_iterator it ;
   bson_iterator itt ;

   bson_init ( &obj ) ;
   // build one record
   rc = buildOneRecord ( &obj ) ;
   if ( rc )
   {
      printf ( "Failed to build record.\n" ) ;
      goto error ;
   }
   // print record
   bson_print ( &obj ) ;
   // get the array field
   bson_iterator_init ( &it, &obj ) ;
   while ( BSON_EOO != bson_iterator_next( &it ) )
   {
      key = bson_iterator_key ( &it ) ;
      if ( !strcmp( FIELD2, key ) )
      {
         break ;
      }
   }
   // use sub iterator to run through array
   bson_iterator_subiterator( &it, &itt ) ;
   while ( BSON_EOO != bson_iterator_next( &itt ) )
   {
      count++ ;
   }
   printf ("The length of sub array is %d\n", count ) ;
done :
   bson_destroy ( &obj ) ;
   return 0 ;
error :
   goto done ;
}

