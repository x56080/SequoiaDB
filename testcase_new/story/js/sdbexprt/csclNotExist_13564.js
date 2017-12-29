/*******************************************************************
* @Description : test export with --cscl multi cs
*                seqDB-13564:--cscl指定的集合空间/集合不存在             
* @author      : Liang XueWang 
*
*******************************************************************/
var csname = COMMCSNAME + "_sdbexprt13564" ;

main() ;

function main()
{
   commCreateCS( db, csname ) ;
  
   testExprtNotExistCs() ;
   testExprtNotExistCl() ;
   
   commDropCS( db, csname ) ;
}

function testExprtNotExistCs()
{
   var csvfile = workDir + "sdbexprt13564.csv" ;
   cmd.run( "rm -rf " + csvfile ) ;
   
   var command = installPath + "bin/sdbexprt" +
                 " -s " + COORDHOSTNAME +
                 " -p " + COORDSVCNAME + 
                 " --file " + csvfile +
                 " --type csv" +
                 " --cscl notExistCs " + 
                 " --force true" ;
   testRunCommand( command, 8 ) ;
   
   cmd.run( "rm -rf " + csvfile ) ;
}

function testExprtNotExistCl()
{
   var jsonfile = workDir + "sdbexprt13564.json" ;
   cmd.run( "rm -rf " + jsonfile ) ;
   
   var command = installPath + "bin/sdbexprt" +
                 " -s " + COORDHOSTNAME +
                 " -p " + COORDSVCNAME + 
                 " --file " + jsonfile +
                 " --type json" +
                 " --cscl " + csname + ".notExistCl" ;
   testRunCommand( command, 8 ) ;
   
   cmd.run( "rm -rf " + jsonfile ) ;
}