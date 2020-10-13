/******************************************************************************
@Description : 1. Query timestamp type sort without index
               2. Query timestamp type forced sort through index
@Modify list :
               2015-01-16 pusheng Ding  Init
               2020-08-14 Zixian Yan    Modify
******************************************************************************/
testConf.clName = COMMCLNAME + "_13746";
main( test );

function test ( testPara )
{
   var cl = testPara.testCL;
   var indexName = "index_13746";
   var data = [ { a: { "$timestamp": "2000-01-01-01.01.01.100000" }, b: 2, c: "abcd" },
	             { a: { "$timestamp": "2000-01-01-01.01.01.000001" }, b: 1, c: "efghi" },
	             { a: { "$timestamp": "2011-11-30-17.04.01.123456" }, b: 4, c: "xyz" },
	             { a: { "$timestamp": "2010-12-31-17.04.01.123456" }, b: 3, c: "jklmn" } ];

   var expectation = [ { a: { "$timestamp": "2000-01-01-01.01.01.000001" }, b: 1, c: "efghi" },
                       { a: { "$timestamp": "2000-01-01-01.01.01.100000" }, b: 2, c: "abcd" },
             	        { a: { "$timestamp": "2010-12-31-17.04.01.123456" }, b: 3, c: "jklmn" },
                       { a: { "$timestamp": "2011-11-30-17.04.01.123456" }, b: 4, c: "xyz" } ];

   cl.insert( data );

   //Query without index
   var query1 = cl.find().sort( {a: 1} );
   checkRec( query1, expectation);

   cl.createIndex( indexName, {a: 1} );

   // Query forced through index
   var query2 = cl.find().hint( { "": indexName } );
   checkRec( query2, expectation );
}
