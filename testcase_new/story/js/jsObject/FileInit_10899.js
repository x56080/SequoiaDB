/******************************************************************************
*@Description : test js object File function: init file with mode
*               TestLink : 10899 创建file对象时指定权限
*                          10910 远程文件对象初始化时文件名参数非法
*@auhor       : Liang XueWang
******************************************************************************/

// 测试本地文件对象初始化时指定权限
function testInitLocal ()
{
   var filename = "/tmp/testInitLocal.txt";
   var modeNumber = [0755, 0x1ED, 493];
   var modeString = "-rwxr-xr-x";
   var command = "ls -al " + filename + " | awk '{print $1}'";
   var cmd = new Cmd();
   cmd.run( "rm -rf " + filename );

   for( var i = 0; i < modeNumber.length; i++ )
   {
      var localfile = new File( filename, modeNumber[i] );
      var tmp = cmd.run( command ).split( "\n" );
      var filemode = tmp[tmp.length - 2].slice( 0, 10 );
      if( filemode !== modeString )
      {
         throw buildException( "testInitLocal", null, "file " + filename,
            modeString, filemode );
      }
      cmd.run( "rm -rf " + filename );
   }
}

// 测试远程文件初始化指定权限
function testInitRemote ()
{
   var remotehost = toolGetRemotehost();
   var remote = new Remote( remotehost["hostname"], CMSVCNAME );
   var cmd = remote.getCmd();

   var filename = "/tmp/testInitRemote.txt";
   var modeNumber = [0755, 0x1ED, 493];
   var modeString = "-rwxr-xr-x";
   var command = "ls -al " + filename + " | awk '{print $1}'";
   cmd.run( "rm -rf " + filename );
   for( var i = 0; i < modeNumber.length; i++ )
   {
      var remotefile = remote.getFile( filename, modeNumber[i] );
      var tmp = cmd.run( command ).split( "\n" );
      var filemode = tmp[tmp.length - 2].slice( 0, 10 );
      if( filemode !== modeString )
      {
         throw buildException( "testInitRemote", null,
            "file " + filename + " hostname: " + remotehost, modeString, filemode );
      }
      cmd.run( "rm -rf " + filename );
   }
   remote.close();
}

// 测试远程文件初始化时文件名参数非法
function testInitRemoteAbnormal ()
{
   var remotehost = toolGetRemotehost();
   var remote = new Remote( remotehost["hostname"], CMSVCNAME );

   var errFilename = [undefined, 123, ""];
   var errno = [-6, -6, -4];
   for( var i = 0; i < errFilename.length; i++ )
   {
      try
      {
         remote.getFile( errFilename[i] );
         throw "get remote file " + errFilename[i] + " should be failed";
      }
      catch( e )
      {
         if( e !== errno[i] )
         {
            throw buildException( "testInitRemoteAbnormal", e,
               "file " + errFilename[i] + " host " + remotehost, errno[i], e );
         }
      }
   }
   remote.close();
}

// 测试本地文件初始化为无权限文件
function testInitLocalNoPermission ()
{
   var cmd = new Cmd();
   var user = cmd.run( "whoami" ).split( "\n" )[0];
   if( user === "root" )
   {
      println( "local user is root" );
      return;
   }
   try
   {
      var file = new File( "/etc/passwd" );
      throw 0;
   }
   catch( e )
   {
      if( e !== -3 )
      {
         throw buildException( "testInitLocalNoPermission", e,
            "test init local file /etc/passwd", -3, e );
      }
   }
}

// 测试远程文件初始化为无权限文件
function testInitRemoteNoPermission ()
{
   var remotehost = toolGetRemotehost();
   var remote = new Remote( remotehost["hostname"], CMSVCNAME );
   var cmd = remote.getCmd();
   var user = cmd.run( "whoami" ).split( "\n" )[0];
   if( user === "root" )
   {
      println( "remote user is root" );
      return;
   }
   try
   {
      var file = remote.getFile( "/etc/passwd" );
      throw 0;
   }
   catch( e )
   {
      if( e !== -3 )
      {
         throw buildException( "testInitRemoteNoPermission", e,
            "test init remote file /etc/passwd", -3, e );
      }
   }
}

function main ()
{
   // 测试本地文件初始化指定权限
   testInitLocal();
   // 测试远程文件初始化指定权限
   testInitRemote();
   // 测试远程文件初始化时文件名参数非法
   testInitRemoteAbnormal();
   // 测试本地文件初始化无权限
   testInitLocalNoPermission();
   // 测试远程文件初始化无权限
   testInitRemoteNoPermission();
}

main()
