/******************************************************************************
*@Description : test sdblobtool import parameter --ignore
*@Modify list :
*               2016-06-24   XueWang Liang  Init
*               覆盖测试用例8794/8795
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
   Args["hostname"] = COORDHOSTNAME;         //  'localhost'
   Args["svcname"] = COORDSVCNAME;           //  11810
   Args["usrname"] = null;
   Args["passwd"] = null;
   Args["operation"] = "import";
   Args["collection"] = impFullCL;
   Args["file"] = importFile;
   Args["ssl"] = false;                     // 使用SSL连接，如果不使用SSL连接，shell命令中不应加入该选项参数
   Args["ignorefe"] = false;                // 当前大对象已经存在于集合中，忽略这个错误



   // 首先创建包含相同大对象的导出集合COMMCLNAME和导入集合impCLNAME
   // 注意：同一个文件作为大对象putLob导入不同集合时OID不同，不是同一个大对象
   var lobfile = toolMakeLobfile();
   var expCl = commCreateCL( db, COMMCSNAME, COMMCLNAME, 0, true, true, false, "create CL to export lob" );
   var lobNum = 1;
   toolPutLobs( expCl, lobfile, lobNum );
   cmd.run( "rm -rf " + lobfile );

   // 从导出集合COMMCLNAME中导出大对象为文件，以便进行导入
   Args["operation"] = "export";
   Args["collection"] = expFullCL;
   toolExport( Args );

   Args["operation"] = "import";
   Args["collection"] = impFullCL;
   commCreateCL( db, COMMCSNAME, impCLNAME, 0, true, true, false, "create CL in the beginning" );
   toolImport( Args );     // 首先将导出文件先导入集合一次，测试再次导入该文件的情况

   // 测试当--ignorefe = false时，报错 
   var errCode = -5;  // 集合中已包含相同的大对象时进行导入，错误码为-5

   try
   {
      toolImport( Args );
      println( ">fail to test sdblobtool import with same lob when --ignorefe is false, something wrong!!!!" );
   }
   catch( e )
   {
      var errNumber = getErr( e );
      if( errNumber == errCode )
         println( ">success to test sdblobtool import with same lob when --ignorefe is false, rc = " + errNumber );
      else
      {
         println( ">fail to test sdblobtool import with same lob when --ignorefe is false, rc = " + errNumber );
         commDropCL( db, COMMCSNAME, impCLNAME, true, true, "clean collection in the end, wrong" );
         cmd.run( "rm -rf " + importFile );
         throw errNumber;
      }
   }

   // 测试当--ignorefe = true时，忽略这个错误，不报错
   Args["ignorefe"] = true;
   try
   {
      toolImport( Args );
      println( ">success to test sdblobtool import with same lob when --ignorefe is true, no error" );
   }
   catch( e )
   {
      var errNumber = getErr( e );
      println( ">fail to test sdblobtool import with same lob when --ignorefe is true, rc = " + errNumber );
      commDropCL( db, COMMCSNAME, impCLNAME, true, true, "clean collection in the end, wrong" );
      cmd.run( "rm -rf " + importFile );
      throw errNumber;
   }

   cmd.run( "rm -rf " + importFile );
   commDropCL( db, COMMCSNAME, impCLNAME, true, true, "clean collection in the end, right" );
   println( ">success to test sbdlobtool import with Same Lob.\n\n" );
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
   cmd.run( "mv sdblobtool.log /tmp/lobtool/importIgnore.log" );
   throw e;
}
finally
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
      "clean collection in the end, wrong" );
   db.close();
}
