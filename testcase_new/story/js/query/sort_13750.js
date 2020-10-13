/******************************************************************************
@Description : 1. Query multiple type of data & sort without index
               2.Query multiple type of data & forced sort through index
@Modify list :
               2015-01-17 pusheng Ding  Init
               2020-08-14 Zixian Yan    Modify
******************************************************************************/
testConf.clName = COMMCLNAME + "_13750";
main( test );

function test ( testPara )
{
   var cl = testPara.testCL;
   var indexName = "index_13750";
   var data = [ { a: 1,  b: 100, type: "int" },
                { a: 2,  b: 123456789012, type: "longInt" },
                { a: 3,  b: 1.1234e-12, type: "float" },
                { a: 4,  b: '1234abcd', type: "string" },
                { a: 5,  b: { "$oid": "123abcd00ef12358902300ef" }, type: "oid" },
                { a: 6,  b: true, type: "boolean" },
                { a: 7,  b: { "$date": "2015-01-17" }, type: "date" },
                { a: 8,  b: { "$timestamp": "2015-01-17-10.59.30.124233" }, type: "timestamp" },
                { a: 9,  b: { "$regex": "^张", "$options": "1" }, type: "regex" },
                { a: 10, b: { "subobj": "value" }, type: "object" },
                { a: 11, b: ["abc", 100, "def"], type: "array" },
                { a: 12, b: null, type: "null" },
                { a: 13, type: "empty" } ];


   var expectation = [ { a: 13, type: "empty" },
                       { a: 12, b: null, type: "null" },
                       { a: 3,  b: 1.1234e-12, type: "float" },
                       { a: 1,  b: 100, type: "int" },
                       { a: 11, b: ["abc", 100, "def"], type: "array" },
                       { a: 2,  b: 123456789012, type: "longInt" },
                       { a: 4,  b: '1234abcd', type: "string" },
                       { a: 10, b: { "subobj": "value" }, type: "object" },
                       { a: 5,  b: { "$oid": "123abcd00ef12358902300ef" }, type: "oid" },
                       { a: 6,  b: true, type: "boolean" },
                       { a: 7,  b: { "$date": "2015-01-17" }, type: "date" },
                       { a: 8,  b: { "$timestamp": "2015-01-17-10.59.30.124233" }, type: "timestamp" },
                       { a: 9,  b: { "$regex": "^张", "$options": "1" }, type: "regex" } ];

   cl.insert( data );
   
   //Query without index
   var query1 = cl.find().sort( {b: 1} );
   checkRec( query1, expectation);

   cl.createIndex( indexName, {b: 1} );

   // Query forced through index
   var query2 = cl.find().hint( { "": indexName } );
   checkRec( query2, expectation );
}
