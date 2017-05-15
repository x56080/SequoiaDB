/******************************************************************************
*@Description : test js object File function: chmod chown chgrp
*               TestLink : 10817 更改File对象的权限
*                          10818 更改File对象的所有者和所属组
*                          10819 更改File对象的用户组
*@auhor       : Liang XueWang
******************************************************************************/

// 测试更改文件权限
FileTest.prototype.testChmod = function()
{
   this.init() ;
   
   var tmpFilename = "/tmp/testMod.txt" ;
   var tmpFile ;   
   if( this.isLocal )
      tmpFile = new File( tmpFilename ) ;
   else
      tmpFile = this.remote.getFile( tmpFilename ) ;
   this.file.chmod( tmpFilename, 0755 ) ;    // 更改权限
   var command = "ls -l " + tmpFilename + " | awk '{print $1}'" ;
   var tmp = this.cmd.run( command ).split( "\n" ) ;
   var mode = tmp[tmp.length-2] ;
   mode = mode.slice( 0, 10 ) ;
   this.cmd.run( "rm -rf " + tmpFilename ) ;
   if( mode !== "-rwxr-xr-x" )
   {
      throw buildException( "testChmod", null, "check mode " + this, "-rwxr-xr-x", mode ) ;
   }
   
   this.release() ; 
}

// 测试更改文件所有者和所属组
FileTest.prototype.testChown = function()
{  
   this.init() ;
   
   // 检查当前用户和cm用户是否有权限
   var currUser = toolGetCurrentUser( this.hostname, this.svcname ) ;
   var cmUser = toolGetSdbcmUser( this.hostname, this.svcname ) ;
   if( this.isLocal )
   {
      if( currUser !== "root" )
      {
         this.release() ;
         return ;
      }
   }
   else if( cmUser !== "root" )
   {
      this.release() ;
      return ;
   }

   var tmpFilename = "/tmp/testOwn.txt" ;
   var tmpFile ;   
   if( this.isLocal )
      tmpFile = new File( tmpFilename ) ;
   else
      tmpFile = this.remote.getFile( tmpFilename ) ;
   this.file.chown( tmpFilename, { username: "root", groupname: "root" } ) ;
   var command = "ls -l " + tmpFilename + " | awk '{print $3,$4}'" ;
   var tmp = this.cmd.run( command ).split( "\n" ) ;
   var owner = tmp[tmp.length-2] ;
   this.cmd.run( "rm -rf " + tmpFilename ) ;
   if( owner !== "root root" )
   {
      throw buildException( "testChown", null, "check owner " + this, "root", owner ) ;
   }
   
   this.release() ; 
}

// 测试更改文件的用户组
FileTest.prototype.testChgrp = function()
{
   this.init() ;
   
   // 检查当前用户和cm用户是否有权限
   var currUser = toolGetCurrentUser( this.hostname, this.svcname ) ;
   var cmUser = toolGetSdbcmUser( this.hostname, this.svcname ) ;
   if( this.isLocal )
   {
      if( currUser !== "root" )
      {
         this.release() ;
         return ;
      }
   }
   else if( cmUser !== "root" )
   {
      this.release() ;
      return ;
   }
   
   var tmpFilename = "/tmp/testGrp.txt" ;
   var tmpFile ;   
   if( this.isLocal )
      tmpFile = new File( tmpFilename ) ;
   else
      tmpFile = this.remote.getFile( tmpFilename ) ;
   this.file.chgrp( tmpFilename, "root" ) ;   // 更改文件用户组
   var command = "ls -l " + tmpFilename + " | awk '{print $4}'"
   var tmp = this.cmd.run( command ).split( "\n" ) ;
   var group = tmp[tmp.length-2] ;
   this.cmd.run( "rm -rf " + tmpFilename ) ;
   if( group !== "root" )
   {
      throw buildException( "testChgrp", null, "check group " + this, "root", group ) ;
   }
   
   this.release() ;
}

function main()
{
   // 获取本地主机和远程主机
   var localhost = toolGetLocalhost() ;
   var remotehost = toolGetRemotehost() ;
   
   var filename = "/tmp/testFileMode10817.txt" ;
   var ft1 = new FileTest( localhost, CMSVCNAME ) ;     // 本地File类类型
   var ft2 = new FileTest( localhost, CMSVCNAME, filename ) ;  // 本地file对象
   var ft3 = new FileTest( remotehost, CMSVCNAME ) ;    // 远程File类类型
   var ft4 = new FileTest( remotehost, CMSVCNAME, filename ) ;  // 远程file对象
   
   var fts = [ ft1, ft2, ft3, ft4 ] ;
   
   for( var i = 0; i < fts.length;i++ )
   {
      // 测试更改文件权限
      fts[i].testChmod() ;
      
      // 测试更改文件所有者和所属组
      fts[i].testChown() ;
      
      // 测试更改文件用户组
      fts[i].testChgrp() ;
   }
}

main()