/******************************************************************************
 * @Description   : seqDB-31529:指定条件范围查询
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.05.26
 * @LastEditTime  : 2023.05.29
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_es_31529";

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   var clName = testConf.clName;

   // 创建全文索引
   var idxName = "idx_31529";
   dbcl.createIndex( idxName, { "tstr1": "text" }, { "Mappings": { "Fields": { "tstr1": { "Type": "text" } } } } );

   // 插入数据
   var docs = [
      { tstr1: "123", tstr2: "123 string", tint: 2147483647, tobj: { "city": "guangzhou" } },
      { tstr1: "123", tstr2: "字符串 123", tint: 1, tobj: { "city": "guangzhou" } },
      { tstr1: "abc", tstr2: "abcstring", tint: 0, tobj: { a: 1.001 } },
      { tstr1: "字符串 string string123", tstr2: "字符串", tint: -2147483648, tobj: { a: 1, b: 2 } },
      { tstr1: "字符串 string test", tstr2: "string", tint: -2147483648, tobj: { a: 1, b: 2 } }
   ];
   dbcl.insert( docs );

   // 检查全文索引结果
   var dbOpr = new DBOperator();
   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, idxName );
   var indexRecordNum = 5;
   checkFullSyncToES( COMMCSNAME, clName, idxName, indexRecordNum, esIndexNames );
   var actRecords1 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { "match_all": {} } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expRecords1 = dbOpr.findFromCL( dbcl, {}, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expRecords1, actRecords1 );

   // 操作符$and匹配
   var andMatchConf = {
      "match": {
         "tstr1": {
            "query": "字符串 string string123",
            "operator": "and"
         }
      }
   };
   var actRecords2 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: andMatchConf } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expRecords2 = [{ tstr1: "字符串 string string123", tstr2: "字符串", tint: -2147483648, tobj: { a: 1, b: 2 } }];
   checkResult( expRecords2, actRecords2 );

   // 操作符$or匹配
   var orMatchConf = {
      "match": {
         "tstr1": {
            "query": "字符串 string string123",
            "operator": "or",
            "minimum_should_match": 2
         }
      }
   };
   var actRecords3 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: orMatchConf } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expRecords3 = [
      { tstr1: "字符串 string string123", tstr2: "字符串", tint: -2147483648, tobj: { a: 1, b: 2 } },
      { tstr1: "字符串 string test", tstr2: "string", tint: -2147483648, tobj: { a: 1, b: 2 } }
   ];
   checkResult( expRecords3, actRecords3 );

   // match_prase短语匹配
   var matchPhraseConf = { "match_phrase": { "tstr1": "字符串" } };
   var actRecords4 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: matchPhraseConf } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expRecords3, actRecords4 );

   // match_phrase_prefix前缀匹配
   var matchPhrasePrefixConf = { "match_phrase_prefix": { "tstr1": "字符" } };
   var actRecords5 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: matchPhrasePrefixConf } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expRecords3, actRecords5 );

   // multi_match多字段匹配
   var multiMatchConf = { "multi_match": { "query": "字符串", "fields": ["tstr1", "tstr2"] } };
   var actRecords6 = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: multiMatchConf } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   var expRecords6 = [
      { tstr1: "字符串 string string123", tstr2: "字符串", tint: -2147483648, tobj: { a: 1, b: 2 } },
      { tstr1: "字符串 string test", tstr2: "string", tint: -2147483648, tobj: { a: 1, b: 2 } }
   ];
   checkResult( expRecords6, actRecords6 );

   // 删除索引
   dbcl.dropIndex( idxName );
   checkIndexNotExistInES( esIndexNames );
}