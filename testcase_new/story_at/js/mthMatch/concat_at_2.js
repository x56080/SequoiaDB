/***************************************************************************************************
 * @Description: $concat作为选择符使用
 * @ATCaseID: concat_at_2
 * @Author: Huang Youquan
 * @TestlinkCase: 无
 * @Change Activity:
 * Date       Who           Description
 * ========== ============= =========================================================
 * 03/16/2023 Huang Youquan    Init
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群
 * 测试场景：
 *     $concat作为选择符使用
 * 测试步骤：
 *    1. $concat作为选择符, 发起查询
 * 期望结果：
 *     期望结果与实际结果一致
 *
 **************************************************************************************************/
testConf.clName = COMMCLNAME + "concat_at_2";

main(test);
function test(testPara) {
  var dbcl = testPara.testCL;
  var docs = [
    { a: "Sequoiadb" },
    { a: "巨杉Sequoiadb" },
    { a: ["Sequoiadb", "巨杉", 999] },
    { a: [999, true] },
    { a: 999 },
    { a: 999.999 },
    { a: true },
    { a: { $decimal: "123.456312313131313131" } },
    { a: { $date: "2022-02-11" } },
    { a: { b: "value" } },
    { a: { $timestamp: "2012-01-01-13.14.26.124233" } },
    { a: null },
    { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
    { a: { $minKey: 1 } },
    { a: { $maxKey: 1 } },
  ];
  dbcl.insert(docs);

  var actResult;
  var expResult;

  var actResult = dbcl.find({}, { a: { $concat: "abc" } });
  var expResult = [
    { a: "Sequoiadbabc" },
    { a: "巨杉Sequoiadbabc" },
    { a: ["Sequoiadbabc", "巨杉abc", "999abc"] },
    { a: ["999abc", "trueabc"] },
    { a: "999abc" },
    { a: "999.999abc" },
    { a: "trueabc" },
    { a: "123.456312313131313131abc" },
    { a: "2022-02-11abc" },
    { a: '{ "b": "value" }abc' },
    { a: "2012-01-01-13.14.26.124233abc" },
    { a: null },
    { a: null },
    { a: null },
    { a: null },
  ];
  commCompareResults(actResult, expResult);

  var actResult = dbcl.find({}, { a: { $concat: [1, ["111", "222", "333"]] } });
  var expResult = [
    { a: "111Sequoiadb222333" },
    { a: "111巨杉Sequoiadb222333" },
    { a: ["111Sequoiadb222333", "111巨杉222333", "111999222333"] },
    { a: ["111999222333", "111true222333"] },
    { a: "111999222333" },
    { a: "111999.999222333" },
    { a: "111true222333" },
    { a: "111123.456312313131313131222333" },
    { a: "1112022-02-11222333" },
    { a: '111{ "b": "value" }222333' },
    { a: "1112012-01-01-13.14.26.124233222333" },
    { a: null },
    { a: null },
    { a: null },
    { a: null },
  ];
  commCompareResults(actResult, expResult);

  var actResult = dbcl.find({}, { a: { $concat: [0, ["111", "222", "333"]] } });
  var expResult = [
    { a: "Sequoiadb111222333" },
    { a: "巨杉Sequoiadb111222333" },
    { a: ["Sequoiadb111222333", "巨杉111222333", "999111222333"] },
    { a: ["999111222333", "true111222333"] },
    { a: "999111222333" },
    { a: "999.999111222333" },
    { a: "true111222333" },
    { a: "123.456312313131313131111222333" },
    { a: "2022-02-11111222333" },
    { a: '{ "b": "value" }111222333' },
    { a: "2012-01-01-13.14.26.124233111222333" },
    { a: null },
    { a: null },
    { a: null },
    { a: null },
  ];
  commCompareResults(actResult, expResult);

  var actResult = dbcl.find({}, { a: { $concat: [-2, ["111", "222", "333"]] } });
  var expResult = [
    { a: "111222Sequoiadb333" },
    { a: "111222巨杉Sequoiadb333" },
    { a: ["111222Sequoiadb333", "111222巨杉333", "111222999333"] },
    { a: ["111222999333", "111222true333"] },
    { a: "111222999333" },
    { a: "111222999.999333" },
    { a: "111222true333" },
    { a: "111222123.456312313131313131333" },
    { a: "1112222022-02-11333" },
    { a: '111222{ "b": "value" }333' },
    { a: "1112222012-01-01-13.14.26.124233333" },
    { a: null },
    { a: null },
    { a: null },
    { a: null },
  ];
  commCompareResults(actResult, expResult);

  var actResult = dbcl.find({}, { a: { $concat: [-1, ["111", "222", "333"]] } });
  var expResult = [
    { a: "111222333Sequoiadb" },
    { a: "111222333巨杉Sequoiadb" },
    { a: ["111222333Sequoiadb", "111222333巨杉", "111222333999"] },
    { a: ["111222333999", "111222333true"] },
    { a: "111222333999" },
    { a: "111222333999.999" },
    { a: "111222333true" },
    { a: "111222333123.456312313131313131" },
    { a: "1112223332022-02-11" },
    { a: '111222333{ "b": "value" }' },
    { a: "1112223332012-01-01-13.14.26.124233" },
    { a: null },
    { a: null },
    { a: null },
    { a: null },
  ];
  commCompareResults(actResult, expResult);

  var actResult = dbcl.find({}, { a: { $concat: [100, ["111", "222", "333"]] } });
  var expResult = [
    { a: "111222333Sequoiadb" },
    { a: "111222333巨杉Sequoiadb" },
    { a: ["111222333Sequoiadb", "111222333巨杉", "111222333999"] },
    { a: ["111222333999", "111222333true"] },
    { a: "111222333999" },
    { a: "111222333999.999" },
    { a: "111222333true" },
    { a: "111222333123.456312313131313131" },
    { a: "1112223332022-02-11" },
    { a: '111222333{ "b": "value" }' },
    { a: "1112223332012-01-01-13.14.26.124233" },
    { a: null },
    { a: null },
    { a: null },
    { a: null },
  ];
  commCompareResults(actResult, expResult);

  var actResult = dbcl.find({}, { a: { $concat: [-100, ["111", "222", "333"]] } });
  var expResult = [
    { a: "Sequoiadb111222333" },
    { a: "巨杉Sequoiadb111222333" },
    { a: ["Sequoiadb111222333", "巨杉111222333", "999111222333"] },
    { a: ["999111222333", "true111222333"] },
    { a: "999111222333" },
    { a: "999.999111222333" },
    { a: "true111222333" },
    { a: "123.456312313131313131111222333" },
    { a: "2022-02-11111222333" },
    { a: '{ "b": "value" }111222333' },
    { a: "2012-01-01-13.14.26.124233111222333" },
    { a: null },
    { a: null },
    { a: null },
    { a: null },
  ];
  commCompareResults(actResult, expResult);

  var actResult = dbcl.find({}, { a: { $concat: [1, []] } });
  var expResult = [
    { a: "Sequoiadb" },
    { a: "巨杉Sequoiadb" },
    { a: ["Sequoiadb", "巨杉", "999"] },
    { a: ["999", "true"] },
    { a: "999" },
    { a: "999.999" },
    { a: "true" },
    { a: "123.456312313131313131" },
    { a: "2022-02-11" },
    { a: '{ "b": "value" }' },
    { a: "2012-01-01-13.14.26.124233" },
    { a: null },
    { a: null },
    { a: null },
    { a: null },
  ];
  commCompareResults(actResult, expResult);
}
