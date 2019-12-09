/******************************************************************************
*@Description : test sdblobtool export when over fsize limit
                手动测试  
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
      var lobNum = 1000;
      toolPutLobs( expCl, lobfile, lobNum );

      // 手动设置创建文件的最大大小为1024k = 1M
      // cmd.run( "ulimit -f 1024" ) ; 
      // var flimit = cmd.run("ulimit -a | sed -n 4p | cut -c 38-").split("\n")[0] ;
      // println( ">file size limit is : " + flimit ) ;

      // 导出大对象到文件exportFile
      // 导出失败，导出文件超过1M，错误码25
      toolExport( Args );
      var fsize = cmd.run( "ls -lh " + exportFile + "|cut -d ' ' -f 5" ).split( "\n" )[0];
      println( ">exportfile size is: " + fsize );
   }
   finally
   {
      cmd.run( "ulimit -f unlimited" );
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
   cmd.run( "mv sdblobtool.log /tmp/lobtool/exportUlimit.log" );
   throw e;
}
finally
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
      "clean collection in the end, wrong" );
   db.close();
} 