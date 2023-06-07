/******************************************************************************
 * @Description   : seqDB-31542:主子表中带sort执行全文检索
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.05.27
 * @LastEditTime  : 2023.05.27
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;

// main( test ); SEQUOIADBMAINSTREAM-9618 
function test ()
{
   var mainCLName = COMMCLNAME + "_main_31542";
   var subCLName1 = COMMCLNAME + "_sub_31542_1";
   var subCLName2 = COMMCLNAME + "_sub_31542_2";

   // 创建主子表，并挂载子表
   var mainCL = commCreateCL( db, COMMCSNAME, mainCLName, { IsMainCL: true, ShardingKey: { no: 1 }, ShardingType: 'range' } );
   commCreateCL( db, COMMCSNAME, subCLName1 );
   commCreateCL( db, COMMCSNAME, subCLName2 );
   mainCL.attachCL( COMMCSNAME + "." + subCLName1, { LowBound: { no: 0 }, UpBound: { no: 100 } } );
   mainCL.attachCL( COMMCSNAME + "." + subCLName2, { LowBound: { no: 100 }, UpBound: { no: 200 } } );

   // 创建全文索引
   var idxName = "index_31542";
   mainCL.createIndex( idxName, {
      no: "text", tlong: "text", tstr: "text", tdouble: "text", tbool: "text", tdate: "text", ttime: "text", tarr: "text", tobj: "text"
   }, {
      "Mappings": {
         "Fields": {
            "no": { "Type": "integer" }, "tlong": { "Type": "long" }, "tstr": { "Type": "text" }, "tdouble": { "Type": "double" },
            "tbool": { "Type": "boolean" }, "tdate": { "Type": "date" }, "ttime": { "Type": "text" }, "tarr": { "Type": "text" }
         }
      }
   }  );

   // 插入数据
   var docs = [
      { no: 1, tlong: 3000000000, tstr: "2041-03-04", tdouble: -1.7E+308, tbool: true, tdate: { "$date": "2012-01-01" }, ttime: { "$timestamp": "2012-01-01-13.14.26.124233" }, tarr: ["test1", "test2"], tobj: { "city": "shanghai" } },
      { no: 66, tlong: { "$numberLong": "-9223372036854775808" }, tstr: "2000-01-01", tdouble: 1.7E+308, tbool: false, tdate: { "$date": "0000-01-01" }, ttime: { "$timestamp": "2000-01-01-00.00.00.000000" }, tarr: ["str@1", "str@2"], tobj: { "city": "beijing" } },
      { no: 100, tlong: { "$numberLong": "9223372036854775807" }, tstr: "test01", tdouble: -0.123, tbool: false, tdate: { "$date": "9999-12-31" }, ttime: { "$timestamp": "2037-12-31-23.59.59.999999" }, tarr: ["arr!0", "atest02"], tobj: { "city": "guangzhou" } },
      { no: 166, tlong: 2300000000, tstr: "2020-01-03T02:00:00", tdouble: 234.56, tbool: true, tdate: { "$date": "1970-01-01" }, ttime: { "$timestamp": "2022-01-01-03.14.26.124233" }, tarr: ["011", "234"], tobj: { "city": "shengzhen" } },
      { no: 188, tlong: { "$numberLong": "-1" }, tstr: "测试数据类型01", tdouble: 5000.46, tbool: false, tdate: { "$date": "2022-05-01" }, ttime: { "$timestamp": "2025-04-01-15.14.26.124233" }, tarr: ["test01", "atest02"], tobj: { "city": "wuhan" } }
   ];
   mainCL.insert( docs );

   // 检查全文索引结果
   var dbOpr = new DBOperator();
   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, mainCLName, idxName );
   var indexRecordNum = 5;
   checkMainCLFullSyncToES( COMMCSNAME, mainCLName, idxName, indexRecordNum, esIndexNames );

   // no字段作为排序字段
   var actRecords1 = dbOpr.findFromCL( mainCL, { "": { "$Text": { query: { "match_all": {} }, "sort": [ { "no": { "order": "asc" } } ] } } }, { _id: { "$include": 0 } } );
   var expectRecords1 = dbOpr.findFromCL( mainCL, {}, { _id: { "$include": 0 } }, { no: 1 } );
   checkResult( expectRecords1, actRecords1 );

   // tlong字段作为排序字段
   var actRecords2 = dbOpr.findFromCL( mainCL, { "": { "$Text": { query: { "match_all": {} }, "sort": [ { "tlong": { "order": "asc" } } ] } } }, { _id: { "$include": 0 } } );
   var expectRecords2 = dbOpr.findFromCL( mainCL, {}, { _id: { "$include": 0 } }, { tlong: 1 } );
   checkResult( expectRecords2, actRecords2 );

   // tstr字段作为排序字段
   var actRecords3 = dbOpr.findFromCL( mainCL, { "": { "$Text": { query: { "match_all": {} }, "sort": [ { "tstr": { "order": "asc" } } ] } } }, { _id: { "$include": 0 } } );
   var expectRecords3 = dbOpr.findFromCL( mainCL, {}, { _id: { "$include": 0 } }, { tstr: 1 } );
   checkResult( expectRecords3, actRecords3 );

   // tdouble字段作为排序字段
   var actRecords4 = dbOpr.findFromCL( mainCL, { "": { "$Text": { query: { "match_all": {} }, "sort": [ { "tdouble": { "order": "asc" } } ] } } }, { _id: { "$include": 0 } } );
   var expectRecords4 = dbOpr.findFromCL( mainCL, {}, { _id: { "$include": 0 } }, { tdouble: 1 } );
   checkResult( expectRecords4, actRecords4 );

   // tbool字段作为排序字段
   var actRecords5 = dbOpr.findFromCL( mainCL, { "": { "$Text": { query: { "match_all": {} }, "sort": [ { "tbool": { "order": "asc" } } ] } } }, { _id: { "$include": 0 } } );
   var expectRecords5 = dbOpr.findFromCL( mainCL, {}, { _id: { "$include": 0 } }, { tbool: 1 } );
   checkResult( expectRecords5, actRecords5 );

   // tdate字段作为排序字段
   var actRecords6 = dbOpr.findFromCL( mainCL, { "": { "$Text": { query: { "match_all": {} }, "sort": [ { "tdate": { "order": "asc" } } ] } } }, { _id: { "$include": 0 } } );
   var expectRecords6 = dbOpr.findFromCL( mainCL, {}, { _id: { "$include": 0 } }, { tdate: 1 } );
   checkResult( expectRecords6, actRecords6 );

   // ttime字段作为排序字段
   var actRecords7 = dbOpr.findFromCL( mainCL, { "": { "$Text": { query: { "match_all": {} }, "sort": [ { "ttime": { "order": "asc" } } ] } } }, { _id: { "$include": 0 } } );
   var expectRecords7 = dbOpr.findFromCL( mainCL, {}, { _id: { "$include": 0 } }, { ttime: 1 } );
   checkResult( expectRecords7, actRecords7 );

   // tarr字段作为排序字段
   var actRecords8 = dbOpr.findFromCL( mainCL, { "": { "$Text": { query: { "match_all": {} }, "sort": [ { "tarr": { "order": "asc" } } ] } } }, { _id: { "$include": 0 } } );
   var expectRecords8 = dbOpr.findFromCL( mainCL, {}, { _id: { "$include": 0 } }, { tarr: 1 } );
   checkResult( expectRecords8, actRecords8 );

   // tobj字段作为排序字段
   var actRecords9 = dbOpr.findFromCL( mainCL, { "": { "$Text": { query: { "match_all": {} }, "sort": [ { "tobj": { "order": "asc" } } ] } } }, { _id: { "$include": 0 } } );
   var expectRecords9 = dbOpr.findFromCL( mainCL, {}, { _id: { "$include": 0 } }, { tobj: 1 } );
   checkResult( expectRecords9, actRecords9 ); 

   // 删除索引
   mainCL.dropIndex( idxName );
   checkIndexNotExistInES( esIndexNames );
}