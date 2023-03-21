/***************************************************************************************************
 * @Description: $concat作为使用
 * @ATCaseID: concat_at_3
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
 *     $concat作为匹配符使用
 * 测试步骤：
 *    1. $concat作为匹配符, 发起查询
 * 期望结果：
 *   期望结果与实际结果一致
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

  var actResult = dbcl.find({ a: { $concat: "abc", $et: "999abc" } });
  var expResult = [{ a: ["Sequoiadb", "巨杉", 999] }, { a: [999, true] }, { a: 999 }];
  commCompareResults(actResult, expResult);

  var actResult = dbcl.find({
    a: { $concat: [1, ["111", "222", "333"]], $et: "111Sequoiadb222333" },
  });
  var expResult = [{ a: "Sequoiadb" }, { a: ["Sequoiadb", "巨杉", 999] }];
  commCompareResults(actResult, expResult);

  var actResult = dbcl.find({
    a: { $concat: [0, ["111", "222", "333"]], $et: "Sequoiadb111222333" },
  });
  var expResult = [{ a: "Sequoiadb" }, { a: ["Sequoiadb", "巨杉", 999] }];
  commCompareResults(actResult, expResult);

  var actResult = dbcl.find({
    a: { $concat: [-2, ["111", "222", "333"]], $et: "111222Sequoiadb333" },
  });
  var expResult = [{ a: "Sequoiadb" }, { a: ["Sequoiadb", "巨杉", 999] }];
  commCompareResults(actResult, expResult);

  var actResult = dbcl.find({
    a: { $concat: [-1, ["111", "222", "333"]], $et: "111222333Sequoiadb" },
  });
  var expResult = [{ a: "Sequoiadb" }, { a: ["Sequoiadb", "巨杉", 999] }];
  commCompareResults(actResult, expResult);
}
