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
 *    复合索引带$Range
 * 测试步骤：
 *    1.创建集合
 *    2.在集合上创建复合索引
 *    3.执行查询，查询条件带上$Range
 *
 * 期望结果：
 *    读取索引记录数较少
 *
 **************************************************************************************************/

testConf.clName = COMMCLNAME + "_compositeIndex_at_18";

main(test)

function test(testPara)
{
   db.setSessionAttr({PreferredInstance:"m"}) ;
   var cl = testPara.testCL;
   cl.createIndex('abc',{a:1,b:1,c:1})
   cl.createIndex('abc2',{a:-1,b:1,c:1})

   data=[];
   for(i=0;i<100000;i++){data.push({_id:i,a:i,b:i,c:i,d:i})}
   cl.insert(data)

   var expectRes = [{_id:2,a:2,b:2,c:2,d:2}]

   var res = commCursor2Array(
      cl.find({$or:[{a:1,b:1},{a:2,b:2}],c:2}).hint({ "$Range": { "IsAllEqual": true, "PrefixNum": 3, "IndexValue": [ { "1": 1, "2": 1, "3": 2 }, { "1": 2, "2": 2, "3": 2 } ] }, "": "abc"}) )
   assert.equal(res, expectRes)

   var res = commCursor2Array(
      cl.find({$or:[{a:1,b:1},{a:2,b:2}],c:2}).hint({ "$Range": { "IsAllEqual": true, "PrefixNum": 3, "IndexValue": [ { "1": 1, "2": 1, "3": 2 }, { "1": 2, "2": 2, "3": 2 } ] }, "": "abc2"}) )
   assert.equal(res, expectRes)

   var res = commCursor2Array(
      cl.find({$or:[{a:1,b:1},{a:2,b:2}],c:2}).sort({a:-1,b:-1,c:-1}).hint({ "$Range": { "IsAllEqual": true, "PrefixNum": 3, "IndexValue": [ { "1": 1, "2": 1, "3": 2 }, { "1": 2, "2": 2, "3": 2 } ] }, "": "abc"}) )
   assert.equal(res, expectRes)

   var res = commCursor2Array(
      cl.find({$or:[{a:1,b:1},{a:2,b:2}],c:2}).sort({a:1,b:-1,c:-1}).hint({ "$Range": { "IsAllEqual": true, "PrefixNum": 3, "IndexValue": [ { "1": 1, "2": 1, "3": 2 }, { "1": 2, "2": 2, "3": 2 } ] }, "": "abc2"}) )
   assert.equal(res, expectRes)
}
