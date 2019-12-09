/******************************************************************************
*@Description : Invalid Argument test sdblobtool import
*@Modify list :
*               2016-06-20   XueWang Liang  Init
*               覆盖测试用例8796/8797/8798/8799/8800/8801
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

   var filename = CHANGEDPREFIX + "_testimport.file";	// CHANGEDPREFIX = "local_test" 文件名
   var importFile = LocalPath + "/" + filename;         // 被导入的文件大对象，是由导出集合COMMCLNAME导出的文件	
   var expFullCL = COMMCSNAME + "." + COMMCLNAME;       // 输出集合 COMMCSNAME = CHANGEDPREFIX + "_cs" , COMMCLNAME = CHANGEDPREFIX + "_cl"
   var impCLNAME = COMMCLNAME + "_new";                 // 导入集合名
   var impFullCL = COMMCSNAME + "." + impCLNAME;        // 导入集合

   // sdblobtool 导入的选项参数
   var Args = {};
   Args["hostname"] = COORDHOSTNAME;    // 'localhost'
   Args["svcname"] = COORDSVCNAME;      // 11810
   Args["usrname"] = null;
   Args["passwd"] = null;
   Args["operation"] = "import";
   Args["collection"] = impFullCL;
   Args["file"] = importFile;
   Args["ssl"] = false;                 // 使用SSL连接，如果不使用SSL连接，shell命令中不应加入该选项参数
   Args["ignorefe"] = false;            // 当前大对象已经存在于集合中，忽略这个错误

   // 创建包含大对象的集合COMMCLNAME并将大对象导入文件
   var lobfile = toolMakeLobfile();
   var expCl = commCreateCL( db, COMMCSNAME, COMMCLNAME, 0, true, true, false, "create CL to export lob" );
   var lobNum = 1;
   toolPutLobs( expCl, lobfile, lobNum );
   cmd.run( "rm -rf " + lobfile );


   cmd.run( "rm -rf " + importFile );
   Args["operation"] = "export";
   Args["collection"] = expFullCL;
   toolExport( Args );



   // 首先生成正确的导入命令并保存，创建导入集合
   Args["operation"] = "import";
   Args["collection"] = impFullCL;
   var ImportCmd = toolGetCmdstr( Args );
   var tmp = ImportCmd;
   commCreateCL( db, COMMCSNAME, impCLNAME, 0, true, true, false, "create import CL in the beginning" );


   // 参数非法报错的错误码
   errCode = -6;

   // 测试导入命令中含有多余不存在的选项 --Illegal
   try
   {
      ImportCmd += " --Illegal something";
      cmd.run( ImportCmd );
      println( ">import lob with illegal option, something wrong!" );
   }
   catch( e )
   {
      var errNumber = getErr( e );
      if( errNumber == errCode )
         println( ">success to test sdblobtool import with illegal option, rc = " + errNumber );
      else
      {
         println( ">fail to test sdblobtool import with illegal option, rc = " + errNumber );
         commDropCL( db, COMMCSNAME, impCLNAME, true, true, "clean collection in the end, wrong" );
         throw errNumber;
      }
   }
   finally
   {
      ImportCmd = tmp;
   }

   // 测试导入命令中选项名称拼写错误，如--colection
   try
   {
      ImportCmd = ImportCmd.replace( "--collection", "--colection" );
      cmd.run( ImportCmd );
      println( ">import lob with wrong option name, something wrong!" );
   }
   catch( e )
   {
      var errNumber = getErr( e );
      if( errNumber == errCode )
         println( ">success to test sdblobtool import with wrong option name, rc = " + errNumber );
      else
      {
         println( ">fail to test sdblobtool import with wrong option name, rc = " + errNumber );
         commDropCL( db, COMMCSNAME, impCLNAME, true, true, "clean collection in the end, wrong" );
         throw errNumber;
      }
   }
   finally
   {
      ImportCmd = tmp;
   }


   // 测试命令中只填了选项名称，没填值的情况，如--hostname 空，该选项必须放在最后，放在其他位置是参数不正确错误，参看lobtool_importErrPara.js
   try
   {
      ImportCmd = ImportCmd.replace( "--hostname " + Args["hostname"], "" );
      ImportCmd += " --hostname";
      cmd.run( ImportCmd );
      println( ">import lob with no option value, something wrong!" );
   }
   catch( e )
   {
      var errNumber = getErr( e );
      if( errNumber == errCode )
         println( ">success to test sdblobtool import with no option value, rc = " + errNumber );
      else
      {
         println( ">fail to test sdblobtool import with no option value, rc = " + errNumber );
         commDropCL( db, COMMCSNAME, impCLNAME, true, true, "clean collection in the end, wrong" );
         throw errNumber;
      }
   }
   finally
   {
      ImportCmd = tmp;
   }

   // 测试导入命令中缺少必填项
   var parameter = ["operation", "collection", "file"];
   for( var index = 0; index < parameter.length; index++ )
   {
      try
      {
         ImportCmd = ImportCmd.replace( "--" + parameter[index] + " " + Args[parameter[index]], "" );
         cmd.run( ImportCmd );
         println( ">import lob with no option " + parameter[index] + ", something wrong!" );
      }
      catch( e )
      {
         var errNumber = getErr( e );
         if( errNumber == errCode )
            println( ">success to test sdblobtool import with no option " + parameter[index] + ", rc = " + errNumber );
         else
         {
            println( ">fail to test sdblobtool import with no option " + parameter[index] + ", rc = " + errNumber );
            commDropCL( db, COMMCSNAME, impCLNAME, true, true, "clean collection in the end, wrong" );
            throw errNumber;
         }
      }
      finally
      {
         ImportCmd = tmp;
      }
   }

   commDropCL( db, COMMCSNAME, impCLNAME, true, true, "clean collection in the end, right" );
   cmd.run( "rm -rf " + importFile );
   println( ">success to test sbdlobtool import with Invalid Argument.\n\n" );
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
   cmd.run( "mv sdblobtool.log /tmp/lobtool/importInvalidArg.log" );
   throw e;
}
finally
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
      "clean collection in the end, wrong" );
   db.close();
}
