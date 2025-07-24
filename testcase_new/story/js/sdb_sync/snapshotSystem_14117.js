/******************************************************************************
 * @Description   : seqDB-14117:获取系统快照后检查/etc/mtab文件句柄泄露  
 * @Author        : Liang XueWang
<<<<<<< HEAD
 * @LastEditTime  : 2023.05.26
=======
 * @LastEditTime  : 2022.08.01
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;
var cmd = new Cmd();

main( test );

function test ()
{
   db.snapshot( SDB_SNAP_SYSTEM );
   var pid = getCataPid();
   var fpNum = getFpNum( pid );
   assert.equal( fpNum, 0 );
}

// get local cata node pid
function getCataPid ()
{
<<<<<<< HEAD
   var remote = new Remote( COORDHOSTNAME, CMSVCNAME );
   var system = remote.getSystem();
   var cataSvcName = db.getCataRG().getMaster().getServiceName();
   var cursor = system.listProcess( {}, { cmd: "sequoiadb(" + cataSvcName + ") C" } );
=======
   var cataSvcName = db.getCataRG().getMaster().getServiceName();
   var cursor = System.listProcess( {}, { cmd: "sequoiadb(" + cataSvcName + ") C" } );
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
   var obj = cursor.next().toObj();
   var pid = obj["pid"];
   remote.close();
   return pid;
}

// get process pid /etc/mtab fp num
function getFpNum ( pid )
{
   var command = "lsof -p " + pid + " | grep /etc/mtab | wc -l";
   var info = cmd.run( command ).split( "\n" );
   return info[info.length - 2];
}
