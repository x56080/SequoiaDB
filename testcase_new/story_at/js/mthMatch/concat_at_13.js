/***************************************************************************************************
 * @Description: $concat类型转换-array
 * @ATCaseID: concat_at_13
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
 *     $concat参数类型为array的使用
 * 测试步骤：
 *     1. $concat参数为array，发起查询
 *     2. $concat参数array里有各种类型的元素，发起查询
 * 期望结果：
 *     期望结果与实际结果一致
 *
 **************************************************************************************************/
testConf.clName = COMMCLNAME + "concat_at_13";

main(test);
function test(testPara) {
  var dbcl = testPara.testCL;
  dbcl.insert({ a: "string" });

  var actResult;
  var expResult;

  actResult = dbcl.find({}, { a: { $concat: [-1, [["abc"]]] } });
  expResult = [{ a: '[ "abc" ]string' }];
  commCompareResults(actResult, expResult);

  actResult = dbcl.find({}, { a: { $concat: [0, [["abc"], 123, true]] } });
  expResult = [{ a: 'string[ "abc" ]123true' }];
  commCompareResults(actResult, expResult);

  actResult = dbcl.find(
    {},
    { a: { $concat: [0, [{ $timestamp: "2012-01-01-13.14.26.124233" }, 123, true]] } }
  );
  expResult = [{ a: "string2012-01-01-13.14.26.124233123true" }];
  commCompareResults(actResult, expResult);

  actResult = dbcl.find(
    {},
    {
      a: {
        $concat: [
          0,
          [
            { $timestamp: "2012-01-01-13.14.26.124233" },
            { $decimal: "123.4563123131313131311111111" },
            { $oid: "123abcd00ef12358902300ef" },
          ],
        ],
      },
    }
  );
  expResult = [
    { a: "string2012-01-01-13.14.26.124233123.4563123131313131311111111123abcd00ef12358902300ef" },
  ];
  commCompareResults(actResult, expResult);

  expResult = [{ a: "string" }];
  actResult = dbcl.find({
    a: {
      $concat: [-1, [["abc"]]],
      $et: '[ "abc" ]string',
    },
  });
  commCompareResults(actResult, expResult);
}
