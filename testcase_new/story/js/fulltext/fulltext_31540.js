/******************************************************************************
 * @Description   : seqDB-31540:创建全文索引接口参数校验
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.05.22
 * @LastEditTime  : 2023.05.22
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_es_31540";

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   var clName = testConf.clName;
   var idxName = "idx_31540";

   // 插入数据
   var docs = [
      { no: 1, tstr: "123", tlong: { "$numberLong": "123" }, tdouble: 123.456, tarr: [123, 456, 789] },
      { no: 2147483647, tstr: "2147483647", tlong: { "$numberLong": "2147483647" }, tarr: [2147483647, 0] },
      { no: -2147483648, tstr: "-2147483648", tlong: { "$numberLong": "-2147483648" }, tarr: [-12147483648, 0] },
      { no: -1, tstr: "-1", tlong: { "$numberLong": "2147483647" }, tarr: [-1, 1, 0, 2] }
   ];
   dbcl.insert( docs );

   // Mappings参数
   // a、Type取值为null、空串“”、非范围内值如“test”
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.createIndex( idxName, { tstr: "text" }, { "Mappings": { "Fields": { "tstr": { "Type": null } } } } );
   } );
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.createIndex( idxName, { tstr: "text" }, { "Mappings": { "Fields": { "tstr": { "Type": "" } } } } );
   } );
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.createIndex( idxName, { tstr: "text" }, { "Mappings": { "Fields": { "tstr": { "Type": "test" } } } } );
   } );

   // b、Index取值不正确，为null、空串“”、非范围内值如“test”
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.createIndex( idxName, { tstr: "text" }, { "Mappings": { "Fields": { "tstr": { "Index": null } } } } );
   } );
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.createIndex( idxName, { tstr: "text" }, { "Mappings": { "Fields": { "tstr": { "Index": "" } } } } );
   } );
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.createIndex( idxName, { tstr: "text" }, { "Mappings": { "Fields": { "tstr": { "Index": "test" } } } } );
   } );

   // c、Type类型不正确，如为int类型
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.createIndex( idxName, { tstr: "text" }, { "Mappings": { "Fields": { "tstr": { "Type": "int" } } } } );
   } );

   // d、结构类型不正确：“Fields”为其它值，如"Field"、"Type"字段名为其它值
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.createIndex( idxName, { tstr: "text" }, { "Mappings": { "Field": { "tstr": { "Type": "integer" } } } } );
   } );
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.createIndex( idxName, { tstr: "text" }, { "Mappings": { "Fields": { "tstr": { "Type1": "integer" } } } } );
   } );

   // e、指定索引字段名和索引键不一致
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.createIndex( idxName, { tstr: "text" }, { "Mappings": { "Fields": { "tstr1": { "Type": "integer" } } } } );
   } );

   // f、创建普通索引指定mappings参数
   var normalIdxName = "normal_idx_31540";
   dbcl.createIndex( normalIdxName, { "tstr": 1 }, { "Mappings": { "Fields": { "tstr": { "Type": "text" } } } } );
   listIndexCheckNoMappings( dbcl, normalIdxName );

   // 正常创建全文索引
   dbcl.createIndex( idxName, {
      no: "text", tstr: "text", tlong: "text", tdouble: "text", tarr: "text"
   }, { "Mappings": { "Fields": { "no": { "Type": "integer" } } } } );
   listIndexCheckMappings( dbcl, idxName, { "Fields": { "no": { "Type": "integer" } } } );
   
   // 检查全文索引结果
   var dbOpr = new DBOperator();
   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, idxName );

   // 删除索引
   dbcl.dropIndex( idxName );
   checkIndexNotExistInES( esIndexNames );
}

function listIndexCheckNoMappings ( dbcl, indexName )
{
   var cursor = dbcl.listIndexes();
   while( cursor.next() )
   {
      var actIndexDef = cursor.current().toObj().IndexDef;
      var actIndexName = actIndexDef.name;
      if( actIndexName == indexName )
      {
         if( "Mappings" in actIndexDef )
         {
            throw new Error( "check exist mappings info !, mappings = " + JSON.stringify( actIndexDef.Mappings ) );

         }
      }
   }
   cursor.close();
}
