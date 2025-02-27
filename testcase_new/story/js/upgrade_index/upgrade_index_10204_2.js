/******************************************************************************
@Description : SEQUOIADBMAINSTREAM-10204：功能测试
               测试步骤：
               1. 正常创建索引，并清空编目索引元数据；
               2. 执行 createIndex / createIdIndex，指定参数 { OnlyUpgradeMeta: true }；
               3. 查看集合主备节点 dataLSN 和 indexLSN 是否一致，编目索引元数据 UniqueID 和数据节点的是否一致，预期结果：检查信息全部一致
               4. 执行 2 步骤，预期结果：重复执行不报错；
               5. 执行 3 步骤，预期结果：检查信息全部一致；
               6. 执行 2 步骤，预期结果：重复执行不报错；
               7. 执行 3 步骤，预期结果：检查信息全部一致，且 UniqueID 跟上一次结果不一致，编目会生成新的 UniqueID；
               8. 以上步骤分别测试普通表，分区表和主子表
@Modify list :
               2025-02-19 fangjiabin  Init
******************************************************************************/

var csName = "upgrade_index_10204_2_cs" ;
var clName1 = "upgrade_index_10204_2_1_cl" ;
var clName2 = "upgrade_index_10204_2_2_cl" ;
var mainCL_Name = "upgrade_index_10204_2_main_cl" ;
var subCL_Name = "upgrade_index_10204_2_sub_cl" ;
var idxName1 = "cIdx" ;
var idxName2 = "dIdx" ;
var idxName3 = "$id" ;

main( test ) ;

function test()
{
   commDropCS( db, csName ) ;

   var cl1 = commCreateCL( db, csName, clName1, { "AutoIndexId": false } ) ;
   var cl2 = commCreateCL( db, csName, clName2, { "AutoIndexId": false, "AutoSplit": true, "ShardingKey": { "a": 1 } } ) ;
   var mainCLOption = { ShardingKey: { "a": 1 }, IsMainCL: true } ;
   var maincl = commCreateCL( db, csName, mainCL_Name, mainCLOption, true, true ) ;
   var subClOption = { ShardingKey: { "b": 1 }, AutoSplit: true, "AutoIndexId": false } ;
   var subcl = commCreateCL( db, csName, subCL_Name, subClOption, true, true ) ;
   var options = { LowBound: { a: 1 }, UpBound: { a: 100 } } ;
   maincl.attachCL( csName + "." + subCL_Name, options ) ;

   cl1.createIndex( idxName1, { "c": 1 } ) ;
   cl2.createIndex( idxName1, { "c": 1 } ) ;
   maincl.createIndex( idxName1, { "c": 1 } ) ;
   subcl.createIndex( idxName2, { "d": 1 } ) ;

   commCheckLSN( db ) ;

   testClusterClCreateIndex( 1, cl1, csName + "." + clName1, idxName1, { "c": 1 } ) ;
   testClusterClCreateIndex( 2, cl2, csName + "." + clName2, idxName1, { "c": 1 } ) ;
   testMainSubClCreateIndex( 3, maincl, subcl, csName + "." + subCL_Name, idxName1, { "c": 1 } ) ;
   testMainSubClCreateIndex( 4, maincl, subcl, csName + "." + subCL_Name, idxName2, { "d": 1 } ) ;

   testClusterClCreateIdIndex( 5, cl1, csName + "." + clName1, idxName3 ) ;
   testClusterClCreateIdIndex( 6, cl2, csName + "." + clName2, idxName3 ) ;
   testMainSubClCreateIdIndex( 7, maincl, subcl, csName + "." + subCL_Name, idxName3 ) ;
}

function clearCataIndexMetas( indexName, clearAll )
{
   clearCataIndexMeta( csName + "." + clName1, indexName, clearAll ) ;
   clearCataIndexMeta( csName + "." + clName2, indexName, clearAll ) ;
   clearCataIndexMeta( csName + "." + mainCL_Name, indexName, clearAll ) ;
   clearCataIndexMeta( csName + "." + subCL_Name, indexName, clearAll ) ;
}

function testClusterClCreateIndex( step, cl, clFullName, indexName, indexKey )
{
   var lastUniqueID = "" ;

   clearCataIndexMetas( indexName, true ) ;
   println( "Begin " + step ) ;
   cl.createIndex( indexName, indexKey, {}, { "OnlyUpgradeMeta": true } ) ;
   checkIndexInfo( cl, clFullName, indexName, false, 0 ) ;
   println( "Continue" + step ) ;
   cl.createIndex( indexName, indexKey, {}, { "OnlyUpgradeMeta": true } ) ;
   lastUniqueID = checkIndexInfo( cl, clFullName, indexName, false, 0 ) ;

   clearCataIndexMetas( indexName, true ) ;
   cl.createIndex( indexName, indexKey, {}, { "OnlyUpgradeMeta": true } ) ;
   checkIndexInfo( cl, clFullName, indexName, false, lastUniqueID ) ;
   println( "End " + step ) ;
}

function testMainSubClCreateIndex( step, maincl, subcl, subClFullName, indexName, indexKey )
{
   var lastUniqueID = "" ;

   clearCataIndexMetas( indexName, true ) ;
   println( "Begin " + step ) ;
   maincl.createIndex( indexName, indexKey, {}, { "OnlyUpgradeMeta": true } ) ;
   checkIndexInfo( subcl, subClFullName, indexName, false, 0 ) ;
   println( "Continue" + step ) ;
   maincl.createIndex( indexName, indexKey, {}, { "OnlyUpgradeMeta": true } ) ;
   lastUniqueID = checkIndexInfo( subcl, subClFullName, indexName, false, 0 ) ;

   clearCataIndexMetas( indexName, true ) ;
   maincl.createIndex( indexName, indexKey, {}, { "OnlyUpgradeMeta": true } ) ;
   checkIndexInfo( subcl, subClFullName, indexName, false, lastUniqueID ) ;
   println( "End " + step ) ;
}

function testClusterClCreateIdIndex( step, cl, clFullName, indexName )
{
   var lastUniqueID = "" ;

   clearCataIndexMetas( indexName, true ) ;
   println( "Begin " + step ) ;
   cl.createIdIndex( { "OnlyUpgradeMeta": true } ) ;
   checkIndexInfo( cl, clFullName, indexName, false, 0 ) ;
   println( "Continue" + step ) ;
   cl.createIdIndex( { "OnlyUpgradeMeta": true } ) ;
   lastUniqueID = checkIndexInfo( cl, clFullName, indexName, false, 0 ) ;

   clearCataIndexMetas( indexName, true ) ;
   cl.createIdIndex( { "OnlyUpgradeMeta": true } ) ;
   checkIndexInfo( cl, clFullName, indexName, false, lastUniqueID ) ;
   println( "End " + step ) ;
}

function testMainSubClCreateIdIndex( step, maincl, subcl, subClFullName, indexName )
{
   var lastUniqueID = "" ;

   clearCataIndexMetas( indexName, true ) ;
   println( "Begin " + step ) ;
   maincl.createIdIndex( { "OnlyUpgradeMeta": true } ) ;
   checkIndexInfo( subcl, subClFullName, indexName, false, 0 ) ;
   println( "Continue" + step ) ;
   maincl.createIdIndex( { "OnlyUpgradeMeta": true } ) ;
   lastUniqueID = checkIndexInfo( subcl, subClFullName, indexName, false, 0 ) ;

   clearCataIndexMetas( indexName, true ) ;
   maincl.createIdIndex( { "OnlyUpgradeMeta": true } ) ;
   checkIndexInfo( subcl, subClFullName, indexName, false, lastUniqueID ) ;
   println( "End " + step ) ;
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
         throw new Error( "cata index UniqueID != data index UniqueID, cata index[" +
                          JSON.stringify( cataIdx ) + "], data index[" +
                          JSON.stringify( dataIdx ) + "]" ) ;
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
