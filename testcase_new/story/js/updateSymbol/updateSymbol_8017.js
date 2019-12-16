/************************************
*@Description: update object use more than 1 update operator a the same time.
*@author:      zhaoyu
*@createdate:  2016.5.19
**************************************/
function main ()
{
   //clear environment before test;
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME );

   //insert data
   var doc1 = [{ age: 1 },
   { arr: [1, "string", false, [40, null, { $date: "2016-05-20" }, 30], 10, 7, 10, 20] },
   { name: { firstName: "han", lastName: "meimei" } }];
   insertData( dbcl, doc1 );

   //update object use more than 1 update operator at the same time 
   var updateCondition1 = { $inc: { age: 100 }, $pull: { arr: 10 }, $set: { "name.firstName": "li" } };
   updateData( dbcl, updateCondition1 );

   //check result
   var expRecs1 = [{ age: 101, name: { firstName: "li" } },
   { age: 100, arr: [1, "string", false, [40, null, { $date: "2016-05-20" }, 30], 7, 20], name: { firstName: "li" } },
   { age: 100, name: { firstName: "li", lastName: "meimei" } }];
   checkResult( dbcl, null, null, expRecs1, { _id: 1 } );


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