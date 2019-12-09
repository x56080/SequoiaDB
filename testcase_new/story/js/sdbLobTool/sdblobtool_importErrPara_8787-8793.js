/******************************************************************************
*@Description : wrong parameter test sdblobtool import
*@Modify list :
*               2016-06-22   XueWang Liang  Init
*               覆盖测试用例8787/8788/8789/8790/8791/8792/8793
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

   var expFullCL = COMMCSNAME + "." + COMMCLNAME;       // 导出集合 COMMCSNAME = CHANGEDPREFIX + "_cs" , COMMCLNAME = CHANGEDPREFIX + "_cl"
   var filename = CHANGEDPREFIX + "_testimport.file";	// CHANGEDPREFIX = "local_test" 文件名	
   var exportFile = LocalPath + "/" + filename;         // 导出文件，将该文件进行导入
   var impCLNAME = COMMCLNAME + "_new";                 // 导入集合名
   var impFullCL = COMMCSNAME + "." + impCLNAME;

   // sdblobtool 导入的选项参数
   var Args = {};
   Args["hostname"] = COORDHOSTNAME;       //  'localhost'
   Args["svcname"] = COORDSVCNAME;         //  11810
   Args["usrname"] = null;
   Args["passwd"] = null;
   Args["operation"] = "import";
   Args["collection"] = impFullCL;
   Args["file"] = exportFile;
   Args["ssl"] = false;                    // 使用SSL连接，如果不使用SSL连接，shell命令中不应加入该选项参数
   Args["ignorefe"] = false;               // 大对象已经存在于集合中，忽略

   // 首先创建一个包含大对象的集合COMMCLNAME，和导入集合impCLNAME
   var lobfile = toolMakeLobfile();
   var expCl = commCreateCL( db, COMMCSNAME, COMMCLNAME, 0, true, true, false, "create CL to export lob" );
   var lobNum = 1;
   toolPutLobs( expCl, lobfile, lobNum );
   cmd.run( "rm -rf " + lobfile );

   var impCl = commCreateCL( db, COMMCSNAME, impCLNAME, 0, true, true, false, "create CL to import lob" );

   // 将COMMCLNAME中的大对象导出到文件
   Args["operation"] = "export";
   Args["collection"] = expFullCL;
   toolExport( Args );

   // 测试错误参数下将文件大对象导入到集合impCLNAME中,
   Args["operation"] = "import";
   Args["collection"] = impFullCL;


   var errParameter = {
      "hostname": "172.168.20.43", "svcname": "90000", "operation": "impo",
      "collection": COMMCSNAME + ".noexists_cl"
   };
   var errCode = { "hostname": "-13", "svcname": "-6", "operation": "-6", "collection": "-23" };
   for( var k in errParameter )
   {
      var tmp = Args[k];
      Args[k] = errParameter[k];
      try
      {
         // 导入
         toolImport( Args );
      }
      catch( e )
      {
         // 对错误日志文件进行解析，获取错误码
         var errNumber = getErr( e );
         if( errNumber == errCode[k] )
            println( ">success to test sdblobtool import with wrong " + k + ":" + errParameter[k] + ", rc = " + errNumber );
         else
         {
            println( ">fail to test sdblobtool import with wrong " + k + ":" + errParameter[k] + ", rc = " + errNumber );
            commDropCL( db, COMMCSNAME, impCLNAME, true, true, "clean collection in the end, wrong" );
            cmd.run( "rm -rf " + exportFile );
            throw errNumber;
         }
      }
      finally
      {
         Args[k] = tmp;
      }
   }

   cmd.run( "rm -rf " + exportFile );


   // 测试导入文件不存在时，文件为空时，文件为普通文件时的导入
   var noexistFileName = LocalPath + "/noexist.file";
   var emptyFileName = LocalPath + "/empty.file";
   var emptyFile = new File( emptyFileName );
   emptyFile.close();
   var normalFileName = LocalPath + "/normal.file";
   var normalFile = new File( normalFileName );
   normalFile.write( "shskhsaochoacakbckaacvavcvagkcgcdd" );
   normalFile.close();

   var errFile = {};
   errFile[noexistFileName] = -4;
   errFile[emptyFileName] = -9;
   errFile[normalFileName] = -6;
   var errFileMessage = {};
   errFileMessage[noexistFileName] = "file not exist";
   errFileMessage[emptyFileName] = "file is empty";
   errFileMessage[normalFileName] = "file is normal";
   for( var k in errFile )
   {
      Args["file"] = k;
      try
      {
         // 导入
         toolImport( Args );
      }
      catch( e )
      {
         // 对错误日志文件进行解析，获取错误码
         var errNumber = getErr( e );
         if( errNumber == errFile[k] )
            println( ">success to test sdblobtool import when --file " + errFileMessage[k] + ", rc = " + errNumber );
         else
         {
            println( ">fail to test sdblobtool import when --file " + errFileMessage[k] + ", rc = " + errNumber );
            commDropCL( db, COMMCSNAME, impCLNAME, true, true, "clean collection in the end, wrong" );
            cmd.run( "rm -rf " + k );
            throw errNumber;
         }
      }
   }

   println( ">success to test sdblobtool import with wrong parameter.\n\n" );

   commDropCL( db, COMMCSNAME, impCLNAME, true, true, "clean collection in the end, right" );
   cmd.run( "rm -rf *.file" );
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
   cmd.run( "mv sdblobtool.log /tmp/lobtool/importErrPara.log" );
   throw e;
}
finally
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
      "clean collection in the end, wrong" );
   db.close();
}