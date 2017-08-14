function test_range_attach_hash_upsert_basic()// NOT Error, test mainCL'ShardingType is range and subCL's ShardingType is hash , insert's result
{
	MainCL_Name = CHANGEDPREFIX + "year" ;
	subCl_Name = CHANGEDPREFIX + "month" ;
   try
   {
      commDropCL( db, COMMCSNAME, subCl_Name + "1", true, true,
                  "clean sub collection" );
      commDropCL( db, COMMCSNAME, subCl_Name + "2", true, true,
                  "clean sub collection" );
      commDropCL( db, COMMCSNAME, MainCL_Name, true, true,
                  "clean main collection" );
   }
   catch( e )
   {
      println( "failed to drop main and sub cl, rc = " + e );
      throw e;
   }
	try
	{
		 //set priority from masterNode
        db.setSessionAttr( {PreferedInstance:"M"} );
        var cs = commCreateCS( db, COMMCSNAME, true, "create cs in the beginning" );
		var mainCL = cs.createCL( MainCL_Name, { ShardingKey:{ a:1 }, ShardingType: "range", Partition:4096, ReplSize:0, Compressed:true, IsMainCL:true } ) ;
		println( "mainCL" );
		var subCL1 = cs.createCL( subCl_Name + "1", { ShardingKey:{ a:1}, ShardingType: "hash", ReplSize:0, Compressed:true, IsMainCL:false } ) ;
		println( "subCL1" );
		var subCL2 = cs.createCL( subCl_Name + "2", { ShardingKey:{ a:1 }, ShardingType: "hash", ReplSize:0, Compressed:true, IsMainCL:false } ) ;
		println( "subCL2" );
		mainCL.attachCL( COMMCSNAME+"."+subCl_Name+"1", { LowBound:{b:0},UpBound:{a:1} } ) ;
		println( "attach subCL1" ) ;
		mainCL.attachCL( COMMCSNAME+"."+subCl_Name+"2", { LowBound:{a:1},UpBound:{a:2} } ) ;
		println( "attach subCL2" ) ;
	}
	catch( e )
	{
		throw e ;
	}
	
   try
   {
      var subCL = [] ;
      subCL.push( subCL1 ) ;
      subCL.push( subCL2 ) ;
      var numberOfsubCl = 2 ;
      for( var i = 0; i < numberOfsubCl; ++i )
      {
         var sourceDataGroupName = getSourceGroupName_alone( COMMCSNAME, subCl_Name + ( i + 1 ) );
         println( "sourceDataGroupName is : " + sourceDataGroupName ) ;

         var desDataGroupName = getOtherDataGroups( sourceDataGroupName ) ;
         println("desDataGroupName is "+desDataGroupName);

         var Partition = getPartition( COMMCSNAME, subCl_Name + ( i + 1 ) );
         println( "Partition is : " + Partition ) ;

         if( !subCL_split_hash( subCL[i], sourceDataGroupName, desDataGroupName, Partition) )
         {
            println( "************SPLIT SUCCED***************" ) ;
         }
      }
   }
   catch( e )
   {
      println( " Error: " + e );
      throw e ;
   }
	
	try
	{
		for(var i = 0; i < 2 ; ++i )
		{
			mainCL.insert( {a:i, b:1} ) ;
		}
	}
	catch( e )
	{
		println( "i = " + i + ", err is :" + e ) ;
	}
	
	try
	{
	   mainCL.upsert( {$set:{b:2}}, {b:1}) ;
	}
	catch ( e )
	{
	   println( "failed to uupsert( {$set:{b:2}}, {b:1}) record, rc= " + e ) ;
	   throw e ;
	}
	
	var rc ;
	try
	{
	   rc = mainCL.find({b:2}) ;
	}
	catch ( e )
	{
	   println( "failed to read record, rc= " + e ) ;
	   throw e ;
	}
	
	var size = rc.count();
	if ( 2 != size )
	{
	   println( " get more than one record after upsert($set:{a:2}}, {a:1})" ) ;
	   throw -1 ;
	}
	
	try
	{
	   mainCL.upsert( {$set:{b:4}}, {b:3}) ;
	}
	catch ( e )
	{
	   println( "failed to insert record {$set:{b:4}}, {b:3}, rc= " + e ) ;
	   throw e ;
	}
	
	try
	{
	   rc = mainCL.find({b:4}) ;
	}
	catch ( e )
	{
	   println( "failed to read record, rc= " + e ) ;
	   throw e ;
	}

	var size = rc.count();
	if ( 1 != size )
	{
	   println( " get more than one record after upsert($set:{a:2}}, {a:1})" ) ;
	   throw -1 ;
	}	
	
	try
	{
	   rc = mainCL.find() ;
	}
	catch ( e )
	{
	   println( "failed to read record, rc= " + e ) ;
	   throw e ;
	}

	var size = rc.count();
	if ( 3 != size )
	{
	   println( " count() err" ) ;
	   throw -1 ;
	}	
	
//	println( "mainCL.find({a:0})" ) ;
//	println( mainCL.find({a:0}) ) ;
//	println( "mainCL.find({a:1})" ) ;
//	println( mainCL.find({a:1}) ) ;
//	
//	println( "subCL1.find({a:0})" ) ;
//	println( subCL1.find({a:0}) ) ;
//	println( "subCL1.find({a:1})" ) ;
//	println( subCL1.find({a:1}) ) ;
//	
//	println( "subCL2.find({a:0})" ) ;
//	println( subCL2.find({a:0}) ) ;
//	println( "subCL2.find({a:1})" ) ;
//	println( subCL2.find({a:1}) ) ;
}

function main()
{
	try
	{
		db.listReplicaGroups();
	}
	catch( e )
	{
		if( e == -159 )
		{
			println( "can't run in standalone" ) ;
			return ;
		}
		println( "fail to check standalone" ) ;
		throw e ;
	}
	println( "test_range_attach_hash_upsert_basic is start" ) ;
	test_range_attach_hash_upsert_basic();
	println( "test_range_attach_hash_upsert_basic is end" ) ;
	println() ;
	
}

main() ;


