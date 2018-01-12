/*******************************************************************
* @Description : test export with -a random ascii
*                seqDB-13513:自定义任意16进制ascii码为字符分隔符              
* @author      : Liang XueWang
* 
*******************************************************************/
var csname = COMMCSNAME ;
var clname = COMMCLNAME + "_sdbexprt13513" ;
var clname1 = COMMCLNAME + "_sdbimprt13513" ;
var doc = { a: 1, b: "exprtTest" } ;
var expRecs = [ "{\"a\":1,\"b\":\"exprtTest\"}" ] ;

main() ;

function main()
{
   var cl = commCreateCL( db, csname, clname, 0 ) ;
   var cl1 = commCreateCL( db, csname, clname1, 0 ) ;
   cl.insert( doc ) ; 
  
   testExprtImprt() ;
   var cursor = cl1.find( {}, { _id: { $include: 0 } } ) ;
   var actRecs = getRecords( cursor ) ; 
   checkRecords( expRecs, actRecs ) ;
   
   commDropCL( db, csname, clname ) ;
   commDropCL( db, csname, clname1 ) ;
}

function testExprtImprt()
{
   var csvfile = workDir + "sdbexprt13513.csv" ;
   cmd.run( "rm -rf " + csvfile ) ;
   var asc = getRandomInt( 0, 128 ) ;
   println( "ascii for delchar is: " + asc ) ;
   var command = installPath + "bin/sdbexprt" +
                 " -s " + COORDHOSTNAME +
                 " -p " + COORDSVCNAME + 
                 " -c " + csname + 
                 " -l " + clname +
                 " --file " + csvfile + 
                 " --type csv" +
                 " -a " + asc + 
                 " --fields a,b" ;
   testRunCommand( command ) ;
   
   command = installPath + "bin/sdbimprt" +
             " -s " + COORDHOSTNAME +
             " -p " + COORDSVCNAME +
             " -c " + csname +
             " -l " + clname1 +
             " --file " + csvfile +
             " --type csv " +
             " -a " + asc +
             " --fields='a int,b string'" ; 
   testRunCommand( command ) ;
   
   cmd.run( "rm -rf " + csvfile ) ;
}