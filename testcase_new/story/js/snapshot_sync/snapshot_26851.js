/******************************************************************************
 * @Description   : seqDB-26851:连接协调节点，获取聚合的系统快照内存和磁盘与系统内存磁盘对比
 * @Author        : HuangHaimei
 * @CreateTime    : 2022.08.31
 * @LastEditTime  : 2022.10.20
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
main( test );
function test ()
{
   var uniqueHostName = unique();
   var sumCpu = 0;
   var sumOther = 0;
   var sumMem = 0;
   for( var i = 0; i < uniqueHostName.length; i++ )
   {
      var remoteObj = new Remote( uniqueHostName[i], CMSVCNAME );
      var cmd = remoteObj.getCmd()
      // cpu
      var strCpu = "cat /proc/stat | grep cpu | awk '{print $6,$7,$8}'";
      var resultCpu = cmd.run( strCpu );
      var cpuInfo = parseInt( resultCpu.split( "\n" )[0].split( " " )[0] );
      var irqTime = parseInt( resultCpu.split( "\n" )[0].split( " " )[1] );
      var softirqTime = parseInt( resultCpu.split( "\n" )[0].split( " " )[2] );
      sumCpu += cpuInfo;
      var other = irqTime + softirqTime;
      sumOther += other;
      // Available
      var strMem = "cat /proc/meminfo | grep MemAvailable | awk '{print $2}'";
      var resultMem = cmd.run( strMem );
      var tmpInfo = resultMem.split( "\n" )[0];
      var memInfo2 = parseInt( tmpInfo.split( " " ) );
      sumMem += memInfo2;
      remoteObj.close();
   }
   var cpuExpect = { "IOWait": sumCpu, "Other": sumOther };
   var memExpect = { "AvailableRAM": sumMem };
   if( typeof ( resultMem ) == "undefined" || resultMem == '' || resultMem == null )
   {
      checkSnapshot( db, cpuExpect, null );
   } else
   {
      checkSnapshot( db, cpuExpect, memExpect );
   }
}

function unique ()
{
   // 获取hostname
   var hostNames = [];
   var cursor = db.snapshot( SDB_SNAP_DATABASE, { RawData: true } )
   while( cursor.next() )
   {
      var obj = cursor.current().toObj();
      hostNames.push( obj["HostName"] );
   }
   var uniqueHostName = [];
   for( var i = 0; i < hostNames.length; i++ )
   {
      if( uniqueHostName.indexOf( hostNames[i] ) == -1 )
      {
         uniqueHostName.push( hostNames[i] )
      }
   }
   cursor.close();
   return uniqueHostName;
}

function checkSnapshot ( sdb, cpuExpect, memExpect )
{
   // 查看聚合系统快照
   var cursor = sdb.snapshot( SDB_SNAP_SYSTEM, {}, { CPU: null, Memory: null } );
   while( cursor.next() )
   {
      var obj = cursor.current().toObj();
      for( var key in cpuExpect )
      {
         var actual = obj["CPU"][key];
         var expected = cpuExpect[key] / 100;
         almostEqual( actual, expected );
      }
      if( memExpect != null )
      {
         for( var key in memExpect )
         {
            var actual = parseInt( obj["Memory"][key] );
            var expected = memExpect[key] * 1024;
            almostEqual( actual, expected, 0.1 );
         }
      }
   }
   cursor.close();
}

function almostEqual ( actual, expected, errorRange )
{
   if( undefined == errorRange ) { errorRange = 0.01; }
   // 计算期望值与实际值的绝对值
   var absolute = Math.abs( actual - expected );
   // 系统获取与快照获取有误差,但误差不能大于实际值 * 误差范围
   if( absolute > ( actual * errorRange ) )
   {
      throw new Error( "数据库快照磁盘或内存与系统不一致！实际值为:" + actual + ",期望值为:" + expected );
   }
}