/******************************************************************************
 * @Description   : seqDB-26733:使用alter修改集合分区键
 * @Author        : Xu Mingxing
 * @CreateTime    : 2022.07.19
 * @LastEditTime  : 2022.07.20
 * @LastEditors   : Xu Mingxing
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_26733";
testConf.clOpt = { "ShardingKey": { "a": 1 }, "AutoSplit": true };
main( test );

function test ( args )
{
   var cl = args.testCL;
   cl.insert( { _id: 1, name: "Mike", age: 15, a: 1 } );

   var obj1 = cl.getIndex( "$shard" );

   assert.tryThrow( SDB_OPTION_NOT_SUPPORT, function()
   {
      cl.alter( { "ShardingKey": { "b": 1 } } );
   } )

   cl.alter( { "ShardingKey": { "a": 1 } } );

   var obj2 = cl.getIndex( "$shard" );

   assert.equal( obj1, obj2 );
}

