/*******************************************************************
* @Description : test export with cata node -s -p
*                seqDB-13490:指定从编目节点导出数据               
* @author      : Liang XueWang 
*
*******************************************************************/
var csname = COMMCSNAME;
var csvContent = "Name\n\"" + csname + "\"\n";
var jsonContent = "{ \"Name\": \"" + csname + "\" }\n";

main();

function main ()
{
   if( commIsStandalone( db ) )
   {
      println( "Run mode is standalone, no cata node" );
      return;
   }
   clearAllCs( db );
   commCreateCS( db, csname );

   var groupName = "SYSCatalogGroup";
   var master = db.getRG( groupName ).getMaster().toString();
   var host = master.split( ":" )[0];
   var svc = master.split( ":" )[1];

   testExprtCsv( host, svc );
   testExprtJson( host, svc );
}

function testExprtCsv ( hostname, svcname )
{
   var csvfile = tmpFileDir + "sdbexprt13490.csv";
   cmd.run( "rm -rf " + csvfile );
   var command = installPath + "bin/sdbexprt" +
      " -s " + hostname +
      " -p " + svcname +
      " -c SYSCAT" +
      " -l SYSCOLLECTIONSPACES" +
      " --file " + csvfile +
      " --type csv" +
      " --fields Name";
   testRunCommand( command );

   checkFileContent( csvfile, csvContent );

   cmd.run( "rm -rf " + csvfile );
}

function testExprtJson ( hostname, svcname )
{
   var jsonfile = tmpFileDir + "sdbexprt13490.json";
   cmd.run( "rm -rf " + jsonfile );
   var command = installPath + "bin/sdbexprt" +
      " -s " + hostname +
      " -p " + svcname +
      " -c SYSCAT" +
      " -l SYSCOLLECTIONSPACES" +
      " --type json" +
      " --file " + jsonfile +
      " --fields Name";
   testRunCommand( command );

   checkFileContent( jsonfile, jsonContent );

   cmd.run( "rm -rf " + jsonfile );
}

function clearAllCs ( db )
{
   var cursor = db.listCollectionSpaces();
   var obj;
   while( obj = cursor.next() )
   {
      var name = obj.toObj()["Name"];
      db.dropCS( name );
   }
}