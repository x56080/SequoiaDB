/******************************************************************************
 * @Description   : EnsureShardingIndex
 * @Author        : fangjiabin
 * @CreateTime    : 2025.09.12
 * @LastEditTime  : 2025.09.12
 * @LastEditors   : fangjiabin
 ******************************************************************************/
testConf.skipStandAlone = true;

var csName = "cs_ensureshardidx_10276_4";

main( test );

function test ()
{
   commDropCS( db, csName );
   commCreateCS( db, csName );

   testMainAndShardCl( false );

   testMainAndShardCl( true );

   commDropCS( db, csName );
}

function check( clName, isMacinCL, shardingKeyDef, mustHasIndex, ensureShardingIndex, uniqueID, cataInfo )
{
   var cl = db.getCS( csName ).getCL( clName );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "ShardingKey", shardingKeyDef );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "EnsureShardingIndex", ensureShardingIndex );
   if ( isMacinCL )
   {
      checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "ShardingType", "range" );
   }
   else
   {
      checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "AutoSplit", true );
      checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "ShardingType", "hash" );
      checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "Partition", 4096 );
   }
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "UniqueID", uniqueID );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "CataInfo", cataInfo );
   checkIndexExist( cl, shardingKeyDef, mustHasIndex );
}

// 主子表，子表为分区表，主子表分区键不一样
function testMainAndShardCl( sameKey )
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

   if ( sameKey )
   {
      commCreateCL( db, csName, subCLName1, { AutoSplit: true, ShardingKey:{ a: 1 }, ReplSize: -1 } );
      commCreateCL( db, csName, subCLName2, { AutoSplit: true, ShardingKey:{ a: 1 }, ReplSize: -1 } );
   }
   else
   {
      commCreateCL( db, csName, subCLName1, { AutoSplit: true, ShardingKey:{ b: 1 }, ReplSize: -1 } );
      commCreateCL( db, csName, subCLName2, { AutoSplit: true, ShardingKey:{ b: 1 }, ReplSize: -1 } );
   }

   maincl.attachCL( csName + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 2048 } } );
   maincl.attachCL( csName + "." + subCLName2, { LowBound: { a: 2048 }, UpBound: { a: 4096 } } );

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

   if ( sameKey )
   {
      maincl.alter( { ShardingKey: { a: 1 } } );
      check( subCLName1, false, { a: 1 }, true, true, uniqueID2, cataInfo2 );
      check( subCLName2, false, { a: 1 }, true, true, uniqueID3, cataInfo3 );

      maincl.alter( { ShardingKey: { a: 1 }, EnsureShardingIndex: false } );
      check( mainclName, true, { a: 1 }, false, false, uniqueID1, cataInfo1 );
      check( subCLName1, false, { a: 1 }, false, false, uniqueID2, cataInfo2 );
      check( subCLName2, false, { a: 1 }, false, false, uniqueID3, cataInfo3 );

      maincl.alter( { ShardingKey: { a: 1 }, EnsureShardingIndex: true } );
      check( subCLName1, false, { a: 1 }, true, true, uniqueID2, cataInfo2 );
      check( subCLName2, false, { a: 1 }, true, true, uniqueID3, cataInfo3 );

      maincl.alter( { EnsureShardingIndex: false } );
      check( mainclName, true, { a: 1 }, false, false, uniqueID1, cataInfo1 );
      check( subCLName1, false, { a: 1 }, false, false, uniqueID2, cataInfo2 );
      check( subCLName2, false, { a: 1 }, false, false, uniqueID3, cataInfo3 );

      maincl.alter( { EnsureShardingIndex: true } );
      check( subCLName1, false, { a: 1 }, true, true, uniqueID2, cataInfo2 );
      check( subCLName2, false, { a: 1 }, true, true, uniqueID3, cataInfo3 );
   }
   else
   {
      assert.tryThrow( SDB_OPTION_NOT_SUPPORT, function()
      {
         maincl.alter( { ShardingKey: { a: 1 }, EnsureShardingIndex: true } );
      } );
      assert.tryThrow( SDB_OPTION_NOT_SUPPORT, function()
      {
         maincl.alter( { ShardingKey: { a: 1 }, EnsureShardingIndex: false } );
      } );
      assert.tryThrow( SDB_OPTION_NOT_SUPPORT, function()
      {
         maincl.alter( { ShardingKey: { a: 1 } } );
      } );

      maincl.alter( { EnsureShardingIndex: false } );
      check( mainclName, true, { a: 1 }, false, false, uniqueID1, cataInfo1 );
      check( subCLName1, false, { b: 1 }, false, false, uniqueID2, cataInfo2 );
      check( subCLName2, false, { b: 1 }, false, false, uniqueID3, cataInfo3 );

      maincl.alter( { EnsureShardingIndex: true } );
      check( subCLName1, false, { b: 1 }, true, true, uniqueID2, cataInfo2 );
      check( subCLName2, false, { b: 1 }, true, true, uniqueID3, cataInfo3 );
   }

   commDropCL( db, csName, mainclName );
}