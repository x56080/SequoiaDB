/******************************************************************************
*@Description : test sdblobtool migration normal condition with SSL
                测试时需要修改COORDSVCNAME配置文档 /conf/local/...
                usessl = TRUE 并重启集群
*@Modify list :
*               2016-06-20   XueWang Liang  Init
******************************************************************************/


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

   var srcFullCL = COMMCSNAME + "." + COMMCLNAME;       // 源集合 COMMCSNAME = CHANGEDPREFIX + "_cs" , COMMCLNAME = CHANGEDPREFIX + "_cl"
   var dstClName = CHANGEDPREFIX + "_newcl";
   var dstFullCL = COMMCSNAME + "." + dstClName;        // 目标集合

   // sdblobtool 迁移的选项参数
   var Args = {};
   Args["hostname"] = COORDHOSTNAME;         // 'localhost'
   Args["svcname"] = COORDSVCNAME;           // 11810
   Args["usrname"] = null;
   Args["passwd"] = null;
   Args["operation"] = "migration";
   Args["collection"] = srcFullCL;
   Args["ssl"] = true;                      // 使用SSL连接，如果不使用SSL连接，shell命令中不应加入该选项参数
   Args["ignorefe"] = false;                 // 当前大对象已经存在于集合中，忽略这个错误
   Args["dsthost"] = COORDHOSTNAME;
   Args["dstservice"] = COORDSVCNAME;
   Args["dstusrname"] = null;
   Args["dstpasswd"] = null;
   Args["dstcollection"] = dstFullCL;

   try
   {
      // 创建包含大对象的源集合
      var lobfile = toolMakeLobfile();
      var srcCl = commCreateCL( db, COMMCSNAME, COMMCLNAME, 0, true, true, false, "create source collection" );
      var lobNum = 1;
      var OID = toolPutLobs( srcCl, lobfile, lobNum );

      // 获取putLob文件的Md5值
      var wMd5sum = cmd.run( "md5sum " + lobfile ).split( " " );
      var wMd5 = wMd5sum[0];

      // 创建导入集合并将大对象导入
      var dstCl = commCreateCL( db, COMMCSNAME, dstClName, 0, true, true, false, "create destination collection" );
      toolMigration( Args );

      // 检验迁移大对象的条数是否正确
      toolCheckLob( dstCl, lobNum, OID );

      // 获得getLob文件的Md5值，检验导入是否成功
      var tempfile = LocalPath + "/_temp.file";
      dstCl.getLob( OID[0], tempfile );
      var rMd5sum = cmd.run( "md5sum " + tempfile ).split( " " );
      var rMd5 = rMd5sum[0];
      if( rMd5 != wMd5 )
      {
         throw ( ">putlob file have md5: " + wMd5 +
            " not equal to getlob file md5: " + rMd5 );
      }
      else
         println( ">success to migrate " + lobNum + " lob." );

      println( ">success to test migration Normal Condition.\n\n" );
   }
   finally
   {
      cmd.run( "rm -rf " + lobfile );
      cmd.run( "rm -rf " + tempfile );
      commDropCL( db, COMMCSNAME, dstClName, true, true, "clean collection in the end." );
   }
}

// Test
try
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
      "clean collection in the beginning" );
   main( db );
   cmd.run( "rm -rf sdblobtool.log" );
}
catch( e )
{
   cmd.run( "mkdir -p /tmp/lobtool" );
   cmd.run( "mv sdblobtool.log /tmp/lobtool/migrationSSL.log" );
   throw e;
}
finally
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
      "clean collection in the end, wrong" );
   db.close();
}


