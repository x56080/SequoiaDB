/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = rcgen.h

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RCGen_H
#define RCGen_H

#include <string>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>

using namespace std;

// xml source file
#define RCXMLSRC "rclist.xml"
// output file path
#define CPPPATH "../../SequoiaDB/engine/oss/ossErr.cpp"
#define CPATH "../../SequoiaDB/engine/include/ossErr.h"
#define CSPATH "../../driver/C#.Net/Driver/exception/Errors.cs"
#define JAVAPATH "../../driver/java/src/main/java/com/sequoiadb/exception/SDBError.java"
#define JSPATH "../../SequoiaDB/engine/spt/error.js"
#define WEBPATH "../../client/admin/admintpl/error_"
#define PYTHONPATH "../../driver/python/pysequoiadb/errcode.py"
#define WEBPATHSUFFIX ".php"
#define RC_MDPATH "../../doc/src/document/reference/Sequoiadb_error_code.md"
// XML element
#define CONSLIST "rclist.conslist"
#define CODELIST "rclist.codelist"
#define NAME "name"
#define VALUE "value"
#define DESCRIPTION "description"
#define RESERVED_ERROR "SDB_ERROR_RESERVED"

struct ErrorCode
{
   string name ;
   string desc_en ;
   string desc_cn ;
   int    value ;
   bool   reserved ;

   ErrorCode():
    value(0), reserved(false)
   {
   }

   string getDesc(string lang) const
   {
      if ("cn" == lang)
      {
          return desc_cn ;
      }
      else if ("en" == lang)
      {
          return desc_en ;
      }
      else
      {
          return "invalid lang" ;
      }
   }
} ;

class RCGen
{
private:
    const char* language;
    std::vector<std::pair<std::string, int> > conslist;
    std::vector<ErrorCode> errcodes;
    int maxErrorNameWidth;
    void loadFromXML ();
    void genC ();
    void genCPP ();
    void genCS ();
    void genJava ();
    void genJS () ;
    void genPython() ;
public:
    RCGen (const char* lang);
    void run ();
    void genDoc () ;
    void genWeb () ;
};
#endif
