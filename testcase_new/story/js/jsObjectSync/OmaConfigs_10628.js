/******************************************************************************
*@Description : test js object oma function: getOmaConfigFile getOmaConfigs 
*                                            setOmaConfigs 
*               TestLink: 10628 Oma获取Oma配置文件路径
*                         10629 Oma获取Oma配置信息
*                         10630 Oma获取Oma配置信息，文件不存在
*                         10631 Oma获取Oma配置信息，文件不是oma配置文件
*                         10632 Oma设置Oma配置信息 
*@author      : Liang XueWang
******************************************************************************/
// 测试正常获取oma配置文件和配置信息
OmaTest.prototype.testGetOmaConfigsNormal = function()
{
   this.testInit() ;
   var remote = new Remote( this.hostname, this.svcname ) ;
   var cmd = remote.getCmd() ;
   
   // 测试getOmaConfigFile
   var configFile = this.oma.getOmaConfigFile() ;
   var sdbDir = toolGetSequoiadbDir( this.hostname, this.svcname ) ;
   var found = false ;
   for( var i = 0;i < sdbDir.length;i++ )
   {
      var file = sdbDir[i] + "/conf/sdbcm.conf" ;
      if( configFile === file )
      {
         found = true ;
         break ;
      }
   }  
   if( found === false )
   {
      throw buildException( "testGetOmaConfigsNormal", null, 
            "get oma config file " + this, sdbDir, configFile ) ;
   }
   
   // 测试getOmaConfigs
   try
   {   
      var configs = this.oma.getOmaConfigs().toObj() ;
      var command = "cat " + configFile + " | tr -s '\r\n' '\n'" ;
      var configFileContent = cmd.run( command ).split( "\n" ) ;
   }
   catch( e )
   {
      throw buildException( "testGetOmaConfigsNormal", e, "get oma configs " + this, 0, e ) ;
   }
   checkResult( configs, configFileContent, "getOmaConfigs" ) ;

   if( this.oma.close !== undefined )
      this.oma.close() ;
   remote.close() ;  
}

// 测试获取oma配置信息异常
OmaTest.prototype.testGetOmaConfigsAbnormal = function()
{
   this.testInit() ;
   var sdbDir = toolGetSequoiadbDir( this.hostname, this.svcname ) ;
   // 测试getOmaConfigs时，文件不存在
   try
   {
      this.oma.getOmaConfigs( "/opt/notexist" ) ;
      throw 0 ;
   }
   catch( e )
   {
      if( e !== -4 )
      {
         throw buildException( "testGetOmaConfigsAbnormal", e, 
               "get oma configs with not exist file " + this, -4, e ) ;
      }  
   }
   
   // 测试getOmaConfigs时，文件不是oma配置文件
   try
   {
      this.oma.getOmaConfigs( sdbDir[0] + "/bin/sdb" ) ;
      throw 0 ;
   }
   catch( e )
   {
      if( e !== -6 )
      {
         throw buildException( "testGetOmaConfigsAbnormal", e, 
               "get oma configs with sdb file " + this, -6, e ) ;
      }
   }
   
   if( this.oma.close !== undefined )
      this.oma.close() ;
}

// 测试设置oma配置信息
OmaTest.prototype.testSetOmaConfigs = function()
{
   this.testInit() ;
   
   if( this.oma === Oma )
   {
      var user = System.getCurrentUser().user ;
      var file = RSRVNODEDIR + "../conf/sdbcm.conf" ;
      var obj = getFileUsrGrp( file ) ;
      if( user !== obj["user"] && user !== "root" )
      {
         println( "static Oma with current user " + user + " is not fit" ) ;
         return ;
      } 
   }
   
   try
   {
      // 测试setOmaConfigs
      var configs = this.oma.getOmaConfigs().toObj() ;  
      configs[ "name" ] = "lxw" ;
      this.oma.setOmaConfigs( configs ) ;
      var name = this.oma.getOmaConfigs().toObj().name ;
      if( name !== "lxw" )
      {
         throw buildException( "testSetOmaConfigs", null, 
               "check set oma configs " + this, "lxw", name ) ;
      }
   }
   catch( e )
   {
      throw e ;
   }
   finally
   {
      // 测试完成后删除新加的配置
      delete configs[ "name" ] ;
      this.oma.setOmaConfigs( configs ) ;
   }
   
   if( this.oma === Oma )
   {
      if( user !== cmuser )
      {
         File.chown( file, obj ) ;
      }      
   }
   else
   {
      this.oma.close() ;
   }
}

function main()
{
   // 获取本地和远程主机
   var localhost = toolGetLocalhost() ;
   var remotehost = toolGetRemotehost() ;
   
   var localOma = new OmaTest( localhost, CMSVCNAME ) ;
   var remoteOma = new OmaTest( remotehost, CMSVCNAME ) ;
   var staticOma = new OmaTest() ;
   
   var omas = [ localOma, remoteOma, staticOma ] ;
   for( var i = 0;i < omas.length;i++ )
   {
      // 测试正常获取Oma配置文件和配置信息
      omas[i].testGetOmaConfigsNormal() ;
      
      // 测试获取Oma配置信息异常
      omas[i].testGetOmaConfigsAbnormal() ;
      
      // 测试设置Oma配置信息
      omas[i].testSetOmaConfigs() ;
   }
}

main()