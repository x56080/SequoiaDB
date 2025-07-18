/******************************************************************************
@Description : SEQUOIADBMAINSTREAM-10132，分区表修改分区键应忽略分区键检查
@Modify list :
               2025-07-21 fangjiabin  Init
******************************************************************************/
testConf.skipStandAlone = true ;
var clName1 = "alter10321_1" ;

main( test ) ;

function test ()
{
   commDropCL( db, COMMCSNAME, clName1 ) ;
   commCreateCL( db, COMMCSNAME, clName1, { "ShardingKey": { "shardField": 1 }, "ReplSize": -1 } ) ;

   testChangeShardingKey();
}

function testChangeShardingKey()
{
   var nodes = commGetCLNodes( db, COMMCSNAME + "." + clName1 );
   // 只有很久的版本才可以创建不包含分区键的唯一键，所以这里只能通过直连数据节点构造这种场景
   for( var j = 0; j < nodes.length; j++ )
   {
      var nodeConn = new Sdb( nodes[j].HostName, nodes[j].svcname );
      var tmpCl = nodeConn.getCS( COMMCSNAME ).getCL( clName1 );
      tmpCl.createIndex( "uniqueIdx", { uniqueField: 1 }, true, true );
   }

   var cl = db.getCS( COMMCSNAME ).getCL( clName1 );
   // 已存在分区键，再次创建相同的分区键，不会报错
   cl.enableSharding( { "ShardingKey": { "shardField": 1 } } );
   // 已存在分区键，再次创建不同的分区键，会报错
   try
   {
      cl.enableSharding( { "ShardingKey": { "shardField2": 1 } } );
   }
   catch( e )
   {
      if ( !commCompareErrorCode( e, SDB_SHARD_KEY_NOT_IN_UNIQUE_KEY ) )
      {
         throw new Error( "Failed to enableSharding: " + e );
      }
   }

   // 已存在分区键，再次创建相同的分区键，不会报错
   cl.setAttributes( { "ShardingKey": { "shardField": 1 } } );
   // 已存在分区键，再次创建不同的分区键，会报错
   try
   {
      cl.setAttributes( { "ShardingKey": { "shardField2": 1 } } );
   }
   catch( e )
   {
      if ( !commCompareErrorCode( e, SDB_SHARD_KEY_NOT_IN_UNIQUE_KEY ) )
      {
         throw new Error( "Failed to setAttributes ShardingKey: " + e );
      }
   }

   // 已存在分区键，再次创建相同的分区键，不会报错
   cl.alter( { "ShardingKey": { "shardField": 1 } } );
   // 已存在分区键，再次创建不同的分区键，会报错
   try
   {
      cl.alter( { "ShardingKey": { "shardField2": 1 } } );
   }
   catch( e )
   {
      if ( !commCompareErrorCode( e, SDB_SHARD_KEY_NOT_IN_UNIQUE_KEY ) )
      {
         throw new Error( "Failed to alter ShardingKey: " + e );
      }
   }

   commDropCL( db, COMMCSNAME, clName1 ) ;
}