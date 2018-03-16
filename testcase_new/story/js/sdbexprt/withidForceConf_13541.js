/*******************************************************************
* @Description : test export with --force --withid
*                seqDB-13541:生成配置文件时包含_id               
* @author      : Liang XueWang
* 
*******************************************************************/
var csname = COMMCSNAME ;

main() ;

function main()
{
   testWithId() ;
}

function testWithId()
{
   var clname = COMMCLNAME + "_sdbexprt13541" ;
   var clname1 = COMMCLNAME + "_sdbimprt13541" ;
   var cl = commCreateCL( db, csname, clname, 0 ) ;
   var cl1 = commCreateCL( db, csname, clname1, 0 ) ;
   cl.insert( { _id: 1, a: 1 } ) ;
   
   var conffile = workDir + "sdbexprt13541.conf" ;
   cmd.run( "rm -rf " + conffile ) ;
   
   var command = installPath + "bin/sdbexprt" +
                 " -s " + COORDHOSTNAME +
                 " -p " + COORDSVCNAME + 
                 " -c " + csname +
                 " -l " + clname +
                 " --genconf " + conffile +
                 " --type csv" +
                 " --withid true" +
                 " --force true" ;
   testRunCommand( command ) ;
   
   var csvfile = workDir + "sdbexprt13541.csv" ;
   cmd.run( "rm -rf " + csvfile ) ; 
   command = installPath + "bin/sdbexprt" +
             " --conf " + conffile +
             " --file " + csvfile ;
   testRunCommand( command ) ;
   
   var content = "_id,a\n1,1\n" ;
   checkFileContent( csvfile, content ) ;
   
   command = installPath + "bin/sdbimprt" +
             " -s " + COORDHOSTNAME +
             " -p " + COORDSVCNAME +
             " -c " + csname +
             " -l " + clname1 +
             " --file " + csvfile +
             " --type csv " +
             " --headerline true" +
             " --fields='_id int,a int'" ;
   testRunCommand( command ) ;
   
   var expRecs = [ "{\"_id\":1,\"a\":1}" ] ;
   var cursor = cl.find() ;
   var actRecs = getRecords( cursor ) ;
   checkRecords( expRecs, actRecs ) ;
   
   cmd.run( "rm -rf " + conffile ) ;
   cmd.run( "rm -rf " + csvfile ) ;
   commDropCL( db, csname, clname ) ;
   commDropCL( db, csname, clname1 ) ;
}