/******************************************************************************
@Description : 1. Test update data . The data have complex structure.
@Modify list :
               2014-6-26  xiaojun Hu  Changed
******************************************************************************/
var csName = COMMCSNAME;
var clName = COMMCLNAME;
try
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop cl in the beginning" );
}
catch( e )
{
   println( "unexpected err happened when clear cs:" + e );
   throw e;
}

try
{
   var cs = commCreateCS( db, csName, true, "create CS in the beginning" );
   var cl = cs.createCL( clName, { ReplSize: 0, Compressed: true } );
}
catch( e )
{
   println( "failed to create table ,e=" + e );
   throw e;
}

for( var i = 1; i <= 100; i++ )
{
   try
   {
      cl.insert( {
         id: "00" + i, province: "", city: [{
            id: "00" + i + "01",
            cityname: "", areaname: [{ id: "00" + i + "0101", name: "" },
            { id: "00" + i + "0102", name: "" }]
         }, {
            id: "00" + i + "02",
            cityname: "", areaname: [{ id: "00" + i + "0201", name: "" },
            { id: "00" + i + "0202", name: "" }]
         }]
      } );
   }
   catch( e )
   {
      println( "failed to insert records ,e=" + e );
      throw e;
   }
}

// inspect the number of data
try
{
   var i = 0;
   do
   {
      var count = cl.count();
      if( 100 == count )
         break;
      ++i;
   } while( i < 20 );
   if( count != 100 )
   {
      println( "Error number of insert record, count = " + count +
         " is not equal 100" );
      throw "ErrNumInsertRecord";
   }
}
catch( e )
{
   throw e;
}


println( "Insert data succeed" );

for( var i = 1; i <= 100; i++ )
{
   try
   {
      cl.update( { $set: { province: "广东省_" + i } }, { id: "00" + i } );
      cl.update( { $set: { "city.$1.cityname": "广州市_" + i + "01" } }, { "city.$1.id": "00" + i + "01" } );
      cl.update( { $set: { "city.$1.areaname.$2.name": "番禺区_" + i + "0101" } }, { "city.$1.areaname.$2.id": "00" + i + "0101" } );
      cl.update( { $set: { "city.$1.areaname.$2.name": "黄浦区" + i + "0102" } }, { "city.$1.areaname.$2.id": "00" + i + "0102" } );
      cl.update( { $set: { "city.$1.cityname": "深圳市_" + i + "02" } }, { "city.$1.id": "00" + i + "02" } );
      cl.update( { $set: { "city.$1.areaname.$2.name": "宝山区_" + i + "0201" } }, { "city.$1.areaname.$2.id": "00" + i + "0201" } );
      cl.update( { $set: { "city.$1.areaname.$2.name": "萝岗区_" + i + "0202" } }, { "city.$1.areaname.$2.id": "00" + i + "0202" } );
   }
   catch( e )
   {
      println( "failed to update the first record, e=" + e );
      throw e;
   }
}

println( "Update data succeed" );

for( i = 1; i <= 100; i++ )
{
   try
   {
      var rc = cl.find( {
         $and: [{
            "city.$1.cityname": "广州市_" + i +
               "01", "city.$1.areaname.$1.name": "番禺区_" + i +
                  "0101", "city.$1.areaname.$2.name": "黄浦区" + i + "0102"
         },
         {
            "city.$2.cityname": "深圳市_" + i + "02",
            "city.$2.areaname.$1.name": "宝山区_" + i + "0201",
            "city.$2.areaname.$2.name": "萝岗区_" + i + "0202"
         }]
      } );
      if( rc.count() != 1 )
      {
         println( "the " + i + " record not find,rc.count=" + rc.count() );
         throw "ErrQueryNumRecord";
      }
   }
   catch( e )
   {
      println( "fail to find the " + i + " records,e=" + e );
      throw e;
   }
}

println( "Find data succeed" );

try
{
   commDropCL( db, csName, clName, true, true,
      "drop colleciton in the end" );
   println( "Success Over" );
}
catch( e )
{
   println( "failed to drop CS" );
   throw e;
}
