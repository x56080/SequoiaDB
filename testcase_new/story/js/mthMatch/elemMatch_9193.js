/************************************
*@Description: $elemMatch
*@author:      zhaoyu
*@createdate:  2016.10.31
*@testlinkCase: 
**************************************/
function main ()
{
   //clean environment before test
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME, 0 );

   //insert data 
   var doc = [{ No: 1, a: [-10, 10], b: [-10, 10] },
   { No: 2, a: { 0: -10, 1: 10 }, b: { 0: -10, 1: 10 } }];
   insertData( dbcl, doc );

   //query
   var findCondition1 = { a: { $elemMatch: { 0: -10 } } };
   var expRecs1 = [{ No: 2, a: { 0: -10, 1: 10 }, b: { 0: -10, 1: 10 } }];
   checkResult( dbcl, findCondition1, null, expRecs1, { _id: 1 } );
}
main()