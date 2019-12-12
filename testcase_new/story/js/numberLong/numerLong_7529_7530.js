/*******************************************************************************
*@Description : seqDB-7529:shell_strict格式的参数校验
seqDB-7530:shell_strict格式的边界值校验
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
   var clName = COMMCLNAME + "_7529";

   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName );

   testMaxBoundary( cl );
   testMinBoundary( cl );
   testOutofBoundary( cl );
   testErrFormat1( cl ); //error format: {$numberLong:123456}
   testErrFormat2( cl ); //error format: {$numberLong:123.56}

   commDropCL( db, COMMCSNAME, clName );
}

function testMaxBoundary ( cl )
{
   cl.remove();
   var rec = { a: { $numberLong: "9223372036854775807" } }; //long max value
   cl.insert( rec );

   var rc = cl.find();
   commCompareResults( rc, [rec] );
}

function testMinBoundary ( cl )
{
   cl.remove();
   var rec = { a: { $numberLong: "-9223372036854775808" } }; //long min value
   cl.insert( rec );

   var rc = cl.find();
   commCompareResults( rc, [rec] );
}

function testOutofBoundary ( cl )
{
   cl.remove();
   var rec = { a: { $numberLong: "9223372036854775808" } };
   cl.insert( rec );

   var expRec = { a: { $numberLong: "9223372036854775807" } };
   var rc = cl.find();
   commCompareResults( rc, [expRec] );

   cl.remove();
   var rec = { a: { $numberLong: "-9223372036854775809" } };
   cl.insert( rec );

   var expRec = { a: { $numberLong: "-9223372036854775808" } };
   var rc = cl.find();
   commCompareResults( rc, [expRec] );
}

function testErrFormat1 ( cl )
{
   cl.remove();

   try
   {
      var rec = { a: { $numberLong: -1 } };
      cl.insert( rec );
      throw new Error( "need throw error" );
   }
   catch( e )
   {
      if( e.message != -6 )
      {
         throw e;
      }
   }

   var rc = cl.find();
   commCompareResults( rc, [] );
}

function testErrFormat2 ( cl )
{
   cl.remove();

   try
   {
      var rec = { a: { $numberLong: "1.1" } };
      cl.insert( rec );
      throw new "need throw error";
   }
   catch( e )
   {
      if( e.message != -6 )
      {
         throw e;
      }
   }

   var rc = cl.find();
   commCompareResults( rc, [] );
}

