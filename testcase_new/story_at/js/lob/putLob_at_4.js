/***************************************************************************************************
 * @Description: putLob 主子表分区测试
 * @ATCaseID: putLob_at_4
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
 *    1. 在主子表中，putLob 根据 oid 进行分区
 * 测试步骤：
 *    1. 创建 3 个集合空间 cs1、cs2、cs3，它们的 lobPageSize 分别为 4K 8KB、16KB
 *    2. 在 cs2 中创建主表
 *    3. 分别在 cs1、cs2、cs3 创建一个子表，并将它们挂载至主表中
 *    4. 通过 putLob 写入超出下界的 lob 数据
 *    5. 通过 putLob 写入超出上界的 lob 数据
 *    6. 使用 putLob 通过主表分别向 3 个子表写入 7KB 的数据
 *    7. 检查主表的 lob 监控指标
 * 期望结果：
 *    步骤 4、5：报超出边界错误
 *    步骤 6：putLob 都执行成功
 *    步骤 7：lob 监控指标正常
 **************************************************************************************************/

 testConf.skipStandAlone = true;

 main(test);
 function test() {
    var csName1 = "put_lob_at_4_cs1";
    var csName2 = "put_lob_at_4_cs2";
    var csName3 = "put_lob_at_4_cs3";

    var mainCLName = "put_lob_at_4_maincl";
    var subCLName1 = "put_lob_at_4_subcl1";
    var subCLName2 = "put_lob_at_4_subcl2";
    var subCLName3 = "put_lob_at_4_subcl3";

    var lobPageSize1 = 4 * 1024;   // 4KB
    var lobPageSize2 = 8 * 1024;   // 8KB
    var lobPageSize3 = 16 * 1024;  // 16KB

    commDropCS( db, csName1, true, "init env" );
    commDropCS( db, csName2, true, "init env" );
    commDropCS( db, csName3, true, "init env" );

    var cs1 = db.createCS( csName1, { "LobPageSize": lobPageSize1 } );
    var cs2 = db.createCS( csName2, { "LobPageSize": lobPageSize2 } );
    var cs3 = db.createCS( csName3, { "LobPageSize": lobPageSize3 } );

    var options = { "IsMainCL": true, "ShardingKey": { "date": 1 }, "LobShardingKeyFormat": "YYYYMMDD", "ShardingType": "range" };
    var mainCL = cs2.createCL( mainCLName, options );

    cs1.createCL( subCLName1 );
    cs2.createCL( subCLName2 );
    cs3.createCL( subCLName3 );

    mainCL.attachCL( csName1 + "." + subCLName1, { "LowBound": { "date": "20220101" }, "UpBound": { "date": "20221231" } } );
    mainCL.attachCL( csName2 + "." + subCLName2, { "LowBound": { "date": "20230101" }, "UpBound": { "date": "20231231" } } );
    mainCL.attachCL( csName3 + "." + subCLName3, { "LowBound": { "date": "20240101" }, "UpBound": { "date": "20241231" } } );

    
    var dataFile = "put_lob_at_4_data.txt";
    var readFile = "put_lob_at_4_read.txt";
    // data is 7KB
    var data = generateLobData( lobPageSize2 - 1024 );
    generateLobFile( dataFile, data );

    // case 1: out of low bounds
    assert.tryThrow( SDB_CAT_NO_MATCH_CATALOG, function () {
        var lobId = mainCL.createLobID( "2021-05-05-01.00.00" );
        res = putAndcheckLob( mainCL, lobId, dataFile, readFile );
      });

    // case 2: out of up bounds
    assert.tryThrow( SDB_CAT_NO_MATCH_CATALOG, function () {
        var lobId = mainCL.createLobID( "2025-05-05-01.00.00" );
        res = putAndcheckLob( mainCL, lobId, dataFile, readFile );
      });

    // case 3: lobPageSize subCL < mainCL
    var lobId = mainCL.createLobID( "2022-05-05-01.00.00" );
    putAndcheckLob( mainCL, lobId, dataFile, readFile );

    // case 4: lobPageSize subCL = mainCL
    lobId = mainCL.createLobID( "2023-05-05-01.00.00" );
    putAndcheckLob( mainCL, lobId, dataFile, readFile );

    // case 5: lobPageSize subCL > mainCL
    lobId = mainCL.createLobID( "2024-05-05-01.00.00" );
    putAndcheckLob( mainCL, lobId, dataFile, readFile );

    checkMainCLInfo( mainCL, 3, dataFile );
  
    removeLobFile( dataFile );
    commDropCS( db, csName1, true, "drop env" );
    commDropCS( db, csName2, true, "drop env" );
    commDropCS( db, csName3, true, "drop env" );
 }

 function checkMainCLInfo( mainCL, lobNum, dataFile ) {

    var dataSize = getDataSize( lobNum, dataFile );

    var totalLobs = 0;
    var totalValidLobSize = 0;
    var totalLobPut = 0;
    var totalLobWriteSize = 0;

    var cursor = mainCL.getDetail();
    while( cursor.next() ) {
        var obj = cursor.current().toObj();
        var detailArr = obj["Details"];
        assert.equal( 1, detailArr.length );

        var info = detailArr[0];

        totalLobs += info["TotalLobs"];
        totalValidLobSize += info["TotalValidLobSize"];
        totalLobPut += info["TotalLobPut"];
        totalLobWriteSize += info["TotalLobWriteSize"];
    }
    cursor.close();

    assert.equal( lobNum, totalLobs );
    assert.equal( dataSize, totalValidLobSize );
    assert.equal( lobNum, totalLobPut );
    assert.equal( dataSize, totalLobWriteSize );
 }
 