/******************************************************************************
 * @Description   : seqDB-26573:stp节点异常时执行split
 * @Author        : HuangHaimei
 * @CreateTime    : 2022.08.10
 * @LastEditTime  : 2022.08.12
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_26573";
testConf.clOpt = { "ShardingKey": { "a": 1 } };
testConf.useSrcGroup = true;
testConf.useDstGroup = true;
testConf.skipOneGroup = true;
testConf.skipStandAlone = true;

main( test );
function test ( testPara )
{
   try
   {
      var oma = new Oma( COORDHOSTNAME, CMSVCNAME );
      oma.stopStp();
      var cl = testPara.testCL;
      var docs = [];
      for( var i = 0; i < 10; ++i )
      {
         docs.push( { a: i } );
      }
      cl.insert( docs );
      var srcGroupName = testPara.srcGroupName;
      var dstGroupNames = testPara.dstGroupNames;
      var beginTime = new Date();
      cl.split( srcGroupName, dstGroupNames[0], 100 );
      var endTime = new Date();
      // 查看切分前后的时间差
      var minute = ( parseInt( endTime - beginTime ) / 1000 / 60 );
      if( minute > 10 )
      {
         // 当切分时间大于10分钟就报错
         throw new Error( "切分时间大于10分钟" );
      }
      var cursor = cl.find().sort( { a: 1 } );
      commCompareResults( cursor, docs );
   }
   finally
   {
      oma.startStp();
      oma.close();
   }
}