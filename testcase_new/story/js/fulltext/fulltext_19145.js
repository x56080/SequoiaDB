/************************************
*@Description: seqDB-19145:���ȫ�������������ֶ�Ϊ����Ԫ�أ�ȫ��/����ͬ��(��֧���ڶ������Ԫ���ϴ�������)
*@author:      zhaoyu
*@createdate:  2019.08.14
*@testlinkCase: seqDB-19145
**************************************/
function main ()
{
   if( commIsStandalone( db ) ) { return; }

   var clName = COMMCLNAME + "_19145";
   var textIndexName = "textIndex_19145";
   dropCL( db, COMMCSNAME, clName, true, true );
   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   //{id:5, a:[{0:"obj1"}, {1:"obj2"}, {2:"obj3"}], b:[{0:"obj1"}, {1:"obj2"}, {2:"obj3"}]}�޷�ͬ����ES
   var objs = new Array( { id: 1, a: "string1", b: "string" },
      { id: 2, a: 1, b: 1 },
      { id: 3, a: ["string1", "string2", "string3"], b: ["string1", "string2", "string3"] },
      { id: 4, a: [1, 2, 3], b: [1, 2, 3] },
      { id: 5, a: [{ 0: "obj1" }, { 1: "obj2" }, { 2: "obj3" }], b: [{ 0: "obj1" }, { 1: "obj2" }, { 2: "obj3" }] },
      { id: 6, a: [{ 0: 1 }, { 1: 2 }, { 2: 3 }], b: [{ 0: 1 }, { 1: 2 }, { 2: 3 }] },
      { id: 7, a: { 0: "obj3", 1: "obj4", 2: "obj5" }, b: { 0: "obj3", 1: 1, 2: "obj5" } },
      { id: 8, a: [["string4", "string5", "string6"], ["string7", "string8", "string9"], ["string10", "string11", "string12"]] },
      { id: 9, a: [[4, 5, 6], [7, 8, 9], [10, 11, 12]] } );
   dbcl.insert( objs );
   dbcl.createIndex( textIndexName, { "a.1": "text", "b.2": "text" } );

   var dbOpr = new DBOperator();
   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 1 );
   var findCond = { "": { "$Text": { "query": { "match_all": {} } } } };
   var actResult = dbOpr.findFromCL( dbcl, findCond, { "a": { "$include": 1 }, "b": { "$include": 1 } }, { _id: 1 } );
   var expResult = [{ a: { 0: "obj3", 1: "obj4", 2: "obj5" }, b: { 0: "obj3", 1: 1, 2: "obj5" } }];
   checkResult( expResult, actResult );

   //��֧�ֲ���������Ԫ��Ϊ�����ļ�¼���ᱨ-37����ʱ���θ�����¼
   objs = new Array( { id: 1, a: "string1", b: "string" },
      { id: 2, a: 1, b: 1 },
      { id: 7, a: { 0: "obj3", 1: "obj4", 2: "obj5" }, b: { 0: "obj3", 1: 1, 2: "obj5" } } );
   dbcl.insert( objs );
   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 2 );
   var actResult = dbOpr.findFromCL( dbcl, findCond, { "a": { "$include": 1 }, "b": { "$include": 1 } }, { _id: 1 } );
   var expResult = [{ a: { 0: "obj3", 1: "obj4", 2: "obj5" }, b: { 0: "obj3", 1: 1, 2: "obj5" } },
   { a: { 0: "obj3", 1: "obj4", 2: "obj5" }, b: { 0: "obj3", 1: 1, 2: "obj5" } }];

   findCond = { "": { $Text: { query: { bool: { must: [{ match: { "a.1": "obj4" } }, { match: { "b.2": "obj5" } }] } } } } };
   var actResult = dbOpr.findFromCL( dbcl, findCond, { "a": { "$include": 1 }, "b": { "$include": 1 } }, { _id: 1 } );
   var expResult = [{ a: { 0: "obj3", 1: "obj4", 2: "obj5" }, b: { 0: "obj3", 1: 1, 2: "obj5" } },
   { a: { 0: "obj3", 1: "obj4", 2: "obj5" }, b: { 0: "obj3", 1: 1, 2: "obj5" } }];

   dbcl.remove();
   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 0 );

   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, textIndexName );
   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}
try
{
   main();
}
catch( e )
{
   if( e.constructor === Error )
   {
      println( e.stack );
   }
   throw e;
}
;