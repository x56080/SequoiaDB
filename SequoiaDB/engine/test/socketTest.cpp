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

#include "ossSocket.hpp"
#include "msgReplicator.hpp"
#include "../bson/bson.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <boost/thread.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"
using namespace bson ;
using namespace std ;
int  MAXLOOP = 100000 ;
bool dumpHex = false ;
int main ( int argc, char** argv )
{
   char *pHost ;
   int port ;
   int sentLen = 0 ;
   if ( argc != 3 && argc != 4 )
   {
      printf ( "Syntax: host port [-dump]\n") ;
      return 0 ;
   }
   pHost = (char*)argv[1] ;
   port = atoi((char*)argv[2]);
   if ( argc == 4 && strcmp ( (char*)argv[3], "-dump" ) == 0)
      dumpHex = true ;
   {
      int rc = 0 ;
      char *pBuffer = NULL ;
      char *pOutBuffer = NULL ;
      vector<BSONObj> resultList ;
      vector<BSONObj>::iterator it ;
      BSONObj dummyObj ;
      boost::posix_time::ptime t1 ;
      boost::posix_time::ptime t2 ;
      boost::posix_time::time_duration diff ;
      printf ( "Connect to host %s port %d\n", pHost, port ) ;
      ossSocket sock ( pHost, port ) ;
      rc = sock.initSocket () ;
      if ( rc )
      {
         printf ( "Failed to init Socket\n" ) ;
         goto client_done ;
      }
      rc = sock.connect () ;
      if ( rc )
      {
         printf ( "Failed to connect Socket \n" ) ;
         goto client_done ;
      }
      sock.disableNagle();
      printf ("Successfully connected to server\n" ) ;
      pBuffer = (char*)malloc(sizeof(char)*100) ;
      t1 =
           boost::posix_time::microsec_clock::local_time() ;
      {
         MsgReplHeader msgHeader ;
         msgHeader.messageLength = sizeof(msgHeader) ;
         rc = sock.send ( (CHAR*)(&msgHeader), sizeof(msgHeader), sentLen ) ;
         if ( rc )
         {
            printf ("wrong\n" ) ;
         }
      }
      
      t2 =
           boost::posix_time::microsec_clock::local_time() ;
      printf ("Disconnected from server\n") ;
client_done :
      if (pBuffer)
         free (pBuffer);
      if (pOutBuffer)
         free (pOutBuffer);
      sock.close () ;
   }
   return 0 ;
}
