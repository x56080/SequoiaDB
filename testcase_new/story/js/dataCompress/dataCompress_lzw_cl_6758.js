/******************************************************************************
 * @Description   : seqDB-6758:开启压缩，创建CL
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2016.03.23
<<<<<<< HEAD
 * @LastEditTime  : 2023.02.08
 * @LastEditors   : liuli
=======
 * @LastEditTime  : 2021.03.03
 * @LastEditors   : XiaoNi Huang
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;
testConf.clName = CHANGEDPREFIX + "_cl_6758";
testConf.clOpt = { Compressed: true, CompressionType: "lzw", ReplSize: 0 };

main( test );
function test ( testPara )
{
   var rgName = testPara.srcGroupName;
   var csName = COMMCSNAME;
   var clName = testConf.clName;
   var cl = testPara.testCL;
   var insertRecsNum = 1200000;
   var checkRecsNum = 3;

   // 检查编目集合属性
   var clInfo = db.snapshot( 8, { Name: csName + "." + clName } ).toArray();
   var details = JSON.parse( clInfo[0] );
   assert.equal( details.Attribute, 1, "clInfo = " + JSON.stringify( clInfo ) );
   assert.equal( details.AttributeDesc, "Compressed", "clInfo = " + JSON.stringify( clInfo ) );
   assert.equal( details.CompressionType, 1, "clInfo = " + JSON.stringify( clInfo ) );
   assert.equal( details.CompressionTypeDesc, "lzw", "clInfo = " + JSON.stringify( clInfo ) );

   // 插入数据
   insertRecs1( cl, insertRecsNum );

<<<<<<< HEAD
   // 等待字典构建
   waitDictionary( db, csName, clName );

   // 再次插入少量数据使数据压缩
   var insertRecsNum2 = 50000;
   insertRecs1( cl, insertRecsNum2 );

   // 检查结果，检查组内每个节点数据正确性
   checkLzwAttributeByDataNode( rgName, csName, clName, true );
   checkRecsByDataNode( rgName, csName, clName, insertRecsNum + insertRecsNum2, checkRecsNum, insertRecsNum );
}

function checkRecsByDataNode ( rgName, csName, clName, insertRecsNum, checkRecsNum, insertRange )
=======
   // 检查结果，检查组内每个节点数据正确性
   checkLzwAttributeByDataNode( rgName, csName, clName, true );
   checkRecsByDataNode( rgName, csName, clName, insertRecsNum, checkRecsNum );
}

function checkRecsByDataNode ( rgName, csName, clName, insertRecsNum, checkRecsNum )
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
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
         assert.equal( recsCnt, insertRecsNum );
         // 随机检查n条记录正确性
         for( j = 0; j < checkRecsNum; j++ )
         {
<<<<<<< HEAD
            var i = parseInt( Math.random() * insertRange );
            var cond = { atest: i, btest: i, ctest: "test" + i, dtest: "abcdefg890abcdefg890abcdefg890" };
            var recsCnt = nodeCL.find( cond ).count();
            if( recsCnt != 1 && recsCnt != 2 )
            {
               throw new Error( "expected result is 1 or 2, actual is " + recsCnt + " ,cond is :" + JSON.stringify( cond ) );
            }
=======
            var i = parseInt( Math.random() * insertRecsNum );
            var recsCnt = nodeCL.find( { atest: i, btest: i, ctest: "test" + i, dtest: "abcdefg890abcdefg890abcdefg890" } ).count();
            assert.equal( recsCnt, 1 );
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
         }
      }
      finally 
      {
         if( nodeDB != null ) nodeDB.close();
      }
   }
}