/******************************************************************************
 * @Description   : seqDB-31517:创建全文索引，设置索引字段不建立映射关系
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.05.22
 * @LastEditTime  : 2023.05.22
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_es_31517";

main( test );
function test ( testPara )
{   
    var dbcl = testPara.testCL;
    var clName = testConf.clName;

    // 插入数据
    var doc = [
        { no: 1, test: "guangzhou", testb: true },
        { no: 2, test: { "city": "shanghai" }, testb: null },
        { no: 3, test: 32, testb: { "test": 1243 } },
        { no: 4, test: { "$numberLong": "3000000000" }, testb: { "$numberLong": "1234500000" } },
        { no: 5, test: -1.7e+308, testb: "false" },
        { no: 6, test: true, testb: 234.567 },
        { no: 7, test: { "$date": "2024-02-01" }, testb: { "$date": "2023-02-01" } },
        { no: 8, test: { "$timestamp": "2022-01-01-13.14.26.124233" }, testb: { "$timestamp": "2023-04-11-13.14.26.124233" } },
        { no: 9, test: { "test": 1002 }, testb: { "test": "true" } },
        { no: 10, test: ["arr01", "arr02"], testb: ["true", "false", "false"] }
    ];
   dbcl.insert( doc );

   // 创建全文索引
    var indexName = "index_31517";
    var mappingsInfo = { "Fields": { "test": { "Index": false } } };
    dbcl.createIndex( indexName, { "test": "text" }, { "Mappings": mappingsInfo } );
    listIndexCheckMappings( dbcl, indexName, mappingsInfo);

    // 检查全文索引结果
    var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, indexName );
    var indexRecordNum = 8;
    checkFullSyncToES( COMMCSNAME, clName, indexName, indexRecordNum, esIndexNames );
    var actRecords1 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match_all": {} } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
    var expectRecords1 = [
        { no: 1, test: "guangzhou", testb: true },
        { no: 3, test: 32, testb: { "test": 1243 } },
        { no: 4, test: 3000000000, testb: 1234500000 },
        { no: 5, test: -1.7e+308, testb: "false" },
        { no: 6, test: true, testb: 234.567 },
        { no: 7, test: { "$date": "2024-02-01" }, testb: { "$date": "2023-02-01" } },
        { no: 8, test: { "$timestamp": "2022-01-01-13.14.26.124233" }, testb: { "$timestamp": "2023-04-11-13.14.26.124233" } },
        { no: 10, test: ["arr01", "arr02"], testb: ["true", "false", "false"] }
    ];
    checkResult( expectRecords1, actRecords1 );

    // 检查全文索引结果
    assert.tryThrow( SDB_INVALIDARG, function () {
        dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match": { "test": "guangzhou" } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
    } );

    assert.tryThrow( SDB_INVALIDARG, function () {
        dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match": { "test": 32 } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
    } );

    assert.tryThrow( SDB_INVALIDARG, function () {
        dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match": { "test": { "city": "shanghai" } } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
    } );

    assert.tryThrow( SDB_INVALIDARG, function () {
        dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match": { "test": 3000000000 } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
    } );

    assert.tryThrow( SDB_INVALIDARG, function () {
        dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match": { "test": -1.7e+308 } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
    } );

    assert.tryThrow( SDB_INVALIDARG, function () {
        dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match": { "test": { "$date": "2024-02-01"} } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
    } );

    assert.tryThrow( SDB_INVALIDARG, function () {
        dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match": { "test": { "$timestamp": "2022-01-01-13.14.26.124233" } } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
    } );

    assert.tryThrow( SDB_INVALIDARG, function () {
        dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match": { "test": "arr01" } } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
    } );

    // 删除全文索引
    dbcl.dropIndex( indexName );
    checkIndexNotExistInES( esIndexNames );
}