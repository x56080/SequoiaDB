/***************************************************************************************************
 * @Description: _putLobFile(<filePath>, [oid]) 接口测试 
 * @ATCaseID: putLob_at_2
 * @Author: Yang Qincheng
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who           Description
 * ========== ============= =========================================================
 * 05/05/2023 Yang Qincheng  Init
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：普通复制组
 * 测试场景：
 *    _putLobFile(<filePath>, [oid]) 接口测试，具体如下：
 *    1. 不指定参数
 *    2. filePath 为空字符串
 *    3. filePath 为错误类型，如数值
 *    4. filePath 为 string
 *    5. filePath 为 string，oid 为错误类型，如数值
 *    6. filePath 为 String，oid 为 createLobID() 生成的 oid
 *    7. file size 大于 255KB
 * 测试步骤：
 *    调用 _putLobFile() 接口，并检查写入的 lob 数据是否正确
 * 期望结果：
 *    1. 报 -259
 *    2. 报 -6
 *    3. 报 -6
 *    4. 正常执行，且查询到的 lob 数据正确
 *    5. 报 -6
 *    6. 正常执行，且查询到的 lob 数据正确
 *    7. 报 -7
 **************************************************************************************************/

 testConf.skipStandAlone = true;

 main(test);
 function test() {
    var csName = "put_lob_at_2_cs";
    var clName = "put_lob_at_2_cl";
    var dataFile = "put_lob_at_2_data.txt";
    var readFile = "put_lob_at_2_read.txt";

    var data = "put_lob_test";
    generateLobFile( dataFile, data );

    commDropCS( db, csName, true, "init env" );

    var cl = db.createCS( csName ).createCL( clName );

    // case 1: no param
    assert.tryThrow( SDB_OUT_OF_BOUND, function () {
        res = cl._putLobFile();
      });

    // case 2: filePath is ""
    assert.tryThrow( SDB_FNE, function () {
        res = cl._putLobFile( "" );
      });

    // case 3: filePath is error type
    assert.tryThrow( SDB_INVALIDARG, function () {
        res = cl._putLobFile( 123 );
      });

    // case 4: normal filePath
    putAndcheckLob( cl, undefined, dataFile, readFile );

    // case 5: oid is error type
    assert.tryThrow( SDB_INVALIDARG, function () {
        res = cl._putLobFile( dataFile, 123 );
      });

    // case 6:  normal filePath and oid
    putAndcheckLob( cl, undefined, dataFile, readFile );

    // case 7: lagre file
    data = generateLobData( 256 * 1024 );
    generateLobFile( dataFile, data );
    assert.tryThrow( SDB_INVALIDSIZE, function () {
        res = cl._putLobFile( dataFile );
      });

    removeLobFile( dataFile );
    commDropCS( db, csName, true, "drop env" );
 }
 