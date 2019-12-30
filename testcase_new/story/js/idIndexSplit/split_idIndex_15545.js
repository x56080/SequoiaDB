/******************************************************************************
@Description : seqDB-15545:指定AutoIndexId:false，加入域并使用自动切分
@Modify list : 2018-08-08  XiaoNi Zhao  Init
               2019-11-22  XiaoNi Huang  Modify
******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;
var groupNames = commGetDataGroupNames( db );
var srcGroupName = groupNames[0];
var dstGroupName = groupNames[1];

var dmName = CHANGEDPREFIX + "_split15545";
commDropDomain( db, dmName, true );
commCreateDomain( db, dmName, groupNames, {}, false );

testConf.csName = CHANGEDPREFIX + "_split15545";
testConf.csOpt = { "Domain": dmName };
testConf.clName = "cl";
testConf.clOpt = { "ShardingKey": { "a": 1 }, "ShardingType": "hash", "AutoIndexId": false, "AutoSplit": true };
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

   // check results
   var cursor = cl.find( {}, { "_id": { "$include": 0 } } ).sort( { "a": 1 } );
   commCompareResults( cursor, docs );
   checkSplitResults( testConf.csName, testConf.clName, groupNames, recsNum );

   // clean
   commDropDomain( db, dmName, false, "drop cs in the end" );
}
