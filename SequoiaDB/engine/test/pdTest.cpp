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

#include "pd.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "../include/ossUtil.h"
#include "../bson/bson.h"
#include "../util/fromjson.hpp"


using namespace engine;
//extern SDB_KRCB pmd_krcb;

#define RECEIVE_BUFFER_SIZE 4095
#define COLLECTION_SPACE_MAX_SZ 127
#define COLLECTION_MAX_SZ 127
char receiveBuffer [ RECEIVE_BUFFER_SIZE+1 ] ;
char *pOutBuffer = NULL ;
int outBufferSize = 0 ;

INT32 readInput ( CHAR *pPrompt, INT32 numIndent );

int main(int argc,char *argv[])
{
   INT32 rc = SDB_OK ;
   if( argc > 2 || argc < 2 )
   {
      std::cout << "Input false !\n" ; 
      return 0;
   }
   //pmdInitialize(pmdGetKRCB());
   //SDB_ASSERT(1!=1, "wrong");
   BSONObj matcher ;
   CHAR BUFF  [ RECEIVE_BUFFER_SIZE ] ;
   //rc = readInput ( "Please input a \"JSOB\"", 0 ) ;
   rc = fromjson ( argv[1], matcher ) ;
   std::cout << matcher <<endl ; 

   ossHexDumpBuffer( matcher.objdata(), matcher.objsize(), BUFF, sizeof(BUFF), NULL, OSS_HEXDUMP_PREFIX_AS_ADDR ) ;
   std::cout << BUFF <<endl ; 
   return rc ;
}

char *readLine ( char* p, int length )
{
   fgets ( p, length, stdin ) ;
   p[strlen(p)-1] = 0 ;
   return p ;
}

INT32 readInput ( CHAR *pPrompt, INT32 numIndent )
{
   memset ( receiveBuffer, 0, sizeof(receiveBuffer) ) ;
   for ( INT32 i = 0; i<numIndent; i++ )
   {
      printf("\t") ;
   }
   printf("%s> ", pPrompt ) ;
   readLine ( receiveBuffer, sizeof(receiveBuffer) ) ;
   // do a loop if the input end with '\\' character
   while ( receiveBuffer[strlen(receiveBuffer)-1] == '\\' &&
           RECEIVE_BUFFER_SIZE - strlen(receiveBuffer) > 0 )
   {
      for ( INT32 i = 0; i<numIndent; i++ )
      {
         printf("\t") ;
      }
      printf ( "> " ) ;
      readLine ( &receiveBuffer[strlen(receiveBuffer)-1],
                 RECEIVE_BUFFER_SIZE - strlen(receiveBuffer) ) ;
   }
   // make sure we don't read out of range
   if ( RECEIVE_BUFFER_SIZE == strlen(receiveBuffer) )
   {
      printf ( "Error: Max input length is %d bytes\n", RECEIVE_BUFFER_SIZE ) ;
      return SDB_INVALIDARG ;
   }
   return SDB_OK ;
}
