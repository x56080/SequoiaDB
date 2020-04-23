/******************************************************************************
@Description : seqDB-22077:不鉴权创建存储过程，鉴权后执行存储过程
@Athor : XiaoNi Huang 2020-04-22
******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var testcaseID = 22077;
   var user1 = CHANGEDPREFIX + "_auth_" + testcaseID + "_1";
   var user2 = CHANGEDPREFIX + "_auth_" + testcaseID + "_2";
   var procName = 'testProce' + testcaseID;
   var procFunc = 'testProce' + testcaseID + '()';
   // clean
   cleanUsers( user1, user2 );
   cleanProcedure( procName );

   var sdb = new Sdb( COORDHOSTNAME, COORDSVCNAME );

   try
   {
      // no auth user, create procedure 
      sdb.createProcedure( function testProce22077 () { return db.list( 0 ); } );
      var cursor = sdb.eval( procFunc );
      checkResults( cursor.size() );

      // create user1
      sdb.createUsr( user1, user1 );
      // use old sdb
      var cursor = sdb.eval( procFunc );
      checkResults( cursor.size() );
      // new db1 by user1
      var db1 = new Sdb( COORDHOSTNAME, COORDSVCNAME, user1, user1 );
      // use old sdb
      var cursor = sdb.eval( procFunc );
      checkResults( cursor.size() );
      // use db1      
      var cursor = db1.eval( procFunc );
      checkResults( cursor.size() );

      // create user2
      sdb.createUsr( user2, user2 );
      // use db1
      var cursor = db1.eval( procFunc );
      checkResults( cursor.size() );
      // new db2 by user2
      var db2 = new Sdb( COORDHOSTNAME, COORDSVCNAME, user2, user2 );
      // use db2
      var cursor = db2.eval( procFunc );
      checkResults( cursor.size() );

      // drop user1
      sdb.dropUsr( user1, user1 );
      // use db1
      try
      {
         db1.eval( procFunc );
      }
      catch( e )
      {
         if( e !== -152 )
         {
            throw new Error( e );
         }
      }
      // use db2
      var cursor = db2.eval( procFunc );
      checkResults( cursor.size() );

      // drop all users
      sdb.dropUsr( user2, user2 );
      // use old sdb
      var cursor = sdb.eval( procFunc );
      checkResults( cursor.size() );
      // use db1      
      var cursor = db1.eval( procFunc );
      checkResults( cursor.size() );
      // use db2
      var cursor = db2.eval( procFunc );
      checkResults( cursor.size() );

      // clean
      db.removeProcedure( procName );
   }
   finally 
   {
      cleanUsers( user1, user2 );
      sdb.close();
   }
}

function checkResults ( actSize )
{
   if( 0 === actSize ) 
   {
      throw new Error( "check failed, expSize != 0, actCursorSize = " + actSize );
   }
}

function cleanProcedure ( procName )
{
   try
   {
      db.removeProcedure( procName );
   }
   catch( e )
   {
      if( e.message !== "-233" )
      {
         throw new Error( e );
      }
   }
}

function cleanUsers ( user1, user2 )
{
   var users = [user1, user2];
   for( var i = 0; i < users.length; i++ )
   {
      try
      {
         db.dropUsr( users[i], users[i] );
      }
      catch( e )
      {
         if( e.message !== "-300" )
         {
            throw new Error( e );
         }
      }
   }
}