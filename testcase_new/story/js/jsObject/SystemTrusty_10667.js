/******************************************************************************
*@Description : test js object System function: buildTrusty removeTrusty
*               TestLink : 10667 System对象设置、解除信赖关系
*@author      : Liang XueWang
******************************************************************************/
function main()
{
   // 获取本地主机和远程主机
   var localhost = toolGetLocalhost() ;
	localhost = localhost["hostname"] ;
   var remotehost = toolGetRemotehost() ;
	remotehost = remotehost["hostname"] ;
   if( remotehost === localhost )
   {
      println( "The cluster has only a host.") ;
      return ;
   }
   
   var remote = new Remote( remotehost, CMSVCNAME ) ;
   var system = remote.getSystem() ;
   
   // 设置信赖关系
   try
   {
      // 手工验证信赖关系的建立和解除,ssh时是否需要输入密码
      system.buildTrusty() ;
      println( ">success to build trusty" ) ;    
      system.removeTrusty() ;
      println( ">success to remove trusty" ) ;
   }
   catch( e )
   {
      throw buildException( "main", e, 
            "test build and remove trusty " + localhost + " " + remotehost, 0, e ) ;
   }
   remote.close() ;
}

main()
