/******************************************************************************
 * @Description   : seqDB-26856:System对象获取cpu和memory消息
 * @Author        : HuangHaimei
 * @CreateTime    : 2022.09.02
 * @LastEditTime  : 2022.10.20
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
main( test );
function test ()
{
   var cmd = new Cmd();
   //cpu
   var strCpu = "cat /proc/stat | grep cpu | awk '{print $6,$7,$8}'";
   var resultCpu = cmd.run( strCpu );
   var cpuInfo = parseInt( resultCpu.split( "\n" )[0].split( " " )[0] );
   var irqTime = parseInt( resultCpu.split( "\n" )[0].split( " " )[1] );
   var softirqTime = parseInt( resultCpu.split( "\n" )[0].split( " " )[2] );
   var other = irqTime + softirqTime;
   // Available
   var strMem = "cat /proc/meminfo | grep MemAvailable | awk '{print $2}'";
   var resultMem = cmd.run( strMem ).split( "\n" )[0];
   var available = parseInt( resultMem.split( " " ) );
   var strFree = "free -b | grep Mem | awk '{print $2,$4}'";
   var memInfo = cmd.run( strFree ).split( "\n" )[0];
   var freeInfo = memInfo.split( " " );
   var free = parseInt( freeInfo[1] );
   var cpuExpect = { "IOWait": cpuInfo, "Other": other };
   var memExpect = { "FreeRAM": free, "AvailableRAM": available };
   if( typeof ( resultMem ) == "undefined" || resultMem == '' || resultMem == null )
   {
      checkSnapshot( cpuExpect, null );
   } else
   {
      checkSnapshot( cpuExpect, memExpect );
   }
}

function checkSnapshot ( cpuExpect, memExpect )
{
   // 通过sdbshell的System相关接口获取cpu消息
   var cpuInfo1 = System.getCpuInfo().toObj();
   var cpuInfo2 = System.snapshotCpuInfo().toObj();
   for( var key in cpuExpect )
   {
      var actual1 = cpuInfo1[key];
      var actual2 = cpuInfo2[key];
      var expected = cpuExpect[key] * 10;
      almostEqual( actual1, expected );
      almostEqual( actual2, expected );
   }
   if( memExpect != null )
   {
      // 通过sdbshell的System相关接口获取memory消息
      var memInfo1 = System.getMemInfo().toObj();
      var memInfo2 = System.snapshotMemInfo().toObj();
      var actualFree1 = memInfo1["Free"];
      var actualFree2 = memInfo2["Free"];
      var expectedFree = memExpect["FreeRAM"] / 1024 / 1024;
      almostEqual( actualFree1, expectedFree, 0.1 );
      almostEqual( actualFree2, expectedFree, 0.1 );
      var actualAvailable1 = memInfo1["Available"];
      var actualAvailable2 = memInfo2["Available"];
      var expectedAvailable = memExpect["AvailableRAM"] / 1024;
      almostEqual( actualAvailable1, expectedAvailable );
      almostEqual( actualAvailable2, expectedAvailable );
   }
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