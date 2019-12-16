/*******************************************************************
* @Description : test export with wrong -u -w
*                seqDB-13493:sdb开启鉴权，指定用户/密码错误
*                
* @author      : Liang XueWang 
*******************************************************************/
var username = "sequoiadb";
var password = "sequoiadb";
var csname = COMMCSNAME;
var clname = COMMCLNAME + "_sdbexprt13493";
var doc = { a: 1 };

main();

function main ()
{
   if( commIsStandalone( db ) )
   {
      println( "Run mode is standalone, can't create user" );
      return;
   }
   try
   {
      db.createUsr( username, password );
      var cl = commCreateCL( db, csname, clname );
      cl.insert( doc );
      testExprtWrongUser();
      testExprtWrongPass();
      commDropCL( db, csname, clname, false, false );
   }
   catch( e )
   {
      throw e;
   }
   finally
   {
      db.dropUsr( username, password );
   }
}

function testExprtWrongUser ()
{
   var csvfile = workDir + "sdbexprt13493.csv";
   cmd.run( "rm -rf " + csvfile );
   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -u wrongUser" +
      " -w " + password +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --type csv" +
      " --fields a";
   testRunCommand( command, 8 );

   cmd.run( "rm -rf " + csvfile );
}

function testExprtWrongPass ()
{
   var jsonfile = workDir + "sdbexprt13493.json";
   cmd.run( "rm -rf " + jsonfile );
   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -u " + username +
      " -w wrongPass" +
      " -c " + csname +
      " -l " + clname +
      " --type json" +
      " --file " + jsonfile +
      " --fields a";
   testRunCommand( command, 8 );

   cmd.run( "rm -rf " + jsonfile );
}