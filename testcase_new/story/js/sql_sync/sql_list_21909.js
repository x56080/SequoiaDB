/******************************************************************************
@Description seqDB-21909:内置SQL语句查询$LIST_USER
@author liyuanyue
@date 2020-3-24
******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   commDropUsr( db, "name_21909_0", "password_201909_0" );
   commDropUsr( db, "name_21909_1", "password_201909_1" );
   commDropUsr( db, "name_21909_2", "password_201909_2" );

   var option = { AuditMask: "ALL" };
   commCreateUsr( db, "name_21909_0", "password_201909_0", option );
   commCreateUsr( db, "name_21909_1", "password_201909_1", option );
   commCreateUsr( db, "name_21909_2", "password_201909_2", option );

   // 内置 sql 语句查询用户信息
   var cur = db.exec( "select * from $LIST_USER" );
   var usrCount = 0;
   while( cur.next() )
   {
      var tmpObj = cur.current().toObj();
      // 找到自己创建的用户
      for( var i = 0; i < 3; i++ )
      {
         if( tmpObj["User"] === ( "name_21909_" + i ) && tmpObj["Options"]["AuditMask"] === "ALL" )
         {
            usrCount++;
         }
      }
   }
   if( usrCount != 3 )
   {
      throw new Error( "expected result is " + 3 + ",but actually result is " + usrCount );
   }

   commDropUsr( db, "name_21909_0", "password_201909_0" );
   commDropUsr( db, "name_21909_1", "password_201909_1" );
   commDropUsr( db, "name_21909_2", "password_201909_2" );
}