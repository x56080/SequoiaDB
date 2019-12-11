/*******************************************************************************
*@Description : Aggregate skip test
*@Modify list :
*               2016-03-18  wenjing wang rewrite
*******************************************************************************/
function testSkipFollowMinBound ( cl, docNumber )
{
   var cursor = cl.execAggregate( { $skip: -1 } );
   var retNumber = getRetNumber( cursor );
   if( retNumber !== docNumber )
   {
      throw buildException( "main", 0, "cl.aggregate( {$skip:-1} )",
         docNumber, retNumber );
   }
}

function testSkipMinBound ( cl, docNumber )
{
   var cursor = cl.execAggregate( { $skip: 0 } );
   var retNumber = getRetNumber( cursor );
   if( retNumber !== docNumber )
   {
      throw buildException( "main", 0, "cl.aggregate( {$skip:0} )",
         docNumber, retNumber );
   }
}

function testSkipAnyValue ( cl )
{
   cursor = cl.execAggregate( { $skip: 10 } );
   retNumber = getRetNumber( cursor );
   if( retNumber !== 7 )
   {
      throw buildException( "main", 0, "cl.aggregate( {$skip:10} )",
         7, retNumber );
   }
}

function testSkipMaxBound ( cl )
{
   var cursor = cl.execAggregate( { $skip: 17 } );
   var retNumber = getRetNumber( cursor );
   if( retNumber !== 0 )
   {
      throw buildException( "main", 0, "cl.aggregate( {$skip:17} )",
         0, retNumber );
   }
}

function main ()
{
   var cl = new collection( db, COMMCSNAME, COMMCLNAME );
   cl.create();
   var docNumber = cl.bulkInsert();
   testSkipFollowMinBound( cl, docNumber );
   testSkipMinBound( cl, docNumber );
   testSkipAnyValue( cl );
   testSkipMaxBound( cl );

   cl.drop();
}

main()
