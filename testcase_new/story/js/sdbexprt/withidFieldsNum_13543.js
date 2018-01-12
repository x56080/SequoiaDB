/*******************************************************************
* @Description : test export with --force --withid
*                seqDB-13543:后面记录比第一条记录字字段数多，
*                            强制导出到json
*                seqDB-13544:后面记录比第一条记录字字段数多，
*                            强制导出到csv             
* @author      : Liang XueWang 
*
*******************************************************************/
var csname = COMMCSNAME ;

main() ;

function main()
{
   testWithIdCsv() ;
   testWithIdJson() ;
}

function testWithIdCsv()
{
   var clname = COMMCLNAME + "_sdbexprt13544" ;
   var clname1 = COMMCLNAME + "_sdbimprt13544" ;
   var cl = commCreateCL( db, csname, clname, 0 ) ;
   var cl1 = commCreateCL( db, csname, clname1, 0 ) ;
   cl.insert( { _id: 1, a: 1 } ) ;
   cl.insert( { _id: 2, a: 2, b: 1 } ) ;
   cl.insert( { _id: 3, a: 3, b: 2, c: 1 } ) ;
   
   var csvfile = workDir + "sdbexprt13544.csv" ;
   cmd.run( "rm -rf " + csvfile ) ; 
   
   var command = installPath + "bin/sdbexprt" +
                 " -s " + COORDHOSTNAME +
                 " -p " + COORDSVCNAME + 
                 " -c " + csname +
                 " -l " + clname +
                 " --file " + csvfile +
                 " --type csv" +
                 " --withid true" +
                 " --sort '{ _id: 1 }'" +
                 " --force true" ;
   testRunCommand( command ) ;
   
   var content = "_id,a\n1,1\n2,2\n3,3\n" ;
   checkFileContent( csvfile, content ) ;
   
   command = installPath + "bin/sdbimprt" +
             " -s " + COORDHOSTNAME +
             " -p " + COORDSVCNAME +
             " -c " + csname +
             " -l " + clname1 +
             " --file " + csvfile +
             " --type csv " +
             " --fields='_id int,a int'" +
             " --headerline true" ;
   testRunCommand( command ) ;
   
   var expRecs = [ "{\"_id\":1,\"a\":1}",
                   "{\"_id\":2,\"a\":2}",
                   "{\"_id\":3,\"a\":3}" ] ;
   var cursor = cl1.find() ;
   var actRecs = getRecords( cursor ) ;
   checkRecords( expRecs, actRecs ) ;
   
   cmd.run( "rm -rf " + csvfile ) ;
   commDropCL( db, csname, clname ) ;
   commDropCL( db, csname, clname1 ) ;
}

function testWithIdJson()
{
   var clname = COMMCLNAME + "_sdbexprt13543" ;
   var clname1 = COMMCLNAME + "_sdbimprt13543" ;
   var cl = commCreateCL( db, csname, clname, 0 ) ;
   var cl1 = commCreateCL( db, csname, clname1, 0 ) ;
   cl.insert( { _id: 1, a: 1 } ) ;
   cl.insert( { _id: 2, a: 2, b: 1 } ) ;
   cl.insert( { _id: 3, a: 3, b: 2, c: 1 } ) ;
   
   var jsonfile = workDir + "sdbexprt13543.json" ;
   cmd.run( "rm -rf " + jsonfile ) ; 
   
   var command = installPath + "bin/sdbexprt" +
                 " -s " + COORDHOSTNAME +
                 " -p " + COORDSVCNAME + 
                 " -c " + csname +
                 " -l " + clname +
                 " --file " + jsonfile +
                 " --type json" +
                 " --withid true" +
                 " --sort '{ _id: 1 }'" +
                 " --force true" ;
   testRunCommand( command ) ;
   
   var content = "{ \"_id\": 1, \"a\": 1 }\n" +
                 "{ \"_id\": 2, \"a\": 2, \"b\": 1 }\n" +
                 "{ \"_id\": 3, \"a\": 3, \"b\": 2, \"c\": 1 }\n" ;
   checkFileContent( jsonfile, content ) ;
   
   command = installPath + "bin/sdbimprt" +
             " -s " + COORDHOSTNAME +
             " -p " + COORDSVCNAME +
             " -c " + csname +
             " -l " + clname1 +
             " --file " + jsonfile +
             " --type json" ;
   testRunCommand( command ) ;
   
   var expRecs = [ "{\"_id\":1,\"a\":1}",
                   "{\"_id\":2,\"a\":2,\"b\":1}",
                   "{\"_id\":3,\"a\":3,\"b\":2,\"c\":1}" ] ;
   var cursor = cl1.find() ;
   var actRecs = getRecords( cursor ) ;
   checkRecords( expRecs, actRecs ) ;
   
   cmd.run( "rm -rf " + jsonfile ) ;
   commDropCL( db, csname, clname ) ;
   commDropCL( db, csname, clname1 ) ;
}