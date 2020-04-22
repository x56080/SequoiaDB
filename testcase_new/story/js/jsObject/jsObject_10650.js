/******************************************************************************
*@Description: seqDB-10650:System对象添加、删除用户
*@author: Zhao Xiaoni
******************************************************************************/
function test()
{
   for( var i = 0; i < systems.length; i++ )
   {
      systems[i].addDelUser( true );
      systems[i].addDelUser( false );
   }
}

//创建用户删除用户，createDir取值为true/false
SystemTest.prototype.addDelUser = function( createDir )
{
   this.init();

   // 检查用户是否有权限
   var user = this.system.getCurrentUser().toObj()["user"];
   if( user !== "root" )
   {
      println( "user is not root, can't add del user" );
      this.release();
      return;
   }

   // 检查用户组tmpGroup testGroup和用户ceateUser是否存在，若存在则删除
   isUserExist( this.hostname, this.svcname, "createUser", this.system, true );
   isGroupExist( this.hostname, this.svcname, "tmpGroup", this.system, true );
   isGroupExist( this.hostname, this.svcname, "testGroup", this.system, true );

   try
   {
      // 创建用户组tmpGroup testGroup
      this.system.addGroup( { name: "tmpGroup" } );
      this.system.addGroup( { name: "testGroup" } );

      var userObj = {};
      userObj["name"] = "createUser";          // 用户名
      userObj["passwd"] = "createUser";        // 用户密码
      userObj["group"] = "tmpGroup";           // 用户组
      userObj["Group"] = "testGroup";          // 附加用户组
      userObj["dir"] = "/home/createUser";     // 用户主目录
      userObj["createDir"] = createDir;        // 是否自动创建用户主目录

      // 创建用户
      this.system.addUser( userObj );

      // 检查用户
      checkUser( this.cmd, userObj.name );

      // 检查用户组和附加用户组
      checkGroup( this.cmd, userObj.group, userObj.name );
      checkGroup( this.cmd, userObj.Group, userObj.name );

      // 检查用户主目录
      checkDir( this.cmd, userObj.dir, createDir );
   }
   finally
   {
      //删除用户和用户组
      isUserExist( this.hostname, this.svcname, "createUser", this.system, true );
      isGroupExist( this.hostname, this.svcname, "tmpGroup", this.system, true );
      isGroupExist( this.hostname, this.svcname, "testGroup", this.system, true );
   }

   this.release();
}

main( test );
