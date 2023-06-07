/******************************************************************************
 * @Description   : seqDB-31523:主子表创建全文索引，设置映射类型
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.05.29
 * @LastEditTime  : 2023.05.29
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var mainCLName = COMMCLNAME + "_main_31523";
   var subCLName1 = COMMCLNAME + "_sub_31523_1";
   var subCLName2 = COMMCLNAME + "_sub_31523_2";

   // 创建主子表，并挂载子表
   var mainCL = commCreateCL( db, COMMCSNAME, mainCLName, { IsMainCL: true, ShardingKey: { no: 1 }, ShardingType: 'range' } );
   commCreateCL( db, COMMCSNAME, subCLName1 );
   commCreateCL( db, COMMCSNAME, subCLName2 );
   mainCL.attachCL( COMMCSNAME + "." + subCLName1, { LowBound: { no: 0 }, UpBound: { no: 100 } } );
   mainCL.attachCL( COMMCSNAME + "." + subCLName2, { LowBound: { no: 100 }, UpBound: { no: 200 } } );

   // 创建全文索引
   var idxName = "index_31523";
   mainCL.createIndex( idxName, { "tdate": "text" }, { "Mappings": { "Fields": { "tdate": { "Type": "date" } } } } );

   // 插入数据
   var doc = [
      { no: 1, tdate: { "$date": "2001-01-01" } },
      { no: 2, tdate: { "$date": "0000-01-01" } },
      { no: 3, tdate: { "$date": "2100-11-01" } },
      { no: 4, tdate: { "$date": "1980-01-01" } },
      { no: 5, tdate: { "$date": "2021-01-11" } },
      { no: 116, tdate: { "$date": "2007-11-30" } },
      { no: 117, tdate: { "$date": "2024-05-01" } },
      { no: 118, tdate: { "$date": "2009-08-10" } },
      { no: 119, tdate: { "$date": "9999-02-01" } },
      { no: 110, tdate: { "$date": "3045-07-05" } }
   ];
   mainCL.insert( doc );

   // 检查全文索引结果
   var dbOpr = new DBOperator();
   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, mainCLName, idxName );
   var indexRecordNum = 10
   checkMainCLFullSyncToES( COMMCSNAME, mainCLName, idxName, indexRecordNum, esIndexNames );
   var actRecords1 = dbOpr.findFromCL( mainCL, { "": { "$Text": { query: { "match_all": {} } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords1 = dbOpr.findFromCL( mainCL, {}, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectRecords1, actRecords1 );

   // 根据范围查询
   var matchConf1 = { "tdate": { "lte": "2021-01-01" } };
   var actRecords2 = dbOpr.findFromCL( mainCL, { "": { "$Text": { query: { "range": matchConf1 } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expectRecords2 = dbOpr.findFromCL( mainCL, { "tdate": { "$lte": { "$date": "2021-01-01" } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectRecords2, actRecords2 );

   // 删除索引
   mainCL.dropIndex( idxName );
   checkIndexNotExistInES( esIndexNames );
}