/******************************************************************************
@Description :  seqDB-15544:删除id索引后，执行切分
@Modify list :  2018-8-8  xiaoni Zhao  Init
******************************************************************************/

function main ()
{
   if( true === commIsStandalone( db ) )
   {
      println( "Standalone environment!" );
      return;
   }

   //get groups from sdb
   var groupNames = getGroupNames();
   if( ( 2 > groupNames.length ) )
   {
      println( "Only one group or standalone environment!" );
      return;
   }

   var clName = COMMCLNAME + "_15544";

   //clean before
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" );

   //create CL
   var varCL = commCreateCL( db, COMMCSNAME, clName, { ShardingKey: { a: 1 }, ShardingType: "range" }, true, false, "create CL" );

   //insert data
   for( var i = 1; i <= 50; i++ )
   {
      varCL.insert( { a: i } );
   }

   //get expRecs
   var expRecs = varCL.find().sort( { a: 1 } ).toArray();

   //delete id index
   varCL.dropIdIndex();

   //check id index not existed
   checkIdIndex( clName, false );

   //get srcGroup
   var srcGroup = getSrcGroup( clName );

   //get desGroup
   var desGroup = getDesGroup( groupNames, srcGroup );

   //split
   try
   {
      varCL.split( srcGroup, desGroup, { partition: 1 }, { partition: 26 } );
      throw "NEED_ERROR"
   }
   catch( e )
   {
      if( e !== -279 )
      {
         throw e;
      }
   }

   //check catalog information
   checkCataInfo( clName, srcGroup, 1 );

   //check data
   checkData( expRecs, clName );
}

main(); 
