/****************************************************
@description:	nested select, basic case
         testlink cases:   seqDB-7447
@input:        1 insert into records
               2 nested select
@output:    return success, and results correct.
@modify list:
      2015-5-13 ShanShan Hu added  2016-3-16 XiaoNi Huang modify
****************************************************/
csName = COMMCSNAME;
clName1 = CHANGEDPREFIX + "_product";
clName2 = CHANGEDPREFIX + "_name";

println( "------Begin to ready cl." );
try
{
  // db.execUpdate("create collection "+csName+"."+clName);
  commDropCL( db, csName, clName1, true, true, "drop cl in begin" );
  commDropCL( db, csName, clName2, true, true, "drop cl in begin" );
  var opt = { ReplSize: 0 };
  var cl_pro = commCreateCLByOption( db, csName, clName1, opt, true, false, "create cl in begin" );
  var cl_name = commCreateCLByOption( db, csName, clName2, opt, true, false, "create cl in begin" );
}
catch( e )
{
  println( "Failed to drop/create cl in the begin." );
  throw e;
}

println( "------Begin to insert into records." );
try
{
  cl_pro.insert( {
    "_id": {
      "$oid": "5267297d4e8d54b12c000003"
    },
    "other1": 1,
    "other2": "xxx",
    "other3": [
      1,
      2,
      3
    ],
    "productCategoryList": [
      {
        "cateID": 1,
        "cateNum": 1,
        "employType": "1",
        "categoryInfoList": [
          {
            "content": "cateID1_1",
            "categroyNum": 1
          },
          {
            "content": "cateID1_2",
            "categroyNum": 1
          }
        ]
      },
      {
        "cateID": 2,
        "cateNum": 1,
        "employType": "1",
        "categoryInfoList": [
          {
            "content": "cateID2_1",
            "categroyNum": 1
          },
          {
            "content": "cateID2_2",
            "categroyNum": 1
          }
        ]
      }
    ]
  } );
}
catch( e )
{
  throw e;
}

try
{
  cl_pro.insert(
    {
      "_id": {
        "$oid": "5267430f4e8d54b12c000004"
      },
      "other1": 2,
      "other2": "xxx",
      "other3": [
        1,
        2,
        3
      ],
      "productCategoryList": [
        {
          "cateID": 1,
          "cateNum": 1,
          "employType": "1",
          "categoryInfoList": [
            {
              "content": "cateID1_1",
              "categroyNum": 1
            },
            {
              "content": "cateID1_2",
              "categroyNum": 1
            }
          ]
        },
        {
          "cateID": 2,
          "cateNum": 1,
          "employType": "1",
          "categoryInfoList": [
            {
              "content": "cateID2_1",
              "categroyNum": 1
            },
            {
              "content": "cateID2_2",
              "categroyNum": 1
            }
          ]
        }
      ]
    } );
}
catch( e )
{
  throw e;
}

try
{
  cl_name.insert( {
    "_id": {
      "$oid": "526728334e8d54b12c000000"
    },
    "cateID": 1,
    "cateName": "name1"
  } );
}
catch( e )
{
  throw e;
}

try
{
  cl_name.insert( {
    "_id": {
      "$oid": "526728384e8d54b12c000001"
    },
    "cateID": 2,
    "cateName": "name2"
  } );
}
catch( e )
{
  throw e;
}

try
{
  cl_name.insert( {
    "_id": {
      "$oid": "5267283d4e8d54b12c000002"
    },
    "cateID": 3,
    "cateName": "name3"
  } );
}
catch( e )
{
  throw e;
}

println( "------Begin to exec select." );
try
{
  var sqlstr = "select T4.other1, T4.other2, T4.other3, push(T4.productCategoryList) as productCategoryList from (select T2.other1, T2.other2, T2.other3, buildobj(T2.cateNum, T2.employType, T2.categoryInfoList, T3.cateName) as productCategoryList from ( select T1.other1, T1.other2, T1.other3, T1.productCategoryList.cateID as cateID, T1.productCategoryList.cateNum as cateNum, T1.productCategoryList.employType as employType, T1.productCategoryList.categoryInfoList as categoryInfoList from ( select * from ";
  sqlstr += csName;
  sqlstr += ".";
  sqlstr += clName1;
  sqlstr += " split by productCategoryList ) as T1  ) as T2 inner join "
  sqlstr += csName;
  sqlstr += ".";
  sqlstr += clName2;
  sqlstr += " as T3 on T2.cateID = T3.cateID) as T4 group by T4.other1, T4.other2, T4.other3"

  var cur = db.exec( sqlstr );
}
catch( e )
{
  println( "Failed to exec select." );
  throw e;
}

println( "------Begin to check results." )
if( 2 != cur.size() )
{
  throw "Failed to check results.";
}

var num = 1;
while( cur.next() )
{
  if( num != cur.current().toObj()["other1"] )
    throw "Error.";
  num = num + 1;
}

println( "------Begin to drop cl in the end." );
try
{
  db.execUpdate( "drop collection " + csName + "." + clName1 );
  db.execUpdate( "drop collection " + csName + "." + clName2 );
}
catch( e )
{
  println( "Failed to drop cl in the end." );
  throw e;
}