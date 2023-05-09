/***************************************************************************************************
 * @Description: _putLobValue(<data>, [oid]) 接口测试 
 * @ATCaseID: putLob_at_1
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
 *    _putLobValue(<data>, [oid]) 接口测试，具体如下：
 *    1. 不指定参数
 *    2. data 为空字符串
 *    3. data 为错误类型，如数值
 *    4. data 为 string
 *    5. data 为 string，oid 为错误类型，如数值
 *    6. data 为 string，oid 为普通 ObjectId() 对象
 *    7. data 为 String，oid 为 createLobID() 生成的 oid
 *    8. data 为 String，oid 为重复
 *    9. data size 超出 255KB
 * 测试步骤：
 *    调用 _putLobValue() 接口，并检查写入的 lob 数据是否正确
 * 期望结果：
 *    1. 报 -259
 *    2. 正常执行，且查询到的 lob 数据正确
 *    3. 报 -6
 *    4. 正常执行，且查询到的 lob 数据正确
 *    5. 报 -6
 *    6. 报 -6
 *    7. 正常执行，且查询到的 lob 数据正确
 *    8. 报 -5， lob 已经存在
 *    9. 正常执行，且查询到的 lob 数据正确
 **************************************************************************************************/

 testConf.skipStandAlone = true;

 main(test);
 function test() {
    var csName = "put_lob_at_1_cs";
    var clName = "put_lob_at_1_cl";
    var data = "put_lob_test";

    commDropCS( db, csName, true, "init env" );

    var cl = db.createCS( csName ).createCL( clName );

    // case 1: no param
    assert.tryThrow( SDB_OUT_OF_BOUND, function () {
        res = cl._putLobValue();
      });

    // case 2: data is ""
    putLobTest( cl, undefined, "" );

    // case 3: data is error type
    assert.tryThrow( SDB_INVALIDARG, function () {
        res = cl._putLobValue( 123 );
      });

    // case 4: oid is undefined
    putLobTest( cl, undefined, data );

    // case 5: oid is error type
    assert.tryThrow( SDB_INVALIDARG, function () {
        res = cl._putLobValue( data, 123 );
      });

    // case 6: oid is error type
    assert.tryThrow( SDB_INVALIDARG, function () {
        var lobId = new ObjectId();
        res = cl._putLobValue( data, lobId );
      });

    // case 7: normal data and oid
    var lobId = cl.createLobID();
    putLobTest( cl, lobId, data );

    // case 8: repeat oid
    assert.tryThrow( SDB_FE, function () {
        res = putLobTest( cl, lobId, data );
      });

    // case 9: large data
    data = generateLobData( 256 * 1024, 'a' ) ;
    putLobTest( cl, undefined, data );

    commDropCS( db, csName, true, "drop env" );
 }

 function putLobTest( cl, lobId, data ) {
    if ( undefined == lobId )
    {
        lobId = cl._putLobValue( data );
    }
    else
    {
        cl._putLobValue( data, lobId );
    }

    var dataFile = "put_lob_at_1_data.txt";
    var readFile = "put_lob_at_1_read.txt";

    generateLobFile( dataFile, data );

    println(lobId);
    cl.getLob( lobId, readFile, true );

    var expectMD5 = File.md5( dataFile );
    var actualMD5 = File.md5( readFile );

    removeLobFile( dataFile );
    removeLobFile( readFile );

    assert.equal( expectMD5, actualMD5 );
 }
 
 