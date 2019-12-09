/************************************
*@Description: find decimal data
*@author:      zhaoyu
*@createdate:  2016.4.25
**************************************/
function main ()
{
   //clean environment before test
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME, 0 );

   //insert data
   var doc = [{ a: { $decimal: "123" } },
   { a: { $numberLong: "123" } },
   { a: 123 }];
   insertData( dbcl, doc );

   //check result
   checkResult( dbcl, { a: { $decimal: "123" } }, null, [{ a: { $decimal: "123" } }, { a: 123 }, { a: 123 }], { _id: 1 } );
   checkResult( dbcl, { a: 123 }, null, [{ a: { $decimal: "123" } }, { a: 123 }, { a: 123 }], { _id: 1 } );
}

main();