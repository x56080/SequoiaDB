/******************************************************************************
*@Description : seqDB-10623:Oma启动本地cm
                seqDB-10624:Oma启动本地cm，alivetime设置为0 
*@author      : Zhao Xiaoni
******************************************************************************/
main( test );

function test ()
{
   // 获取集群所有机器的主机名
   var hostNames = [];
   var cursor = db.snapshot( SDB_SNAP_DATABASE, { RawData: true } );
   while( cursor.next() )
   {
      var obj = cursor.current().toObj();
      var hostName1 = obj["HostName"];
      if( hostNames.indexOf( hostName1 ) == -1 )
      {
         hostNames.push( hostName1 );
      }
   }
   cursor.close();
   // 获取当前机器的主机名
   var hostName2 = System.getHostName();

   // 集群中不包含当前机器的主机名跳过测试
   if( hostNames.indexOf( hostName2 ) == -1 )
   {
      return;
   }

   var svcName = toolGetIdleSvcName( hostName2, CMSVCNAME );
   if( svcName === undefined )
   {
      return;
   }

   // 先停止当前的cm
   var InstallPath = commGetInstallPath();
   var cmd = new Cmd();
   cmd.run( InstallPath + "/bin/sdbcmtop" );

   try
   {
      //测试启动超时时间为10的cm
      var options = { "port": svcName, "alivetime": 10, "standalone": true };
      Oma.start( options );

      // 检查cm是否启动
      var oma = new Oma( hostName2, svcName );
      oma.close();

      sleep( 20 * 1000 );
      var nodeArray = Sdbtool.listNodes( { type: "cm", showalone: true } ).toArray();
      if( nodeArray.length != 0 )
      {
         throw new Error( "nodeArray: " + nodeArray );
      }

      // 测试启动超时时间为0的cm
      options = { "port": svcName, "alivetime": 0, "standalone": false };
      Oma.start( options );

      oma = new Oma( hostName2, svcName );
      oma.close();

      sleep( 20 * 1000 );
      nodeArray = Sdbtool.listNodes( { type: "cm" } ).toArray();
      if( nodeArray.length !== 2 )
      {
         throw new Error( "nodeArray: " + nodeArray );
      }
   }
   finally
   {
      cmd.run( InstallPath + "/bin/sdbcmtop --I" );
      cmd.run( InstallPath + "/bin/sdbcmart" );
   }
}