/***************************************************************************
@Description :seqDB-17726：increment为正值，超大范围调整
@Modify list :
              2019-1-28  zhaoyu  Create
****************************************************************************/
main( test );
function test ()
{
   var coordNodes = getCoordNodeNames( db );
   var coordNum = coordNodes.length;
   if( commIsStandalone( db ) || coordNum !== 3 )
   {
      return;
   }
   var sortField = 0;
   var increment = 5;
   var cacheSize = 1000;
   var acquireSize = 11;
   var maxValue = { $numberLong: "9223372036854775807" };
   var minValue = { $numberLong: "-9223372036854775808" };
   var clName = COMMCLNAME + "_17726";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: "id", AcquireSize: acquireSize, CacheSize: cacheSize, Cycled: false, MaxValue: maxValue, Increment: increment, MinValue: minValue } } );
   commCreateIndex( dbcl, "id", { id: 1 }, { Unique: true }, true );

   //alter currentValue为minValue,会清空所有coord上的缓存
   dbcl.setAttributes( { AutoIncrement: { Field: "id", CurrentValue: minValue } } );

   //连接所有coord插入部分记录,coord缓存分别为[1,51],[56,106],[111,161]
   var expR = [];
   var cl = new Array();
   var coord = new Array();
   for( var k = 0; k < coordNum; k++ )
   {
      coord[k] = new Sdb( coordNodes[k] );
      cl[k] = coord[k].getCS( COMMCSNAME ).getCL( clName );
      //连接所有coord插入部分记录,coord缓存分别为[1,51],[56,106],[111,161]
      var doc = [];
      for( var i = 0; i < 3; i++ )
      {
         doc.push( { a: sortField } );
         sortField++;
      }
      cl[k].insert( doc );
   }
   //[{$numberLong:"-9223372036854775803"}, {$numberLong:"-9223372036854775753"}]
   expR.push( { a: 0, id: { $numberLong: "-9223372036854775803" } } );
   expR.push( { a: 1, id: { $numberLong: "-9223372036854775798" } } );
   expR.push( { a: 2, id: { $numberLong: "-9223372036854775793" } } );

   //[{$numberLong:"-9223372036854775748"}, {$numberLong:"-9223372036854775698"}]
   expR.push( { a: 3, id: { $numberLong: "-9223372036854775748" } } );
   expR.push( { a: 4, id: { $numberLong: "-9223372036854775743" } } );
   expR.push( { a: 5, id: { $numberLong: "-9223372036854775738" } } );

   //[{$numberLong:"-9223372036854775693"}, {$numberLong:"-9223372036854775643"}]
   expR.push( { a: 6, id: { $numberLong: "-9223372036854775693" } } );
   expR.push( { a: 7, id: { $numberLong: "-9223372036854775688" } } );
   expR.push( { a: 8, id: { $numberLong: "-9223372036854775683" } } );

   //coordB指定自增字段插入记录，指定自增字段值为最大值
   cl[1].insert( { a: sortField, id: { $numberLong: "9223372036854775807" } } );
   expR.push( { a: sortField, id: { $numberLong: "9223372036854775807" } } );
   sortField++;

   //coordA插入记录，消耗完本coord的缓存，[{$numberLong:"-9223372036854775803"}, {$numberLong:"-9223372036854775753"}]
   for( var i = 0; i < 8; i++ )
   {
      cl[0].insert( { a: sortField } );
      sortField++;
   }
   expR.push( { a: 10, id: { $numberLong: "-9223372036854775788" } } );
   expR.push( { a: 11, id: { $numberLong: "-9223372036854775783" } } );
   expR.push( { a: 12, id: { $numberLong: "-9223372036854775778" } } );
   expR.push( { a: 13, id: { $numberLong: "-9223372036854775773" } } );
   expR.push( { a: 14, id: { $numberLong: "-9223372036854775768" } } );
   expR.push( { a: 15, id: { $numberLong: "-9223372036854775763" } } );
   expR.push( { a: 16, id: { $numberLong: "-9223372036854775758" } } );
   expR.push( { a: 17, id: { $numberLong: "-9223372036854775753" } } );

   //coordA插入记录，插入失败，超出序列值返回
   assert.tryThrow( -325, function()
   {
      cl[0].insert( { a: sortField } );
   } );

   //coordB插入记录，插入失败，超出序列值范围
   assert.tryThrow( -325, function()
   {
      cl[1].insert( { a: sortField } );
   } );

   //coordC插入记录，消耗完本coord的缓存，[{$numberLong:"-9223372036854775693"}, {$numberLong:"-9223372036854775643"}]
   for( var i = 0; i < 8; i++ )
   {
      cl[2].insert( { a: sortField } );
      sortField++;
   }
   expR.push( { a: 18, id: { $numberLong: "-9223372036854775678" } } );
   expR.push( { a: 19, id: { $numberLong: "-9223372036854775673" } } );
   expR.push( { a: 20, id: { $numberLong: "-9223372036854775668" } } );
   expR.push( { a: 21, id: { $numberLong: "-9223372036854775663" } } );
   expR.push( { a: 22, id: { $numberLong: "-9223372036854775658" } } );
   expR.push( { a: 23, id: { $numberLong: "-9223372036854775653" } } );
   expR.push( { a: 24, id: { $numberLong: "-9223372036854775648" } } );
   expR.push( { a: 25, id: { $numberLong: "-9223372036854775643" } } );

   //coordC插入记录，插入失败，超出序列值范围
   assert.tryThrow( -325, function()
   {
      cl[2].insert( { a: sortField } );
   } );

   var actR = dbcl.find().sort( { a: 1 } );
   checkRec( actR, expR );

   commDropCL( db, COMMCSNAME, clName, true, true );
}
