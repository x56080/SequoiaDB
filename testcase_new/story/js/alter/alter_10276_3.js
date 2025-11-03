/******************************************************************************
 * @Description   : EnsureShardingIndex
 * @Author        : fangjiabin
 * @CreateTime    : 2025.09.12
 * @LastEditTime  : 2025.09.12
 * @LastEditors   : fangjiabin
 ******************************************************************************/
testConf.skipStandAlone = true;

var csName = "cs_ensureshardidx_10276_3";

main( test );

function test ()
{
   commDropCS( db, csName );
   commCreateCS( db, csName );

   testMainAndNormalCl();

   commDropCS( db, csName );
}

function checkCataInfo( clName, shardingKeyDef, ensureShardingIndex, shardingType, uniqueID, cataInfo )
{
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "ShardingKey", shardingKeyDef );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "EnsureShardingIndex", ensureShardingIndex );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "ShardingType", shardingType );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "UniqueID", uniqueID );
   if ( undefined != cataInfo )
   {
      checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "CataInfo", cataInfo );
   }
}

function checkMainCL( clName, shardingKeyDef, mustHasIndex, ensureShardingIndex, uniqueID, cataInfo )
{
   var cl = db.getCS( csName ).getCL( clName );
   checkCataInfo( clName, shardingKeyDef, ensureShardingIndex, "range", uniqueID, cataInfo );
   checkIndexExist( cl, shardingKeyDef, mustHasIndex );
}

function checkSubCL( clName, uniqueID, cataInfo )
{
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "UniqueID", uniqueID );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "CataInfo", cataInfo );
}

// 主子表，子表为普通表
function testMainAndNormalCl()
{
   var mainclName = "maincl1";
   var subCLName1 = "subcl1";
   var subCLName2 = "subcl2";

   var clFullName1 = csName + "." + mainclName;
   var clFullName2 = csName + "." + subCLName1;
   var clFullName3 = csName + "." + subCLName2;

   commDropCL( db, csName, subCLName1 );
   commDropCL( db, csName, subCLName2 );
   commDropCL( db, csName, mainclName );
   var maincl = commCreateCL( db, csName, mainclName, { ShardingKey:{ a: 1 }, ShardingType: "range", IsMainCL: true, ReplSize: -1 } );
   commCreateCL( db, csName, subCLName1, { ReplSize: -1 } );
   commCreateCL( db, csName, subCLName2, { ReplSize: -1 } );

   maincl.attachCL( csName + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 10000 } } );
   maincl.attachCL( csName + "." + subCLName2, { LowBound: { a: 10001 }, UpBound: { a: 20000 } } );

   var recArray = [];
   for (var i = 0; i < 10; i++) {
      recArray.push( { a: i, b: i, c: i } );
   }
   maincl.insert( recArray );

   var cursor = db.snapshot( 8, { Name: clFullName1 } );
   var uniqueID1 = cursor.current().toObj().UniqueID;
   var cataInfo1 = cursor.current().toObj().CataInfo;

   cursor = db.snapshot( 8, { Name: clFullName2 } );
   var uniqueID2 = cursor.current().toObj().UniqueID;
   var cataInfo2 = cursor.current().toObj().CataInfo;

   cursor = db.snapshot( 8, { Name: clFullName3 } );
   var uniqueID3 = cursor.current().toObj().UniqueID;
   var cataInfo3 = cursor.current().toObj().CataInfo;

   assert.tryThrow( SDB_OPTION_NOT_SUPPORT, function()
   {
      maincl.alter( { ShardingKey: { b: 1 }, EnsureShardingIndex: true } );
   } );
   assert.tryThrow( SDB_OPTION_NOT_SUPPORT, function()
   {
      maincl.alter( { ShardingKey: { b: 1 }, EnsureShardingIndex: false } );
   } );
   assert.tryThrow( SDB_OPTION_NOT_SUPPORT, function()
   {
      maincl.alter( { ShardingKey: { b: 1 } } );
   } );
   assert.tryThrow( SDB_NO_SHARDINGKEY, function()
   {
      maincl.alter( { EnsureShardingIndex: false } );
   } );
   assert.tryThrow( SDB_NO_SHARDINGKEY, function()
   {
      maincl.alter( { EnsureShardingIndex: true } );
   } );

   maincl.alter( { ShardingKey: { a: 1 } } );
   checkCataInfo( mainclName, { a: 1 }, true, "range", uniqueID1, cataInfo1 );
   checkCataInfo( subCLName1, { a: 1 }, true, "hash", uniqueID2, undefined );
   checkCataInfo( subCLName2, { a: 1 }, true, "hash", uniqueID3, undefined );

   cursor = db.snapshot( 8, { Name: clFullName2 } );
   uniqueID2 = cursor.current().toObj().UniqueID;
   cataInfo2 = cursor.current().toObj().CataInfo;

   cursor = db.snapshot( 8, { Name: clFullName3 } );
   uniqueID3 = cursor.current().toObj().UniqueID;
   cataInfo3 = cursor.current().toObj().CataInfo;

   maincl.alter( { ShardingKey: { a: 1 }, EnsureShardingIndex: true } );
   checkCataInfo( mainclName, { a: 1 }, true, "range", uniqueID1, cataInfo1 );
   checkCataInfo( subCLName1, { a: 1 }, true, "hash", uniqueID2, cataInfo2 );
   checkCataInfo( subCLName2, { a: 1 }, true, "hash", uniqueID3, cataInfo3 );

   maincl.alter( { ShardingKey: { a: 1 }, EnsureShardingIndex: false } );
   checkCataInfo( mainclName, { a: 1 }, false, "range", uniqueID1, cataInfo1 );
   checkCataInfo( subCLName1, { a: 1 }, false, "hash", uniqueID2, cataInfo2 );
   checkCataInfo( subCLName2, { a: 1 }, false, "hash", uniqueID3, cataInfo3 );
}