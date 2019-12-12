/*******************************************************************************
*@Description : seqDB-7526::shell_输入strict格式，查询显示
seqDB-7527::shell_输入js格式，查询显示
*@Modify List : 2016-3-28  Ting YU  Init
*******************************************************************************/
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


function main ()
{
   var clName = COMMCLNAME + "_7526";

   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName );

   testStrictFormat( cl );
   testJSFormat( cl );

   commDropCL( db, COMMCSNAME, clName );
}

function testStrictFormat ( cl )
{
   cl.remove();

   var val = 2147483647;
   var rec = { a: { $numberLong: val.toString() } };
   cl.insert( rec );

   var rc = cl.find();
   var expRec = { a: 2147483647 };
   commCompareResults( rc, [expRec] );

   cl.remove();

   var val = 9007199254740992;
   var rec = { a: { $numberLong: val.toString() } };
   cl.insert( rec );

   var rc = cl.find();
   var expRec = rec;
   commCompareResults( rc, [expRec] );
}

function testJSFormat ( cl )
{
   cl.remove();

   var val = -2147483647;
   var rec = { a: NumberLong( val.toString() ) };
   cl.insert( rec );

   var rc = cl.find();
   var expRec = { a: val };
   commCompareResults( rc, [expRec] );

   cl.remove();

   var val = -2147483648;
   var rec = { a: NumberLong( val ) };
   var expRec = { a: val };
   cl.insert( rec );
   commCompareResults( rc, [expRec] );
}
