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

#include "openssl/md5.h"
#include <stdio.h>
#include <string.h>
int main ( int argc, char **argv )
{
   MD5_CTX c ;
   unsigned char md[MD5_DIGEST_LENGTH] ;
   if ( argc !=2 )
   {
      printf ( "Syntax: %s <string>\n", (char*)argv[0]) ;
      return 0 ;
   }
   char *pString = (char*)argv[1] ;
   MD5_Init(&c) ;
   MD5_Update(&c, pString, strlen(pString)) ;
   MD5_Final(&(md[0]), &c) ;
   for ( int i=0; i<MD5_DIGEST_LENGTH; i++)
   {
      printf("%02x", md[i]) ;
   }
   printf("\n") ;
   return 0 ;
}
