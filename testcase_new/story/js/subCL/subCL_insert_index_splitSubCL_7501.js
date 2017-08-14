/* *****************************************************************************
@Description: attach hashCL and insert before split with index
@modify list:
   2014-07-30 pusheng Ding  Init
***************************************************************************** */

function test_range_attach_hash_insert_large_same_before_SplitandIndex() // 
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

   try {
	   //set priority from masterNode
      db.setSessionAttr( {PreferedInstance:"M"} );
      var cs = commCreateCS( db, COMMCSNAME, true, "create cs in the beginning" );
   }catch(e){
      println( "failed to create cs, rc = " + e );
      throw e;
   }
   println( COMMCSNAME ) ;

   try
   {
      var mainCL = cs.createCL( MainCL_Name, { ShardingKey:{ a:1,b:-1 }, ShardingType: "range", ReplSize:0, Compressed:true, IsMainCL:true } ) ;
      println( "mainCL" );
      var subCL1 = cs.createCL( subCl_Name + "1", { ShardingKey:{ a:1 }, ShardingType: "hash", ReplSize:0, Compressed:true, IsMainCL:false } ) ;
      println( "subCL1" );
      var subCL2 = cs.createCL( subCl_Name + "2", { ShardingKey:{ a:1 }, ShardingType: "hash", ReplSize:0, Compressed:true, IsMainCL:false } ) ;
      println( "subCL2" );
      mainCL.attachCL( COMMCSNAME+"."+subCl_Name+"1", { LowBound:{a:0},UpBound:{a:100} } ) ;
      println( "attach subCL1" ) ;
      mainCL.attachCL( COMMCSNAME+"."+subCl_Name+"2", { LowBound:{a:100},UpBound:{a:200} } ) ;
      println( "attach subCL2" ) ;
   }
   catch( e )
   {
      throw e ;
   }
	
	sleep(2000);

   try 
   {
      for(var i = 0; i < 200 ; ++i )
      {
         mainCL.insert( {a:i} ) ;
         mainCL.insert( {a:i+1,b:i+1} ) ;
      }
      println( " The first same data " ) ;
      for(var i = 0; i < 200 ; ++i )
      {
         mainCL.insert( {a:i} ) ;
         mainCL.insert( {a:i+1,b:i+1} ) ;
      }
      println( " The second same data " ) ;
      for(var i = 0; i < 200 ; ++i )
      {
         mainCL.insert( {a:i} ) ;
         mainCL.insert( {a:i+1,b:i+1} ) ;
      }
   }
   catch( e )
   {
      println( "i = " + i + ", err is :" + e ) ;
      throw e ;
   }

   try
   {
      mainCL.createIndex( "aIndex", {a:1, b:-1} ) ;
      println( "************mainCL.createIndex SUCCED***************" ) ;
   }
   catch( e )
   {
      println( " Error: " + e );
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

	println( "Begin to check max/min value of mainCL." ) ;
	var maxValue1 = mainCL.find().sort( {a:1}  ).limit( 1 ).current().toObj()["a"] ;
	var minValue1 = mainCL.find().sort( {a:-1} ).limit( 1 ).current().toObj()["a"] ;
	var mainCnt = mainCL.count() ;
	if( maxValue1 !== 0 || minValue1 !== 200 || Number(mainCnt) !== 1200 )
	{
	   throw buildException("", null, "[check result for mainCL]",
                     "[maxValue1: 0, minValue1: 200, mainCnt: 1200]", 
                     "[maxValue1: "+ maxValue1 +", minValue1: "+ minValue1 +", mainCnt: "+ Number(mainCnt) +"]");
	}
	
	println( "Begin to check max/min value of subCL1." ) ;
	var maxValue1 = subCL1.find().sort( {a:1}  ).limit( 1 ).current().toObj()["a"] ;
	var minValue1 = subCL1.find().sort( {a:-1} ).limit( 1 ).current().toObj()["a"] ;
	var subCnt1 = subCL1.count() ;
	if( maxValue1 !== 0 || minValue1 !== 100 || Number(subCnt1) !== 600 )
	{
	   throw buildException("", null, "[check result for subCL1]",
                     "[maxValue1: 0, minValue1: 200, subCnt1: 1200]", 
                     "[maxValue1: "+ maxValue1 +", minValue1: "+ minValue1 +", subCnt1: "+ Number(subCnt1) +"]");
	}
	
	println( "Begin to check max/min value of subCL2." ) ;
	var maxValue2 = subCL2.find().sort( {a:1}  ).limit( 1 ).current().toObj()["a"] ;
	var minValue2 = subCL2.find().sort( {a:-1} ).limit( 1 ).current().toObj()["a"] ;
	var subCnt2 = subCL2.count() ;
	if( maxValue2 !== 100 || minValue2 !== 200 || Number(subCnt2) !== 600 )
	{
	   throw buildException("", null, "[check result for subCL1]",
                     "[maxValue2: 0, minValue2: 200, subCnt2: 1200]", 
                     "[maxValue2: "+ maxValue2 +", minValue2: "+ minValue2 +", subCnt2: "+ Number(subCnt2) +"]");
	}

}

// Add inspect standalone run mode
try
{ 
   // Inspect the run mode is standalone or not
   if( true == commIsStandalone( db ) )
      throw "ModeStandAlone" ;
   test_range_attach_hash_insert_large_same_before_SplitandIndex();
}
catch( e )
{
   if( "ModeStandAlone" == e )
      println( "The run mode is standalone" ) ;
   else
      throw e ;
}
