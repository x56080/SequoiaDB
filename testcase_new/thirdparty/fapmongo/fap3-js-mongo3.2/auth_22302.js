/******************************************** 
@description : createUser / getUser / getUsers / auth / dropUser
@testcase    : seqDB-22302
@author      : XiaoNi Huang 2020-02-24
*********************************************/
main();

function main ()
{
   var rc = db.getUsers();
   assert.eq( rc, [] );

   var users = ["auth_22302_1", "auth_22302_2", "auth_22302_3", "auth_22302_4"];
   var pwds = ["sequoiadb", "", "   ", "_test_"];
   var clName = "cl22302";
   var cl = db.getCollection( clName );
   cl.drop();

   try
   {
      // not any user
      // getUser
      var rc = db.getUser( "notExist" );
      assert.eq( rc, null );

      // getUsers
      var rc = db.getUsers();
      assert.eq( rc, [] );

      // get usersInfo
      var rc = db.runCommand( { "usersInfo": 1 } );
      assert.eq( rc, { "users": [], "ok": 1 } );

      // auth
      // exec success, return Error: Authentication has been disabled. Because sdb return success. Not bug
      db.auth( users[0], pwds[0] );


      // createUser
      for( var i = 0; i < users.length; i++ )
      {
         var rc = db.runCommand( { "createUser": users[i], "pwd": pwds[i], "roles": [] } );
         assert.eq( rc, { "ok": 1 } );
      }

      // not auth, business operations
      cl.insert( { "a": "t1" } );
      assert.eq( cl.count(), 1 );
      cl.remove( { "a": "t1" } );
      assert.eq( cl.count(), 0 );

      // auth, business operations
      for( var i = 0; i < users.length; i++ )
      {
         var rc = db.auth( users[i], pwds[i] );
         assert.eq( rc, 1 );
         cl.insert( { "a": i } );
         assert.eq( cl.count( { "a": i } ), 1 );
      }
      assert.eq( cl.count(), users.length );


      // exist multi users
      // getUser
      var rc = db.getUser( users[1] );
      assert.eq( rc, { "user": users[1] } );

      // getUsers
      var rc = db.getUsers();
      var expRc = [];
      for( var i = 0; i < users.length; i++ )
      {
         expRc.push( { "user": users[i] } );
      }
      assert.eq( JSON.stringify( rc.sort( sortBy( "user" ) ) ), JSON.stringify( expRc ) );

      // usersInfo
      // get all info
      var rc = db.runCommand( { "usersInfo": 1 } );
      rc["users"] = rc["users"].sort( sortBy( "user" ) );
      assert.eq( JSON.stringify( rc ), '{"users":' + JSON.stringify( expRc ) + ',"ok":1}' );
      // get one info
      var rc = db.runCommand( { "usersInfo": [{ "user": users[0] }] } );
      assert.eq( JSON.stringify( rc ), '{"users":' + JSON.stringify( expRc.slice( 0, 1 ) ) + ',"ok":1}' );
      // get more info
      var rc = db.runCommand( { "usersInfo": [{ "user": users[0] }, { "user": users[1] }] } );
      assert.eq( JSON.stringify( rc ), '{"users":' + JSON.stringify( expRc.slice( 0, 2 ) ) + ',"ok":1}' );
      // user not exist      
      var rc = db.runCommand( { "usersInfo": [{ "user": "notExist" }] } );
      assert.eq( rc, { "users": [], "ok": 1 } );


      // user exist, create the user
      var rc = db.runCommand( { "createUser": users[0], "pwd": pwds[0], "roles": [] } );
      assert.eq( rc, { "ok": 0, "code": -295, "errmsg": "The specified user already exist" } );
      var rc = db.getLastError();
      assert.eq( rc, "The specified user already exist" );


      // drop the user
      db.auth( users[0], pwds[0] );
      var rc = db.dropUser( users[0] );
      assert.eq( rc, true );
      // repeat drop the user
      try
      {
         db.dropUser( users[0] );
         throw new Error( "expect fail, but actual success" );
      }
      catch( e )
      {
         assert.eq( e, 'Error: user specified is not exist or password is invalid' );
      }
      var rc = db.getLastError();
      assert.eq( rc, "user specified is not exist or password is invalid" );

      // createUser again after drop the user
      var rc = db.runCommand( { "createUser": users[0], "pwd": pwds[0], "roles": [] } );
      assert.eq( rc, { "ok": 1 } );
      var rc = db.getUser( users[0] );
      assert.eq( rc, { "user": users[0] } );


      // password error
      var rc = db.auth( users[0], "errorPwd" );
      assert.eq( rc, 0 );
      var rc = db.getLastError();
      assert.eq( rc, "Authority is forbidden" );
   }
   finally
   {
      for( var i = 0; i < users.length; i++ )
      {
         var rc = db.auth( users[i], pwds[i] );
         if( rc === 1 )
         {
            db.dropUser( users[i] );
         }
      }
      var rc = db.getUsers();
      assert.eq( rc, [] );
   }

   cl.drop();


   // logout
   var rc = db.logout();
   assert.eq( rc, { "ok": 1 } );
   var rc = db.getLastError();
   assert.eq( rc, null );
}

function sortBy ( field )
{
   return function( a, b )
   {
      return a[field] > b[field];
   }
}