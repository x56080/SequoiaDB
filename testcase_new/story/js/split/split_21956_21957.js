/************************************
*@description：seqDB-21956:分区表，包含非分区键查询
               seqDB-21957:分区表，不带匹配符精确匹配 
*@author ：2020-3-31 liyuanyue
**************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;

main( test );

function test ()
{
   var dataGroupNames = commGetDataGroupNames( db );
   dataGroupNames.sort();
   var csName = COMMCSNAME;

   var recsNum = 200;
   var docs = [];
   for( var i = 0; i < recsNum; i++ )
   {
      docs.push( { "a": i, "b": i } );
   }

   testHash( csName, dataGroupNames, docs );
   testRange( csName, dataGroupNames, docs );
   testSubCL( csName, dataGroupNames, docs )
}
function testRange ( csName, dataGroupNames, docs )
{
   var rangeclName = CHANGEDPREFIX + "_rangecl_21956_21957";
   commDropCL( db, csName, rangeclName );
   var rangecl = commCreateCL( db, csName, rangeclName, { ShardingKey: { "a": 1 }, ShardingType: "range", Group: dataGroupNames[0] }, false );

   rangecl.insert( docs );
   rangecl.split( dataGroupNames[0], dataGroupNames[1], 50 );

   // range表非分区键查询
   var findCond = { "b": { "$gte": 100 } };
   commCompareResults( rangecl.find( findCond ).sort( { "a": 1 } ), docs.slice( 100 ) );
   checkHitDataGroups( rangecl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[0], dataGroupNames[1]] );

   // range表分区键+非分区键查询
   var findCond = { "b": 100, "a": 100 };
   commCompareResults( rangecl.find( findCond ), docs.slice( 100, 101 ) );
   checkHitDataGroups( rangecl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[1]] );

   // range表不带匹配符精确匹配
   var findCond = { "a": 100 };
   commCompareResults( rangecl.find( findCond ), docs.slice( 100, 101 ) );
   checkHitDataGroups( rangecl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[1]] );

   commDropCL( db, csName, rangeclName, false );
}

function testHash ( csName, dataGroupNames, docs )
{
   var hashclName = CHANGEDPREFIX + "_hashcl_21956_21957";

   commDropCL( db, csName, hashclName );

   var hashcl = commCreateCL( db, csName, hashclName, { ShardingKey: { "a": 1 }, ShardingType: "range", Group: dataGroupNames[0] }, false );

   hashcl.insert( docs );
   hashcl.split( dataGroupNames[0], dataGroupNames[1], 50 );

   // hash表非分区键查询
   var findCond = { "b": { "$gte": 100 } };
   commCompareResults( hashcl.find( findCond ).sort( { "a": 1 } ), docs.slice( 100 ) );
   checkHitDataGroups( hashcl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[0], dataGroupNames[1]] );

   // hash表分区键+非分区键查询
   var findCond = { "b": 100, "a": 100 };
   commCompareResults( hashcl.find( findCond ), docs.slice( 100, 101 ) );
   checkHitDataGroups( hashcl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[1]] );

   // hash表不带匹配符精确匹配
   var findCond = { "a": 100 };
   commCompareResults( hashcl.find( findCond ), docs.slice( 100, 101 ) );
   checkHitDataGroups( hashcl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[1]] );

   commDropCL( db, csName, hashclName, false );
}
function testSubCL ( csName, dataGroupNames, docs )
{
   var mclName = CHANGEDPREFIX + "_mcl_21956_21957";
   var sclName1 = CHANGEDPREFIX + "_scl_21956_21957_1";
   var sclName2 = CHANGEDPREFIX + "_scl_21956_21957_2";
   var sclFullName1 = csName + "." + sclName1;
   var sclFullName2 = csName + "." + sclName2;

   commDropCL( db, csName, mclName );
   commDropCL( db, csName, sclName1 );
   commDropCL( db, csName, sclName2 );

   var mcl = commCreateCL( db, csName, mclName, { ShardingKey: { "a": 1 }, IsMainCL: true }, false );
   var scl1 = commCreateCL( db, csName, sclName1, { ShardingKey: { "a": 1 }, "ShardingType": "range", Group: dataGroupNames[0] }, false );
   var scl2 = commCreateCL( db, csName, sclName2, { ShardingKey: { "a": 1 }, "ShardingType": "range", Group: dataGroupNames[0] }, false );

   mcl.attachCL( sclFullName1, { LowBound: { "a": 0 }, UpBound: { "a": 100 } } );
   mcl.attachCL( sclFullName2, { LowBound: { "a": 100 }, UpBound: { "a": 200 } } );

   mcl.insert( docs );

   // scl1 [min,50) [50,max)
   // scl2 [min,150) [150,max)
   scl1.split( dataGroupNames[0], dataGroupNames[1], 50 );
   scl2.split( dataGroupNames[0], dataGroupNames[1], 50 );

   // 主子表非分区键查询
   var findCond = { "b": { "$gte": 150 } };
   commCompareResults( mcl.find( findCond ).sort( { "a": 1 } ), docs.slice( 150 ) );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[0], dataGroupNames[1]], true, [[sclFullName2, sclFullName1], [sclFullName2, sclFullName1]] );

   // 主子表分区键+非分区键查询
   var findCond = { "b": 150, "a": 150 };
   commCompareResults( mcl.find( findCond ), docs.slice( 150, 151 ) );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[1]], true, [[sclFullName2]] );

   // 主子表不带匹配符精确匹配
   var findCond = { "a": 50 };
   commCompareResults( mcl.find( findCond ), docs.slice( 50, 51 ) );
   checkHitDataGroups( mcl.find( findCond ).explain( { "Run": true } ), [dataGroupNames[1]], true, [[sclFullName1]] );

   commDropCL( db, csName, mclName, false );
}