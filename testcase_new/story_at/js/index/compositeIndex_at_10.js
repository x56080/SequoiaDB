/***************************************************************************************************
 * @Description: 复合索引选择
 * @ATCaseID: <填写 story 文档中验收用例的用例编号>
 * @Author: He Guoming
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who           Description
 * ========== ============= =========================================================
 * 07/20/2023 HGM           Init
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：
 * 测试场景：
 *    索引选择考虑limit
 * 测试步骤：
 *    1.创建集合
 *    2.在集合上创建复合索引
 *    3.写入一定量数据
 *    4.执行查询，排序字段覆盖索引字段，且带有limit
 *
 * 期望结果：
 *    访问计划选择的索引为字段a,c的复合索引
 *
 **************************************************************************************************/
testConf.clName = COMMCLNAME + "_compositeIndex_at_10";

main(testWrap);

function testWrap(testPara)
{
   try
   {
      test(testPara)
   }
   finally
   {
      db.deleteConf({optstartcostlimit:1})
   }
}

function testExplain( cl, query, orderBy, expectedIndexName, affectedByAnalyze )
{
   var indexName;

   indexName = cl.find(query).sort(orderBy).explain().current().toObj()["IndexName"];
   assert.equal( indexName, affectedByAnalyze ? "" : expectedIndexName );

   indexName = cl.find(query).sort(orderBy).limit(1).explain().current().toObj()["IndexName"];
   assert.equal( indexName, expectedIndexName );

   indexName = cl.find(query).sort(orderBy).limit(1).skip(1).explain().current().toObj()["IndexName"];
   assert.equal( indexName, expectedIndexName );

   indexName = cl.find(query).sort(orderBy).limit(1).skip(500).explain().current().toObj()["IndexName"];
   assert.equal( indexName, expectedIndexName );

   indexName = cl.find(query).sort(orderBy).limit(500).explain().current().toObj()["IndexName"];
   assert.equal( indexName, expectedIndexName );

   indexName = cl.find(query).sort(orderBy).limit(500).skip(500).explain().current().toObj()["IndexName"];
   assert.equal( indexName, expectedIndexName );
}

function test(testPara)
{
   db.setSessionAttr({PreferredInstance:"m"}) ;
   var cl = testPara.testCL;

   cl.createIndex("a",{"a":1});
   cl.createIndex("ab",{"a":1,"b":1});
   cl.createIndex("ac",{"a":1,"c":1});
   cl.createIndex("bc",{"b":1,"c":1});
   cl.createIndex("abc",{"a":1,"b":1,"c":1});

   // empty collection
   testExplain( cl, {}, {a:-1}, "a", false );
   testExplain( cl, {}, {a:1}, "a", false );
   testExplain( cl, {}, {a:1,b:1}, "ab", false );
   testExplain( cl, {}, {a:-1,c:-1}, "ac", false );
   testExplain( cl, {}, {b:1,c:1}, "bc", false );
   testExplain( cl, {}, {a:1,b:1,c:1}, "abc", false );
   testExplain( cl, {}, {d:1,a:1}, "", false );

   data = [] ;
   for ( i = 0 ; i < 100000 ; ++i )
   {
      data.push( {a:i, b: Math.round(( Math.random() * 100000 )), c:i, d:i } )
   }
   cl.insert(data);

   // collection with data
   testExplain( cl, {}, {a:-1}, "a", false );
   testExplain( cl, {}, {a:1}, "a", false );
   testExplain( cl, {}, {a:1,b:1}, "ab", false );
   testExplain( cl, {}, {a:-1,c:-1}, "ac", false );
   testExplain( cl, {}, {b:1,c:1}, "bc", false );
   testExplain( cl, {}, {a:1,b:1,c:1}, "abc", false );
   testExplain( cl, {}, {d:1,a:1}, "", false );

   // collection with data and analyze
   // cost calculation will be affected by limit
   db.analyze({Collection: testConf.csName + "." + testConf.clName});

   testExplain( cl, {}, {a:-1}, "a", true );
   testExplain( cl, {}, {a:1}, "a", true );
   testExplain( cl, {}, {a:1,b:1}, "ab", true );
   testExplain( cl, {}, {a:-1,c:-1}, "ac", true );
   testExplain( cl, {}, {b:1,c:1}, "bc", true );
   testExplain( cl, {}, {a:1,b:1,c:1}, "abc", true );
   testExplain( cl, {}, {d:1,a:1}, "", true );

   db.updateConf({optstartcostlimit:0})
   var indexName = cl.find().sort({a:1}).limit(1).explain().current().toObj()["IndexName"];
   assert.equal( indexName, "a" );

   db.updateConf({optstartcostlimit:1000})
   var indexName = cl.find().sort({a:1}).limit(50000).explain().current().toObj()["IndexName"];
   assert.equal( indexName, "" );
}