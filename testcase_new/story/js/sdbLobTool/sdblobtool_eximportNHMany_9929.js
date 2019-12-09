/******************************************************************************
*@Description : test sdblobtool export and import between normal collection
                and hash collection with many lobs
*@Modify list :
*               2016-09-12   XueWang Liang  Init
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
   Args["prefer"] = "M";               //  优先选择的实例
   Args["ssl"] = false;                //  使用SSL连接，如果不使用SSL连接，shell命令中不应加入该选项参数
   Args["ignorefe"] = false;           //  大对象已经存在于集合中，忽略

   try
   {
      // 创建包含大对象的导出集合（普通集合）
      var lobfile = toolMakeLobfile();
      var expCl = commCreateCL( db, COMMCSNAME, COMMCLNAME, 0, true, true, false,
         "create normal CL to export lob" );
      var lobNum = 5;
      var OID = toolPutLobs( expCl, lobfile, lobNum );

      // 导出大对象到文件exportFile
      toolExport( Args );

      // 创建导入集合并将大对象导入（分区集合）
      var impCl = commCreateCLByOption( db, COMMCSNAME, impClName,
         { ReplSize: 0, "ShardingKey": { "OID": 1 }, "ShardingType": "hash", "Partition": 2048 },
         true, true, true, "create hash cl to import lob" );
      Args["operation"] = "import";
      Args["collection"] = impFullCL;
      toolImport( Args );

      // 检验导入大对象的条数和OID
      toolCheckLob( impCl, lobNum, OID );

      println( ">success to test eximport NH with many lobs.\n\n" );
   }
   catch( e )
   {
      println( ">fail to test eximport NHMany,rc = " + e );
      throw e;
   }
   finally
   {
      cmd.run( "rm -rf " + lobfile );
      cmd.run( "rm -rf " + exportFile );
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
   cmd.run( "mv sdblobtool.log /tmp/lobtool/eximportNHMany.log" );
   throw e;
}
finally
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
      "clean collection in the end, wrong" );
   db.close();
}


