/******************************************************************************
*@Description : test js object oma function: start
*               TestLink: 10623 Oma启动本地cm
*                         10624 Oma启动本地cm，alivetime设置为0
*@author      : Liang XueWang
******************************************************************************/
import( "../jsObjectSync/commlib.js" );
function testStandaloneCM ( svcname )
{
   // option为port:svcname standalone:true alivetime:10时启动cm
   var option = {};
   option["port"] = svcname;
   option["alivetime"] = 10;
   option["standalone"] = true;
   try
   {
      Oma.start( option );
      println( "start standalone cm: " + svcname + " time: " + getCurrentTime() );
   }
   catch( e )
   {
      throw buildException( "testStandaloneCM", e, "start 10s standalone cm", 0, e );
   }

   var begin = new Date().getTime();
   var end;

   // 检查cm是否启动
   var oma;
   try
   {
      oma = new Oma( COORDHOSTNAME, svcname );
      oma.close();
   }
   catch( e )
   {
      throw buildException( "testStandaloneCM", null,
         "check standalone cm started", 0, e );
   }

   // 检查cm超时
   var arr = Sdbtool.listNodes( { type: "cm", showalone: true } ).toArray();
   while( arr.length !== 0 )
   {
      sleep( 1000 );
      arr = Sdbtool.listNodes( { type: "cm", showalone: true } ).toArray();
      end = new Date().getTime();
      if( end - begin > 60 * 1000 ) break;
   }
   if( arr.length !== 0 )
   {
      println( "list cm: " + arr );
      throw buildException( "testStandaloneCM", null, "10s standalone cm don't exit after 60s",
         10, ( end - begin ) / 1000 );
   }
}

function testCM ( svcname )
{
   // option为port:svcname standalone:false alivetime:0时启动cm
   var option = {};
   option["port"] = svcname;
   option["alivetime"] = 0;
   option["standalone"] = false;

   try
   {
      Oma.start( option );   // 启动的cm为当前用户所有
      println( "start cm: " + svcname + " time: " + getCurrentTime() );
   }
   catch( e )
   {
      throw buildException( "testCM", e, "start cm", 0, e );
   }

   var begin = new Date().getTime();
   var end;

   // 检查cm是否启动
   var oma;
   try
   {
      oma = new Oma( COORDHOSTNAME, svcname );
      oma.close();
   }
   catch( e )
   {
      throw buildException( "testCM", null,
         "check cm started", 0, e );
   }


   // 检查cm不超时
   var arr = Sdbtool.listNodes( { type: "cm" } ).toArray();
   while( arr.length === 2 )
   {
      sleep( 1000 );
      arr = Sdbtool.listNodes( { type: "cm" } ).toArray();
      end = new Date().getTime();
      if( end - begin > 60 * 1000 ) break;
   }
   if( arr.length !== 2 )
   {
      println( "list cm: " + arr );
      throw buildException( "testCM", null, "alivetime=0 cm exits after 60s",
         2, arr.length );
   }
}

/******************************************************************************
 *@Description : test function start
 *               测试oma对象启动cm（静态方法，不具备远程能力）
 *@author      : Liang XueWang
 ******************************************************************************/
function main ()
{
   // 获取空闲端口
   var svcname = toolGetIdleSvcName( COORDHOSTNAME, CMSVCNAME );
   if( svcname === undefined )
   {
      println( "No idle svcname between RSRVPORTBEGIN and RSRVPORTEND" );
      return;
   }

   try
   {
      // 先停止当前的cm
      var InstallPath = commGetInstallPath();
      var cmd = new Cmd();
      cmd.run( InstallPath + "/bin/sdbcmtop" );

      // 测试启动超时时间不为0的独立模式cm并等待其超时
      testStandaloneCM( svcname );

      // 测试启动超时时间为0的cm并检查其不超时
      testCM( svcname );
   }
   catch( e )
   {
      throw e;
   }
   finally
   {
      // 测试完成后，恢复原有的cm端口
      cmd.run( InstallPath + "/bin/sdbcmtop --I" );
      cmd.run( InstallPath + "/bin/sdbcmart" );
      println( "start cm in the end: " + CMSVCNAME + " time: " + getCurrentTime() );
   }
}

//main();
