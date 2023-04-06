/***************************************************************************************************
 * @Description: $day作用于不可处理类型字段
 * @ATCaseID: day_at_6
 * @Author: Huang Youquan
 * @TestlinkCase: 无
 * @Change Activity:
 * Date       Who           Description
 * ========== ============= =========================================================
 * 03/30/2023 Huang Youquan    Init
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群
 * 测试场景：
 *     $day作用于不可处理类型字段
 * 测试步骤：
 *    1. $day作为选择符, 发起查询
 *    2. $day作为匹配符, 发起查询
 * 期望结果：
 *    返回null
 *
 **************************************************************************************************/

testConf.clName = COMMCLNAME + "day_at_6";

main(test);
function test(testPara) {
  var dbcl = testPara.testCL;
  var docs = [
    { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
    { a: null },
    { a: true },
    { a: { $regex: "^张", $options: "i" } },
    { a: { $minKey: 1 } },
    { a: { $maxKey: 1 } },
  ];
  dbcl.insert(docs);

  var actResult;
  var expResult;
  actResult = dbcl.find({}, { a: { $day: 1 } });
  expResult = [{ a: null }, { a: null }, { a: null }, { a: null }, { a: null }, { a: null }];
  commCompareResults(actResult, expResult);
  actResult = dbcl.find({ a: { $day: 1, $et: 31 } });
  expResult = [];
  commCompareResults(actResult, expResult);
}
