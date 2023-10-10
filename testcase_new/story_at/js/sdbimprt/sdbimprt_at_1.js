/***************************************************************************************************
 * @Description: 验证sdbimprt功能的upsert功能
 * @ATCaseID: sdbimprt_at_1
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
 *    验证matchfield参数
 * 测试步骤：
 *    1. 测试集合准备数据
 *    2. 准备导入数据
 *    3. 导入工具upsert模式导入
 *    4. 校验集合数据
 * 预期结果：
 *    1. 集合记录实际与预期相同
 *    2. 导入结果实际与预期相同
 **************************************************************************************************/
testConf.csName = COMMCSNAME;
testConf.clName = COMMCLNAME + "sdbimprt_at_1";

main(test);

function test(testPara) {
   var cl = testPara.testCL;

   var imprtFile = tmpFileDir + "sdbimprt_at_1.json";
   readyData(imprtFile);

   // 匹配单个字段
   prepareCLData(cl);
   var rcResults = importData(testConf.csName, testConf.clName, imprtFile, "json", "upsert", "a", "_id");
   checkImportRC(rcResults, 4, 3, 1);
   var expResult = [{ a: 1, b: 3, c: 3 }, { a: 2, b: 4, c: 4 }, { a: 3, b: 5, c: 5 }, { a: 4, b: 5, c: 6 }, { a: 5, d: 5 }];
   checkCLData(cl, expResult);
   cl.truncate();

   // 匹配多个字段
   prepareCLData(cl);
   var rcResults = importData(testConf.csName, testConf.clName, imprtFile, "json", "upsert", "a, _id, c", "_id");
   checkImportRC(rcResults, 4, 3, 1);
   var expResult = [{ a: 1, b: 3, c: 3 }, { a: 2, b: 4, c: 4 }, { a: 3, b: 5, c: 5 }, { a: 4, b: 5, c: 6 }, { a: 5, d: 5 }];
   checkCLData(cl, expResult);

   cmd.run("rm -rf " + COMMCSNAME + "_" + testConf.clName + "*.rec");
   cmd.run("rm -rf " + tmpFileDir);
}

function prepareCLData(cl) {
   cl.insert({ _id: 1, a: 1, b: 2, c: 3 });
   cl.insert({ _id: 2, a: 2, b: 3, c: 4 });
   cl.insert({ _id: 3, a: 3, b: 4, c: 5 });
   cl.insert({ _id: 4, a: 4, b: 5, c: 6 });
}

function readyData(imprtFile) {
   var file = fileInit(imprtFile);
   file.write("{ _id : 1, a: 1, b: 3 }\n");
   file.write("{ _id : 2, a: 2, b: 4 }\n");
   file.write("{ _id : 3, a: 3, b: 5 }\n");
   file.write("{ _id : 4, a: 4, b: 5 }\n");
   file.write("{ _id : 5, a: 5, d: 5 }");
   file.close();
}