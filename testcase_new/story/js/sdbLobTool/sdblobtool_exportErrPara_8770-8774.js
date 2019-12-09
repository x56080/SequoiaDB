/******************************************************************************
*@Description : wrong parameter test sdblobtool export
*@Modify list :
*               2016-06-20   XueWang Liang  Init
*               覆盖测试用例8770/8771/8772/8773/8774
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

   var filename = CHANGEDPREFIX + "_testexport.file";	// CHANGEDPREFIX = "local_test" 文件名
   var exportFile = LocalPath + "/" + filename;         // 输出文件	
   var expFullCL = COMMCSNAME + "." + COMMCLNAME;       // 输出集合 COMMCSNAME = CHANGEDPREFIX + "_cs" , COMMCLNAME = CHANGEDPREFIX + "_cl"

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


   // 错误参数：--hostname, --svcname, --operation, --collection
   var errParameter = {
      "hostname": "172.168.20.43", "svcname": "90000", "operation": "expo",
      "collection": COMMCSNAME + ".noexists_cl"
   };
   // 对应的错误码(集群模式和独立模式)
   var errCode = { "hostname": "-13", "svcname": "-6", "operation": "-6", "collection": "-23" };


   // 执行导出操作前，首先需要创建一个含有大对象的集合COMMCLNAME
   var lobfile = toolMakeLobfile();
   var expCl = commCreateCL( db, COMMCSNAME, COMMCLNAME, 0, true, true, false, "create CL to export lob" );
   var lobNum = 1;
   toolPutLobs( expCl, lobfile, lobNum );
   cmd.run( "rm -rf " + lobfile );

   for( var k in errParameter )
   {
      try
      {
         var tmp = Args[k];
         Args[k] = errParameter[k];
         toolExport( Args );
      }
      catch( e )
      {
         // 对错误日志文件进行解析，获取错误码
         var errNumber = getErr( e );
         if( errNumber == errCode[k] )
            println( ">success to test sdblobtool export with wrong " + k + ":" + errParameter[k] + ", rc = " + errNumber );
         else
         {
            println( ">fail to test sdblobtool export with wrong " + k + ":" + errParameter[k] + ", rc = " + errNumber );
            throw errNumber;
         }
      }
      finally
      {
         // 一次测试完成后，将参数重置
         Args[k] = tmp;
      }
   }


   // 测试导出文件已经存在时，大对象工具的导出
   var existFile = LocalPath + "/exist.file";
   var file = new File( existFile );
   // file.write( "adhklljjiocjoaljaljcjioaih" ) ;  不论已存在的文件是否为空，都不能从集合中导出大对象进入该文件
   file.close();

   var Filerr = "-5";     // 文件存在时，导出大对象到文件产生的错误码为-5
   Args["file"] = existFile;
   try
   {
      var ExportCmd = toolGetCmdstr( Args );
      cmd.run( ExportCmd );
      println( ">export successful." );
   }
   catch( e )
   {
      var errNumber = getErr( e );
      if( errNumber == Filerr )
         println( ">success to test sdblobtool export when the file exists, rc = " + errNumber );
      else
      {
         println( ">fail to test sdblobtool when the file exists, rc = " + errNumber );
         throw errNumber;
      }

   }
   finally
   {
      // 测试完成后，删除文件，日志文件，重置参数
      cmd.run( "rm -rf " + existFile );
      Args["file"] = exportFile;
   }
   println( ">success to test export with wrong parameter.\n\n" );
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
   cmd.run( "mv sdblobtool.log /tmp/lobtool/exportErrPara.log" );
   throw e;
}
finally
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
      "clean collection in the end, wrong" );
   db.close();
}
