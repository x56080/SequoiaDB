/***************************************************************************************************
 * @Description: selector带scalar函数的情况下计算为indexCover
 * @ATCaseID: <填写 story 文档中验收用例的用例编号>
 * @Author: JiangFeng You
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who           Description
 * ========== ============= =========================================================
 * 04/06/2022 JiangFeng You Init
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：
 * 测试场景：
 *    selector带scalar函数的情况下计算为indexCover，索引为{ "a": 1, "b": 1 }
 * 测试步骤：
 *    1. a是scalar函数
 *    2. a是{"a":{"$include":1}
 *    3. a是{"a":{"$include":0}
 *    4. a和b都是scalar函数
 *    5. c是scalar函数
 *    6. c是{"c":{"$include":1}
 *    7. c是{"c":{"$include":0}
 *    8. a是scalar函数且$include为0，b是scalar函数
 *    9. a是scalar函数且$include为1，b是scalar函数
 *    10. a是scalar函数，b是普通字段
 *    11. a是{a:{$include:1}}，b是普通字段
 *    12. a是普通字段，b是scalar函数
 *    13. a是scalar函数，b是scalar函数且$include为0
 *    14. a是scalar函数，b是scalar函数且$include为1
 *    15. a和b都是scalar函数，c是普通字段
 *    16. a和b都是scalar函数，c是{"c":{"$include":0}
 *    17. a是普通字段，b是scalar函数且$include为0，c是普通字段
 * 期望结果：
 *    步骤1、3、5、6、7、8、13、15、16、17为indexcover为false，步骤1、2、4、9、10、11、12、14为true
 **************************************************************************************************/

testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_indexCover_1";

main(test);
function test(args)
{
   try
   {
   db.updateConf( { "indexcoveron": true } );
   var cl = args.testCL ;
   var fullclName = COMMCSNAME + "." + testConf.clName;
   var idxName = "idx";
   cl.createIndex( idxName, { "a": 1, "b": 1 }, { "NotArray": true } );
   db.analyze( { "Collection": fullclName} );
   // a是scalar函数
   var explain = cl.find( {}, {"a":{"$add":1}} ).hint( { "": idxName } ).explain();
   checkIndexCover( explain, true );
   // a是{"a":{"$include":1}
   var explain = cl.find( {}, {"a":{"$include":1}} ).hint( { "": idxName } ).explain();
   checkIndexCover( explain, true );
   // a是{"a":{"$include":0}
   var explain = cl.find( {}, {"a":{"$include":0}} ).hint( { "": idxName } ).explain();
   checkIndexCover( explain, false );
   // a和b都是scalar函数
   var explain = cl.find( {}, {"a":{"$floor":1},"b":{"$size":1 }} ).hint( { "": idxName } ).explain();
   checkIndexCover( explain, true );
   // c是scalar函数
   var explain = cl.find( {}, {"c":{"$substr":1}} ).hint( { "": idxName } ).explain();
   checkIndexCover( explain, false );
   // c是{"c":{"$include":1}
   var explain = cl.find( {}, {"c":{"$include":1}} ).hint( { "": idxName } ).explain();
   checkIndexCover( explain, false );
   // c是{"c":{"$include":0}
   var explain = cl.find( {}, {"c":{"$include":0}} ).hint( { "": idxName } ).explain();
   checkIndexCover( explain, false );
   // a是scalar函数且$include为0，b是scalar函数
   var explain = cl.find( {}, {"a":{"$multiply":1,"$include":0},"b":{"$divide":1}} ).hint( { "": idxName } ).explain();
   checkIndexCover( explain, false );
   // a是scalar函数且$include为1，b为scalar函数
   var explain = cl.find( {}, {"a":{"$abs":1,"$include":1},"b":{"$strlen": 1}} ).hint( { "": idxName } ).explain();
   checkIndexCover( explain, true );
   // a是scalar函数，b是普通字段
   var explain = cl.find( {}, {"a":{"$floor":1},"b":1} ).hint( { "": idxName } ).explain();
   checkIndexCover( explain, true );
   // a是{a:{$include:1}}，b是普通字段
   var explain = cl.find( {}, {"a":{"$include":1},"b":1} ).hint( { "": idxName } ).explain();
   checkIndexCover( explain, true );
   // a是普通字段，b是scalar函数
   var explain = cl.find( {}, {"a":1,"b":{"$slice":1}} ).hint( { "": idxName } ).explain();
   checkIndexCover( explain, true );
   // a是scalar函数，b是scalar函数且$include为0
   var explain = cl.find( {}, {"a":{"$multiply":1},"b":{"$divide":1,"$include":0}} ).hint( { "": idxName } ).explain();
   checkIndexCover( explain, false );
   // a是scalar函数，b为scalar函数且$include为1
   var explain = cl.find( {}, {"a":{"$abs":1},"b":{"$strlen": 1,"$include":1}} ).hint( { "": idxName } ).explain();
   checkIndexCover( explain, true );
   // a和b都是scalar函数，c是普通字段
   var explain = cl.find( {}, {"a":{"$upper":1},"b":{"$lower":1},"c":1} ).hint( { "": idxName } ).explain();
   checkIndexCover( explain, false );
   // a和b都是scalar函数，c是{"c":{"$include":0}
   var explain = cl.find( {}, {"a":{"$concat":1},"b":{"$default":1},"c":{"$include":0}} ).hint( { "": idxName } ).explain();
   checkIndexCover( explain, false );
   // a是普通字段，b是scalar函数且$include为0，c是普通字段
   var explain = cl.find( {}, {"a":1,"b":{"$abs":1},"c":1} ).hint( { "": idxName } ).explain();
   checkIndexCover( explain, false );
   }
   finally
   {
      db.deleteConf( { "indexcoveron": true } );
   }

}

