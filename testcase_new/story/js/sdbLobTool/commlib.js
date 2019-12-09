/******************************************************************************
*@Description : common function for lob import/export/migration tool
*               the testcase about user/password cannot run in concurrent.
*@Modify list :
*               2016-06-20  XueWang Liang  Change
******************************************************************************/

var hostName = COORDHOSTNAME;	// 主机名：'localhost'
var coordPort = COORDSVCNAME;	// 端口号：11810
var user = null;                // 用户名：
var passwd = null;              // 密码：
var db = new Sdb( hostName, coordPort, user, passwd );
var cmd = new Cmd();
var LocalPath = null;           // 当前目录
var InstallPath = null;         // 安装目录



/******************************************************************************
*@Description : initalize the global variable in the begninning.
                初始化全局变量LocalPath、InstallPath
******************************************************************************/
function initPath ()
{
   try
   {
      var local = cmd.run( "pwd" ).split( "\n" );
      LocalPath = local[0];
      try
      {
         var install = cmd.run( "sed -n '3p'  /etc/default/sequoiadb" ).split( "=" )[1].split( "\n" );
         InstallPath = install[0];   //获得默认安装目录 /opt/sequoiadb
      }
      catch( e )
      {
         if( 2 == e ) // Linux错误码2：No such file or directory	                                     
            InstallPath = toolGetInstallPath( LocalPath );    // 检验当前目录是否为安装目录
         else
            throw ( "failed to excute : sed -n '3p'  /etc/default/sequoiadb,rc = " + e );
      }
   }
   catch( e )
   {
      println( "failed to get global variable : LocalPath/InstallPath,rc = " + e );
      throw e;
   }
   println( "LocalPath: " + LocalPath );
   println( "InstallPath: " + InstallPath );
}


/******************************************************************************
*@Description : when run these testcase in sequoiadb or trunk fold that not
*               installed, get home fold.   <?how to get sequoiadb home fold?>
******************************************************************************/
function toolGetInstallPath ( localPath )
{
   try
   {
      var folder = cmd.run( 'ls ' + localPath ).split( '\n' );
      var fcnt = 0;
      for( var i = 0; i < folder.length; ++i )
      {
         if( "bin" == folder[i] || "SequoiaDB" == folder[i] || "testcases" == folder[i] )
         {
            fcnt++;
         }
      }
      if( 2 <= fcnt )
         InstallPath = localPath;
      return InstallPath;
   }
   catch( e )
   {
      println( "failed to get install path in source install, rc = " + e );
      throw e;
   }
}


/******************************************************************************
*@Description : check sdblobtool exists or not
*               检查sdblobtool工具是否存在
******************************************************************************/
function checkLobtool ()
{
   try
   {
      var Cmd = InstallPath + "/bin/sdblobtool -v";
      cmd.run( Cmd );
   }
   catch( e )
   {
      if( e == 127 )
         println( ">No sdblobtool in the computer!!!" );
      else
         println( ">fail to check sdblobtool,rc = " + e );
      throw e;
   }
}

/******************************************************************************
*@Description : command for sdblobtool
*               stdlobtool命令执行语句的生成
******************************************************************************/
function toolGetCmdstr ( Args )
{
   var Cmd = InstallPath + "/bin/sdblobtool";
   for( var k in Args )
   {
      if( k == "prefer" )
         Cmd += " --" + k + " " + Args[k];
      else if( Args[k] == true )
         Cmd += " --" + k;
      else if( Args[k] != false )
         Cmd += " --" + k + " " + Args[k];
   }
   return Cmd;
}


/******************************************************************************
*@Description : test lob export/import/migration with wrong parameter
*               大对象工具sdblobtool的导出导入迁移操作
******************************************************************************/
function toolExport ( Args )
{
   // 执行导出操作
   cmd.run( "rm -rf " + Args["file"] );
   var Cmd = toolGetCmdstr( Args );
   println( ">" + Cmd );
   try
   {
      cmd.run( Cmd );
      println( ">export successful" );
   }
   catch( e )
   {
      println( ">fail to export,rc = " + e );
      throw e;
   }
}

function toolImport ( Args )
{
   // 执行导入操作
   var Cmd = toolGetCmdstr( Args );
   println( ">" + Cmd );
   try
   {
      cmd.run( Cmd );
      println( ">import successful" );
   }
   catch( e )
   {
      println( ">fail to import,rc = " + e );
      throw e;
   }
}

function toolMigration ( Args )
{
   // 执行迁移操作
   var Cmd = toolGetCmdstr( Args );
   println( ">" + Cmd );
   try
   {
      cmd.run( Cmd );
      println( ">migrate successful" );
   }
   catch( e )
   {
      println( ">fail to migrate,rc = " + e );
      throw e;
   }
}


/******************************************************************************
*@Description : the function of make lobfile to be a lob  
                创建lobfile文件作为大对象
******************************************************************************/
function toolMakeLobfile ()
{
   var fileName = CHANGEDPREFIX + "_toolFile.txt";
   var lobfile = LocalPath + "/" + fileName;
   var file = new File( lobfile );
   var loopNum = 100;
   var content = null;
   for( var i = 0; i < loopNum; ++i )
   {
      content = content + i + "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
   }
   file.write( content );
   file.close();

   return lobfile;
}


/******************************************************************************
*@Description : the function of write lob to the collection with db 
                向集合cl中插入lobNum条lobfile大对象
******************************************************************************/
function toolPutLobs ( cl, lobfile, lobNum )     
{
   try
   {
      var OID = [];
      for( var i = 0; i < lobNum; i++ )
      {
         OID[i] = cl.putLob( lobfile );     // putLob上传大对象成功后返回其OID
      }
      return OID;
   }
   catch( e )
   {
      println( "fail to put lobs, rc = " + e );
      throw e;
   }
}

/******************************************************************************
*@Description : the function of check lob in the collection 
                检查集合中的lob数和oid值是否正确
******************************************************************************/
function toolCheckLob ( cl, lobnum, OID )
{
   var lobs = cl.listLobs( new SdbQueryOption().sort( { Oid: 1 } ) ).toArray();
   var num = lobs.length;
   if( num != lobnum )
      throw ( ">fail to check lob num,num = " + num + ",lobnum = " + lobnum );
   for( var i = 0; i < num; ++i )
   {
      var obj = JSON.parse( lobs[i] );
      var oid = obj["Oid"]["$oid"];
      if( oid != OID[i] )
         throw ( ">fail to check lob oid,oid = " + oid + ",OID = " + OID[i] );
   }
   println( ">success to check lobnum and oid" );
}

/******************************************************************************
*@Description : the function of get ErrNumber from e
                从返回的错误码中获得真实的错误参数，错误码转换
******************************************************************************/
function getErr ( e )
{
   var errMessage = cmd.run( "grep 'shell rc: " + e + "' sdblobtool.log |tail -n 1|cut -d , -f1|cut -d ' ' -f4" ).split( "\n" );
   return errMessage[0];
}
