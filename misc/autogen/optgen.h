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

   Source File Name = optgen.h

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OptGen_H
#define OptGen_H

#include "core.hpp"
#include <string>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>

// xml source file
#define OPTXMLSRC "optlist.xml"
// output file path
#define HEADERNAME "pmdOptions"
#define HEADERPATHCPP "../../SequoiaDB/engine/include/"HEADERNAME".hpp"
#define HEADERPATHC "../../SequoiaDB/engine/include/"HEADERNAME".h"
#define COORDSAMPLEPATH "../../conf/samples/sdb.conf.coord"
#define CATALOGSAMPLEPATH "../../conf/samples/sdb.conf.catalog"
#define DATASAMPLEPATH "../../conf/samples/sdb.conf.data"
#define STANDALONESAMPLEPATH "../../conf/samples/sdb.conf.standalone"

// XML element

#define OPTLISTTAG "optlist"
#define OPTTAG "opt"
#define NAMETAG "name"
#define LONGTAG "long"
#define SHORTTAG "short"
#define DESCRIPTIONTAG "description"
#define DEFAULTTAG "default"
#define CATALOGTAG "catalogdft"
#define COORDTAG "coorddft"
#define DATATAG "datadft"
#define STANDTAG "standdft"
#define TYPETAG "type"
#define HIDDENTAG "hidden"

#define NONETYPE "none"

class OptElement
{
public :
    std::string nametag ;
    std::string longtag ;
    std::string desctag ;
    std::string defttag ;
    std::string catatag ;
    std::string cordtag ;
    std::string datatag ;
	std::string standtag;
    std::string typetag ;
    BOOLEAN hiddentag ;
    CHAR shorttag ;
    OptElement ()
    {
       hiddentag = FALSE ;
       shorttag = 0 ;
    }
} ;
class OptGen
{
    const char* language;
    std::vector<OptElement*> optlist;
    void loadFromXML ();
    void genHeaderC () ;
    void genHeaderCpp () ;
    void genCatalogSample () ;
    void genCoordSample () ;
    void genDataSample () ;
	  void getStandAloneSample() ;
public:
    OptGen (const char* lang);
    void genConfPair ( std::ofstream &fout, std::string key, std::string value1,
                       std::string value2, std::string desc ) ;
    void genSampleHeader ( std::ofstream &fout ) ;
    ~OptGen () ;
    void run ();
};
#endif
