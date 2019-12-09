/******************************************************************************
*@Description : test SdbQueryOption
*               TestLink :  seqDB-15740:ָ��update��ѯ��¼
*@auhor       : CSQ 
******************************************************************************/
function main ()
{
   if( commGetGroupsNum( db ) < 2 )
   {
      return;
   }
   try
   {
      commDropCS( db, COMMCSNAME + "15740", true, "drop CS " + COMMCSNAME + "15740" );
   } catch( e ) { }
   var groups = commGetGroups( db );
   var srcGroupName = groups[0][0].GroupName;
   var destGroupName = groups[1][0].GroupName;
   var varCS = commCreateCS( db, COMMCSNAME + "15740", true, "create CS" );
   var varCL = varCS.createCL( COMMCLNAME + "15740", { ShardingKey: { typeint: 1 }, ShardingType: "hash", Group: srcGroupName } );
   insertRecord( varCL );
   testupdate15740( varCL );

   try
   {
      commDropCS( db, COMMCSNAME + "15740", true, "drop CS " + COMMCSNAME + "15740" );
   } catch( e ) { }
}

function testupdate15740 ( varCL )
{

   try
   {
      var cur = varCL.find( new SdbQueryOption().update( { $inc: { typeint: 1 }, $set: { typefloat: 1.3 } }, true, { KeepShardingKey: true } ) );
   }
   catch( e )//
   {
      if( e != "-178" )
      {
         throw buildException( "compare testupdate15740 fail", e, "check", "-178", e );
      }
   }
   var cur = varCL.find( new SdbQueryOption().update( { $set: { typefloat: 1.4 } }, true, { KeepShardingKey: false } ) );
   var size = 0;
   while( cur.next() )
   {
      var ret = cur.current();
      size++;
      if( ret.toObj().typefloat != 1.4 )
      {
         throw buildException( "compare testupdate15740 update({$set:{typefloat:1.4}} fail", "expected result", "check", 1.4, ret.toObj().typefloat );
      }
   }
   if( size != 1 )
   {
      throw buildException( "compare testupdate15740 update({$set:{typefloat:1.4}} fail", "expected result", "check", 1, size );
   }

   var cur = varCL.find( new SdbQueryOption().update( { $inc: { typefloat: 1 } }, false, { KeepShardingKey: false } ) );
   var size = 0;
   while( cur.next() )
   {
      var ret = cur.current();
      size++;
      if( ret.toObj().typefloat != 1.4 )
      {
         throw buildException( "compare testupdate15740 update({$inc:{typefloat:1}}, false, {KeepShardingKey:false} ) fail", "expected result", "check", 1.4, ret.toObj().typefloat );
      }
   }
   if( size != 1 )
   {
      throw buildException( "compare testupdate15740 update({$inc:{typefloat:1}}, false, {KeepShardingKey:false} ) fail", "expected result", "check", 1, size );
   }
}

function insertRecord ( varCL )
{
   varCL.insert( { typeint: 123, typefloat: 123.456 } );
}

main( db );