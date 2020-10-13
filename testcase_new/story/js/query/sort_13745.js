/******************************************************************************
@Description : 1. Query1 string type fields sort wihtout index
               2. Query2 string type fields forced sort by index
@Modify list :
               2015-01-16 pusheng Ding  Init
               2020-08-12 Zixian Yan    Modify
******************************************************************************/
testConf.clName = COMMCLNAME + "_13745";
main( test );

function test ( testPara )
{
   var cl = testPara.testCL;
   var indexName = "index_13745";
   var data = [ { a: "book", b: 2, c: "abcd" },
                { a: "agree", b: 1, c: "efghi" },
                { a: "dog", b: 4, c: "xyz" },
                { a: "cat", b: 3, c: "jklmn" } ];

   var expectation = [ {a:"agree",b:1}, {a:"book",b:2}, {a:"cat",b:3}, {a:"dog",b:4} ];
   cl.insert( data );

   var query1 = cl.find( null, { a: "default", b: 0 } ).sort( { a: 1 } );
   checkRec( query1, expectation );

   cl.createIndex( indexName, {a: 1} );
   var query2 = cl.find( null, { a: "default", b: 0, c: "default" } ).hint( { "": indexName } );
   checkRec( query2, expectation );
}
