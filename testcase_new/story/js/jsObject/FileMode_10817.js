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

// 测试更改无权限文件的权限
FileTest.prototype.testChmodNoPermission = function()
{
   this.init() ;
    
   var user = this.cmd.run( "whoami" ).split( "\n" )[0] ;
   if( user === "root" )
   {
      println( "user is root, cann't testChmodNoPermission" ) ;
      this.release() ;
      return ;
   }
   try
   {
      this.file.chmod( "/etc/passwd", 0755 ) ;
      throw 0 ;
   }
   catch( e )
   {
      if( e !== 1 )
      {
         throw buildException( "testChmodNoPermission", e,
               "test chmod /etc/passwd", 1, e ) ;
      }
   }
    
   this.release() ;
}

// 测试递归更改目录权限
FileTest.prototype.testChmodRecursive = function()
{
   this.init() ;
   
   var tmpDir = "/tmp/testChmodDir" ;
   this.file.mkdir( tmpDir, 0700 ) ;
   for( var i = 0;i < 5;i++ )
   {
      var tmpFileName = tmpDir + "/testChmodFile" + i ;
      var tmpFile ;
      if( this.isLocal )
         tmpFile = new File( tmpFileName ) ;
      else
         tmpFile = this.remote.getFile( tmpFileName ) ;
      tmpFile.write( "abc" ) ;
      tmpFile.close()
   }
   
   this.file.chmod( tmpDir, 0755, true ) ;
   
   var mode = this.file.stat( tmpDir ).toObj()["mode"].slice( 0, 10 ) ;
   if( mode !== "rwxr-xr-x" )
   {
      throw buildException( "testChmodRecursive", null, 
            "check mode " + this, "rwxr-xr-x", mode ) ;
   }
   for( var i = 0;i < 5;i++ )
   {
      var tmpFileName = tmpDir + "/testChmodFile" + i ;
      var mode = this.file.stat( tmpFileName ).toObj()["mode"].slice( 0, 10 ) ;
      if( mode !== "rwxr-xr-x" )
      {
         throw buildException( "testChmodRecursive", null,
               "check mode " + this, "rwxr-xr-x", mode ) ;
      }
   }
   
   this.cmd.run( "rm -rf " + tmpDir ) ;
   
   this.release() ;
}

// 测试更改文件所有者和所属组
FileTest.prototype.testChown = function()
{  
   this.init() ;
   
   var user = this.cmd.run( "whoami" ).split( "\n" )[0] ;
   var group = this.cmd.run( "id -gn " + user ).split( "\n" )[0] ;

   var tmpFilename = "/tmp/testChown.txt" ;
   var tmpFile ;   
   if( this.isLocal )
      tmpFile = new File( tmpFilename ) ;
   else
      tmpFile = this.remote.getFile( tmpFilename ) ;
   this.file.chown( tmpFilename, { username: user, groupname: group } ) ;
   var command = "ls -l " + tmpFilename + " | awk '{print $3,$4}'" ;
   var tmp = this.cmd.run( command ).split( "\n" ) ;
   var owner = tmp[tmp.length-2] ;
   this.cmd.run( "rm -rf " + tmpFilename ) ;
   if( owner !== user + " " + group )
   {
      throw buildException( "testChown", null, "check owner " + this, 
            user + " " + group, owner ) ;
   }
   
   this.release() ; 
}

// 测试递归更改目录用户
FileTest.prototype.testChownRecursive = function()
{
   this.init() ;
   
   var user = this.system.getCurrentUser().toObj()["user"] ;
   if( user !== "root" )
   {
      println( user + " is not root,cann't testChownRecursive" ) ;
      this.release() ;
      return ;
   }
   var tmpUser = "tmpUser" ;
   var tmpGroup = "tmpGroup" ;
   createUserAndGroup( this, tmpUser, tmpGroup ) ;
   
   var tmpDir = "/tmp/testChownDir" ;
   this.file.mkdir( tmpDir ) ;
   for( var i = 0;i < 5;i++ )
   {
      var tmpFileName = tmpDir + "/testChownFile" + i ;
      var tmpFile ;
      if( this.isLocal )
         tmpFile = new File( tmpFileName ) ;
      else
         tmpFile = this.remote.getFile( tmpFileName ) ;
      tmpFile.write( "abc" ) ;
      tmpFile.close()
   }
   
   this.file.chown( tmpDir, { username: tmpUser }, true ) ;
   
   var user = this.file.stat( tmpDir ).toObj()["user"] ;
   if( user !== tmpUser )
   {
      throw buildException( "testChownRecursive", null, 
            "check owner " + this, tmpUser, user ) ;
   }
   for( var i = 0;i < 5;i++ )
   {
      var tmpFileName = tmpDir + "/testChownFile" + i ;
      var user = this.file.stat( tmpFileName ).toObj()["user"] ;
      if( user !== tmpUser )
      {
         throw buildException( "testChownRecursive", null,
               "check owner " + this, tmpUser, user ) ;
      }
   }
   
   deleteUserAndGroup( this, tmpUser, tmpGroup ) ;
   this.cmd.run( "rm -rf " + tmpDir ) ;
   
   this.release() ;
}

// 测试更改文件的用户组
FileTest.prototype.testChgrp = function()
{
   this.init() ;
   
   // 检查用户是否有权限
   var user = this.cmd.run( "whoami" ).split( "\n" )[0] ;
   var group = this.cmd.run( "id -gn " + user ).split( "\n" )[0] ;
   
   var tmpFilename = "/tmp/testGrp.txt" ;
   var tmpFile ;   
   if( this.isLocal )
      tmpFile = new File( tmpFilename ) ;
   else
      tmpFile = this.remote.getFile( tmpFilename ) ;
   this.file.chgrp( tmpFilename, group ) ;   // 更改文件用户组
   var command = "ls -l " + tmpFilename + " | awk '{print $4}'"
   var tmp = this.cmd.run( command ).split( "\n" ) ;
   var grp = tmp[tmp.length-2] ;
   this.cmd.run( "rm -rf " + tmpFilename ) ;
   if( grp !== group )
   {
      throw buildException( "testChgrp", null, "check group " + this, group, grp ) ;
   }
   
   this.release() ;
}

// 测试递归更改目录用户
FileTest.prototype.testChgrpRecursive = function()
{
   this.init() ;
   
   var user = this.system.getCurrentUser().toObj()["user"] ;
   if( user !== "root" )
   {
      println( user + " is not root,cann't testChgrpRecursive" ) ;
      this.release() ;
      return ;
   }
   var tmpUser = "tmpUser" ;
   var tmpGroup = "tmpGroup" ;
   createUserAndGroup( this, tmpUser, tmpGroup ) ;
   
   var tmpDir = "/tmp/testChgrpDir" ;
   this.file.mkdir( tmpDir ) ;
   for( var i = 0;i < 5;i++ )
   {
      var tmpFileName = tmpDir + "/testChgrpFile" + i ;
      var tmpFile ;
      if( this.isLocal )
         tmpFile = new File( tmpFileName ) ;
      else
         tmpFile = this.remote.getFile( tmpFileName ) ;
      tmpFile.write( "abc" ) ;
      tmpFile.close()
   }
   
   this.file.chgrp( tmpDir, tmpGroup, true ) ;
   
   var group = this.file.stat( tmpDir ).toObj()["group"] ;
   if( group !== tmpGroup )
   {
      throw buildException( "testChgrpRecursive", null, 
            "check group " + this, tmpGroup, group ) ;
   }
   for( var i = 0;i < 5;i++ )
   {
      var tmpFileName = tmpDir + "/testChgrpFile" + i ;
      var group = this.file.stat( tmpFileName ).toObj()["group"] ;
      if( group !== tmpGroup )
      {
         throw buildException( "testChgrpRecursive", null,
               "check group " + this, tmpGroup, group ) ;
      }
   }
   
   deleteUserAndGroup( this, tmpUser, tmpGroup ) ;
   this.cmd.run( "rm -rf " + tmpDir ) ;
   
   this.release() ;
}

function createUserAndGroup( ft, user, group )
{
   try
   {
      if( !isGroupExist( ft.hostname, ft.svcname, "tmpGroup" ) )
         ft.system.addGroup( { "name": group } ) ;
      if( !isUserExist( ft.hostname, ft.svcname, "tmpUser" ) )
         ft.system.addUser( { "name": user, "group": group } ) ;
   }
   catch( e )
   {
      throw buildException( "createUserAndGroup", e,
            "create " + user + " " + group + " " + this,
            0, e ) ;
   }
}

function deleteUserAndGroup( ft, user, group )
{
   try
   {
      ft.cmd.run( "userdel -f " + user ) ;
   }
   catch( e )
   {
      var msg = ft.cmd.getLastOut() ;
      if( msg.indexOf( "logged in" ) === -1 )
      {
         throw buildException( "deleteUserAndGroup", e, 
               "delete user " + user + " " + msg + " " + this, 0, e ) ;
      }
   }
   try
   {
      ft.system.delGroup( group ) ;
   }
   catch( e )
   {
      throw buildException( "deleteUserAndGroup", e,
            "delete group " + group + " " + this,
            0, e ) ;
   }
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
      
      // 测试更改无权限文件的权限
      fts[i].testChmodNoPermission() ;
      
      // 测试递归更改文件目录权限
      fts[i].testChmodRecursive() ;
      
      // 测试更改文件所有者和所属组
      fts[i].testChown() ;
      
      // 测试递归更改文件目录用户和用户组
      fts[i].testChownRecursive() ;
      
      // 测试更改文件用户组
      fts[i].testChgrp() ;
      
      // 测试递归更改文件目录用户组
      fts[i].testChgrpRecursive() ;
   }
}

main()