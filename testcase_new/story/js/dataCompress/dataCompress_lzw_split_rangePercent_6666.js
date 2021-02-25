/******************************************************************************
 * @Description   : seqDB-6666:range百分比切分，分区键为多个字段
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2016.03.23
 * @LastEditTime  : 2021.02.23
 * @LastEditors   : XiaoNi Huang
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;
testConf.useSrcGroup = true;
testConf.useDstGroup = true;
testConf.clName = CHANGEDPREFIX + "_cl_6666";
testConf.clOpt = {
   ShardingKey: { INNER_NO: 1 }, ShardingType: "range",
   Compressed: true, CompressionType: "lzw", ReplSize: 0
};

main( test );
function test ( testPara )
{
   var srcRgName = testPara.srcGroupName;
   var dstRgName = testPara.dstGroupNames[0];
   var csName = COMMCSNAME;
   var clName = testConf.clName;
   var cl = testPara.testCL;
   var insertRecsNum = 1600000;
   var checkRecsNum = 3;

   // 插入数据并切分
   insertRecs2( cl, insertRecsNum );
   cl.split( srcRgName, dstRgName, 50 );

   // 检查结果，检查组内每个节点数据正确性
   checkLzwAttributeByDataNode( srcRgName, csName, clName, true );
   checkRecsByDataNode( srcRgName, csName, clName, insertRecsNum, 0, checkRecsNum );

   checkLzwAttributeByDataNode( dstRgName, csName, clName, true );
   checkRecsByDataNode( dstRgName, csName, clName, insertRecsNum, insertRecsNum / 2, checkRecsNum );
}

function checkRecsByDataNode ( rgName, csName, clName, insertRecsNum, min, checkRecsNum )
{
   var rc = db.exec( "select NodeName from $SNAPSHOT_SYSTEM where GroupName='" + rgName + "'" );
   while( rc.next() )
   {
      var nodeName = rc.current().toObj()["NodeName"];
      var nodeDB = null;
      try
      {
         nodeDB = new Sdb( nodeName );
         var nodeCL = nodeDB.getCS( csName ).getCL( clName );
         // 检查数据总数
         var recsCnt = nodeCL.count();
         assert.equal( recsCnt, insertRecsNum / 2 );
         // 随机检查n条记录正确性
         for( j = 0; j < checkRecsNum; j++ )
         {
            var i = parseInt( Math.random() * ( insertRecsNum / 2 ) ) + min;
            var recsCnt = nodeCL.find( {
               INNER_NO: i, SA_ACCT_NO: i, EVT_ID: "lwy20120702" + i,
               IVC_NAME: "电子银行业务回单(付款)", OPEN_BRANCH_NAME: "中国民生银行福州闽江支行"
            } ).count();
            var expctCnt = 1;
            assert.equal( recsCnt, expctCnt );
         }
      }
      finally 
      {
         if( nodeDB != null ) nodeDB.close();
      }
   }
}