/******************************************************************************
*@Description : Hash split srcCollection and dstCollection differently
*               test sdblobtool migration putlobfile and getlobfile md5
*@Modify list :
*               2016-06-24   XueWang Liang  Init
*******************************************************************************/

function main ( db )
{
   initPath();

   // 检验是否存在sdblobtool工具
   try
   {
      checkLobtool();
   }
   catch( e )
   {
      if( e == 127 )
         return;
      throw e;
   }

   var filename = CHANGEDPREFIX + "_test.file";	      // CHANGEDPREFIX = "local_test" 文件名
   var testFile = LocalPath + "/" + filename;           // getLob读取大对象文件	
   var CsName = CHANGEDPREFIX + "_testHashCS";          // 集合空间名
   var srcClName = CHANGEDPREFIX + "_srcCl";            // 源集合名
   var srcFullCL = CsName + "." + srcClName;            // 源集合
   var dstClName = CHANGEDPREFIX + "_dstCl";            // 目标集合名
   var dstFullCL = CsName + "." + dstClName;            // 目标集合

   // sdblobtool 迁移的选项参数
   var Args = {};
   Args["hostname"] = COORDHOSTNAME;         // 'localhost'
   Args["svcname"] = COORDSVCNAME;           // 11810
   Args["usrname"] = null;
   Args["passwd"] = null;
   Args["operation"] = "migration";
   Args["collection"] = srcFullCL;
   Args["ssl"] = false;                      // 使用SSL连接，如果不使用SSL连接，shell命令中不应加入该选项参数
   Args["ignorefe"] = false;                 // 当前大对象已经存在于集合中，忽略这个错误
   Args["dsthost"] = COORDHOSTNAME;
   Args["dstservice"] = COORDSVCNAME;
   Args["dstusrname"] = null;
   Args["dstpasswd"] = null;
   Args["dstcollection"] = dstFullCL;



   // 创建一个包含两个复制组的自动切分域,并在域中创建集合空间和导出集合
   var rg = commGetGroups( db );
   if( rg.length < 3 )
   {
      println( "datagroup num:" + rg.length + " too few!!!\n\n" );
      return;
   }
   var group = [];
   group[0] = rg[0][0].GroupName;
   group[1] = rg[1][0].GroupName;
   group[2] = rg[2][0].GroupName;
   var lobdomain;
   var domName = 'lobdomain';
   commDropDomain( db, domName );
   lobdomain = commCreateDomain( db, domName, [group[0], group[1]], { AutoSplit: true } );

   try
   {
      commCreateCS( db, CsName, true, "create testHashCS in the begining", { "Domain": domName } );
      var srcCl = commCreateCLByOption( db, CsName, srcClName,
         { "ShardingKey": { "OID": 1 }, "ShardingType": "hash", "Partition": 2048, ReplSize: 0 },
         true, true, true, "create source cl in the begining" );

      // 计算wMd5值
      var lobfile = toolMakeLobfile();
      var lobNum = 1;
      var OID = toolPutLobs( srcCl, lobfile, lobNum );
      var wMd5sum = cmd.run( "md5sum " + lobfile ).split( " " );
      var wMd5 = wMd5sum[0];


      // 将域的复制组个数改成三个
      lobdomain.alter( { Groups: [group[0], group[1], group[2]] } );

      // 新建目标集合
      var dstCl = commCreateCLByOption( db, CsName, dstClName,
         { "ShardingKey": { "OID": 1 }, "ShardingType": "hash", "Partition": 2048, ReplSize: 0 },
         true, true, true, "create destination CL in the beginning" );

      toolMigration( Args );

      // 检查大对象的条数和OID
      toolCheckLob( dstCl, lobNum, OID );

      // 使用getLob方法将大对象导出为文件，计算读取Md5值，与写入Md5值比较
      cmd.run( "rm -rf " + testFile );
      dstCl.getLob( OID[0], testFile );
      var rMd5sum = cmd.run( "md5sum " + testFile ).split( " " );
      var rMd5 = rMd5sum[0];
      if( rMd5 == wMd5 )
         println( ">success to test migrationHash with rMd5 = wMd5.\n\n" );
      else
         throw ( ">fail to test migrationHash with rMd5 != wMd5.\n\n" );
   }
   finally
   {
      cmd.run( "rm -rf " + lobfile );
      cmd.run( "rm -rf " + testFile );
      commDropCL( db, CsName, dstClName, true, true, "clean destination CL in the end" );
      commDropCL( db, CsName, srcClName, true, true, "clean source CL in the end" );
      commDropCS( db, CsName, true, "clean the collection space in the end" );
      commDropDomain( db, domName );
   }
}

// Test
try
{
   main( db );
   cmd.run( "rm -rf sdblobtool.log" );
}
catch( e )
{
   cmd.run( "mkdir -p /tmp/lobtool" );
   cmd.run( "mv sdblobtool.log /tmp/lobtool/migrationHash.log" );
   throw e;
}
finally
{
   db.close();
}