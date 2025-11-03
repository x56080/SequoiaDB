/******************************************************************************
 * @Description   : EnsureShardingIndex
 * @Author        : fangjiabin
 * @CreateTime    : 2025.09.03
 * @LastEditTime  : 2025.09.03
 * @LastEditors   : fangjiabin
 ******************************************************************************/
testConf.skipStandAlone = true;

var csName = "cs_ensureshardidx_10276_1";
var clName = "cl_ensureshardidx_10276_1";
var clFullName = csName + "." + clName;

main( test );

function test ()
{
   var step = 1;
   commDropCS( db, csName );
   commCreateCS( db, csName );
   commCreateCL( db, csName, clName, { ShardingKey:{ a: 1 }, ReplSize: -1 } );
   var cl = db.getCS( csName ).getCL( clName );
   var recArray = [];
   for (var i = 0; i < 10; i++) {
      recArray.push( { a: i, b: i, c: i } );
   }
   cl.insert( recArray );

   check( step++, { "a": 1 }, true, true );

   cl.alter({EnsureShardingIndex:false})
   check( step++, { "a": 1 }, false, false );

   cl.alter({EnsureShardingIndex:false})
   check( step++, { "a": 1 }, false, false );

   cl.alter({ShardingKey:{a:1}});
   check( step++, { "a": 1 }, false, false );

   cl.alter({ShardingKey:{a:1}});
   check( step++, { "a": 1 }, false, false );

   cl.alter({EnsureShardingIndex:false,ShardingKey:{a:1}});
   check( step++, { "a": 1 }, false, false );

   cl.alter({EnsureShardingIndex:false,ShardingKey:{a:1}});
   check( step++, { "a": 1 }, false, false );

   cl.alter({ShardingKey:{b:1}});
   check( step++, { "b": 1 }, false, false );

   cl.alter({ShardingKey:{b:1}});
   check( step++, { "b": 1 }, false, false );

   cl.alter({EnsureShardingIndex:false,ShardingKey:{b:1}});
   check( step++, { "b": 1 }, false, false );

   cl.alter({EnsureShardingIndex:false,ShardingKey:{b:1}});
   check( step++, { "b": 1 }, false, false );

   cl.alter({EnsureShardingIndex:true,ShardingKey:{b:1}});
   check( step++, { "b": 1 }, true, true );

   cl.alter({EnsureShardingIndex:true,ShardingKey:{b:1}});
   check( step++, { "b": 1 }, true, true );

   cl.alter({ShardingKey:{a:1}});
   check( step++, { "a": 1 }, true, true );

   cl.alter({ShardingKey:{a:1}});
   check( step++, { "a": 1 }, true, true );

   cl.alter({EnsureShardingIndex:false})
   check( step++, { "a": 1 }, false, false );

   cl.alter({EnsureShardingIndex:false})
   check( step++, { "a": 1 }, false, false );

   commDropCS( db, csName );
}

function check( step, shardingKeyDef, mustHasIndex, ensureShardingIndex )
{
   println("Begin "+step);
   var cl = db.getCS( csName ).getCL( clName );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "ShardingKey", shardingKeyDef );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "EnsureShardingIndex", ensureShardingIndex );
   checkIndexExist( cl, shardingKeyDef, mustHasIndex );
   checkIdxLSN( clFullName );
   println("End "+step);
}
