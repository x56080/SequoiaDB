/*******************************************************************
* @Description : test export with cata node -s -p
*                seqDB-13490:指定从编目节点导出数据               
* @author      : Liang XueWang 
*
*******************************************************************/
var csname = COMMCSNAME ;
var clname = COMMCLNAME + "_sdbexprt13490" ;
var doc = { a: 1 } ;
var csvContent = "a\n1\n" ;
var jsonContent = "{ \"a\": 1 }\n" ;

main() ;

function main()
{  
   if( commIsStandalone( db ) )
   {
      println( "Run mode is standalone, no cata node" ) ;
      return ;
   }
   
   var groupName = "SYSCatalogGroup" ;
   var master = db.getRG( groupName ).getMaster().toString() ;
   var host = master.split( ":" )[0] ;
   var svc = master.split( ":" )[1] ;
   
   var catadb = new Sdb( host, svc ) ;
   var cl = commCreateCL( catadb, csname, clname, 0 ) ;
   cl.insert( doc ) ;
   testExprtCsv( host, svc ) ;
   testExprtJson( host, svc ) ;
   commDropCL( catadb, csname, clname, false, false ) ;
   catadb.close() ;
}

function testExprtCsv( hostname, svcname )
{
   var csvfile = workDir + "sdbexprt13490.csv" ;
   cmd.run( "rm -rf " + csvfile ) ;
   var command = installPath + "bin/sdbexprt" + 
                 " -s " + hostname + 
                 " -p " + svcname +  
                 " -c " + csname + 
                 " -l " + clname + 
                 " --file " + csvfile + 
                 " --type csv" + 
                 " --fields a" ;
   testRunCommand( command ) ;

   checkFileContent( csvfile, csvContent ) ;
   
   cmd.run( "rm -rf " + csvfile ) ;
}

function testExprtJson( hostname, svcname )
{
   var jsonfile = workDir + "sdbexprt13490.json" ;
   cmd.run( "rm -rf " + jsonfile ) ;
   var command = installPath + "bin/sdbexprt" + 
                 " -s " + hostname +
                 " -p " + svcname +
                 " -c " + csname + 
                 " -l " + clname + 
                 " --type json" + 
                 " --file " + jsonfile + 
                 " --fields a" ;
   testRunCommand( command ) ;
   
   checkFileContent( jsonfile, jsonContent ) ;
   
   cmd.run( "rm -rf " + jsonfile ) ;
}