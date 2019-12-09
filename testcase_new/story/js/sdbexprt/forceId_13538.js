/*******************************************************************
* @Description : test export with --force --withid
*                seqDB-13538:不指定fields，强制导出数据，包含_id
*                seqDB-13539:不指定fields，强制导出数据，不包含_id
*                seqDB-13540:指定fields并强制导出数据
*                seqDB-13542:非强制导出，指定包含_id              
* @author      : Liang XueWang
* 
*******************************************************************/
var csname = COMMCSNAME;

main();

function main ()
{
   testForceWithId();     // test force withid true
   testForceWithoutId();  // test force withid false
   testForceWithFields(); // test force with fields
}

function testForceWithId ()
{
   var clname = COMMCLNAME + "_sdbexprt13538";
   var clname1 = COMMCLNAME + "_sdbimprt13538";
   var cl = commCreateCL( db, csname, clname, 0 );
   var cl1 = commCreateCL( db, csname, clname1, 0 );
   cl.insert( { _id: 1, a: 1 } );

   var csvfile = workDir + "sdbexprt13538.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --type csv" +
      " --withid true " +
      " --force true";
   testRunCommand( command );

   var expect = "_id,a\n1,1\n";
   checkFileContent( csvfile, expect );

   command = installPath + "bin/sdbimprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname1 +
      " --file " + csvfile +
      " --type csv " +
      " --headerline true" +
      " --fields='_id int,a int'";
   testRunCommand( command );

   var expRecs = ["{\"_id\":1,\"a\":1}"];
   var cursor = cl.find();
   var actRecs = getRecords( cursor );
   checkRecords( expRecs, actRecs );

   cmd.run( "rm -rf " + csvfile );
   commDropCL( db, csname, clname );
   commDropCL( db, csname, clname1 );
}

function testForceWithoutId ()
{
   var clname = COMMCLNAME + "_sdbexprt13539";
   var clname1 = COMMCLNAME + "_sdbimprt13539";
   var cl = commCreateCL( db, csname, clname, 0 );
   var cl1 = commCreateCL( db, csname, clname1, 0 );
   cl.insert( { _id: 1, a: 1 } );

   var csvfile = workDir + "sdbexprt13539.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --type csv" +
      " --withid false " +
      " --force true";
   testRunCommand( command );

   var expect = "a\n1\n";
   checkFileContent( csvfile, expect );

   command = installPath + "bin/sdbimprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname1 +
      " --file " + csvfile +
      " --type csv " +
      " --headerline true" +
      " --fields='a int'";
   testRunCommand( command );

   var expRecs = ["{\"a\":1}"];
   var cursor = cl.find( {}, { _id: { $include: 0 } } );
   var actRecs = getRecords( cursor );
   checkRecords( expRecs, actRecs );

   cmd.run( "rm -rf " + csvfile );
   commDropCL( db, csname, clname );
   commDropCL( db, csname, clname1 );
}

function testForceWithFields ()
{
   var clname = COMMCLNAME + "_sdbexprt13540";
   var clname1 = COMMCLNAME + "_sdbimprt13540";
   var cl = commCreateCL( db, csname, clname, 0 );
   var cl1 = commCreateCL( db, csname, clname1, 0 );
   cl.insert( { _id: 1, a: 1 } );

   var csvfile = workDir + "sdbexprt13540.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --type csv" +
      " --withid false " +
      " --force true " +
      " --fields a";
   testRunCommand( command );

   var expect = "a\n1\n";
   checkFileContent( csvfile, expect );

   command = installPath + "bin/sdbimprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname1 +
      " --file " + csvfile +
      " --type csv " +
      " --headerline true" +
      " --fields='a int'";
   testRunCommand( command );

   var expRecs = ["{\"a\":1}"];
   var cursor = cl.find( {}, { _id: { $include: 0 } } );
   var actRecs = getRecords( cursor );
   checkRecords( expRecs, actRecs );

   cmd.run( "rm -rf " + csvfile );
   commDropCL( db, csname, clname );
   commDropCL( db, csname, clname1 );
}