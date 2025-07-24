<<<<<<< HEAD
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
=======
/******************************************************************************
*
* Name: connect.cpp
* Description: This program demostrates how to connect to SequoiaDB database.
*
* Parameters:
*              HostName: The hostname for database server
*              ServiceName: The service name or port number for the database
*                           service
*              Username: The user name for database server
*              Password: The password  for user
* Auto Compile:
* Linux: ./buildApp.sh connect
* Win: buildApp.bat connect
* Manual Compile:
*    Dynamic Linking:
*    Linux:
*       if GCC version >= 5.1
*          g++ connect.cpp common.cpp -o connect -I../../include -O0 -ggdb \
*             -Wno-deprecated -L../../lib -lsdbcpp -lm -ldl -D_GLIBCXX_USE_CXX11_ABI=0
*       if GCC version < 5.1
*          g++ connect.cpp common.cpp -o connect -I../../include -O0 -ggdb \
*             -Wno-deprecated -L../../lib -lsdbcpp -lm -ldl
*    Win:
*       cl /Foconnect.obj /c connect.cpp /I..\..\include /wd4047 /Od /MDd /RTC1 /Z7 /TP
*       cl /Focommon.obj /c common.cpp /I..\..\include /wd4047 /Od /MDd /RTC1 /Z7 /TP
*       link /OUT:connect.exe /LIBPATH:..\..\lib\cpp\debug\dll sdbcppd.lib connect.obj common.obj /debug
*       copy ..\..\lib\cpp\debug\dll\sdbcppd.dll . 
*    Static Linking:
*    Linux: 
*       if GCC version >= 5.1
*          g++ connect.cpp common.cpp -o connect.static -I../../include -O0 \
*             -ggdb -Wno-deprecated ../../lib/libstaticsdbcpp.a -lm -ldl -lpthread -D_GLIBCXX_USE_CXX11_ABI=0
*       if GCC version < 5.1      
*          g++ connect.cpp common.cpp -o connect.static -I../../include -O0 \
*            -ggdb -Wno-deprecated ../../lib/libstaticsdbcpp.a -lm -ldl -lpthread
* Run:
*    Linux: LD_LIBRARY_PATH=<path for libsdbcpp.so> ./connect <hostname> \
*           <servicename> <username> <password>
*    Win: connect.exe <hostname> <servicename> <username> <password>
*
******************************************************************************/
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
#include <iostream>
#include "common.hpp"

using namespace std ;
using namespace sdbclient ;
using namespace sample ;

// Display Syntax Error
void displaySyntax ( CHAR *pCommand ) ;

INT32 main ( INT32 argc, CHAR **argv )
{
  // verify syntax
  if ( 5 != argc )
  {
     displaySyntax ( (CHAR*)argv[0] ) ;
     exit ( 0 ) ;
  }
  // read argument
  CHAR *pHostName    = (CHAR*)argv[1] ;
  CHAR *pPort        = (CHAR*)argv[2] ;
  CHAR *pUsr         = (CHAR*)argv[3] ;
  CHAR *pPasswd      = (CHAR*)argv[4] ;

  // define local variable
  sdb connection ;
  INT32 rc = SDB_OK ;

  // connect to database
  rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
  if( rc!=SDB_OK )
  {
     cout<<"Fail to connet to database, rc = "<<rc<<endl ;
     goto error ;
  }
  else
     cout<<"Connect success!"<<endl ;

done:
  // disconnect from database
  connection.disconnect () ;
  return 0 ;
error:
  goto done ;
}

// Display Syntax Error
void displaySyntax ( CHAR *pCommand )
{
  cout << "Syntax:" << pCommand 
       << " <hostname> <servicename> <username> <password>" << endl ;
}

