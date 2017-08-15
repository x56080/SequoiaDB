/******************************************************************************
*@Description : test js object oma function: createOM removeOM
*               TestLink: 10612 Oma创建、删除sdbom服务进程
*                         10613 Oma创建sdbom服务进程，服务进程已存在
*@author      : Liang XueWang
******************************************************************************/

// 测试创建删除om
OmaTest.prototype.testOMOperation = function( svcname, isOmExist )
{
   this.testInit() ;
   var dbpath = RSRVNODEDIR + svcname ;
   if( isOmExist )
   {
      try
      {
         this.oma.createOM( svcname, dbpath ) ;
         throw "create om when om exist should be failed" ;
      }
      catch( e )
      {
         if( e !== -145 )
         {
            println( "svcname = " + svcname + " dbpath = " + dbpath ) ;
            throw buildException( "testOMOperation", e, 
                                  "createOm when om exist " + this, -145, e ) ;
         }
      }
   }
   else
   {
      try
      {
         this.oma.createOM( svcname, dbpath ) ;
      }
      catch( e )
      {
         println( "svcname = " + svcname + " dbpath = " + dbpath ) ;
         throw buildException( "testOMOperation", e, 
                               "createOm when om not exist " + this, 0, e ) ;
      }
      this.oma.removeOM( svcname ) ;
   }
   this.oma.close() ;
}

function main()
{
   // 获取本地主机和远程主机
   var localhost = toolGetLocalhost() ;
   var remotehost = toolGetRemotehost() ;
   
   // 获取本地和远程空闲的端口号
   var svcname1 = toolGetIdleSvcName( localhost["hostname"], CMSVCNAME ) ;
   if( svcname1 === undefined )
   {
      println( "No idle svcname between RSRVPORTBEGIN and RSRVPORTEND local" ) ;
      return ;
   }
   var svcname2 = toolGetIdleSvcName( remotehost["hostname"], CMSVCNAME ) ;
   if( svcname2 === undefined )
   {
      println( "No idle svcname between RSRVPORTBEGIN and RSRVPORTEND remote" ) ;
      return ;
   }
   
   // 判断OM是否存在
   var OmExist1 = isOmExist( localhost["hostname"], CMSVCNAME ) ;
   var OmExist2 = isOmExist( remotehost["hostname"], CMSVCNAME ) ;
   
   var localOma = new OmaTest( localhost, CMSVCNAME ) ;
   var remoteOma = new OmaTest( remotehost, CMSVCNAME ) ;
   
   var omas = [ localOma, remoteOma ] ;
   var svcnames = [ svcname1, svcname2 ] ;
   var OmExists = [ OmExist1, OmExist2 ] ;
   
   for( var i = 0;i < omas.length;i++ )
   {
      // 测试OM的正常操作
      omas[i].testOMOperation( svcnames[i], OmExists[i] ) ;
   }
}

main()
