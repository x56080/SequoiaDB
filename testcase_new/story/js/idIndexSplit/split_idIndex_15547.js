/******************************************************************************
@Description : seqDB-15547:创建id索引后，执行切分
@Modify list : 2018-08-08  XiaoNi Zhao  Init
               2019-11-22  XiaoNi Huang  Modify
******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;
var groupNames = commGetDataGroupNames( db );
var srcGroupName = groupNames[0];
var dstGroupName = groupNames[1];

testConf.csName = COMMCSNAME;
testConf.clName = CHANGEDPREFIX + "_split15547";
testConf.clOpt = { "ShardingKey": { "a": 1 }, "ShardingType": "hash", "Group": srcGroupName, "AutoIndexId": false };
var recsNum = 100;

main( test );
function test ( arg )
{
   var cl = arg.testCL;

   // insert
   var docs = [];
   for( var i = 0; i < recsNum; i++ )
   {
      docs.push( { "a": i } );
   }
   cl.insert( docs );

   // create idIndex, then split
   cl.createIdIndex();
   cl.split( srcGroupName, dstGroupName, 50 );

   // check results
   var cursor = cl.find( {}, { "_id": { "$include": 0 } } ).sort( { "a": 1 } );
   commCompareResults( cursor, docs );
   checkSplitResults( testConf.csName, testConf.clName, [srcGroupName, dstGroupName], recsNum );
}