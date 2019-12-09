/******************************************************************************
@Description :   seqDB-15547:创建id索引后，执行切分
@Modify list :   2018-8-9  xiaoni Zhao  Init
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

   var clName = COMMCLNAME + "_15547";

   //clean before
   commDropCL( db, COMMCSNAME, clName, true, true, "drop the CL before!" );

   var varCL = commCreateCLByOption( db, COMMCSNAME, clName, { ShardingKey: { a: 1 }, ShardingType: "hash", AutoIndexId: false, ReplSize: 2, Compressed: true, CompressionType: "lzw" }, true, false, "create CL" );

   //check id index not existed
   checkIdIndex( clName, false );

   //insert data
   for( var i = 0; i < 50; i++ )
   {
      varCL.insert( { a: i } );
   }

   //get expRecs
   var expRecs = varCL.find().toArray();

   //create id index
   varCL.createIdIndex();

   //get srcGroup
   var srcGroup = getSrcGroup( clName );

   //get desGroup
   var desGroup = getDesGroup( groupNames, srcGroup );

   //split
   varCL.split( srcGroup, desGroup, 50 );

   //check the succeed split result
   checkSplitResult( srcGroup, desGroup, clName )
}

main(); 
