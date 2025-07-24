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

   Source File Name = dbConfForWeb.h

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DBConfForWeb_H
#define DBConfForWeb_H

#include "core.hpp"
#include <string>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>

// xml source file
#define OPTXMLSRCFILE          "optlist.xml"
#define OPTOTHERINFOFORWEBFILE "optOtherInfoForWeb.xml"

// output file path
// #define DBCONFFORWEBPATH "../../doc/administration/database/topics/runtime_configuration"
#define DBCONFFORWEBPATH "../../doc/bak/runtime_configuration"
#define FILESUFFIX ".dita"

class OptEle
{
public:
    std::string longtag ;
    std::string shorttag ;
    std::string typeofwebtag ;
    std::string detailtag ;
    BOOLEAN hiddentag ;
    OptEle()
    {
       longtag = "--" ;
       shorttag = "-" ;
       hiddentag = FALSE ;
    }
} ;

class OptOtherInfoEle
{
public:
    std::string titletag ;
    std::string subtitletag ;
    // stentry tags
    std::string stentry_nametag ;
    std::string stentry_acronymtag ;
    std::string stentry_typetag ;
    std::string stentry_desttag ;
    // note tags
    std::string firsttag ;
    std::string secondtag ;
} ;

class OptGenForWeb
{
    const char *language ;
    std::vector<OptOtherInfoEle*> optOtherInfo ;
    std::vector<OptEle*> optlist ;
    void loadOtherInfoFromXML () ;
    void loadFromXML () ;
    std::string genOptions () ;
    void gendoc () ;

public:
    OptGenForWeb ( const char* lang ) ;
    ~OptGenForWeb () ;
    void run () ;
};

#endif
