/***************************************************************************************************
 * @Description: 验证全文索引扩展字段检索类型的正确性
 * @ATCaseID: createIndex_with_mappings_at_1
 * @Author: FangJiabin
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                   并在 Testlink 系统中标记本用例文件名）
 * @Change    Activity:
 * Date       Who         Description
 * ========== =========== =========================================================
 * 04/21/2023 FangJiabin  Test the correctness of create fulltext index with mappings
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：SDB 正常集群环境即可，适配器正常启动，ES 一分片一副本即可
 * 测试场景：
 *    1. 验证 SDB 创建索引的正确性：
 *      （1）验证接口参数；
 *      （2）索引快照列表正确显示 mappings 信息；
 *    2. 验证 ES 索引映射和记录的正确性：
 *      （1）验证 ES 索引映射的正确性；
 *      （2）全量索引和增量索引数据时，支持检索的类型字段都会同步到 ES，其中显式指定 Date 类型字段时，bson::Date 和 bson::Timestamp 字段会转换成 String 类型；
 * 测试步骤：
 *    1. SDB创建普通索引，指定 Mappings；
 *    2. SDB创建全文索引，不指定 Mappings
 *    3. SDB创建全文索引，指定错误的 Mappings；
 *    4. SDB创建全文索引，指定正确的 Mappings；
 * 预期结果：
 *    1. 步骤 1 不论 Mappings 格式是否正确，都成功创建索引，索引快照和列表不会显示 Mappings 字段；
 *    2. 步骤 2 成功创建索引，索引快照和列表不会显示 Mappings 字段；
 *    3. 步骤 3 创建索引失败，报错 -6；
 *    4. 步骤 4 成功创建索引：
 *      （1）索引快照和列表正确显示 Mappings 字段；
 *      （2）ES 索引字段映射和映射模板正确设置；
 *      （3）全量索引数据（集合先插测试数据，再创建全文索引），所有支持类型的字段都能同步到 ES；
 *      （4）增量索引数据（集合先创建全文索引，再插测试数据），所有支持类型的字段都能同步到 ES；
 **************************************************************************************************/

testConf.clName = COMMCLNAME + "_fulltext_with_mappings_at_1";

main(test);

function test(testPara) {
  var cl = testPara.testCL;

  createNormalIndexWithMappings( cl ) ;

  createTextIndexWithoutMappings( cl ) ;

  createTextIndexWhitWrongMappings( cl );

  createTextIndexWhitCorrectMappings( cl );
}

function createTextIndexWhitCorrectMappings( cl )
{
  var indexName = "text_idx_3" ;
	
  checkIncIndex( cl, indexName );
  
  cl.dropIndex( indexName );
  cl.remove();
  
  checkFullIndex( cl, indexName );
}

function checkFullIndex( cl, indexName )
{
  var ret ;
  var indexDef = { "tag_id":"text","tag_type":"text","custom_tag":"text","tag":"text","date_1":"text","date_2":"text","date_3":"text"} ;
  var sdbMappings = { "Fields": { "tag_id": { "Index": false },"tag_type": { "Index": false }, "date_1": { "Type": "date" }, "date_2": { "Type": "date" }, "custom_tag.key": { "Type": "keyword" }, "custom_tag.value": { "Type": "keyword" }, "tag": { "Type": "keyword" } } } ;
  var esMappings = {"dynamic_templates":[{"template0":{"match":"tag_id","mapping":{"index":false}}},{"template1":{"match":"tag_type","mapping":{"index":false}}},{"template2":{"path_match":"custom_tag.key","mapping":{"type":"keyword"}}},{"template3":{"path_match":"custom_tag.value","mapping":{"type":"keyword"}}},{"template4":{"match_mapping_type":"double","mapping":{"type":"double"}}},{"template5":{"match_mapping_type":"string","mapping":{"type":"text"}}}],"properties":{"_cllid":{"type":"long"},"_cluid":{"type":"long"},"_hash":{"type":"long"},"_idxlid":{"type":"long"},"_lid":{"type":"long"},"custom_tag":{"properties":{"date_1":{"properties":{"value":{"type":"date"}}},"date_2":{"type":"date"},"key":{"type":"keyword"},"value":{"type":"keyword"}}},"date_1":{"type":"date"},"date_2":{"type":"date"},"date_3":{"type":"date"},"tag":{"type":"keyword"},"tag_id":{"type":"long","index":false},"tag_type":{"type":"text","index":false}}} ;
  var esExpRecords = [] ;

  insertData( cl, esExpRecords ) ;

  cl.createIndex( indexName, indexDef, { "Mappings": sdbMappings } );

  commCheckIndexConsistent( db, COMMCSNAME, testConf.clName, indexName, true ); 

  checkSDBMappings( cl, indexName, sdbMappings );

  checkFullSyncToES( COMMCSNAME, testConf.clName, indexName, esExpRecords.length );
  
  checkESMappingsAndRecords( indexName, esMappings, esExpRecords );
}

function checkIncIndex( cl, indexName )
{
  var ret ;
  var indexDef = { "tag_id":"text","tag_type":"text","custom_tag":"text","tag":"text","date_1":"text","date_2":"text","date_3":"text"} ;
  var sdbMappings = { "Fields": { "tag_id": { "Index": false },"tag_type": { "Index": false }, "date_1": { "Type": "date" }, "date_2": { "Type": "date" }, "custom_tag.key": { "Type": "keyword" }, "custom_tag.value": { "Type": "keyword" }, "tag": { "Type": "keyword" } } } ;
  var esMappings = {"dynamic_templates":[{"template0":{"match":"tag_id","mapping":{"index":false}}},{"template1":{"match":"tag_type","mapping":{"index":false}}},{"template2":{"path_match":"custom_tag.key","mapping":{"type":"keyword"}}},{"template3":{"path_match":"custom_tag.value","mapping":{"type":"keyword"}}},{"template4":{"match_mapping_type":"double","mapping":{"type":"double"}}},{"template5":{"match_mapping_type":"string","mapping":{"type":"text"}}}],"properties":{"_cllid":{"type":"long"},"_cluid":{"type":"long"},"_hash":{"type":"long"},"_idxlid":{"type":"long"},"_lid":{"type":"long"},"custom_tag":{"properties":{"date_1":{"properties":{"value":{"type":"date"}}},"date_2":{"type":"date"},"key":{"type":"keyword"},"value":{"type":"keyword"}}},"date_1":{"type":"date"},"date_2":{"type":"date"},"date_3":{"type":"date"},"tag":{"type":"keyword"},"tag_id":{"type":"long","index":false},"tag_type":{"type":"text","index":false}}} ;
  var esExpRecords = [] ;

  cl.createIndex( indexName, indexDef, { "Mappings": sdbMappings } );

  commCheckIndexConsistent( db, COMMCSNAME, testConf.clName, indexName, true ); 

  checkSDBMappings( cl, indexName, sdbMappings ); 

  insertData( cl, esExpRecords ) ;
  
  checkFullSyncToES( COMMCSNAME, testConf.clName, indexName, esExpRecords.length );
  
  checkESMappingsAndRecords( indexName, esMappings, esExpRecords ) ;
}

function checkESMappingsAndRecords( indexName, expMappings, esExpRecords )
{
  var esOpr = new ESOperator();
  var dbOpr = new DBOperator();
  var queryCond = '{"query" : {"match_all" : {}}}';
  var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, testConf.clName, indexName );
  var actMappings = esOpr.getIndexMappingsFromES( esIndexNames[0] );
  
  if ( JSON.stringify(actMappings) != JSON.stringify(expMappings) )
  {
	println( "exp mappings: " + JSON.stringify(expMappings) );
	println( "act mappings: " + JSON.stringify(actMappings) );	  
    throw new Error("Error es mappings");
  }
  
  var actESRecords = esOpr.findFromES( esIndexNames[0], queryCond );
  checkResult( esExpRecords, actESRecords );
}

function checkSDBMappings( cl, indexName, expMappings )
{
  var ret ;

  try
  {
    ret = cl.getIndex( indexName ) ;
  }
  catch( e )
  {
    if( e != SDB_IXM_NOTEXIST )
    {
      throw new Error( e );
    }
  }

  var actMappings = ret.toObj().IndexDef.Mappings ;
  if ( undefined == actMappings )
  {
     throw new Error("The index meta in catalog doesn't hava Mappings field");
  }

  if ( JSON.stringify(actMappings) != JSON.stringify(expMappings) )
  {
	println( "exp mappings: " + JSON.stringify(expMappings) );
	println( "act mappings: " + JSON.stringify(actMappings) );
    throw new Error("Error mappings in index meta");
  }

  ret = cl.snapshotIndexes({"IndexDef.name":indexName});
  if (!ret.next()) {
    throw new Error("The data node has no "+ indexName + " index");
  }

  actMappings = ret.current().toObj().IndexDef.Mappings ;
  if ( undefined == actMappings )
  {
    throw new Error("The index meta in data doesn't hava Mappings field");
  }

  if ( JSON.stringify(actMappings) != JSON.stringify(expMappings) )
  {
	println( "exp mappings: " + JSON.stringify(expMappings) );
	println( "act mappings: " + JSON.stringify(actMappings) );	  
    throw new Error("Error mappings in index meta");
  }
}

function insertData( cl, esExpRecords )
{
  cl.insert( { tag_id: 0, tag_type: "custom_tag", tag: [], custom_tag: { key: "guangzhou", value: "fang" }, date_1: Timestamp("2023-04-21-16.10.33.123456"), date_2: SdbDate("2023-04-21") } );
  esExpRecords.push( { "tag_id": 0, "tag_type": "custom_tag", "custom_tag": { "key": "guangzhou", "value": "fang" }, "tag": [ ], "date_1": "2023-04-21T16:10:33.123456", "date_2": "2023-04-21" } );
 
  cl.insert( { tag_id: 1, tag_type: "custom_tag", tag: [], custom_tag: { key: "beijing", value: "jia" }, date_1: Timestamp("2023-04-23-16.10.33.123456"), date_2: SdbDate("2023-04-23") } );
  esExpRecords.push( { "tag_id": 1, "tag_type": "custom_tag", "custom_tag": { "key": "beijing", "value": "jia" }, "tag": [ ], "date_1": "2023-04-23T16:10:33.123456", "date_2": "2023-04-23" } );
  
  cl.insert( { tag_id: 2, tag_type: "tag", tag: [ "guangzhou", "fang" ], custom_tag: {}, date_1: Timestamp("2023-04-24-16.10.33.123456"), date_2: SdbDate("2023-04-24") } );
  esExpRecords.push( { "tag_id": 2, "tag_type": "tag", "custom_tag": { }, "tag": [ "fang", "guangzhou" ], "date_1": "2023-04-24T16:10:33.123456", "date_2": "2023-04-24" } );
  
  cl.insert( { tag_id: 3, tag_type: "tag", tag: [ "shenzhen", "bin" ], custom_tag: {}, date_1: Timestamp("2023-04-25-16.10.33.123456"), date_2: SdbDate("2023-04-25") } );
  esExpRecords.push( { "tag_id": 3, "tag_type": "tag", "custom_tag": { }, "tag": [ "bin", "shenzhen" ], "date_1": "2023-04-25T16:10:33.123456", "date_2": "2023-04-25" } );
  
  cl.insert( { tag_id: 4, tag: [ "shanghai", 123, "shenzhen", 456 ], custom_tag: { key: "mixTypeArray" } } );
  esExpRecords.push( { "tag_id": 4, "custom_tag": { "key" : "mixTypeArray" } } );
  
  cl.insert( { tag_id: ObjectId("55713f7953e6769804000001"), custom_tag: { key: "hasUnsupportBSONType" } } );
  esExpRecords.push( { "custom_tag" : { "key" : "hasUnsupportBSONType" } } );
  
  cl.insert( { tag_id: 6, custom_tag: { key: "dateFieldWithoutMappings" }, date_3: SdbDate("2023-04-27") } );
  esExpRecords.push( { "tag_id" : 6, "custom_tag" : { "key" : "dateFieldWithoutMappings" }, "date_3": "2023-04-27" } );
  
  cl.insert( { tag_id: 7, tag_type: "custom_tag", tag: [], custom_tag: { key: "guangzhou", value: "fang", date_1: { value: Timestamp("2023-04-21-16.10.33.123456") }, date_2: SdbDate("2023-04-21") }, date_1: Timestamp("2023-04-21-16.10.33.123456"), date_2: SdbDate("2023-04-21") } );
  esExpRecords.push( { "tag_id": 7, "tag_type": "custom_tag", "custom_tag": { "key": "guangzhou", "value": "fang", "date_1": { "value": "2023-04-21T16:10:33.123456" }, "date_2": "2023-04-21" }, "tag": [ ], "date_1": "2023-04-21T16:10:33.123456", "date_2": "2023-04-21" } );
  
  cl.insert( { tag_id: 8, tag_type: "tag", tag: [ Timestamp("2023-04-24-16.10.33.123456"), Timestamp("2023-04-25-16.10.33.123456") ], custom_tag: {} } );
  esExpRecords.push( { "tag_id": 8, "tag_type": "tag", "custom_tag": { }, "tag": [ "2023-04-24T16:10:33.123456", "2023-04-25T16:10:33.123456" ] } ); 
}

function _createTextIndexWhitWrongMappings( cl, indexName, indexDef, mappings )
{
  try
  { 
    cl.createIndex( indexName, indexDef, mappings );
    throw new Error("createIndex is expected to fail, but actually succeeds");
  }
  catch( e )
  {
    if( e != SDB_INVALIDARG )
    {
      throw new Error( e );
    }
  }
}

function createTextIndexWhitWrongMappings( cl )
{
  var indexName = "text_idx_2" ;
  var indexDef = { "a": "text" } ;
  
  // Mappings 参数只能包含 Fields 字段
  _createTextIndexWhitWrongMappings( cl, indexName, indexDef, { "Mappings": { "Fields": { "a": { "Type": "text" } }, "xxx": "xxx" } } ) ;
  
  // Mappings.Fields 必须是 object
  _createTextIndexWhitWrongMappings( cl, indexName, indexDef, { "Mappings": { "Fields": "xxx" } } ) ;

  // Mappings.Fields 每个字段都必须是 object
  _createTextIndexWhitWrongMappings( cl, indexName, indexDef, { "Mappings": { "Fields": { "a": { "Type": "text" }, "b": 123 } } } ) ;
  
  // Mappings.Fields 字段属性只能设置 Type 和 Index
  _createTextIndexWhitWrongMappings( cl, indexName, indexDef, { "Mappings": { "Fields": { "a": { "Type": "text", "Index": true, "xxx": "xxx" } } } } ) ;
  
  // Mappings.Fields Type 属性值必须是 string
  _createTextIndexWhitWrongMappings( cl, indexName, indexDef, { "Mappings": { "Fields": { "a": { "Type": 123 } } } } ) ;
  
  // Mappings.Fields Index 属性值必须是 bool
  _createTextIndexWhitWrongMappings( cl, indexName, indexDef, { "Mappings": { "Fields": { "a": { "Index": "text" } } } } ) ;
  
  // Mappings.Fields Type 值只能是 text, wildcard, keyword, integer, long, float, double, date
  _createTextIndexWhitWrongMappings( cl, indexName, indexDef, { "Mappings": { "Fields": { "a": { "Type": "xxx" } } } } ) ;
  
  // 索引建必须包含 Mappings.Fields 定义的字段
  // 如果索引建是 { "custom_tags": "text" }, Mappings.Fields 定义的字段可以是 { "custom_tags.key": { "Type": "text" } }，custom_tags 也是包含 custom_tags.key 
  _createTextIndexWhitWrongMappings( cl, "abc.dd", indexDef, { "Mappings": { "Fields": { "abc.d": { "Type": "text" } } } } ) ;  
  _createTextIndexWhitWrongMappings( cl, "abc", indexDef, { "Mappings": { "Fields": { "abcd.d": { "Type": "text" } } } } ) ;
  _createTextIndexWhitWrongMappings( cl, "a", indexDef, { "Mappings": { "Fields": { "abc": { "Type": "text" } } } } ) ;
  _createTextIndexWhitWrongMappings( cl, "abcd", indexDef, { "Mappings": { "Fields": { "abc": { "Type": "text" } } } } ) ;
}

function createTextIndexWithoutMappings( cl )
{
  var indexName = "text_idx_1";
  var ret ;
 
  // 创建全文索引，如果没有指定 Mappings 参数，快照和列表不会显示该字段
  cl.createIndex( indexName, { "a": "text" } );
  commCheckIndexConsistent( db, COMMCSNAME, testConf.clName, indexName, true ); 

  try
  {
    ret = cl.getIndex( indexName ) ;
  }
  catch( e )
  {
    if( e != SDB_IXM_NOTEXIST )
    {
      throw new Error( e );
    }
  }

  if ( undefined != ret.toObj().IndexDef.Mappings )
  {
     throw new Error("The index meta in catalog can't hava Mappings field");
  }

  ret = cl.snapshotIndexes({"IndexDef.name":indexName});
  if (!ret.next()) {
    throw new Error("The data node has no "+ indexName + " index");
  }

  if ( undefined != ret.current().toObj().IndexDef.Mappings )
  {
    throw new Error("The index meta in data can't hava Mappings field");
  }

  cl.dropIndex( indexName );
}

function createNormalIndexWithMappings( cl )
{
  // 创建普通索引，不会检查 Mappings 参数格式是否正确
  // 索引可以创建成功，快照和列表不会显示 Mappings
  var indexName = "normal_idx_1" ;
  var ret ;
  cl.createIndex( indexName, { "a": 1 }, { "Mappings": { "Fieldssss": "a" } } );
  commCheckIndexConsistent( db, COMMCSNAME, testConf.clName, indexName, true );

  try
  {
    ret = cl.getIndex( indexName ) ;
  }
  catch( e )
  {
    if( e != SDB_IXM_NOTEXIST )
    {
      throw new Error( e );
    }
  }

  if ( undefined != ret.toObj().IndexDef.Mappings )
  {
     throw new Error("The index meta in catalog can't hava Mappings field");
  }

  ret = cl.snapshotIndexes({"IndexDef.name":indexName});
  if (!ret.next()) {
    throw new Error("The data node has no "+ indexName + " index");
  }

  if ( undefined != ret.current().toObj().IndexDef.Mappings )
  {
    throw new Error("The index meta in data can't hava Mappings field");
  }
}

