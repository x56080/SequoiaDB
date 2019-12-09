/******************************************************************************
*@Description : test js object oma function: getNodeConfigs setNodeConfigs
*                                            updateNodeConfigs
*               TestLink: 10620 Oma获取特定节点的配置信息
*                         10621 Oma设置特定节点的配置信息
*                         10622 Oma更新特定节点的配置信息
*@author      : Liang XueWang
******************************************************************************/

// 测试设置、更新、获取节点配置
OmaTest.prototype.testNodeConfigs = function()
{
   this.testInit();
   var remote = new Remote( this.hostname, this.svcname );
   var cmd = remote.getCmd();

   // 测试getNodeConfigs
   var configs = this.oma.getNodeConfigs( COORDSVCNAME ).toObj();
   var sdbDir = toolGetSequoiadbDir( this.hostname, this.svcname );
   var command = "cat " + sdbDir[0] + "/conf/local/" + COORDSVCNAME + "/sdb.conf";
   try
   {
      var contents = cmd.run( command ).split( "\n" );
   }
   catch( e )
   {
      println( "run command " + command );
      throw buildException( "testNodeConfigs", e, "get node configs " + this, 0, e );
   }
   checkResult( configs, contents, "getNodeConfigs" );

   // 测试setNodeConfigs
   try
   {
      configs["tmpConf"] = "foo";
      this.oma.setNodeConfigs( COORDSVCNAME, configs );
      var tmpConf = this.oma.getNodeConfigs( COORDSVCNAME ).toObj().tmpConf;
      if( tmpConf !== "foo" )
      {
         throw buildException( "testNodeConfigs", null, "set node configs " + this,
            "foo", tmpConf );
      }

      // 测试updateNodeConfigs
      configs["tmpConf"] = "bar";
      this.oma.updateNodeConfigs( COORDSVCNAME, configs );
      tmpConf = this.oma.getNodeConfigs( COORDSVCNAME ).toObj().tmpConf;
      if( tmpConf !== "bar" )
      {
         throw buildException( "testNodeConfigs", null, "update node configs " + this,
            "bar", tmpConf );
      }
   }
   catch( e )
   {
      throw e;
   }
   finally
   {
      // 测试完成后，清除配置文件中的tmpConf
      delete configs.tmpConf;
      this.oma.setNodeConfigs( COORDSVCNAME, configs );
   }

   this.oma.close();
   remote.close();
}


function main ()
{
   // 获取本地和远程主机
   var localhost = toolGetLocalhost();
   var remotehost = toolGetRemotehost();

   var localOma = new OmaTest( localhost, CMSVCNAME );
   var remoteOma = new OmaTest( remotehost, CMSVCNAME );
   var omas = [localOma, remoteOma];

   for( var i = 0; i < omas.length; i++ )
   {
      // 测试获取、设置、更新节点配置
      omas[i].testNodeConfigs();
   }
}

main()