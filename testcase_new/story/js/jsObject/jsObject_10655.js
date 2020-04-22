/******************************************************************************
*@Description: seqDB-10655:System对象设置用户信息
*@author: Zhao Xiaoni
******************************************************************************/
function test()
{
   for( var i = 0; i < systems.length; i++ )
   {
      systems[i].setUserConfigs();
   }
}

SystemTest.prototype.setUserConfigs = function()
{
   this.init();

   // 检查当前用户是否有权限
   var user = this.system.getCurrentUser().toObj()["user"];
   if( user !== "root" )
   {
      println( "user is not root, can't add del user" );
      this.release();
      return;
   }

   isUserExist( this.hostname, this.svcname, "modifyUser", this.system, true );
   isGroupExist( this.hostname, this.svcname, "tmpGroup", this.system, true );
   isGroupExist( this.hostname, this.svcname, "testGroup", this.system, true );
   isGroupExist( this.hostname, this.svcname, "modifyUser", this.system, true );  
 
   try
   {
      //创建用户
      var userObj = {};
      userObj["name"] = "modifyUser";
      userObj["dir"] = "/tmp/modifyUser";
      userObj["createDir"] = true;
      this.system.addUser( userObj );

      //默认用户组和用户名相同
      checkGroup( this.cmd, userObj.name, userObj.name );

      // 创建用户组tmpGroup testGroup
      this.system.addGroup( { name: "tmpGroup" } );
      this.system.addGroup( { name: "testGroup" } );

      // 设置用户属性
      var option = {};
      option["name"] = "modifyUser";
      option["group"] = "tmpGroup";
      option["Group"] = "testGroup";
      option["isAppend"] = true;
      option["dir"] = "/home/modifyUser";
      option["isMove"] = true;
      this.system.setUserConfigs( option );

      //检查用户组和附加用户组
      checkGroup( this.cmd, option.group, option.name );
      checkGroup( this.cmd, option.Group, option.name );

      // 检查用户主目录
      checkDir( this.cmd, option.dir, true );
   }
   finally
   {
      //检查用户和组存在，并删除
      isUserExist( this.hostname, this.svcname, "modifyUser", this.system, true );
      isGroupExist( this.hostname, this.svcname, "tmpGroup", this.system, true );
      isGroupExist( this.hostname, this.svcname, "testGroup", this.system, true );
      isGroupExist( this.hostname, this.svcname, "modifyUser", this.system, true );
   }

   this.release();
}

main( test );
