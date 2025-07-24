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

   Source File Name = main.cpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "taskmng.hpp"
#include "common.h"

#define DEFAULT_TOTAL_INSERT_NUM 100000LL
#define DEFAULT_INTERVAL 1000000L

void help ( const char* pName )
{
   printf( "Syntax:%s <-m mode> [-t totalNum] [-c connectionString] [-s dbName] [-n threadNumber]  [-i interval]\n",  pName) ;
   printf( "\tmode:\n" ) ;
   printf( "\t\t%d: put files to db\n", (int) MODE_PUT) ;
   printf( "\t\t%d: get files from db\n", (int)MODE_GET) ;
   printf( "\t\t%d: put files to db and get files from db\n", (int)MODE_MIX) ;
}

void printArguments ( int runMode, int totalNum )
{
   printf ( "RunMode:\t" ) ;
   switch ( runMode )
   {
      case MODE_PUT:
               printf ( "put files" ) ;
               break ;
       case MODE_GET:
               printf ( "get files" ) ;
               break ;
       case MODE_MIX:
               printf ( "put files and get files" ) ;
               break ;
   }
   printf ( "\n" ) ;
   printf ( "totalNum:\t%d\n", totalNum ) ;
}

int main ( int argc, char* argv[] )
{
   int opt ;
   int runMode ;
   int totalNum = DEFAULT_TOTAL_INSERT_NUM ;
   int thrNum = 1 ;
   char* dbName = "lobtest" ;
   int interval = DEFAULT_INTERVAL ;
   char* connectStr = NULL ;

   while ( ( opt = getopt ( argc, argv, "m:t:c:s:n:i:h" ) )  != -1 ) 
   {
      switch ( opt )
      {
         case 'm':
               runMode = atoi ( optarg ) ;
               break ;
         case 't':
               totalNum = atoi ( optarg ) ;
               break ;
         case 'c':
               connectStr = optarg ;
               break ;
         case 's':
               dbName = optarg ;
               break ;
         case 'n':
               thrNum = atoi ( optarg ) ;
               break ;
         case 'i':
               interval = atoi ( optarg ) ;
               break ;
         case 'h':
               help ( argv[0] ) ;
               return 0 ;
         default:
               help ( argv[0] ) ;
               return -1 ;    
      }
   }

   if ( NULL == connectStr )
   {
      connectStr = "localhost:11810" ;
   }

   if ( interval > totalNum ) 
   {
      interval = totalNum ;
   }

   printArguments( runMode, totalNum ) ;

   CTaskMng::getInstance( ).setRunPara( runMode, totalNum, thrNum, interval, connectStr, dbName) ;

   CTaskMng::getInstance( ).Init( ) ;
   CTaskMng::getInstance( ).run( ) ;

   CTaskMng::getInstance( ).wait( ) ;

   return 0 ;
   
}

