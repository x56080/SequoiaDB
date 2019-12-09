/******************************************************************************
*@Description : test SdbQueryOption
*               TestLink :  seqDB-15735:ָ��sort��ѯ��¼
                            seqDB-15736:ָ��hint��ѯ��¼
                            seqDB-15737:ָ��skip��ѯ��¼
                            seqDB-15738:ָ��limit��ѯ��¼
                            seqDB-15739:ָ��remove��ѯ��¼
*@auhor       : CSQ 
******************************************************************************/

function main ()
{
   try
   {
      commDropCS( db, COMMCSNAME + "15735", true, "drop CS " + COMMCSNAME + "15735" );
   } catch( e ) { }
   var varCS = commCreateCS( db, COMMCSNAME + "15735", true, "create CS" );
   var varCL = varCS.createCL( COMMCLNAME + "15735" );
   insertRecord( varCL );
   testsort15735( varCL );
   testhint15736( varCL );
   testskip15737( varCL );
   testlimit15738( varCL );
   testremove15739( varCL );

   try
   {
      commDropCS( db, COMMCSNAME + "15735", true, "drop CS " + COMMCSNAME + "15735" );
   } catch( e ) { }
}

function testsort15735 ( varCL )
{
   //ָ������
   var cur = varCL.find( new SdbQueryOption().sort( { typeint: 1 } ) );
   var expFindResult1 = [
      {
         _id: 1,
         typeint: 1,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 1.23 * 1,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 1,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 1 },
         typearr: ["abc" + 1],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-1] }],
         typeobj2: { "boj1": { "obj2": "value2" + 1 } },
         typearr3: ["name", [1]]
      },
      {
         _id: 2,
         typeint: 2,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 2.46,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 2,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 2 },
         typearr: ["abc" + 2],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-2] }],
         typeobj2: { "boj1": { "obj2": "value2" + 2 } },
         typearr3: ["name", [2]]
      },
      {
         _id: 3,
         typeint: 3,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 3.69,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 3,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 3 },
         typearr: ["abc" + 3],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-3] }],
         typeobj2: { "boj1": { "obj2": "value2" + 3 } },
         typearr3: ["name", [3]]
      },
      {
         _id: 4,
         typeint: 4,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 4.92,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 4,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 4 },
         typearr: ["abc" + 4],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-4] }],
         typeobj2: { "boj1": { "obj2": "value2" + 4 } },
         typearr3: ["name", [4]]
      },
      {
         _id: 5,
         typeint: 5,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 6.15,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 5,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 5 },
         typearr: ["abc" + 5],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-5] }],
         typeobj2: { "boj1": { "obj2": "value2" + 5 } },
         typearr3: ["name", [5]]
      }
   ];
   checkRec( cur, expFindResult1 );

   //����
   var cur = varCL.find( new SdbQueryOption().sort( { typeint: -1 } ) );
   var expFindResult2 = [
      {
         _id: 5,
         typeint: 5,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 6.15,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 5,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 5 },
         typearr: ["abc" + 5],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-5] }],
         typeobj2: { "boj1": { "obj2": "value2" + 5 } },
         typearr3: ["name", [5]]
      },
      {
         _id: 4,
         typeint: 4,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 4.92,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 4,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 4 },
         typearr: ["abc" + 4],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-4] }],
         typeobj2: { "boj1": { "obj2": "value2" + 4 } },
         typearr3: ["name", [4]]
      },
      {
         _id: 3,
         typeint: 3,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 3.69,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 3,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 3 },
         typearr: ["abc" + 3],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-3] }],
         typeobj2: { "boj1": { "obj2": "value2" + 3 } },
         typearr3: ["name", [3]]
      },
      {
         _id: 2,
         typeint: 2,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 2.46,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 2,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 2 },
         typearr: ["abc" + 2],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-2] }],
         typeobj2: { "boj1": { "obj2": "value2" + 2 } },
         typearr3: ["name", [2]]
      },
      {
         _id: 1,
         typeint: 1,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 1.23 * 1,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 1,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 1 },
         typearr: ["abc" + 1],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-1] }],
         typeobj2: { "boj1": { "obj2": "value2" + 1 } },
         typearr3: ["name", [1]]
      }
   ];
   checkRec( cur, expFindResult2 );

   var cur = varCL.find( new SdbQueryOption().sort( { "typeobj.subobj": -1, "typeobj2.boj1": 1, "typearr3": -1, "typefloat": 1 } ) );
   checkRec( cur, expFindResult2 );
}

function testhint15736 ( varCL )
{
   varCL.createIndex( "typefloatIndex", { typefloat: 1 }, false );
   var cur = varCL.find( new SdbQueryOption().hint( { "": "typefloatIndex" } ).sort( { typeint: 1 } ) );
   var expFindResult1 = [
      {
         _id: 1,
         typeint: 1,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 1.23 * 1,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 1,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 1 },
         typearr: ["abc" + 1],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-1] }],
         typeobj2: { "boj1": { "obj2": "value2" + 1 } },
         typearr3: ["name", [1]]
      },
      {
         _id: 2,
         typeint: 2,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 2.46,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 2,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 2 },
         typearr: ["abc" + 2],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-2] }],
         typeobj2: { "boj1": { "obj2": "value2" + 2 } },
         typearr3: ["name", [2]]
      },
      {
         _id: 3,
         typeint: 3,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 3.69,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 3,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 3 },
         typearr: ["abc" + 3],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-3] }],
         typeobj2: { "boj1": { "obj2": "value2" + 3 } },
         typearr3: ["name", [3]]
      },
      {
         _id: 4,
         typeint: 4,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 4.92,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 4,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 4 },
         typearr: ["abc" + 4],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-4] }],
         typeobj2: { "boj1": { "obj2": "value2" + 4 } },
         typearr3: ["name", [4]]
      },
      {
         _id: 5,
         typeint: 5,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 6.15,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 5,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 5 },
         typearr: ["abc" + 5],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-5] }],
         typeobj2: { "boj1": { "obj2": "value2" + 5 } },
         typearr3: ["name", [5]]
      }
   ];
   checkRec( cur, expFindResult1 );

}

function testskip15737 ( varCL )
{
   var cur = varCL.find( new SdbQueryOption().sort( { typeint: 1 } ).skip( 0 ) );
   var expFindResult1 = [
      {
         _id: 1,
         typeint: 1,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 1.23 * 1,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 1,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 1 },
         typearr: ["abc" + 1],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-1] }],
         typeobj2: { "boj1": { "obj2": "value2" + 1 } },
         typearr3: ["name", [1]]
      },
      {
         _id: 2,
         typeint: 2,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 2.46,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 2,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 2 },
         typearr: ["abc" + 2],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-2] }],
         typeobj2: { "boj1": { "obj2": "value2" + 2 } },
         typearr3: ["name", [2]]
      },
      {
         _id: 3,
         typeint: 3,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 3.69,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 3,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 3 },
         typearr: ["abc" + 3],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-3] }],
         typeobj2: { "boj1": { "obj2": "value2" + 3 } },
         typearr3: ["name", [3]]
      },
      {
         _id: 4,
         typeint: 4,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 4.92,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 4,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 4 },
         typearr: ["abc" + 4],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-4] }],
         typeobj2: { "boj1": { "obj2": "value2" + 4 } },
         typearr3: ["name", [4]]
      },
      {
         _id: 5,
         typeint: 5,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 6.15,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 5,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 5 },
         typearr: ["abc" + 5],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-5] }],
         typeobj2: { "boj1": { "obj2": "value2" + 5 } },
         typearr3: ["name", [5]]
      }
   ];
   checkRec( cur, expFindResult1 );

   var cur = varCL.find( new SdbQueryOption().sort( { typeint: 1 } ).skip( 2 ) );
   var expFindResult1 = [
      {
         _id: 3,
         typeint: 3,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 3.69,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 3,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 3 },
         typearr: ["abc" + 3],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-3] }],
         typeobj2: { "boj1": { "obj2": "value2" + 3 } },
         typearr3: ["name", [3]]
      },
      {
         _id: 4,
         typeint: 4,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 4.92,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 4,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 4 },
         typearr: ["abc" + 4],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-4] }],
         typeobj2: { "boj1": { "obj2": "value2" + 4 } },
         typearr3: ["name", [4]]
      },
      {
         _id: 5,
         typeint: 5,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 6.15,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 5,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 5 },
         typearr: ["abc" + 5],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-5] }],
         typeobj2: { "boj1": { "obj2": "value2" + 5 } },
         typearr3: ["name", [5]]
      }
   ];
   checkRec( cur, expFindResult1 );

   var cur = varCL.find( new SdbQueryOption().sort( { typeint: 1 } ).skip( 10 ) );
   var size = 0;
   while( cur.next() )
   {
      var ret = cur.current();
      size++;
   }
   if( size != 0 )
   {
      throw buildException( "compare testskip15737 skip(10) fail", "", "compare", 0, size );
   }
}

function testlimit15738 ( varCL )
{
   var cur = varCL.find( new SdbQueryOption().sort( { typeint: 1 } ).limit( 1 ) );
   var expFindResult1 = [
      {
         _id: 1,
         typeint: 1,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 1.23 * 1,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 1,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 1 },
         typearr: ["abc" + 1],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-1] }],
         typeobj2: { "boj1": { "obj2": "value2" + 1 } },
         typearr3: ["name", [1]]
      }
   ];
   checkRec( cur, expFindResult1 );

   var cur = varCL.find( new SdbQueryOption().sort( { typeint: 1 } ).limit( 2 ) );
   var expFindResult1 = [
      {
         _id: 1,
         typeint: 1,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 1.23 * 1,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 1,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 1 },
         typearr: ["abc" + 1],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-1] }],
         typeobj2: { "boj1": { "obj2": "value2" + 1 } },
         typearr3: ["name", [1]]
      },
      {
         _id: 2,
         typeint: 2,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 2.46,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 2,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 2 },
         typearr: ["abc" + 2],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-2] }],
         typeobj2: { "boj1": { "obj2": "value2" + 2 } },
         typearr3: ["name", [2]]
      }
   ];
   checkRec( cur, expFindResult1 );

   var cur = varCL.find( new SdbQueryOption().sort( { typeint: 1 } ).limit( 10 ) );
   var expFindResult1 = [
      {
         _id: 1,
         typeint: 1,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 1.23 * 1,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 1,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 1 },
         typearr: ["abc" + 1],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-1] }],
         typeobj2: { "boj1": { "obj2": "value2" + 1 } },
         typearr3: ["name", [1]]
      },
      {
         _id: 2,
         typeint: 2,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 2.46,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 2,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 2 },
         typearr: ["abc" + 2],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-2] }],
         typeobj2: { "boj1": { "obj2": "value2" + 2 } },
         typearr3: ["name", [2]]
      },
      {
         _id: 3,
         typeint: 3,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 3.69,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 3,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 3 },
         typearr: ["abc" + 3],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-3] }],
         typeobj2: { "boj1": { "obj2": "value2" + 3 } },
         typearr3: ["name", [3]]
      },
      {
         _id: 4,
         typeint: 4,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 4.92,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 4,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 4 },
         typearr: ["abc" + 4],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-4] }],
         typeobj2: { "boj1": { "obj2": "value2" + 4 } },
         typearr3: ["name", [4]]
      },
      {
         _id: 5,
         typeint: 5,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 6.15,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + 5,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + 5 },
         typearr: ["abc" + 5],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [-5] }],
         typeobj2: { "boj1": { "obj2": "value2" + 5 } },
         typearr3: ["name", [5]]
      }
   ];
   checkRec( cur, expFindResult1 );
}

function testremove15739 ( varCL )
{
   var rc = varCL.find( new SdbQueryOption().cond( { "typeint": { "$gt": 0 } } ).remove() ).toArray();
   var act = varCL.find().count();
   if( act != 0 )
   {
      throw buildException( "compare testremove15739 fail", "", "compare", 0, act );
   }
}

function insertRecord ( varCL )
{
   for( var i = 1; i <= 5; i++ )
   {
      varCL.insert( {
         _id: i,
         typeint: i,
         typelong: { "$numberLong": "9223372036854775807" },
         typefloat: 1.23 * i,
         typedecimal: { $decimal: "123.456" },
         typestr: "value" + i,
         typeoid: { "$oid": "123abcd00ef12358902300ef" },
         typebool: true,
         typedate: { "$date": "2012-01-01" },
         typetimestamp: { "$timestamp": "2012-01-01-13.14.26.124233" },
         typebinary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
         typeregex: { "$regex": "^csq", "$options": "i" },
         typeobj: { "subobj": "value" + i },
         typearr: ["abc" + i],
         typenull: null,
         typeminkey: { "$minKey": 1 },
         typemaxkey: { "$maxKey": 1 },
         typearr2: [1, { "arr": [i * -1] }],
         typeobj2: { "boj1": { "obj2": "value2" + i } },
         typearr3: ["name", [i]]
      } );
   }
}


main( db );