/*******************************************************************
* @Description : test export with --included
*                seqDB-13534:导出多个集合的多个字段名到文件首行，
*                            其中包括不存在字段                             
* @author      : Liang XueWang
* 
*******************************************************************/
var csname = COMMCSNAME + "_sdbexprt13534" ;
var num = 5 ;
var clnames = [] ;
var doc = { a: 1, b: 2, c: 3 } ;
var csvContent = "a,b,c,d\n1,2,3,\n" ;

main() ;

function main()
{
   for( var i = 0;i < num;i++ )
   {
      var clname = COMMCLNAME + "_sdbexprt13534_" + i ;
      var cl = commCreateCL( db, csname, clname, 0 ) ;
      cl.insert( doc ) ;
      clnames.push( clname ) ;
   }
  
   testExprtCsv() ;
   
   for( var i = 0;i < num;i++ )
   {
      commDropCS( db, csname ) ;
   }
}

function testExprtCsv()
{
   var csvDir = workDir + "sdbexprt13534/" ;
   cmd.run( "rm -rf " + csvDir ) ;
   commMakeDir( "localhost", csvDir ) ;
   
   var command = installPath + "bin/sdbexprt" +
                 " -s " + COORDHOSTNAME +
                 " -p " + COORDSVCNAME + 
                 " --cscl " + csname +
                 " --dir " + csvDir +
                 " --type csv" +
                 " --included true" ;
   for( var i = 0;i < num;i++ )
   {
      command += " --fields " + csname + "." + clnames[i] + 
                 ":a,b,c,d" ;
   }
   
   testRunCommand( command ) ;
   
   for( var i = 0;i < num;i++ )
   {
      var filename = csvDir + csname + "." + clnames[i] + ".csv" ;
      checkFileContent( filename, csvContent ) ;
   }
   
   cmd.run( "rm -rf " + csvDir ) ;
}