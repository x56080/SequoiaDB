/******************************************************************************
 * @Description   : seqDB-6658:findAndRemove删除记录
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2016.03.23
<<<<<<< HEAD
 * @LastEditTime  : 2023.02.07
 * @LastEditors   : liuli
=======
 * @LastEditTime  : 2021.02.23
 * @LastEditors   : XiaoNi Huang
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;
testConf.clName = CHANGEDPREFIX + "_cl_6658";
testConf.clOpt = { Compressed: true, CompressionType: "lzw", ReplSize: 0 };

main( test );
function test ( testPara )
{
   var rgName = testPara.srcGroupName;
   var csName = COMMCSNAME;
   var clName = testConf.clName;
   var cl = testPara.testCL;
   var insertRecsNum = 800000;
   var checkRecsNum = 10;
<<<<<<< HEAD

   // insert  
   insertRecs2( cl, insertRecsNum );

   // findAndRemove
   var rc = cl.find( {
      $and: [{ INNER_NO: { $gte: 200000 } },
      { IVC_NAME: { $et: "电子银行业务回单(付款)" } }]
   } ).remove();
   while( rc.next() );

   // 等待字典构建
   waitDictionary( db, csName, clName );

   // 再次插入少量数据使数据压缩
   var insertRecsNum2 = 200000;
   insertRecs2( cl, insertRecsNum2 );

   // 检查结果，检查组内每个节点数据正确性
   checkLzwAttributeByDataNode( rgName, csName, clName, true );
   var expRecsNum = 400000;
   checkRecsByDataNode( rgName, csName, clName, expRecsNum, checkRecsNum );
=======

   // insert  
   insertRecs2( cl, insertRecsNum );

   // findAndRemove
   var rc = cl.find( {
      $and: [{ INNER_NO: { $gte: 200000 } },
      { IVC_NAME: { $et: "电子银行业务回单(付款)" } }]
   } ).remove();
   while( rc.next() );

   // 检查结果，检查组内每个节点数据正确性
   checkLzwAttributeByDataNode( rgName, csName, clName, true );
   checkRecsByDataNode( rgName, csName, clName, insertRecsNum, checkRecsNum );
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
}

function checkRecsByDataNode ( rgName, csName, clName, insertRecsNum, checkRecsNum )
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
<<<<<<< HEAD
         assert.equal( recsCnt, insertRecsNum );
         // 随机检查n条记录正确性
         for( j = 0; j < checkRecsNum; j++ )
         {
            var i = parseInt( Math.random() * 800000 );
=======
         assert.equal( recsCnt, 200000 );
         // 随机检查n条记录正确性
         for( j = 0; j < checkRecsNum; j++ )
         {
            var i = parseInt( Math.random() * insertRecsNum );
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
            var recsCnt = nodeCL.find( {
               INNER_NO: i, SA_ACCT_NO: i, EVT_ID: "lwy20120702" + i,
               IVC_NAME: "电子银行业务回单(付款)", OPEN_BRANCH_NAME: "中国民生银行福州闽江支行"
            } ).count();
            if( i < 200000 )
            {
               // 检查未被删除的记录
<<<<<<< HEAD
               var expctCnt = 2;
=======
               var expctCnt = 1;
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
            }
            else
            {
               // 检查被删除的记录
               var expctCnt = 0;
            }
            assert.equal( recsCnt, expctCnt );
         }
      }
      finally 
      {
         if( nodeDB != null ) nodeDB.close();
      }
   }
}