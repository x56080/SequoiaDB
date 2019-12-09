/******************************************************************************
*@Description : test + - * / % with sql of special decimal value
*               seqDB-13997:使用内置sql对特殊decimal值做数学运算           
*@author      : Liang XueWang 
******************************************************************************/
main();

function main ()
{
   var docs = [{ a: { $decimal: "MAX" } },
   { a: { $decimal: "MIN" } },
   { a: { $decimal: "NaN" } }];

   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );
   var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, 0 );
   insertData( cl, docs );

   var operators = ["+", "-", "*", "/", "%"];
   var expRecs = [{ a: { $decimal: "NaN" } },
   { a: { $decimal: "NaN" } },
   { a: { $decimal: "NaN" } }];
   for( var i = 0; i < operators.length; i++ )
   {
      var op = operators[i];
      println( "test " + op );
      var sql = "select a" + op + "2 from " + COMMCSNAME + "." + COMMCLNAME;
      checkSqlResult( db, sql, expRecs );
   }
}