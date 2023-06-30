/******************************************** 
@description : seqDB-22304:createUser/auth配置鉴权算法（mongo4.0及以上版本）
@author      : XiaoNi Huang 2020-09-01
*********************************************/
main();

function main ()
{
   var rc = db.getUsers();
   assert.eq( rc, [] );

   var user = "auth_22304";
   var pwd = "sequoiadb";
   var clName = "cl22304";
   var cl = db.getCollection( clName );
   cl.drop();

   try
   {
      testSHA1( db, cl, user, pwd );
      testOtherMechanism( db, user, pwd );
      testParameter( db, user, pwd );
   }
   finally
   {
      db.auth( user, pwd );
      try
      {
         db.dropUser( user );
      }
      catch( e )
      {
         assert.eq( e, "Error: user specified is not exist or password is invalid" );
      }
   }
   cl.drop();
}

function testSHA1 ( db, cl, user, pwd )
{
   // createUser, mechanisms: ["SCRAM-SHA-1"]
   var rc = db.runCommand( { "createUser": user, "pwd": pwd, "roles": [], mechanisms: ["SCRAM-SHA-1"] } );
   assert.eq( rc, { "ok": 1 } );
   var rc = db.getUsers();
   assert.eq( rc, [{ "user": user }] );

   // auth and exec business operations
   var rc = db.auth( { "user": user, "pwd": pwd, mechanisms: "SCRAM-SHA-1" } );
   assert.eq( rc, 1 );
   cl.insert( { "a": 0 } );
   assert.eq( cl.count( { "a": 0 } ), 1 );

   // auth, mechanisms is array
   var rc = db.auth( { "user": user, "pwd": pwd, mechanisms: ["SCRAM-SHA-1"] } );
   assert.eq( rc, 1 );

   // auth, the mechanisms is different from the mechanisms when it was created, and exec business operations
   var rc = db.auth( { "user": user, "pwd": pwd, mechanisms: "SCRAM-SHA-256" } );
   assert.eq( rc, 1 );

   // auth, mechanisms is error
   var rc = db.auth( { "user": user, "pwd": pwd, mechanisms: "SCRAM-SHA-111" } );
   assert.eq( rc, 1 );

   // drop the user
   var rc = db.dropUser( user );
   assert.eq( rc, true );
}

function testOtherMechanism ( db, user, pwd )
{
   // 目前只支持SCAM-SHA-1
   // createUser, mechanisms: ["SCRAM-SHA-256"]
   var rc = db.runCommand( { "createUser": user, "pwd": pwd, "roles": [], mechanisms: ["SCRAM-SHA-256"] } );
   assert.eq( rc, { "ok": 0, "errmsg": "Unsupported auth mechanism[SCRAM-SHA-256]", "code": -6 } );
   var rc = db.getUsers();
   assert.eq( rc.indexOf( user ), -1 );

   // createUser, mechanisms: ["MONGODB-CR"]
   var rc = db.runCommand( { "createUser": user, "pwd": pwd, "roles": [], mechanisms: ["MONGODB-CR"] } );
   assert.eq( rc, { "ok": 0, "errmsg": "Unsupported auth mechanism[MONGODB-CR]", "code": -6 } );
   var rc = db.getUsers();
   assert.eq( rc.indexOf( user ), -1 );

   // createUser, mechanisms: ["MONGODB-AWS"]
   var rc = db.runCommand( { "createUser": user, "pwd": pwd, "roles": [], mechanisms: ["MONGODB-AWS"] } );
   assert.eq( rc, { "ok": 0, "errmsg": "Unsupported auth mechanism[MONGODB-AWS]", "code": -6 } );
   var rc = db.getUsers();
   assert.eq( rc.indexOf( user ), -1 );

   // createUser, mechanisms: ["MONGODB-X509"]
   var rc = db.runCommand( { "createUser": user, "pwd": pwd, "roles": [], mechanisms: ["MONGODB-X509"] } );
   assert.eq( rc, { "ok": 0, "errmsg": "Unsupported auth mechanism[MONGODB-X509]", "code": -6 } );
   var rc = db.getUsers();
   assert.eq( rc.indexOf( user ), -1 );

   // createUser, mechanisms: ["GSSAPI"]
   var rc = db.runCommand( { "createUser": user, "pwd": pwd, "roles": [], mechanisms: ["GSSAPI"] } );
   assert.eq( rc, { "ok": 0, "errmsg": "Unsupported auth mechanism[GSSAPI]", "code": -6 } );
   var rc = db.getUsers();
   assert.eq( rc.indexOf( user ), -1 );

   // createUser, mechanisms: ["PLAIN"]
   var rc = db.runCommand( { "createUser": user, "pwd": pwd, "roles": [], mechanisms: ["PLAIN"] } );
   assert.eq( rc, { "ok": 0, "errmsg": "Unsupported auth mechanism[PLAIN]", "code": -6 } );
   var rc = db.getUsers();
   assert.eq( rc.indexOf( user ), -1 );

   // createUser, mechanisms: ["SCRAM-SHA-1","SCRAM-SHA-256"]
   var rc = db.runCommand( { "createUser": user, "pwd": pwd, "roles": [], mechanisms: ["SCRAM-SHA-1", "SCRAM-SHA-256"] } );
   assert.eq( rc, { "ok": 0, "errmsg": "Unsupported auth mechanism[SCRAM-SHA-256]", "code": -6 } );
   var rc = db.getUsers();
   assert.eq( rc.indexOf( user ), -1 );
}

function testParameter ( db, user, pwd )
{
   // createUser, mechanisms error
   var rc = db.runCommand( { "createUser": user, "pwd": pwd, "roles": [], mechanisms: ["SCRAM-SHA-123"] } );
   assert.eq( rc, { "ok": 0, "errmsg": "Unsupported auth mechanism[SCRAM-SHA-123]", "code": -6 } );
   var rc = db.getLastError();
   assert.eq( JSON.stringify( rc ), "\"Unsupported auth mechanism[SCRAM-SHA-123]\"" );
}