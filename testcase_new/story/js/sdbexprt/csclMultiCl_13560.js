/*******************************************************************
* @Description : test export with --cscl multi cl
*                seqDB-13560:--cscl指定多个集合导出数据到json
*                seqDB-13561:--cscl指定多个集合导出数据到csv
*                seqDB-13574:--dir导出数据到指定目录                
* @author      : Liang XueWang 
*
*******************************************************************/
var clnum = 5 ;
var clnames = [] ;
var csnames = [] ;
var doc = { a: 1 } ;
var csvContent = "a\n1\n" ;
var jsonContent = "{ \"a\": 1 }\n"

main() ;

function main()
{
   for( var i = 0;i < clnum;i++ )
   {
      var csname = COMMCSNAME + "_sdbexprt13560_" + i ;
      var clname = COMMCLNAME + "_sdbexprt13560_" + i ;
      var cl = commCreateCL( db, csname, clname ) ;
      cl.insert( doc ) ;
      clnames.push( clname ) ;
      csnames.push( csname ) ;
   }
  
   testExprtCsv() ;
   testExprtJson() ;
   
   for( var i = 0;i < clnum;i++ )
   {
      commDropCS( db, csnames[i] ) ;
   }
}

function testExprtCsv()
{
   var csvDir = workDir + "13561/" ;
   commMakeDir( "localhost", csvDir ) ;
   
   var command = installPath + "bin/sdbexprt" +
                 " -s " + COORDHOSTNAME +
                 " -p " + COORDSVCNAME + 
                 " --dir " + csvDir +
                 " --type csv" ;
   command += " --cscl " ;
   for( var i = 0;i < clnum;i++ )
   {
      command += csnames[i] + "." + clnames[i] ;
      if( i !== clnum-1 )
         command += "," ;
   }
   for( var i = 0;i < clnum;i++ )
   {
      command += " --fields " + csnames[i] + "." + clnames[i] + ":a" ;
   }
   testRunCommand( command ) ;
   
   for( var i = 0;i < clnum;i++ )
   {
      var filename = csvDir + csnames[i] + "." + clnames[i] + ".csv" ;
      checkFileContent( filename, csvContent ) ;
   }
   
   cmd.run( "rm -rf " + csvDir ) ;
}

function testExprtJson()
{
   var jsonDir = workDir + "13560/" ;
   commMakeDir( "localhost", jsonDir ) ;
   
   var command = installPath + "bin/sdbexprt" +
                 " -s " + COORDHOSTNAME +
                 " -p " + COORDSVCNAME + 
                 " --dir " + jsonDir +
                 " --type json" ;
   command += " --cscl " ;
   for( var i = 0;i < clnum;i++ )
   {
      command += csnames[i] + "." + clnames[i] ;
      if( i !== clnum-1 )
         command += "," ;
   }
   for( var i = 0;i < clnum;i++ )
   {
      command += " --fields " + csnames[i] + "." + clnames[i] + ":a" ;
   }
   testRunCommand( command ) ;

   for( var i = 0;i < clnum;i++ )
   {
      var filename = jsonDir + csnames[i] + "." + clnames[i] + ".json" ;
      checkFileContent( filename, jsonContent ) ;
   }
   
   cmd.run( "rm -rf " + jsonDir ) ;
}