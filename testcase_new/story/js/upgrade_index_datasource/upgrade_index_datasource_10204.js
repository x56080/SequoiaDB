/******************************************************************************
@Description : SEQUOIADBMAINSTREAM-10204：功能测试(数据源场景)
               测试步骤：
               1. 创建数据源，ErrorControlLevel 为默认值 low
               2. 创建映射表：主子表，两个子表，其中一个子表映射数据源普通表
               3. 主表正常创建索引，并清空编目索引元数据
               4. 执行 createIndex ，指定参数 { OnlyUpgradeMeta: true }
               5. 查看集合主备节点 dataLSN 和 indexLSN 是否一致，编目索引元数据 UniqueID 和数据节点的是否一致，预期结果：检查信息全部一致
               6. 创建数据源，ErrorControlLevel 为默认值 high，重复 2 - 4 步骤，预期结果：第 4 步骤会报错

               7. 创建数据源，ErrorControlLevel 为默认值 low
               8. 创建映射表：主子表，一个子表，子表映射数据源普通表
               9. 主表正常创建索引，并清空编目索引元数据
               10. 执行 createIndex ，指定参数 { OnlyUpgradeMeta: true }
               11. snapshotIndexes 查看是否有索引数据，预期结果：没有索引数据
               12. 创建数据源，ErrorControlLevel 为默认值 high，重复 8 - 10 步骤，预期结果：第 10 步骤会报错 -315

               13. 创建数据源，ErrorControlLevel 为默认值 low
               14. 创建映射表：一个普通表，映射数据源普通表
               15. 表正常创建索引，并清空编目索引元数据
               16. 执行 createIndex ，指定参数 { OnlyUpgradeMeta: true }
               17. snapshotIndexes 查看是否有索引数据，预期结果：没有索引数据
               18. 创建数据源，ErrorControlLevel 为默认值 high，重复 14 - 16 步骤，预期结果：第 16 步骤会报错 -315

               19. 创建数据源，ErrorControlLevel 为默认值 low
               20. 创建映射表：主子表，两个子表，其中一个子表映射数据源普通表
               21. 执行 createIdIndex ，指定参数 { OnlyUpgradeMeta: true }
               22. 查看集合主备节点非映射子表 dataLSN 和 indexLSN 是否一致，编目索引元数据 UniqueID 和数据节点的是否一致，预期结果：检查信息全部一致
               23. 创建数据源，ErrorControlLevel 为默认值 high，重复 20 - 21 步骤，预期结果：检查信息全部一致，不报错 -315
@Modify list :
               2025-02-26 fangjiabin  Init
******************************************************************************/

var datasrcIp = DSHOSTNAME;
var datasrcPort = DSSVCNAME;
var userName = "sdbadmin";
var passwd = "sdbadmin";
var datasrcUrl = datasrcIp + ":" + datasrcPort;
var datasrcDB;

//main( test ) ;

function test()
{
   datasrcDB = new Sdb( datasrcIp, datasrcPort, userName, passwd );

   test1() ;

   test2() ;

   test3() ;

   test4() ;

   test5() ;

   test6() ;

   test7() ;

   test8() ;
}

function test7()
{
   var dataSrcName = "datasrc10204";
   var csName = "cs_10204";
   var srcCSName = "datasrcCS_10204";
   var clName = "cl_10204";
   var mainCLName = "mainCL_10204";
   var subCLName = "subCL_10204";
   var idxName = "$id";

   commDropCS( datasrcDB, srcCSName );
   commDropCS( db, csName );
   clearDataSource( csName, dataSrcName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );

   // 主子表
   var srccl = commCreateCL( datasrcDB, srcCSName, clName );
   var dbcs = commCreateCS( db, csName );
   var maincl = commCreateCL( db, csName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   var subcl = commCreateCL( db, csName, subCLName, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );

   maincl.attachCL( csName + "." + subCLName, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );
   maincl.attachCL( csName + "." + clName, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );

   commCheckLSN( db );

   clearCataIndexMeta( csName + "." + subCLName, idxName, true );
   maincl.createIdIndex( { "OnlyUpgradeMeta": true } );
   checkIndexInfo( subcl, csName + "." + subCLName, idxName, false, 0 );
}

function test8()
{
   var dataSrcName = "datasrc10204";
   var csName = "cs_10204";
   var srcCSName = "datasrcCS_10204";
   var clName = "cl_10204";
   var mainCLName = "mainCL_10204";
   var subCLName = "subCL_10204";
   var idxName = "$id";

   commDropCS( datasrcDB, srcCSName );
   commDropCS( db, csName );
   clearDataSource( csName, dataSrcName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd, "SequoiaDB", { ErrorControlLevel: "high" } );

   // 主子表
   var srccl = commCreateCL( datasrcDB, srcCSName, clName );
   var dbcs = commCreateCS( db, csName );
   var maincl = commCreateCL( db, csName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   var subcl = commCreateCL( db, csName, subCLName, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );

   maincl.attachCL( csName + "." + subCLName, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );
   maincl.attachCL( csName + "." + clName, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );

   commCheckLSN( db ) ;

   clearCataIndexMeta( csName + "." + subCLName, idxName, true ) ;
   maincl.createIdIndex( { "OnlyUpgradeMeta": true } );
   checkIndexInfo( subcl, csName + "." + subCLName, idxName, false, 0 ) ;
}

function test1()
{
   var dataSrcName = "datasrc10204";
   var csName = "cs_10204";
   var srcCSName = "datasrcCS_10204";
   var clName = "cl_10204";
   var mainCLName = "mainCL_10204";
   var subCLName = "subCL_10204";
   var idxName = "index_10204";

   commDropCS( datasrcDB, srcCSName );
   commDropCS( db, csName );
   clearDataSource( csName, dataSrcName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );

   // 主子表
   var srccl = commCreateCL( datasrcDB, srcCSName, clName );
   var dbcs = commCreateCS( db, csName );
   var maincl = commCreateCL( db, csName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   var subcl = commCreateCL( db, csName, subCLName, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );

   maincl.attachCL( csName + "." + subCLName, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );
   maincl.attachCL( csName + "." + clName, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );

   maincl.createIndex( idxName, { c: 1 } );

   commCheckLSN( db ) ;

   clearCataIndexMeta( csName + "." + mainCLName, idxName, true ) ;
   clearCataIndexMeta( csName + "." + subCLName, idxName, true ) ;
   maincl.createIndex( idxName, { c: 1 }, {}, { "OnlyUpgradeMeta": true } );
   checkIndexInfo( subcl, csName + "." + subCLName, idxName, false, 0 ) ;
}

function test2()
{
   var dataSrcName = "datasrc10204";
   var csName = "cs_10204";
   var srcCSName = "datasrcCS_10204";
   var clName = "cl_10204";
   var mainCLName = "mainCL_10204";
   var subCLName = "subCL_10204";
   var idxName = "index_10204";

   commDropCS( datasrcDB, srcCSName );
   commDropCS( db, csName );
   clearDataSource( csName, dataSrcName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd, "SequoiaDB", { ErrorControlLevel: "high" } );

   // 主子表
   var srccl = commCreateCL( datasrcDB, srcCSName, clName );
   var dbcs = commCreateCS( db, csName );
   var maincl = commCreateCL( db, csName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   var subcl = commCreateCL( db, csName, subCLName, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );

   maincl.attachCL( csName + "." + subCLName, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );
   maincl.attachCL( csName + "." + clName, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );

   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      maincl.createIndex( idxName, { c: 1 }, {}, { "OnlyUpgradeMeta": true } );
   } );
}

function test3()
{
   var dataSrcName = "datasrc10204";
   var csName = "cs_10204";
   var srcCSName = "datasrcCS_10204";
   var clName = "cl_10204";
   var mainCLName = "mainCL_10204";
   var idxName = "index_10204";

   commDropCS( datasrcDB, srcCSName );
   commDropCS( db, csName );
   clearDataSource( csName, dataSrcName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );

   // 主子表
   var srccl = commCreateCL( datasrcDB, srcCSName, clName );
   var dbcs = commCreateCS( db, csName );
   var maincl = commCreateCL( db, csName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   maincl.attachCL( csName + "." + clName, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );

   maincl.createIndex( idxName, { c: 1 } );

   commCheckLSN( db ) ;

   clearCataIndexMeta( csName + "." + mainCLName, idxName, true ) ;
   maincl.createIndex( idxName, { c: 1 }, {}, { "OnlyUpgradeMeta": true } );
   checkLSN( csName + "." + clName ) ;
   var rc = maincl.snapshotIndexes() ;
   while( rc.next() )
   {
      throw new Error( "Expect no index data: " + JSON.stringify( rc.current().toObj() ) ) ;
   }
}

function test4()
{
   var dataSrcName = "datasrc10204";
   var csName = "cs_10204";
   var srcCSName = "datasrcCS_10204";
   var clName = "cl_10204";
   var mainCLName = "mainCL_10204";
   var idxName = "index_10204";

   commDropCS( datasrcDB, srcCSName );
   commDropCS( db, csName );
   clearDataSource( csName, dataSrcName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd, "SequoiaDB", { ErrorControlLevel: "high" } );

   // 主子表
   var srccl = commCreateCL( datasrcDB, srcCSName, clName );
   var dbcs = commCreateCS( db, csName );
   var maincl = commCreateCL( db, csName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   maincl.attachCL( csName + "." + clName, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );

   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      maincl.createIndex( idxName, { c: 1 }, {}, { "OnlyUpgradeMeta": true } );
   } );
}

function test5()
{
   var dataSrcName = "datasrc10204";
   var csName = "cs_10204";
   var srcCSName = "datasrcCS_10204";
   var clName = "cl_10204";
   var idxName = "index_10204";

   commDropCS( datasrcDB, srcCSName );
   commDropCS( db, csName );
   clearDataSource( csName, dataSrcName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );

   var srccl = commCreateCL( datasrcDB, srcCSName, clName );
   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );

   dbcl.createIndex( idxName, { c: 1 } );

   commCheckLSN( db ) ;

   clearCataIndexMeta( csName + "." + clName, idxName, true ) ;
   dbcl.createIndex( idxName, { c: 1 }, {}, { "OnlyUpgradeMeta": true } );
   checkLSN( csName + "." + clName ) ;
   var rc = dbcl.snapshotIndexes() ;
   while( rc.next() )
   {
      throw new Error( "Expect no index data: " + JSON.stringify( rc.current().toObj() ) ) ;
   }
}

function test6()
{
   var dataSrcName = "datasrc10204";
   var csName = "cs_10204";
   var srcCSName = "datasrcCS_10204";
   var clName = "cl_10204";
   var idxName = "index_10204";

   commDropCS( datasrcDB, srcCSName );
   commDropCS( db, csName );
   clearDataSource( csName, dataSrcName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd, "SequoiaDB", { ErrorControlLevel: "high" } );

   var srccl = commCreateCL( datasrcDB, srcCSName, clName );
   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );

   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      dbcl.createIndex( idxName, { c: 1 }, {}, { "OnlyUpgradeMeta": true } );
   } );
}

function checkLSN( clFullName )
{
   var dataCommitLSN = "" ;
   var indexCommitLSN = "" ;
   var groups = commGetGroups( db, true ) ;
   for ( var i = 0 ; i < groups.length ; i++ )
   {
      var group = groups[i] ;
      var groupName = group[0].GroupName ;
      var t1_sqlStr1 = "select * from $SNAPSHOT_CL where Name='" + clFullName + "' split by Details" ;
      var t2_sqlStr1 = "select T1.Name, T1.Details.DataCommitLSN, T1.Details.IndexCommitLSN, T1.Details.NodeName from ( " + t1_sqlStr1 + " ) as T1 where T1.Details.GroupName='" + groupName + "'" ;

      var isFirst = true ;
      var firstObj = "" ;
      var rc = db.exec( t2_sqlStr1 ) ;
      println( t2_sqlStr1 ) ;
      while ( rc.next() )
      {
         var obj = rc.current().toObj() ;
         println( JSON.stringify( obj ) ) ;

         if ( isFirst )
         {
            dataCommitLSN = obj.DataCommitLSN ;
            indexCommitLSN = obj.IndexCommitLSN ;
            firstObj = obj ;
            isFirst = false ;
            continue ;
         }

         println( JSON.stringify( { "DataCommitLSN": dataCommitLSN, "IndexCommitLSN": indexCommitLSN } ) ) ;

         if ( dataCommitLSN != obj.DataCommitLSN ||
              indexCommitLSN != obj.IndexCommitLSN )
         {
            println( "Diff info: \nexpect: " + JSON.stringify( firstObj ) + ", \nbut found: " + JSON.stringify( obj ) ) ;
            return false ;
         }
      }
   }
   return true ;
}

function checkIndexInfo( cl, clFullName, indexName, isMaincl, uniqueID )
{
   try
   {
      // 1. check LSN
      // 2. check index meta
      var cataIdxUniqueID = "" ;
      var dataIdxUniqueID = "" ;
      var dataIdx = "" ;
      var cataIdx = "" ;
      var retryTime = 5 ;

      if ( isMaincl )
      {
         return ;
      }

      db.sync() ;

      while( true )
      {
         if ( checkLSN( clFullName ) )
         {
            break ;
         }
         else
         {
            if ( 0 == retryTime )
            {
               throw new Error( "Diff LSN" ) ;
            }
            sleep( 2000 ) ;
            retryTime-- ;
         }
      }

      var t1_sqlStr2 = "select * from $LIST_INDEXES where Collection='" + clFullName + "' and Name='" + indexName + "'"
      println( t1_sqlStr2 ) ;
      rc = db.exec( t1_sqlStr2 ) ;
      if ( rc.next() )
      {
         cataIdx = rc.current().toObj() ;
         cataIdxUniqueID = cataIdx.IndexDef.UniqueID ;
      }
      else
      {
         throw new Error( "Can't find cata index data[clFullName:'" + clFullName + "', indexName:'" + indexName + "']" ) ;
      }

      rc = cl.snapshotIndexes( { "IndexDef.name": indexName } )
      if ( rc.next() )
      {
         dataIdx = rc.current().toObj() ;
         dataIdxUniqueID = dataIdx.IndexDef.UniqueID
      }
      else
      {
         throw new Error( "Can't find data index data[clFullName:'" + clFullName + "', indexName:'" + indexName + "']" ) ;
      }

      if ( cataIdxUniqueID != dataIdxUniqueID )
      {
         println( "cata index[" + JSON.stringify( cataIdx ) + "]" ) ;
         println( "data index[" + JSON.stringify( dataIdx ) + "]" ) ;
         throw new Error( "cata index UniqueID != data index UniqueID" ) ;
      }
      else
      {
         println( "cataIdxUniqueID: " + cataIdxUniqueID + ", dataIdxUniqueID: " + dataIdxUniqueID ) ;
      }

      if ( 0 != uniqueID && uniqueID == cataIdxUniqueID )
      {
         throw new Error( "Can't generate new uniqueID, cata index[" +
                          JSON.stringify( cataIdx ) + "], data index[" +
                          JSON.stringify( dataIdx ) + "]" ) ;
      }

      return cataIdxUniqueID ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         print( "Error Stack:\n" + e.stack ) ;
      }
      throw e ;
   }
}