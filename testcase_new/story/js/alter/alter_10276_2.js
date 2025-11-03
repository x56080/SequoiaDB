/******************************************************************************
 * @Description   : EnsureShardingIndex
 * @Author        : fangjiabin
 * @CreateTime    : 2025.09.12
 * @LastEditTime  : 2025.09.12
 * @LastEditors   : fangjiabin
 ******************************************************************************/
testConf.skipStandAlone = true;

var csName = "cs_ensureshardidx_10276_2";

main( test );

function test ()
{
   commDropCS( db, csName );
   commCreateCS( db, csName );

   testMultiGroupShardingCl();

   commDropCS( db, csName );
}

function check( clName, shardingKeyDef, mustHasIndex, ensureShardingIndex, uniqueID, cataInfo )
{
   var cl = db.getCS( csName ).getCL( clName );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "ShardingKey", shardingKeyDef );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "EnsureShardingIndex", ensureShardingIndex );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "AutoSplit", true );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "ShardingType", "hash" );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "Partition", 4096 );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "UniqueID", uniqueID );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "CataInfo", cataInfo );
   checkIndexExist( cl, shardingKeyDef, mustHasIndex );
}

// 多组分区表
function testMultiGroupShardingCl()
{
   var clName = "sharding_cl" ;
   var clFullName = csName + "." + clName;
   commDropCL( db, csName, clName );
   commCreateCL( db, csName, clName, { ShardingKey:{ a: 1 }, AutoSplit: true, ReplSize: -1 } );

   var cursor = db.snapshot( 8, { Name: clFullName } );
   var uniqueID = cursor.current().toObj().UniqueID;
   var cataInfo = cursor.current().toObj().CataInfo;

   var cl = db.getCS( csName ).getCL( clName );
   var recArray = [];
   for (var i = 0; i < 10; i++) {
      recArray.push( { a: i, b: i, c: i } );
   }
   cl.insert( recArray );

   assert.tryThrow( SDB_OPTION_NOT_SUPPORT, function()
   {
      cl.alter( { ShardingKey: { b: 1 }, EnsureShardingIndex: false } );
   } );

   assert.tryThrow( SDB_OPTION_NOT_SUPPORT, function()
   {
      cl.alter( { ShardingKey: { b: 1 }, EnsureShardingIndex: true } );
   } );

   cl.alter( { ShardingKey: { a: 1 }, EnsureShardingIndex: false } );
   check( clName, { a: 1 }, false, false, uniqueID, cataInfo );

   cl.alter( { ShardingKey: { a: 1 }, EnsureShardingIndex: true } );
   check( clName, { a: 1 }, true, true, uniqueID, cataInfo );

   cl.alter( { EnsureShardingIndex: false } );
   check( clName, { a: 1 }, false, false, uniqueID, cataInfo );

   cl.alter( { EnsureShardingIndex: true } );
   check( clName, { a: 1 }, true, true, uniqueID, cataInfo );
}
