/******************************************************************************
*@Description : test js object System function: getUserEnv addUser delUser
*               setUserConfigs listLoginUsers listAllUsers getCurrentUser
*               isUserExist
*               TestLink : 10649 System对象获取当前用户环境信息
*                          10650 System对象添加、删除用户
*                          10651 System对象添加用户，已存在用户
*                          10655 System对象设置用户信息
*                          10656 System对象枚举已登录用户
*                          10657 System对象枚举所有用户
*                          10659 System对象获取当前用户信息
*                          10660 System对象判断用户是否存在
*@author      : Liang XueWang
******************************************************************************/

// 测试获取用户环境变量
SystemTest.prototype.testGetUserEnv = function()
{
   this.init() ;
   
   var environ = this.system.getUserEnv().toObj() ;
   var tmpInfo = this.cmd.run( "env" ).split( "\n" ) ;
   var tmpObj = {} ;
   var keys = [] ;
   var values = [] ;
   var j ;
   for( var i = 0;i < tmpInfo.length-1;i++ )
   {
      var index = tmpInfo[i].indexOf( "=" ) ;   
      if( index === -1 )
      {
         values[j] += tmpInfo[i] ;
         continue ;
      }
      keys[i] = tmpInfo[i].slice( 0, index ) ;
      values[i] = tmpInfo[i].slice( index + 1 ) ;
      j = i ;   
   }
   for( i = 0;i < keys.length;i++ )
      tmpObj[keys[i]] = values[i] ;
   
   for( var k in tmpObj )
   {
      if( tmpObj[k] !== environ[k] )
      {
         throw buildException( "testGetUserEnv", null, 
               "check key: " + k + " " + this, tmpObj[k], environ[k] ) ;
      }   
   }
   
   this.release() ;   
}

// 测试创建用户删除用户，createDir取值为true/false
SystemTest.prototype.testAddDelUser = function( createDir )
{
   this.init() ;
   
   // 检查用户是否有权限
   var user = this.system.getCurrentUser().toObj()["user"] ;
   if( user !== "root" )
   {
      println( "user is not root, can't add del user" ) ;
      this.release() ;
      return ;
   }
      
   // 检查用户组tmpGroup testGroup是否存在
   if( isGroupExist( this.hostname, this.svcname, "tmpGroup" ) || 
       isGroupExist( this.hostname, this.svcname, "testGroup" ) ||
       isUserExist( this.hostname, this.svcname, "createUser" ) )
   {
      println( "tmpGroup testGroup or createUser existed" ) ;
      this.release() ;
      return ;
   }
      
   // 创建用户组tmpGroup testGroup
   this.system.addGroup( { name: "tmpGroup" } ) ;
   this.system.addGroup( { name: "testGroup" } ) ;
   
   var userObj = {} ;
   userObj["name"] = "createUser" ;          // 用户名
   userObj["passwd"] = "createUser" ;        // 用户密码
   userObj["group"] = "tmpGroup" ;           // 用户组
   userObj["Group"] = "testGroup" ;          // 附加用户组
   userObj["dir"] = "/home/createUser" ;     // 用户主目录
   userObj["createDir"] = createDir ;        // 是否自动创建用户主目录
   
   try
   {
      // 创建用户
      this.system.addUser( userObj ) ;
   }
   catch( e )
   {
      throw buildException( "testAddDelUser", e, 
                            "add user: " + userObj.name + " " + this, 0, e ) ;
   }
   
   // 检查用户
   checkUser( this.cmd, userObj.name ) ;
   
   // 检查用户组和附加用户组
   checkGroup( this.cmd, userObj.group, userObj.name ) ;
   checkGroup( this.cmd, userObj.Group, userObj.name ) ;
   
   // 检查用户主目录
   checkDir( this.cmd, userObj.dir, createDir ) ;
   
   // 测试完成后，删除用户用户组
   var option = {} ;
   option["name"] = userObj.name ;
   option["isRemoveDir"] = createDir ;
   this.system.delUser( option ) ;
   if( isUserExist( this.hostname, this.svcname, userObj.name ) )
   {
      throw buildException( "testAddDelUser", null, "check user after del",
            userObj.name + " should be deleted", "not deleted" ) ;
   }
   checkDir( this.cmd, userObj.dir, false ) ;
   
   this.system.delGroup( "tmpGroup" ) ;
   this.system.delGroup( "testGroup" ) ;
   if( isGroupExist( this.hostname, this.svcname, "tmpGroup" ) || 
       isGroupExist( this.hostname, this.svcname, "testGroup" ) )
   {
      throw buildException( "testAddDelUser", null, "check del group in the end",
            "tmpGroup testGroup should be deleted", "not delete" ) ;
   }
       
   this.release() ;
}

// 测试创建已存在用户 root
SystemTest.prototype.testAddExistUser = function()
{
   this.init() ;
   
   // 检查用户是否有权限
   var user = this.system.getCurrentUser().toObj()["user"] ;
   if( user !== "root" )
   {
      this.release() ;
      return ;
   }
   
   try
   {
      this.system.addUser( { name: "root" } ) ;
      throw "create user root should be failed" ;
   }
   catch( e )
   {
      if( e !== 9 )
      {
         throw buildException( "testAddExistUser", e, "add exist user " + this, 9, e ) ;
      }   
   }
   this.release() ;
}

// 测试设置用户属性
SystemTest.prototype.testSetUserConfigs = function()
{
   this.init() ;
   
   // 检查当前用户和cm用户是否有权限
   var user = this.system.getCurrentUser().toObj()["user"] ;
   if( user !== "root" )
   {
      this.release() ;
      return ;
   }
   
   if( isGroupExist( this.hostname, this.svcname, "tmpGroup" ) ||
       isGroupExist( this.hostname, this.svcname, "testGroup" ) ||
       isUserExist( this.hostname, this.svcname, "modifyUser" ) )
      return ;
   // 创建用户组tmpGroup testGroup
   this.system.addGroup( { name: "tmpGroup" } ) ;
   this.system.addGroup( { name: "testGroup" } ) ;
   
   // 首先创建用户
   var userObj = {} ;
   userObj["name"] = "modifyUser" ;
   userObj["passwd"] = "modifyUser" ;
   userObj["dir"] = "/tmp/modifyUser" ;
   userObj["createDir"] = true ;
   this.system.addUser( userObj ) ;
   
   // 设置用户属性
   var option = {} ;
   option["name"] = "modifyUser" ;
   option["group"] = "tmpGroup" ;
   option["Group"] = "testGroup" ;
   option["isAppend"] = true ;
   option["dir"] = "/home/modifyUser" ;
   option["isMove"] = true ;
   
   try
   {
      this.system.setUserConfigs( option ) ;
   }
   catch( e )
   {
      throw buildException( "testSetUserConfigs", e, 
            "set user: " + option["name"] + " config " + this, 0, e ) ;
   }
   
   // 检查用户组和附加用户组
   checkGroup( this.cmd, option.group, option.name ) ;
   checkGroup( this.cmd, option.Group, option.name ) ;
      
   // 检查用户主目录
   checkDir( this.cmd, option.dir, true ) ;
   
   // 测试完成后，删除用户
   var option = {} ;
   option["name"] = userObj.name ;
   option["isRemoveDir"] = true ;
   this.system.delUser( option ) ;
   if( isUserExist( this.hostname, this.svcname, userObj.name ) )
   {
      throw buildException( "testSetUserConfigs", null, "check user after del",
            userObj.name + " should be deled", "not deled" ) ;
   }
   
   this.system.delGroup( "tmpGroup" ) ;
   this.system.delGroup( "testGroup" ) ;
   if( isGroupExist( this.hostname, this.svcname, "tmpGroup" ) || 
       isGroupExist( this.hostname, this.svcname, "testGroup" ) )
   {
      throw buildException( "testSetUserConfigs", null, "check del group in the end",
            "tmpGroup testGroup should be deleted", "not delete" ) ;
   }
   
   this.release() ;
}

// 测试枚举登录用户
SystemTest.prototype.testListLoginUsers = function()
{
   this.init() ;
   var users = this.system.listLoginUsers( { detail: true } ).toArray() ;
   var info = this.cmd.run( "who | sed 's/  */ /g'" ).split( "\n" ) ;
   for( var i = 0;i < users.length;i++ )
   {
      var userObj = JSON.parse( users[i] ) ;
      var tmp = info[i].split( " " ) ;
      var len = tmp.length ;
      var username = tmp[0] ;             // 用户名
      var tty = tmp[1] ;                  // 登录终端
      var time = tmp[2] ;                 // 登录时间
      for( var j = 3;j < len;j++ )
      {
         if( tmp[j].indexOf( "(" ) !== -1 )
            break ;
         time += " " + tmp[j] ;   
      }
      var addr ;                          // 登录的主机名或者ip
      if( tmp[j] === undefined )
      {
         addr = "" ;
      }
      else
      {
         addr = tmp[j].slice( 1, tmp[j].length-1 ) ;
      }  
      if( username !== userObj.user || tty !== userObj.tty ||
          time !== userObj.time || addr !== userObj.from )
      {
         throw buildException( "testListLoginUsers", null, 
               "check info " + this, tmp, JSON.stringify( userObj ) ) ;
      }
   }
   this.release() ;
}

// 测试枚举所有用户
SystemTest.prototype.testListAllUsers = function()
{
   this.init() ;
   var users = this.system.listAllUsers( { detail: true } ).toArray() ;
   var info = this.cmd.run( "cat /etc/passwd" ).split( "\n" ) ;
   for( var i = 0;i < users.length;i++ )
   {
      var userObj = JSON.parse( users[i] ) ;
      var tmp = info[i].split( ":" ) ;
      var username = tmp[0] ;    // 用户名
      var groupid = tmp[3] ;     // 用户组id
      var dir = tmp[5] ;         // 用户主目录
      if( username !== userObj.user || groupid !== userObj.gid || 
          dir !== userObj.dir )
      {
         throw buildException( "testListAllUsers", null, 
               "check info " + this, tmp, JSON.stringify( userObj) ) ;
      }   
   }
   this.release() ;
}

// 测试获取当前用户信息
SystemTest.prototype.testGetCurrentUser = function()
{
   this.init() ;
   var userObj = this.system.getCurrentUser().toObj() ;
   var name = this.cmd.run( "whoami 2>/dev/null" ).split( "\n" )[0] ;
   var gid = this.cmd.run( "id -g " + name + " 2>/dev/null" ).split( "\n" )[0] ;
   var tmp = this.cmd.run( "echo ~" + name ).split( "\n" ) ;
   var dir = tmp[tmp.length-2] ;
   if( name !== userObj.user || gid !== userObj.gid ||
       dir !== userObj.dir )
   {
      throw buildException( "testGetCurrentUser", null,
            "check current user " + this, name + " " + gid + " " + dir, JSON.stringify( userObj ) ) ;
   }
   this.release() ; 
}

// 测试判断用户是否存在
SystemTest.prototype.testIsUserExist = function()
{
   this.init() ;
   
   var result = this.system.isUserExist( "root" ) ;
   if( result !== true )
   {
      throw buildException( "testIsUserExist", null, "test user root " + this, 
                            true, result ) ;
   }
   result = this.system.isUserExist( "!@#$%" ) ;
   if( result !== false )
   {
      throw buildException( "testIsUserExist", null, "test user !@#$% " + this, 
                            false, result ) ;
   }
   
   this.release() ;  
}


/******************************************************************************
*@Description : check user name  in /etc/passwd
*@author      : Liang XueWang            
******************************************************************************/
function checkUser( cmd, username )
{
   var names = cmd.run( "cat /etc/passwd | grep -w " + username + 
                       " | awk -F : '{print $1}'").split( "\n" ) ;
   if( names.indexOf( username ) === -1 )
   {
      throw buildException( "checkUser", null, "check user name", username, name ) ;   
   } 
}

/******************************************************************************
*@Description : check group and Group of user with id command
*@author      : Liang XueWang            
******************************************************************************/
function checkGroup( cmd, groupname, username )
{
   var info = cmd.run( "id " + username ).split( "\n" )[0] ; 
   if( info.indexOf( groupname ) === -1 )
   {
      throw buildException( "checkGroup", null, "check group: " + groupname + 
                            " of user: " + username, groupname, info ) ;
   }
}

/******************************************************************************
*@Description : check user dir
*@author      : Liang XueWang            
******************************************************************************/
function checkDir( cmd, dir, createDir )
{
   try
   {
      cmd.run( "ls -al " + dir ) ;
      if( !createDir )
        throw buildException( "checkDir", null, "check user without dir", 2, 0 ) ;
   }
   catch( e )
   {
      if( !createDir && e === 2 )
         ;
      else
         throw buildException( "checkDir", null, "check user dir", 0, e ) ;
   }
}

function main()
{
   // 获取本地主机和远程主机
   var localhost = toolGetLocalhost() ;
   var remotehost = toolGetRemotehost() ;
   
   var localSystem = new SystemTest( localhost, CMSVCNAME ) ;
   var remoteSystem = new SystemTest( remotehost, CMSVCNAME ) ;
   var systems = [ localSystem, remoteSystem ] ;
   
   for( var i = 0;i < systems.length;i++ )
   {
      // 测试获取用户环境
      systems[i].testGetUserEnv() ;
      
      // 测试创建删除用户，自动创建删除用户主目录
      systems[i].testAddDelUser( true ) ;
      
      // 测试创建删除用户，不创建删除用户主目录
      systems[i].testAddDelUser( false ) ;
      
      // 测试创建已存在用户
      systems[i].testAddExistUser() ;
      
      // 测试修改用户属性
      systems[i].testSetUserConfigs() ;
      
      // 测试枚举登录用户
      systems[i].testListLoginUsers() ;
      
      // 测试枚举所有用户
      systems[i].testListAllUsers() ;
      
      // 测试获取当前用户信息
      systems[i].testGetCurrentUser() ;
      
      // 测试判断用户是否存在
      systems[i].testIsUserExist() ;
   }
}
   
main()