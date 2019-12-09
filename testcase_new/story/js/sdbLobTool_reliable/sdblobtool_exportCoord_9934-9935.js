/******************************************************************************
*@Description : test sdblobtool export when kill -9 11810 or 
                手动执行                    service sdbcm stop
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
   var exportFile = LocalPath + "/" + filename;         // 导出文件	
   var expFullCL = COMMCSNAME + "." + COMMCLNAME;       // 导出集合 COMMCSNAME = CHANGEDPREFIX + "_cs" , COMMCLNAME = CHANGEDPREFIX + "_cl"

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
   Args["ssl"] = false;                //  使用SSL连接，如果不使用SSL连接，shell命令中不应加入该选项参数
   Args["ignorefe"] = false;           //  大对象已经存在于集合中，忽略

   try
   {
      // 创建包含大对象的导出集合
      var lobfile = toolMakeLobfile();
      var expCl = commCreateCL( db, COMMCSNAME, COMMCLNAME, 0, true, true, false,
         "create CL to export lob" );
      var lobNum = 10000;
      toolPutLobs( expCl, lobfile, lobNum );

      println( ">begin to export lob after 5 seconds" );
      cmd.run( "sleep 5" );
      // 导出大对象到文件exportFile，
      // 在导出过程中kill协调节点，导出失败，错误码为8
      // 在导出过程中service sdbcm stop，导出失败，错误码为8
      // 因为断开协调节点，最后删除集合也会出错，rc=-16
      toolExport( Args );
      println( ">end of export lob" );
   }
   finally
   {
      cmd.run( "rm -rf " + lobfile );
      cmd.run( "rm -rf " + exportFile );
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
   cmd.run( "mv sdblobtool.log /tmp/lobtool/exportCoord.log" );
   throw e;
}
finally
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
      "clean collection in the end, wrong" );
   db.close();
} 