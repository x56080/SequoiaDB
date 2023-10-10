/***************************************************************************************************
 * @Description: 验证sdbimprt功能的upsert功能
 * @ATCaseID: sdbimprt_at_2
 * @Author: HuangYouquan
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                   并在 Testlink 系统中标记本用例文件名）
 * @Change    Activity:
 * Date       Who         Description
 * ========== =========== =========================================================
 * 09/15/2023 HuangYouquan
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：SDB 正常集群环境
 * 测试场景：
 *    验证指定hint参数
 * 测试步骤：
 *    1. 测试集合准备数据
 *    2. 准备导入数据
 *    3. 导入工具upsert模式导入
 *    4. 校验hint是否生效
 * 预期结果：
 *    1. 集合记录实际与预期相同
 *    2. 导入结果实际与预期相同
 **************************************************************************************************/
testConf.csName = COMMCSNAME;
testConf.clName = COMMCLNAME + "sdbimprt_at_2";

main(test);

function test(testPara) {
   var cl = testPara.testCL;

   var imprtFile = tmpFileDir + "sdbimprt_at_2.json";
   prepareCLData(cl, true);
   exportData(testConf.csName, testConf.clName, imprtFile, "json");
   cl.createIndex("aIndex", { a: 1 });
   cl.createIndex("bIndex", { b: 1 });
   var costTime1, costTime2;
   var start, end;

   // 指定索引aIndex
   cl.truncate();
   prepareCLData(cl, false);
   start = new Date().getTime();
   importData(testConf.csName, testConf.clName, imprtFile, "json", "upsert", "a,b", "aIndex");
   end = new Date().getTime();
   costTime1 = end - start;

   // 指定索引bIndex
   cl.truncate();
   prepareCLData(cl, false);
   start = new Date().getTime();
   importData(testConf.csName, testConf.clName, imprtFile, "json", "upsert", "a,b", "bIndex");
   end = new Date().getTime();
   costTime2 = end - start;

   assert.equal(costTime1 < costTime2, true);
   cmd.run("rm -rf " + COMMCSNAME + "_" + testConf.clName + "*.rec");
   cmd.run("rm -rf " + tmpFileDir);
}

function prepareCLData(cl, forexprt) {
   if (forexprt) {
      for (var i = 0; i < 500; i++) {
         cl.insert({ a: i, b: 500, c: 2 })
      }
   }
   else {
      for (var i = 0; i < 500; i++) {
         cl.insert({ a: i, b: 500, c: i })
      }
   }
}