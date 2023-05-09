/***************************************************************************************************
 * @Description: 修改 lobPageSize
 * @ATCaseID: putLob_at_5
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
 *    修改 cs 的 lobPageSize 后，再执行 putLob
 * 测试步骤：
 *    1. 创建普通表，lobPageSize 设置为 8KB
 *    2. 写入 1000 条普通数据
 *    3. 将 lobPageSize 修改为 4KB
 *    5. 使用 putLob 分别写入 1KB、4KB、8KB 的数据
 *    6. 检查写入的 lob 数据是否正确
 * 期望结果：
 *    步骤 5：putLob 都执行成功
 *    步骤 6：putLob 数据正确
 **************************************************************************************************/

 testConf.skipStandAlone = true;

 main(test);
 function test() {
    var csName = "put_lob_at_5_cs";
    var clName = "put_lob_at_5_cl";
    var lobPageSize1 = 8 * 1024; // 8KB
    var lobPageSize2 = 4 * 1024; // 4KB
    var dataFile = "put_lob_at_5_data.txt";
    var readFile = "put_lob_at_5_read.txt";

    commDropCS( db, csName, true, "init env" );
    var cs = db.createCS( csName, { "LobPageSize": lobPageSize1 } );
    var cl = cs.createCL( clName );
    insertDoc( cl, 1000 );

    cs.alter( { "LobPageSize": lobPageSize2 } );

    // case 1: put 1KB
    var data1 = generateLobData( 1024 );
    generateLobFile( dataFile, data1 );
    putAndcheckLob( cl, undefined, dataFile, readFile );

    // case 2: put 4KB
    var data2 = generateLobData( lobPageSize2 );
    generateLobFile( dataFile, data2 );
    putAndcheckLob( cl, undefined, dataFile, readFile );

    // case 3: put 8KB
    var data3 = generateLobData( lobPageSize1 );
    generateLobFile( dataFile, data3 );
    putAndcheckLob( cl, undefined, dataFile, readFile );

    removeLobFile( dataFile );
    commDropCS( db, csName, true, "drop env" );
 }
 