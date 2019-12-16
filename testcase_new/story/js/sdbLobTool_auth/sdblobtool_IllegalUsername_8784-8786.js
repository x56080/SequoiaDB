/******************************************************************************
*@Description : test sdblobtool import/export/migration with Illegal Username
*@Modify list :
*               2016-06-22   XueWang Liang  Init
*               覆盖测试用例8784/8785/8786
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

   if( commIsStandalone( db ) )     // 独立模式下不能创建用户，直接跳过
      return;

   // 新建用户和连接
   var user = "sdbadmin";
   var passwd = "sdbadmin";
   db.createUsr( user, passwd );
   db = new Sdb( hostName, coordPort, user, passwd );

   var expFullCL = COMMCSNAME + "." + COMMCLNAME;       // 导出（源）集合 COMMCSNAME = CHANGEDPREFIX + "_cs" , COMMCLNAME = CHANGEDPREFIX + "_cl"
   var filename = CHANGEDPREFIX + "_testimport.file";	// CHANGEDPREFIX = "local_test" 文件名	
   var exportFile = LocalPath + "/" + filename;         // 导出文件，将该文件进行导入
   var impCLNAME = COMMCLNAME + "_new";                 // 导入（目标）集合名
   var impFullCL = COMMCSNAME + "." + impCLNAME;        // 导入（目标）集合

   // sdblobtool选项参数
   var Args = {};
   Args["hostname"] = COORDHOSTNAME;       //  'localhost'
   Args["svcname"] = COORDSVCNAME;         //  11810
   Args["usrname"] = user;
   Args["passwd"] = passwd;
   Args["operation"] = "import";
   Args["collection"] = impFullCL;
   Args["file"] = exportFile;
   Args["prefer"] = "A";
   Args["ssl"] = false;                    // 使用SSL连接，如果不使用SSL连接，shell命令中不应加入该选项参数
   Args["ignorefe"] = true;                // 大对象已经存在于集合中，忽略
   Args["dsthost"] = COORDHOSTNAME;
   Args["dstservice"] = COORDSVCNAME;
   Args["dstusrname"] = user;
   Args["dstpasswd"] = passwd;
   Args["dstcollection"] = impFullCL;


   // 首先创建一个包含大对象的导出（源）集合COMMCLNAME，和导入（目标）集合impCLNAME
   var lobfile = toolMakeLobfile();
   var expCl = commCreateCL( db, COMMCSNAME, COMMCLNAME, {}, true, true, "create CL to export lob" );
   var lobNum = 1;
   toolPutLobs( expCl, lobfile, lobNum );
   cmd.run( "rm -rf " + lobfile );
   var impCl = commCreateCL( db, COMMCSNAME, impCLNAME, {}, true, true, "create CL to import lob" );

   var erruser = "root";
   Args["usrname"] = erruser;
   Args["dstusrname"] = erruser;
   var errCode = -179;

   try
   {
      // 将COMMCLNAME中的大对象导出到文件
      Args["operation"] = "export";
      Args["collection"] = expFullCL;
      try
      {
         toolExport( Args );
      }
      catch( e )
      {
         var errNumber = getErr( e );
         if( errNumber == errCode )
            println( ">success to test export with illegal usrname." );
         else
         {
            println( ">fail to test export with illegal usrname. rc = " + errNumber );
            throw errNumber;
         }
      }

      Args["operation"] = "import";
      Args["collection"] = impFullCL;
      try
      {
         toolImport( Args );
      }
      catch( e )
      {
         var errNumber = getErr( e );
         if( errNumber == errCode )
            println( ">success to test import with illegal usrname." );
         else
         {
            println( ">fail to test import with illegal usrname. rc = " + errNumber );
            throw errNumber;
         }
      }

      Args["operation"] = "migration";
      Args["collection"] = expFullCL;
      Args["dstcollection"] = impFullCL;
      try
      {
         toolMigration( Args );
      }
      catch( e )
      {
         var errNumber = getErr( e );
         if( errNumber == errCode )
            println( ">success to test migrate with illegal usrname." );
         else
         {
            println( ">fail to test migrate with illegal usrname. rc = " + errNumber );
            throw errNumber;
         }
      }

      println( ">success to test sdblobtool with illegal usrname.\n\n" );
   }
   catch( e )
   {
      println( ">fail to test sdblobtool with illegal usrname. rc = " + e + "\n\n" );
      throw e;
   }
   finally
   {
      cmd.run( "rm -rf " + exportFile );
      commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "clean collection in the end" );
      commDropCL( db, COMMCSNAME, impCLNAME, true, true, "clean collection in the end" );
      db.dropUsr( user, passwd );
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
   cmd.run( "mv sdblobtool.log /tmp/lobtool/illegalUsrname.log" );
   throw e;
}
finally
{
   db.close();
}