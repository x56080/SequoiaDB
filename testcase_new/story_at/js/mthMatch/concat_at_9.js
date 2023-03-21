/***************************************************************************************************
 * @Description: $concat类型转换-OID
 * @ATCaseID: concat_at_9
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
 *     $concat参数类型为OID的使用
 * 测试步骤：
 *     1. $concat参数为OID，发起查询
 * 期望结果：
 *     期望结果与实际结果一致
 *
 **************************************************************************************************/
testConf.clName = COMMCLNAME + "concat_at_9";

main(test);
function test(testPara) {
  var dbcl = testPara.testCL;
  dbcl.insert({ a: "string" });

  var actResult;
  var expResult;

  actResult = dbcl.find({}, { a: { $concat: { $oid: "123abcd00ef12358902300ef" } } });
  expResult = [{ a: "string123abcd00ef12358902300ef" }];
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({}, { a: { $concat: [-1, [{ $oid: "123abcd00ef12358902300ef" }]] } });
  expResult = [{ a: "123abcd00ef12358902300efstring" }];
  commCompareResults(actResult, expResult);

  expResult = [{ a: "string" }];
  actResult = dbcl.find({
    a: {
      $concat: [0, [{ $oid: "123abcd00ef12358902300ef" }]],
      $et: "string123abcd00ef12358902300ef",
    },
  });
  commCompareResults(actResult, expResult);
}
