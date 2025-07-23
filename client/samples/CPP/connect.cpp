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

