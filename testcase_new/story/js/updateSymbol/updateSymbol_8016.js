/************************************
*@Description: update object as index.
*@author:      zhaoyu
*@createdate:  2016.5.19
**************************************/
function main ()
{
   //clear environment before test;
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME, 0 );

   //create index
   commCreateIndex( dbcl, "ageIndex", { age: 1 } );
   commCreateIndex( dbcl, "arr", { arr: -1 } );
   commCreateIndex( dbcl, "name", { name: 1 } );

   //insert data
   var doc1 = [{ age: 1 },
   { arr: [1, "string", false, [40, null, { $date: "2016-05-20" }, 30], 10, 7, 10, 20] },
   { name: { firstName: "han", lastName: "meimei" } }];
   insertData( dbcl, doc1 );

   //update common object as index 
   var updateCondition1 = { $set: { age: 100 } };
   var findCondition1 = { age: { $exists: 1 } };
   updateData( dbcl, updateCondition1, findCondition1 );

   //check result
   var expRecs1 = [{ age: 100 },
   { arr: [1, "string", false, [40, null, { $date: "2016-05-20" }, 30], 10, 7, 10, 20] },
   { name: { firstName: "han", lastName: "meimei" } }];
   checkResult( dbcl, null, null, expRecs1, { _id: 1 } );

   //update arr object as index 
   var updateCondition2 = { $pull: { arr: 10 } };
   var findCondition2 = { arr: { $exists: 1 } };
   updateData( dbcl, updateCondition2, findCondition2 );

   //check result
   var expRecs2 = [{ age: 100 },
   { arr: [1, "string", false, [40, null, { $date: "2016-05-20" }, 30], 7, 20] },
   { name: { firstName: "han", lastName: "meimei" } }];
   checkResult( dbcl, null, null, expRecs2, { _id: 1 } );

   //update nested arr's element 
   var updateCondition3 = { $pull: { "arr.3": null } };
   var findCondition3 = { arr: { $exists: 1 } };
   updateData( dbcl, updateCondition3, findCondition3 );

   //check result
   var expRecs3 = [{ age: 100 },
   { arr: [1, "string", false, [40, { $date: "2016-05-20" }, 30], 7, 20] },
   { name: { firstName: "han", lastName: "meimei" } }];
   checkResult( dbcl, null, null, expRecs3, { _id: 1 } );

   //update nested object 
   var updateCondition4 = { $set: { "name.firstName": "li" } };
   var findCondition4 = { name: { $exists: 1 } };
   updateData( dbcl, updateCondition4, findCondition4 );

   //check result
   var expRecs4 = [{ age: 100 },
   { arr: [1, "string", false, [40, { $date: "2016-05-20" }, 30], 7, 20] },
   { name: { firstName: "li", lastName: "meimei" } }];
   checkResult( dbcl, null, null, expRecs4, { _id: 1 } );
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