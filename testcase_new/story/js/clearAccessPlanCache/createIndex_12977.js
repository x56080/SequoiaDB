/************************************
*@Description:  创建Id索引更新统计信息 、清空缓存
*@author:      liuxiaoxuan
*@createdate:  2017.11.09
*@testlinkCase: seqDB-12977
**************************************/
function main ()
{
   var csName = COMMCSNAME + "12977";
   commDropCS( db, csName, true, "drop CS in the beginning" );

   //create cs
   var csOption = { PageSize: 4096 };
   commCreateCS( db, csName, false, "", csOption );

   //create cl
   var clName = COMMCLNAME + "12977";
   var clOption = { AutoIndexId: false };
   var dbcl = commCreateCL( db, csName, clName, clOption, true );

   var clFullName = csName + "." + clName;

   //get master/slave datanode
   var db1 = new Sdb( db );
   db1.setSessionAttr( { PreferedInstance: "m" } );
   var dbclPrimary = db1.getCS( csName ).getCL( clName );

   db1 = new Sdb( db );
   db1.setSessionAttr( { PreferedInstance: "s" } );
   var dbclSlave = db1.getCS( csName ).getCL( clName );

   //insert data
   var insertNums = 5000;
   var sameValues = 9000;

   insertDifferentDatas( dbcl, insertNums );
   insertSameDatas( dbcl, insertNums, sameValues );

   //invoke analyze
   var options = { CollectionSpace: csName };
   analyze( db, options );

   //检查主备同步
   checkConsistency( db, csName, clName );

   //check after analyze
   checkStat( db, csName, clName, "$id", true, false );
   checkStat( db, csName, clName, "a", true, false );

   //query from primary/slave node
   var findConf1 = { _id: 4000 };
   var findConf2 = { a: sameValues };

   query( dbclPrimary, findConf1, null, null, 1 );
   query( dbclPrimary, findConf2, null, null, insertNums );
   query( dbclSlave, findConf1, null, null, 1 );
   query( dbclSlave, findConf2, null, null, insertNums );

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };

   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [{ ScanType: "tbscan", IndexName: "" },
   { ScanType: "tbscan", IndexName: "" },
   { ScanType: "tbscan", IndexName: "" },
   { ScanType: "tbscan", IndexName: "" }];

   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );

   println( "check result after create id index!" );

   //create id index
   createIdIndex( dbcl );

   //检查主备同步
   checkConsistency( db, csName, clName );
   checkIndexExistsInAllNodes( csName, clName, "$id" );

   checkStat( db, csName, clName, "$id", true, false );
   checkStat( db, csName, clName, "a", true, false );

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };
   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [];
   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );

   //query from primary/slave node
   var findConf1 = { _id: 4000 };
   var findConf2 = { a: sameValues };

   query( dbclPrimary, findConf1, null, null, 1 );
   query( dbclPrimary, findConf2, null, null, insertNums );
   query( dbclSlave, findConf1, null, null, 1 );
   query( dbclSlave, findConf2, null, null, insertNums );

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };

   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [{ ScanType: "ixscan", IndexName: "$id" },
   { ScanType: "tbscan", IndexName: "" },
   { ScanType: "ixscan", IndexName: "$id" },
   { ScanType: "tbscan", IndexName: "" }];

   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );

   println( "check result after create id index!" );

   //create common index
   commCreateIndex( dbcl, "a", { a: 1 }, false );

   //检查主备同步
   checkConsistency( db, csName, clName );
   checkIndexExistsInAllNodes( csName, clName, "a" );

   checkStat( db, csName, clName, "$id", true, false );
   checkStat( db, csName, clName, "a", true, false );

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };
   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [];
   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );

   //query from primary/slave node
   var findConf1 = { _id: 4000 };
   var findConf2 = { a: sameValues };

   query( dbclPrimary, findConf1, null, null, 1 );
   query( dbclPrimary, findConf2, null, null, insertNums );
   query( dbclSlave, findConf1, null, null, 1 );
   query( dbclSlave, findConf2, null, null, insertNums );

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };

   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [{ ScanType: "ixscan", IndexName: "$id" },
   { ScanType: "ixscan", IndexName: "a" },
   { ScanType: "ixscan", IndexName: "$id" },
   { ScanType: "ixscan", IndexName: "a" }];

   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );

   println( "check result after create common index!" );

   //analyze after create id index
   var options = { CollectionSpace: csName };
   analyze( db, options );

   //检查主备同步
   checkConsistency( db, csName, clName );

   checkStat( db, csName, clName, "$id", true, true );
   checkStat( db, csName, clName, "a", true, true );

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };
   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [];
   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );

   //query from primary/slave node
   var findConf1 = { _id: 4000 };
   var findConf2 = { a: sameValues };

   query( dbclPrimary, findConf1, null, null, 1 );
   query( dbclPrimary, findConf2, null, null, insertNums );
   query( dbclSlave, findConf1, null, null, 1 );
   query( dbclSlave, findConf2, null, null, insertNums );

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };

   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [{ ScanType: "ixscan", IndexName: "$id" },
   { ScanType: "tbscan", IndexName: "" },
   { ScanType: "ixscan", IndexName: "$id" },
   { ScanType: "tbscan", IndexName: "" }];

   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );
   println( "check result after analyze success!" );

   //drop id index
   dropIdIndex( dbcl );

   //检查主备同步
   checkConsistency( db, csName, clName );

   //check analyze result
   checkStat( db, csName, clName, "$id", true, false );

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };
   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [];
   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );

   //query from primary/slave node
   var findConf = { _id: 4000 };

   query( dbclPrimary, findConf, null, null, 1 );
   query( dbclSlave, findConf, null, null, 1 );

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName };

   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [{ ScanType: "tbscan", IndexName: "" },
   { ScanType: "tbscan", IndexName: "" }];

   checkSnapShotAccessPlans( clFullName, expAccessPlans, actAccessPlans );

   println( "check result after drop id success!" );

   db1.close();
   commDropCS( db, csName, true, "drop CS in the end" );
}

function insertDifferentDatas ( dbcl, insertNum )
{
   try
   {
      var doc = [];
      for( var i = 0; i < insertNum; i++ )
      {
         doc.push( { _id: i, a: i } );
      }
      dbcl.insert( doc );
   }
   catch( e )
   {
      throw buildException( "insert datas", e, "insert", "insert success", e );
   }
}
function createIdIndex ( dbcl )
{
   try
   {
      dbcl.createIdIndex();
   }
   catch( e )
   {
      throw buildException( "create id index", e, "create IdIndex", "success", e );
   }
}

function dropIdIndex ( dbcl )
{
   try
   {
      dbcl.dropIdIndex();
   }
   catch( e )
   {
      throw buildException( "drop id index", e, "drop IdIndex", "success", e );
   }
}
main(); 
