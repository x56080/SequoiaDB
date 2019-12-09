/************************************
*@Description: update object(basic types,null array) with pull_by
*@author:      liuxiaoxuan
*@createdate:  2017.09.18
*@testlinkCase: seqDB-12755/seqDB-12756
**************************************/
function main ()
{
   //clean environment before test
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME, 0 );

   //insert data   
   var doc = [{ a1: 1 },
   { a2: 'aaa' },
   { a3: [] }];
   insertData( dbcl, doc );

   //pull_by
   var updateRule = { $pull_by: { a1: 1, a2: 'aaa', a3: [] } };
   updateData( dbcl, updateRule );

   //check result
   var expResult = [{ a1: 1 },
   { a2: 'aaa' },
   { a3: [] }];
   checkResult( dbcl, null, null, expResult, { _id: 1 } );
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