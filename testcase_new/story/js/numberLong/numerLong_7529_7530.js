/*******************************************************************************
*@Description : seqDB-7529:shell_strict格式的参数校验
seqDB-7530:shell_strict格式的边界值校验
*@Modify List : 2016-3-28  Ting YU  Init
*******************************************************************************/
main();

function main ()
{
   try
   {
      var csName = COMMCSNAME;
      var clName = COMMCLNAME;

      var clObj = new Collection( csName, clName, { ReplSize: 0 } );
      var cl = clObj.create();

      testMaxBoundary( cl );
      testMinBoundary( cl );
      testOutofBoundary( cl );
      testErrFormat1( cl ); //error format: {$numberLong:123456}
      testErrFormat2( cl ); //error format: {$numberLong:123.56}
   }
   catch( e )
   {
      throw e;
   }
}

function testMaxBoundary ( cl )
{
   println( '---begin to insert long max value' );

   cl.remove();
   var rec = { a: { $numberLong: "9223372036854775807" } }; //long max value
   cl.insert( rec );

   var rc = cl.find();
   checkRec( rc, [rec] );
}

function testMinBoundary ( cl )
{
   println( '---begin to insert long min value' );

   cl.remove();
   var rec = { a: { $numberLong: "-9223372036854775808" } }; //long min value
   cl.insert( rec );

   var rc = cl.find();
   checkRec( rc, [rec] );
}

function testOutofBoundary ( cl )
{
   println( '---begin to insert the value greater than long max value' );

   cl.remove();
   var rec = { a: { $numberLong: "9223372036854775808" } };
   cl.insert( rec );

   var expRec = { a: { $numberLong: "9223372036854775807" } };
   var rc = cl.find();
   checkRec( rc, [expRec] );

   println( '---begin to insert the value less than long min value' );

   cl.remove();
   var rec = { a: { $numberLong: "-9223372036854775809" } };
   cl.insert( rec );

   var expRec = { a: { $numberLong: "-9223372036854775808" } };
   var rc = cl.find();
   checkRec( rc, [expRec] );
}

function testErrFormat1 ( cl )
{
   println( '---begin to insert error format: {$numberLong:-1}' );
   cl.remove();

   try
   {
      var rec = { a: { $numberLong: -1 } };
      cl.insert( rec );
      throw "did not throw error";
   }
   catch( e )
   {
      if( e !== -6 )
      {
         throw buildException( "check return code", "", "cl.insert( {a:{$numberLong:-1}} )",
            "throw -6", e );
      }
   }

   var rc = cl.find();
   checkRec( rc, [] );
}

function testErrFormat2 ( cl )
{
   println( '---begin to insert error format: {$numberLong:"1.1"}' );
   cl.remove();

   try
   {
      var rec = { a: { $numberLong: "1.1" } };
      cl.insert( rec );
      throw "did not throw error";
   }
   catch( e )
   {
      if( e !== -6 )
      {
         throw buildException( "check return code", "", 'cl.insert( {a:{$numberLong:"1.1"}} )',
            "throw -6", e );
      }
   }

   var rc = cl.find();
   checkRec( rc, [] );
}



