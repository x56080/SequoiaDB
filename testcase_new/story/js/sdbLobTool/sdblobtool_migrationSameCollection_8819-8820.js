/******************************************************************************
*@Description : collection and dstcollection is the Same Collection 
*               test sdblobtool migration
*@Modify list :
*               2016-06-24   XueWang Liang  Init
*               覆盖测试用例8819/8820
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
   var impFullCL = COMMCSNAME + "." + COMMCLNAME;       // 目标集合 与源集合为同一集合

   // sdblobtool 迁移的选项参数
   var Args = {};
   Args["hostname"] = COORDHOSTNAME;         // 'localhost'
   Args["svcname"] = COORDSVCNAME;           // 11810
   Args["usrname"] = null;
   Args["passwd"] = null;
   Args["operation"] = "migration";
   Args["collection"] = expFullCL;
   Args["ssl"] = false;                      // 使用SSL连接，如果不使用SSL连接，shell命令中不应加入该选项参数
   Args["ignorefe"] = false;                 // 当前大对象已经存在于集合中，忽略这个错误
   Args["dsthost"] = COORDHOSTNAME;
   Args["dstservice"] = COORDSVCNAME;
   Args["dstusrname"] = null;
   Args["dstpasswd"] = null;
   Args["dstcollection"] = impFullCL;


   // 首先创建包含大对象的源集合COMMCLNAME
   var lobfile = toolMakeLobfile();
   var expCl = commCreateCL( db, COMMCSNAME, COMMCLNAME, 0, true, true, false, "create CL to export lob" );
   var lobNum = 1;
   toolPutLobs( expCl, lobfile, lobNum );
   cmd.run( "rm -rf " + lobfile );


   // 测试当--ignorefe = false时，报错 
   var errCode = -5;  // 迁移时源集合与目标集合相同时，错误码为-5

   try
   {
      var MigrationCmd = toolGetCmdstr( Args );
      cmd.run( MigrationCmd );
      println( ">success to migrate lob when --ignorefe is false, something wrong!!!!" );
   }
   catch( e )
   {
      var errNumber = getErr( e );
      if( errNumber == errCode )
         println( ">success to test sdblobtool migrate with same collection when --ignorefe is false, rc = " + errNumber );
      else
      {
         println( ">fail to test sdblobtool migrate with same collection when --ignorefe is false, rc = " + errNumber );
         throw errNumber;
      }
   }

   // 测试当--ignorefe = true时，忽略这个错误，不报错
   Args["ignorefe"] = true;
   try
   {
      MigrationCmd += " --ignorefe";
      cmd.run( MigrationCmd );
      println( ">success to test sdblobtool migrate with same collection when --ignorefe is true, no error" );
   }
   catch( e )
   {
      var errNumber = getErr( e );
      println( ">fail to test sdblobtool migrate with same collection when --ignorefe is true, rc = " + errNumber );
      throw errNumber;
   }
   println( ">success to test sbdlobtool migrate with Same Collection.\n\n" );
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
   cmd.run( "mv sdblobtool.log /tmp/lobtool/migrationSameCl.log" );
   throw e;
}
finally
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
      "clean collection in the end, wrong" );
   db.close();
}
