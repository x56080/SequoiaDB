/******************************************************************************
*@Description : test CRUD with NumberLong function
*@Modify list :
*               2016-07-11   XueWang Liang  Init
******************************************************************************/

function main ( db )
{
   var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, 0, true, true, true, "create CL in the begining..." );


   // 以NumberLong函数的方式插入数据
   cl.insert( { number: NumberLong( 100 ) } );
   cl.insert( { number: NumberLong( "200" ) } );

   // 以NumberLong函数的方式查询更新数据
   var rc = cl.find( { number: NumberLong( 100 ) } );
   var expRecs = { number: 100 };
   checkRec( rc, [expRecs] );

   cl.update( { $inc: { number: 2 } }, { number: NumberLong( "200" ) } );
   rc = cl.find( { number: NumberLong( "202" ) } );
   expRecs = { number: 202 };
   checkRec( rc, [expRecs] );

   // 以普通方式查询数据
   rc = cl.find( { number: 100 } );
   expRecs = { number: 100 };
   checkRec( rc, [expRecs] );

   // 使用$type匹配符查询数据
   rc = cl.find( { number: { $type: 1, $et: 18 } } );
   expRecs = [{ number: 100 }, { number: 202 }];
   checkRec( rc, expRecs );


   println( ">success to test CRUD with NumberLong function.\n\n" );
}


// Test
try
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
      "clean collection in the beginning" );
   main( db );
}
catch( e )
{
   throw e;
}
finally
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
      "clean collection in the end, wrong" );
   db.close();
}
