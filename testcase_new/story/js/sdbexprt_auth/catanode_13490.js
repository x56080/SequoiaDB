/*******************************************************************
* @Description : test export with cata node -s -p
*                seqDB-13490:指定从编目节点导出数据               
* @author      : Liang XueWang 
*
*******************************************************************/
var csvContent = "Name\n\"" + COMMCSNAME + "\"\n";
var jsonContent = "{ \"Name\": \"" + COMMCSNAME + "\" }\n";

main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

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

   // 多了哪个 CS，哪个CS就没删干净
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
