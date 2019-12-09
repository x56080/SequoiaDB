/******************************************************************************
@Description :  seqDB-15545:指定AutoIndexId:false，加入域并使用自动切分
@Modify list :  2018-8-8  xiaoni Zhao  Init
******************************************************************************/
function main ()
{
   if( commIsStandalone( db ) )
   {
      println( "standalone environment!" );
      return;
   }

   var groupNames = getGroupNames();
   if( groupNames.length < 2 )
   {
      println( "Only one group or standalone environment!" );
      return;
   }

   var csName = COMMCSNAME + "_15545";
   var clName = COMMCLNAME + "_15545";
   var domName = "mydomain" + "_15545"
   var srcGroup = groupNames[0];
   var desGroup = groupNames[1];

   commDropDomain( db, domName );
   commCreateDomain( db, domName, [srcGroup, desGroup] );

   var dbcl = db.createCS( csName, { Domain: domName } ).createCL( clName, { ShardingKey: { a: 1 }, ShardingType: "hash", AutoIndexId: false, AutoSplit: true } )

   //check id index not existed; 
   checkIdIndex( clName, false, csName );

   //check catalog information
   checkCataInfo( clName, srcGroup, 2, csName );

   for( var i = 1; i <= 50; i++ )
   {
      dbcl.insert( { a: i } );
   }

   checkSplitResult( srcGroup, desGroup, clName, csName )

   commDropCS( db, csName )
   commDropDomain( db, domName );
}

main()
