/*******************************************************************
* @Description : test export with --filelimit
*                seqDB-13524:指定文件大小=数据量大小
*                seqDB-13525:指定文件大小<数据量大小，导出数据到json
*                seqDB-13537:指定文件大小<数据量大小，导出数据到csv          
* @author      : Liang XueWang 
*
*******************************************************************/
var csname = COMMCSNAME ;
var clname = COMMCLNAME + "_sdbexprt13524" ;
var clname1 = COMMCLNAME + "_sdbimprt13524" ;
var kbs = [ 10, 12 ] ;
var filelimit = "10K" ;
var expSize = 10*1024 ;
var kb ;

main() ;

function main()
{
   for( var i = 0;i < kbs.length;i++ )
   {
      kb = kbs[i] ;
      println( "test filelimit: " + filelimit + " data: " + kb ) ;
      testFileLimit() ;
   }
}

function testFileLimit()
{
   var cl = commCreateCL( db, csname, clname, 0 ) ;
   var cl1 = commCreateCL( db, csname, clname1, 0 ) ;
   
   insertDocs( cl, kb ) ; 
  
   testExprtImprtCsv() ;
   testExprtImprtJson() ;
  
   if( parseInt( cl1.count() ) !== 2*(kb*1024-1) )
   {
      throw buildException( "testFileLimit", null, "check import cl count", 
            2*(kb*1024-1), cl1.count() ) ;   
   }
   var cursor = cl1.find() ;
   var obj ;
   while( obj = cursor.next() )
   {
      var actVal = obj.toObj()["a"] ;
      if( actVal !== 1 )
      {
         throw buildException( "testFileLimit", null, "check import cl rec",
               1, actVal ) ;
      }
   }
   
   commDropCL( db, csname, clname ) ;
   commDropCL( db, csname, clname1 ) ;
}

// insert { a: 1 } repeatly until kb
function insertDocs( cl, kb )
{
   var bytes = kb*1024 ;
   for( var i = 0;i < bytes-1;i++ )
   {
      cl.insert( { a: 1 } ) ;
   }
}

function testExprtImprtCsv()
{
   var csvDir = workDir + "13524/" ;
   commMakeDir( "localhost", csvDir ) ;
   var csvfile = csvDir + "sdbexprt13524.csv" ;
   cmd.run( "rm -rf " + csvfile ) ;
   
   var command = installPath + "bin/sdbexprt" +
                 " -s " + COORDHOSTNAME +
                 " -p " + COORDSVCNAME + 
                 " -c " + csname + 
                 " -l " + clname +
                 " --file " + csvfile + 
                 " --filelimit " + filelimit +
                 " --type csv" +
                 " --included false" +
                 " --fields a" ;
   testRunCommand( command ) ;
   
   var actSize = parseInt( File.stat( csvfile ).toObj()["size"] ) ;
   if( actSize !== expSize )
   {
      throw buildException( "testExprtImprtCsv", null, "check file size",
            expSize, actSize ) ;
   }
   
   command = installPath + "bin/sdbimprt" +
             " -s " + COORDHOSTNAME +
             " -p " + COORDSVCNAME +
             " -c " + csname +
             " -l " + clname1 +
             " --file " + csvDir +
             " --type csv " +
             " --fields='a int'" ;  
   testRunCommand( command ) ;
   
   cmd.run( "rm -rf " + csvDir ) ;
}

function testExprtImprtJson()
{
   var jsonDir = workDir + "13527/" ;
   commMakeDir( "localhost", jsonDir ) ;
   var jsonfile = jsonDir + "sdbexprt13527.json" ;
   cmd.run( "rm -rf " + jsonfile ) ;
   
   var command = installPath + "bin/sdbexprt" +
                 " -s " + COORDHOSTNAME +
                 " -p " + COORDSVCNAME + 
                 " -c " + csname + 
                 " -l " + clname +
                 " --file " + jsonfile + 
                 " --filelimit " + filelimit +
                 " --type json" +
                 " --fields a" ;
   testRunCommand( command ) ;
   
   var actSize = parseInt( File.stat( jsonfile ).toObj()["size"] ) ;
   if( actSize > expSize )
   {
      throw buildException( "testExprtImprtJson", null, "check file size",
            expSize, actSize ) ;
   }
   
   command = installPath + "bin/sdbimprt" +
             " -s " + COORDHOSTNAME +
             " -p " + COORDSVCNAME +
             " -c " + csname +
             " -l " + clname1 +
             " --file " + jsonDir +
             " --type json" ;  
   testRunCommand( command ) ;
   
   cmd.run( "rm -rf " + jsonDir ) ;
}