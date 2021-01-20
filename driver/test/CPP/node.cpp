#include <stdio.h>
#include <gtest/gtest.h>
#include "client.hpp"
#include "testcommon.hpp"
#include "deleteFile.hpp"
#include <string>
#include <iostream>

using namespace std ;
using namespace sdbclient ;

/*************************************
 *           test begine             *
 *************************************/

void authConnTest(bool useSsl)
{
   sdb connect( useSsl ) ;
   sdbReplicaGroup group ;
   sdbNode node ;
   const CHAR *pHostName       = HOST ;
   const CHAR *pPort           = SERVER ;
   INT32 rc                    = SDB_OK ;
   const CHAR *pName           = GROUPNAME ;
   const CHAR *usrName         = "testUsr" ;
   const CHAR *pwd             = "123" ;
   sdb db ;

   // connect to database
   connect.connect( pHostName, pPort, usrName, pwd ) ;
   connect.createUsr( usrName, pwd ) ;
   rc = connect.getReplicaGroup( GROUPNAME, group ) ;
   if ( rc != SDB_OK )
   {
      goto done ;
   }

   rc = group.getMaster( node ) ;
   if ( rc != SDB_OK )
   {
      goto done ;
   }

   rc = node.connect( db ) ;
   if ( rc != SDB_OK )
   {
      goto done ;
   }
   cout << "Whether the node connection is successful: " << db.isValid() << endl ;
   db.disconnect() ;

done :
   connect.removeUsr( usrName, pwd ) ;
   connect.disconnect() ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

TEST(node, auth_connect)
{
   // case 1: not enable ssl
   authConnTest( FALSE ) ;
   // case 2: use ssl
   authConnTest( TRUE ) ;
}

