/******************************************************************************
*@Description : test sdblobtool export and import normal condition with SSL
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

   var filename = CHANGEDPREFIX + "_test.file";	      // CHANGEDPREFIX = "local_test" 文件名
   var exportFile = LocalPath + "/" + filename;         // 导出文件，导入文件	
   var expFullCL = COMMCSNAME + "." + COMMCLNAME;       // 导出集合 COMMCSNAME = CHANGEDPREFIX + "_cs" , COMMCLNAME = CHANGEDPREFIX + "_cl"
   var impClName = CHANGEDPREFIX + "_newcl";
   var impFullCL = COMMCSNAME + "." + impClName;        // 导入集合


   // sdblobtool 导出的选项参数
   var Args = {};
   Args["hostname"] = COORDHOSTNAME;   // 'localhost'
   Args["svcname"] = COORDSVCNAME;     //  11810 独立模式下为50000
   Args["usrname"] = null;
   Args["passwd"] = null;
   Args["operation"] = "export";
   Args["collection"] = expFullCL;
   Args["file"] = exportFile;
   Args["prefer"] = "A";               //  优先选择的实例
   Args["ssl"] = true;                 //  使用SSL连接，如果不使用SSL连接，shell命令中不应加入该选项参数
   Args["ignorefe"] = false;           //  大对象已经存在于集合中，忽略

   try
   {
      // 创建包含大对象的导出集合
      var lobfile = toolMakeLobfile();
      var expCl = commCreateCL( db, COMMCSNAME, COMMCLNAME, 0, true, true, false, "create CL to export lob" );
      var lobNum = 1;
      var OID = toolPutLobs( expCl, lobfile, lobNum );

      // 获取putLob文件的Md5值
      var wMd5sum = cmd.run( "md5sum " + lobfile ).split( " " );
      var wMd5 = wMd5sum[0];

      // 导出大对象到文件exportFile
      toolExport( Args );


      // 创建导入集合并将大对象导入
      var impCl = commCreateCL( db, COMMCSNAME, impClName, 0, true, true, false, "create CL to import lob" );
      Args["operation"] = "import";
      Args["collection"] = impFullCL;
      toolImport( Args );

      // 检验导入大对象的条数是否正确
      toolCheckLob( impCl, lobNum, OID );

      // 获得getLob文件的Md5值，检验导入是否成功
      var tempfile = LocalPath + "/_temp.file";
      cmd.run( "rm -rf " + tempfile );
      impCl.getLob( OID[0], tempfile );
      var rMd5sum = cmd.run( "md5sum " + tempfile ).split( " " );
      var rMd5 = rMd5sum[0];
      if( rMd5 != wMd5 )
      {
         throw ( ">putlob file have md5: " + wMd5 +
            " not equal to getlob file md5: " + rMd5 );
      }
      else
         println( ">success to import " + lobNum + " lob." );

      println( ">success to test eximport Normal Condition.\n\n" );
   }
   finally
   {
      cmd.run( "rm -rf " + lobfile );
      cmd.run( "rm -rf " + tempfile );
      commDropCL( db, COMMCSNAME, impClName, true, true, "clean collection in the end." );
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
   cmd.run( "mv sdblobtool.log /tmp/lobtool/eximportSSL.log" );
   throw e;
}
finally
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
      "clean collection in the end, wrong" );
   db.close();
}  