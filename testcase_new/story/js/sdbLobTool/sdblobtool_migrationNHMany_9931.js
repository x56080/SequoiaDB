/******************************************************************************
*@Description : test sdblobtool migration between normal collection and hash 
                collection with many lobs
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

   var srcFullCL = COMMCSNAME + "." + COMMCLNAME;       // 源集合 COMMCSNAME = CHANGEDPREFIX + "_cs" , COMMCLNAME = CHANGEDPREFIX + "_cl"
   var dstClName = CHANGEDPREFIX + "_newcl";
   var dstFullCL = COMMCSNAME + "." + dstClName;        // 目标集合


   // sdblobtool 迁移的选项参数
   var Args = {};
   Args["hostname"] = COORDHOSTNAME;         // 'localhost'
   Args["svcname"] = COORDSVCNAME;           // 11810
   Args["usrname"] = null;
   Args["passwd"] = null;
   Args["operation"] = "migration";
   Args["collection"] = srcFullCL;
   Args["ssl"] = false;                      // 使用SSL连接，如果不使用SSL连接，shell命令中不应加入该选项参数
   Args["ignorefe"] = false;                 // 当前大对象已经存在于集合中，忽略这个错误
   Args["dsthost"] = COORDHOSTNAME;
   Args["dstservice"] = COORDSVCNAME;
   Args["dstusrname"] = null;
   Args["dstpasswd"] = null;
   Args["dstcollection"] = dstFullCL;

   try
   {
      // 创建包含大对象的源集合（普通集合）
      var lobfile = toolMakeLobfile();
      var srcCl = commCreateCL( db, COMMCSNAME, COMMCLNAME, 0, true, true, false,
         "create normal source collection" );
      var lobNum = 5;
      var OID = toolPutLobs( srcCl, lobfile, lobNum );

      // 创建目标集合并迁移大对象（分区集合）
      var dstCl = commCreateCLByOption( db, COMMCSNAME, dstClName,
         { ReplSize: 0, "ShardingKey": { "OID": 1 }, "ShardingType": "hash", "Partition": 2048 },
         true, true, true, "create hash destnation collection" );
      toolMigration( Args );

      // 检验迁移大对象的条数是否正确
      toolCheckLob( dstCl, lobNum, OID );

      println( ">success to test migration NH with many lobs.\n\n" );
   }
   catch( e )
   {
      println( ">fail to test migration NHMany,rc = " + e );
      throw e;
   }
   finally
   {
      cmd.run( "rm -rf " + lobfile );
      commDropCL( db, COMMCSNAME, dstClName, true, true, "clean collection in the end." );
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
   cmd.run( "mv sdblobtool.log /tmp/lobtool/migrationNHMany.log" );
   throw e;
}
finally
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
      "clean collection in the end, wrong" );
   db.close();
}


