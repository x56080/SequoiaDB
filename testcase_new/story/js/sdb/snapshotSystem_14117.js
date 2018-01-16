/*******************************************************************
* @Description : test snapshot system with /etc/mtab leak
*                seqDB-14117:获取系统快照后检查/etc/mtab文件句柄泄露                 
* @author      : Liang XueWang
* 
*******************************************************************/
var cmd = new Cmd() ;

main( db ) ;

function main( db )
{
   if( commIsStandalone( db ) )
   {
      println( "Run mode is standalone" ) ;
      return ;
   }
   
   db.snapshot( SDB_SNAP_SYSTEM ) ;
   var pid = getCataPid() ;
   var fpNum = getFpNum( pid ) ;
   if( parseInt( fpNum ) !== 0 )
   {
      throw buildException( "main", null, "check /etc/mtab fp num after snapshot system", 
                            0, fpNum ) ;
   }
}

// get local cata node pid
function getCataPid()
{
   try
   {
      var cursor = System.listProcess( {}, { cmd: "sequoiadb(" + CATASVCNAME + ") C" } ) ;
      var obj = cursor.next().toObj() ;
   }
   catch( e )
   {
      throw buildException( "getCataPid", e, "get cata node pid", 0, e ) ;
   }
   var pid = obj["pid"] ;
   println( "cata node pid: " + pid ) ;
   return pid ;
}

// get process pid /etc/mtab fp num
function getFpNum( pid )
{
   var command = "lsof -p " + pid + " | grep /etc/mtab | wc -l" ;
   try
   {
      var info = cmd.run( command ).split( "\n" ) ;
   }
   catch( e )
   {
      throw buildException( "getFpNum", e, "cmd run command: " + command, 0, e ) ;
   }
   println( "lsof res: " + info ) ;
   return info[ info.length-2 ] ;
}