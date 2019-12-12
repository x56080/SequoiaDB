/*******************************************************************************
*@Description : seqDB-7531:shell_切分范围使用numberLong
seqDB-7533:shell_切分的数据类型为numberLong
seqDB-7532:shell_挂载子表的attach范围使用numberLong
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
   if( commIsStandalone( db ) )
   {
      println( "standalone mode" );
      return;
   }
   if( commGetGroupsNum( db ) < 2 )
   {
      println( "need at least 2 groups" );
      return;
   }

   var csName = COMMCSNAME + "_7531";
   var mainclName = COMMCLNAME + "_7531_main";
   var subclName = COMMCLNAME + "_7531_sub";

   var groups = select2RG();

   //create main cl
   commDropCS( db, csName );
   var clOpt = { IsMainCL: true, ShardingKey: { mk: 1 }, ShardingType: "range" };
   var maincl = commCreateCLByOption( db, csName, mainclName, clOpt );

   //create sub cl( splited )
   var opt = { ShardingKey: { sk: 1 }, ShardingType: "range", Group: groups.srcRG };
   commCreateCLByOption( db, csName, subclName, opt );

   istNumberRecs( csName, subclName );
   splitByLong( csName, subclName, groups.srcRG, groups.tgtRG );

   //attch subcl
   attachByLong( maincl, csName, subclName );
   commDropCS( db, csName );
}

function istNumberRecs ( csName, clName )
{
   var recs = [];
   recs.push( { sk: { $numberLong: "9223372036854775807" } } );
   recs.push( { sk: { $numberLong: "1" } } );
   recs.push( { sk: 2 } );
   recs.push( { sk: 1.95 } );

   db.getCS( csName ).getCL( clName ).insert( recs );
}

function splitByLong ( csName, clName, srcRG, tgtRG )
{
   db.getCS( csName ).getCL( clName ).
      split( srcRG, tgtRG, { sk: { $numberLong: "9223372036854775001" } }, { sk: MaxKey() } );

   var srcNode = db.getRG( srcRG ).getMaster();
   var tgtNode = db.getRG( tgtRG ).getMaster();

   var srcRC = new Sdb( srcNode ).getCS( csName ).getCL( clName ).find().sort( { sk: 1 } );
   var tgtRC = new Sdb( tgtNode ).getCS( csName ).getCL( clName ).find().sort( { sk: 1 } );
   commCompareResults( srcRC, [{ sk: 1 }, { sk: 1.95 }, { sk: 2 }] );
   commCompareResults( tgtRC, [{ sk: { $numberLong: "9223372036854775807" } }] );
}

function attachByLong ( maincl, csName, subclName )
{
   maincl.attachCL( csName + "." + subclName, {
      LowBound: { mk: MinKey() },
      UpBound: { mk: { $numberLong: "9223372036854775806" } }
   } );

   try
   {
      var rec = { mk: { $numberLong: "9223372036854775807" } };
      maincl.insert( rec );
      throw new Error( "need throw error" );
   }
   catch( e )
   {
      if( e.message != -135 )
      {
         throw e;
      }
   }

   var rec = { mk: { $numberLong: "9223372036854775805" } };
   maincl.insert( rec );
   var rc = maincl.find( rec );
   commCompareResults( rc, [rec] );
}
