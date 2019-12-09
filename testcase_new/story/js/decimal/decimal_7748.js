/************************************
*@Description: delete decimal data
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
   var doc = [{ a: { $decimal: "9223372036854775807198410" } },
   { a: { $decimal: "-9223372036854775808197101" } },
   { a: { $decimal: "-1.7E+398" } },
   { a: { $decimal: "1.7E+378" } },
   { a: { $decimal: "-4.94065645841246544E-380" } },
   { a: { $decimal: "4.94065645841246544E-390" } },
   { a: { $decimal: "9223372036854775807198411", $precision: [1000, 100] } },
   { a: { $decimal: "-9223372036854775808197102", $precision: [1000, 100] } },
   { a: { $decimal: "-1.71E+398", $precision: [1000, 100] } },
   { a: { $decimal: "1.71E+378", $precision: [1000, 100] } },
   { a: { $decimal: "-4.964065645841246544E-380", $precision: [1000, 999] } },
   { a: { $decimal: "4.964065645841246544E-390", $precision: [1000, 999] } }];
   insertData( dbcl, doc );

   //delete data
   deleteData( dbcl );

   //check result
   var recordNum = dbcl.count();
   if( 0 !== parseInt( recordNum ) )
   {
      println( "remove decimal data failure!\nthe acture recordNum is:" + recordNum + ",and the expect recordNum is 0!" );
      throw "check result failed.";
   }
   else
   {
      println( "check date ok!" );
   }
}

main();