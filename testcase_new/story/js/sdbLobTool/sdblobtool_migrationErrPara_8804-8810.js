/******************************************************************************
*@Description : wrong parameter test sdblobtool migration
*@Modify list :
*               2016-06-22   XueWang Liang  Init
*               覆盖测试用例8804/8805/8806/8807/8808/8809/8810
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
   var impCLNAME = COMMCLNAME + "_new";
   var impFullCL = COMMCSNAME + "." + impCLNAME;        // 目标集合 

   // sdblobtool 迁移的选项参数
   var Args = {};
   Args["hostname"] = COORDHOSTNAME;        // 'localhost'
   Args["svcname"] = COORDSVCNAME;          // 11810
   Args["usrname"] = null;
   Args["passwd"] = null;
   Args["operation"] = "migration";
   Args["collection"] = expFullCL;
   Args["ssl"] = false;                    // 使用SSL连接，如果不使用SSL连接，shell命令中不应加入该选项参数
   Args["ignorefe"] = false;               // 当前大对象已经存在于集合中，忽略这个错误
   Args["dsthost"] = COORDHOSTNAME;
   Args["dstservice"] = COORDSVCNAME;
   Args["dstusrname"] = null;
   Args["dstpasswd"] = null;
   Args["dstcollection"] = impFullCL;


   // 首先创建包含大对象的源集合COMMCLNAME和目标集合impCLNAME
   var lobfile = toolMakeLobfile();
   var expCl = commCreateCL( db, COMMCSNAME, COMMCLNAME, 0, true, true, false, "create CL to export lob" );
   var lobNum = 1;
   toolPutLobs( expCl, lobfile, lobNum );
   cmd.run( "rm -rf " + lobfile );


   var impCl = commCreateCL( db, COMMCSNAME, impCLNAME, 0, true, true, false, "create CL to migrate lob from COMMCLNAME" );

   var errParameter = {
      "hostname": "172.168.20.43", "svcname": "90000", "operation": "migra",
      "collection": COMMCSNAME + ".noexist_fcl",
      "dsthost": "172.168.20.43", "dstservice": "90000",
      "dstcollection": COMMCSNAME + ".noexist_tcl"
   };
   var errCode = {
      "hostname": "-13", "svcname": "-6", "operation": "-6", "collection": "-23",
      "dsthost": "-13", "dstservice": "-6", "dstcollection": "-23"
   };

   for( var k in errParameter )
   {
      var tmp = Args[k];
      Args[k] = errParameter[k];
      try
      {
         // 迁移
         toolMigration( Args );
      }
      catch( e )
      {
         // 对错误日志文件进行解析，获取错误码
         var errNumber = getErr( e );
         if( errNumber == errCode[k] )
            println( ">success to test sdblobtool migration with wrong " + k + ":" + errParameter[k] + ", rc = " + errNumber );
         else
         {
            println( ">fail to test sdblobtool migration with wrong " + k + ":" + errParameter[k] + ", rc = " + errNumber );
            commDropCL( db, COMMCSNAME, impCLNAME, true, true, "clean collection in the end, wrong" );
            throw errNumber;
         }
      }
      finally
      {
         Args[k] = tmp;
      }
   }
   println( ">success to test sdblobtool migration with wrong parameter.\n\n" );

   commDropCL( db, COMMCSNAME, impCLNAME, true, true, "clean collection in the end, right" );
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
   cmd.run( "mv sdblobtool.log /tmp/lobtool/migrationErrPara.log" );
   throw e;
}
finally
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
      "clean collection in the end, wrong" );
   db.close();
}