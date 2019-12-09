/******************************************************************************
*@Description : Invalid Argument test sdblobtool migration
*@Modify list :
*               2016-06-24   XueWang Liang  Init
*               覆盖测试用例8812/8813/8814/8815/8816/8817
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

   var expFullCL = COMMCSNAME + "." + COMMCLNAME;       // 源集合 COMMCSNAME = CHANGEDPREFIX + "_cs" , COMMCLNAME = CHANGEDPREFIX + "_cl"
   var impCLNAME = COMMCLNAME + "_new";                 // 目标集合名
   var impFullCL = COMMCSNAME + "." + impCLNAME;        // 目标集合

   // sdblobtool 迁移的选项参数
   var Args = {};
   Args["hostname"] = COORDHOSTNAME;        // 'localhost'
   Args["svcname"] = COORDSVCNAME;          // 11810
   Args["usrname"] = null;
   Args["passwd"] = null;
   Args["operation"] = "migration";
   Args["collection"] = expFullCL;
   Args["ssl"] = false;                     // 使用SSL连接，如果不使用SSL连接，shell命令中不应加入该选项参数
   Args["ignorefe"] = false;                // 当前大对象已经存在于集合中，忽略这个错误
   Args["dsthost"] = COORDHOSTNAME;
   Args["dstservice"] = COORDSVCNAME;
   Args["dstusrname"] = null;
   Args["dstpasswd"] = null;
   Args["dstcollection"] = impFullCL;


   // 创建包含大对象的集合COMMCLNAME作为源集合
   var lobfile = toolMakeLobfile();
   var expCl = commCreateCL( db, COMMCSNAME, COMMCLNAME, 0, true, true, false, "create CL to export lob" );
   var lobNum = 1;
   toolPutLobs( expCl, lobfile, lobNum );
   cmd.run( "rm -rf " + lobfile );


   // 首先生成正确的迁移命令并保存，创建目标集合
   var MigrationCmd = toolGetCmdstr( Args );
   var tmp = MigrationCmd;

   commCreateCL( db, COMMCSNAME, impCLNAME, 0, true, true, false, "create destination CL in the beginning" );

   // 参数非法报错的错误码
   errCode = -6;

   // 测试迁移命令中含有多余不存在的选项 --Illegal
   try
   {
      MigrationCmd += " --Illegal something";
      cmd.run( MigrationCmd );
      println( ">migrate lob with illegal option, something wrong!" );
   }
   catch( e )
   {
      var errNumber = getErr( e );
      if( errNumber == errCode )
         println( ">success to test sdblobtool migrate with illegal option, rc = " + errNumber );
      else
      {
         println( ">fail to test sdblobtool migrate with illegal option, rc = " + errNumber );
         commDropCL( db, COMMCSNAME, impCLNAME, true, true, "clean collection in the end, wrong" );
         throw errNumber;
      }
   }
   finally
   {
      MigrationCmd = tmp;
   }

   // 测试迁移命令中选项名称拼写错误，如--colection
   try
   {
      MigrationCmd = MigrationCmd.replace( "--collection", "--colection" );
      cmd.run( MigrationCmd );
      println( ">migrate lob with wrong option name, something wrong!" );
   }
   catch( e )
   {
      var errNumber = getErr( e );
      if( errNumber == errCode )
         println( ">success to test sdblobtool migrate with wrong option name, rc = " + errNumber );
      else
      {
         println( ">fail to test sdblobtool migrate with wrong option name, rc = " + errNumber );
         commDropCL( db, COMMCSNAME, impCLNAME, true, true, "clean collection in the end, wrong" );
         throw errNumber;
      }
   }
   finally
   {
      MigrationCmd = tmp;
   }


   // 测试命令中只填了选项名称，没填值的情况，如--hostname 空，该选项必须放在最后，放在其他位置是参数不正确错误，参看lobtool_migrateErrPara.js
   try
   {
      MigrationCmd = MigrationCmd.replace( "--hostname " + Args["hostname"], "" );
      MigrationCmd += " --hostname";
      cmd.run( MigrationCmd );
      println( ">migrate lob with no option value, something wrong!" );
   }
   catch( e )
   {
      var errNumber = getErr( e );
      if( errNumber == errCode )
         println( ">success to test sdblobtool migrate with no option value, rc = " + errNumber );
      else
      {
         println( ">fail to test sdblobtool migrate with no option value, rc = " + errNumber );
         commDropCL( db, COMMCSNAME, impCLNAME, true, true, "clean collection in the end, wrong" );
         throw errNumber;
      }
   }
   finally
   {
      MigrationCmd = tmp;
   }

   // 测试迁移命令中缺少必填项
   var parameter = ["operation", "collection", "dstcollection"];
   for( var index = 0; index < parameter.length; index++ )
   {
      try
      {
         MigrationCmd = MigrationCmd.replace( "--" + parameter[index] + " " +
            Args[parameter[index]], "" );
         cmd.run( MigrationCmd );
         println( ">migrate lob with no option " + parameter[index] + ", something wrong!" );
      }
      catch( e )
      {
         var errNumber = getErr( e );
         if( errNumber == errCode )
            println( ">success to test sdblobtool migrate with no option " + parameter[index]
               + ", rc = " + errNumber );
         else
         {
            println( ">fail to test sdblobtool migrate with no option " + parameter[index]
               + ", rc = " + errNumber );
            commDropCL( db, COMMCSNAME, impCLNAME, true, true, "clean collection in the end, wrong" );
            throw errNumber;
         }
      }
      finally
      {
         MigrationCmd = tmp;
      }
   }

   commDropCL( db, COMMCSNAME, impCLNAME, true, true, "clean collection in the end, right" );
   println( ">success to test sbdlobtool migrate with Invalid Argument.\n\n" );
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
   cmd.run( "mv sdblobtool.log /tmp/lobtool/migrationInvalidArg.log" );
   throw e;
}
finally
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
      "clean collection in the end, wrong" );
   db.close();
}
