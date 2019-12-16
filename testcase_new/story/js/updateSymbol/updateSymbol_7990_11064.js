/************************************
*@Description: upsert any object(exist or not exist) use operator set            
*@author:      zhaoyu
*@createdate:  2016.5.17
*@update:      2017.2.17/zhaoyu
**************************************/
function main ()
{
   //clean environment before test
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME );

   //insert object
   var Doc = { a: 1 };
   insertData( dbcl, Doc );

   //upsert any object when match nothing,use matches and
   var upsertCondition1 = {
      $set: {
         object1: 2147483640,
         object2: { $numberLong: "-9223372036854775800" },
         object3: { $decimal: "9223372036854775800" },
         object4: -1.7E+308,
         object5: "string",
         object6: { $oid: "573920accc332f037c000013" },
         object7: false,
         object8: { $date: "2016-05-16" },
         object9: { $timestamp: "2016-05-16-13.14.26.124233" },
         object10: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" },
         object11: { $regex: "^z", $options: "i" },
         object12: { name: "hanmeimei" },
         object13: [12, 34, 36],
         object14: null
      }
   };
   var findCondition1 = {
      $and: [{ object1: { $et: { $decimal: "2" } } },
      { object13: { $all: [10, 20, 30] } },
      { c: { $gt: 100 } },
      { d: 1000 },
      { "e.name.firstName": null }]
   };
   upsertData( dbcl, upsertCondition1, findCondition1 );

   //check result
   var expRecs1 = [{
      object1: 2147483640,
      object2: { $numberLong: "-9223372036854775800" },
      object3: { $decimal: "9223372036854775800" },
      object4: -1.7E+308,
      object5: "string",
      object6: { $oid: "573920accc332f037c000013" },
      object7: false,
      object8: { $date: "2016-05-16" },
      object9: { $timestamp: "2016-05-16-13.14.26.124233" },
      object10: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" },
      object11: { $regex: "^z", $options: "i" },
      object12: { name: "hanmeimei" },
      object13: [12, 34, 36],
      object14: null,
      d: 1000,
      e: { name: { firstName: null } }
   },
   { a: 1 }];
   checkResult( dbcl, null, null, expRecs1, { a: 1 } );

   //upsert any object when match nothing,use matches or
   var upsertCondition2 = {
      $set: {
         object1: 2147483640,
         object2: { $numberLong: "-9223372036854775800" },
         object3: { $decimal: "9223372036854775800" },
         object4: -1.7E+308,
         object5: "string",
         object6: { $oid: "573920accc332f037c000013" },
         object7: false,
         object8: { $date: "2016-05-16" },
         object9: { $timestamp: "2016-05-16-13.14.26.124233" },
         object10: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" },
         object11: { $regex: "^z", $options: "i" },
         object12: { name: "hanmeimei" },
         object13: [12, 34, 36],
         object14: null
      }
   };
   var findCondition2 = {
      $or: [{ object1: { $et: { $decimal: "2" } } },
      { object13: { $all: [10, 20, 30] } },
      { c: { $gt: 100 } },
      { d: 1001 },
      { "e.name.firstName": "han" }]
   };
   upsertData( dbcl, upsertCondition2, findCondition2 );

   //check result
   var expRecs2 = [{
      object1: 2147483640,
      object2: { $numberLong: "-9223372036854775800" },
      object3: { $decimal: "9223372036854775800" },
      object4: -1.7E+308,
      object5: "string",
      object6: { $oid: "573920accc332f037c000013" },
      object7: false,
      object8: { $date: "2016-05-16" },
      object9: { $timestamp: "2016-05-16-13.14.26.124233" },
      object10: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" },
      object11: { $regex: "^z", $options: "i" },
      object12: { name: "hanmeimei" },
      object13: [12, 34, 36],
      object14: null,
      d: 1000,
      e: { name: { firstName: null } }
   },
   {
      object1: 2147483640,
      object2: { $numberLong: "-9223372036854775800" },
      object3: { $decimal: "9223372036854775800" },
      object4: -1.7E+308,
      object5: "string",
      object6: { $oid: "573920accc332f037c000013" },
      object7: false,
      object8: { $date: "2016-05-16" },
      object9: { $timestamp: "2016-05-16-13.14.26.124233" },
      object10: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" },
      object11: { $regex: "^z", $options: "i" },
      object12: { name: "hanmeimei" },
      object13: [12, 34, 36],
      object14: null
   },
   { a: 1 }];
   checkResult( dbcl, null, null, expRecs2, { a: 1 } );

   //delete all data
   deleteData( dbcl, null );

   //upsert any object when match nothing,use matches not
   var upsertCondition3 = {
      $set: {
         object1: 2147483640,
         object2: { $numberLong: "-9223372036854775800" },
         object3: { $decimal: "9223372036854775800" },
         object4: -1.7E+308,
         object5: "string",
         object6: { $oid: "573920accc332f037c000013" },
         object7: false,
         object8: { $date: "2016-05-16" },
         object9: { $timestamp: "2016-05-16-13.14.26.124233" },
         object10: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" },
         object11: { $regex: "^z", $options: "i" },
         object12: { name: "hanmeimei" },
         object13: [12, 34, 36],
         object14: null
      }
   };
   var findCondition3 = {
      $not: [{ object1: { $et: { $decimal: "2" } } },
      { object13: { $all: [10, 20, 30] } },
      { c: { $gt: 100 } },
      { d: 1001 },
      { "e.name.firstName": "han" }]
   };
   upsertData( dbcl, upsertCondition3, findCondition3 );

   //check result
   var expRecs3 = [{
      object1: 2147483640,
      object2: { $numberLong: "-9223372036854775800" },
      object3: { $decimal: "9223372036854775800" },
      object4: -1.7E+308,
      object5: "string",
      object6: { $oid: "573920accc332f037c000013" },
      object7: false,
      object8: { $date: "2016-05-16" },
      object9: { $timestamp: "2016-05-16-13.14.26.124233" },
      object10: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" },
      object11: { $regex: "^z", $options: "i" },
      object12: { name: "hanmeimei" },
      object13: [12, 34, 36],
      object14: null
   }];
   checkResult( dbcl, null, null, expRecs3, { a: 1 } );

   //delete all data
   deleteData( dbcl, null );

   //upsert use or has one condition,2017.2.17/zhaoyu/seqDB-11064
   var upsertCondition4 = { $set: { a: 1 } };
   var findCondition4 = { $or: [{ b: 1 }] };
   upsertData( dbcl, upsertCondition4, findCondition4 );
   var expRecs4 = [{ a: 1, b: 1 }];
   checkResult( dbcl, null, null, expRecs4, { a: 1 } );

   var upsertCondition5 = { $set: { a: 2 } };
   var findCondition5 = { $or: [{ b: { $et: 2 } }] };
   upsertData( dbcl, upsertCondition5, findCondition5 );
   var expRecs5 = [{ a: 1, b: 1 },
   { a: 2, b: 2 }];
   checkResult( dbcl, null, null, expRecs5, { a: 1 } );

   var upsertCondition6 = { $set: { a: 3 } };
   var findCondition6 = { $or: [{ b: { $all: [3] } }] };
   upsertData( dbcl, upsertCondition6, findCondition6 );
   var expRecs6 = [{ a: 1, b: 1 },
   { a: 2, b: 2 },
   { a: 3, b: [3] }];
   checkResult( dbcl, null, null, expRecs6, { a: 1 } );

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