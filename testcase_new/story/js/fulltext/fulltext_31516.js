/******************************************************************************
 * @Description   : seqDB-31516:全文索引字段映射为数组类型，插入索引字段类型为兼容转换类型
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.05.26
 * @LastEditTime  : 2023.05.26
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_es_31516";

main( test );
function test ( testPara )
{
    var dbcl = testPara.testCL;
    var clName = testConf.clName;

    // 创建全文索引
    var idxName = "idx_31516";
    dbcl.createIndex( idxName, { "tarr": "text" } );

    // 插入数据
    var docs = [
        { no: 1, tarr: "shanghai" },
        { no: 2, tarr: 1 },
        { no: 3, tarr: { "$numberLong": "3000000000" } },
        { no: 4, tarr: 1.1 },
        { no: 5, tarr: -3.4e+38 },
        { no: 6, tarr: -1.7e+308 },
        { no: 7, tarr: { "$date": "2012-01-01" } },
        { no: 8, tarr: { "$timestamp": "2012-01-01-13.14.26.124233" } }
    ];
    dbcl.insert( docs );

    // 检查全文索引结果
    var dbOpr = new DBOperator();
    var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, idxName );
    var indexRecordNum = 8
    checkFullSyncToES( COMMCSNAME, clName, idxName, indexRecordNum, esIndexNames );
    var actRecords1 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match_all": {} } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
    var expectRecords1 = dbOpr.findFromCL( dbcl, {}, { _id: { "$include": 0 } }, { _id: 1 } );
    checkResult( expectRecords1, actRecords1 );

    // 删除索引
    dbcl.dropIndex( idxName );
    checkIndexNotExistInES( esIndexNames );
}