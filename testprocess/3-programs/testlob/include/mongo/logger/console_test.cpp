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

   
*******************************************************************************/
/**
 * Unit tests of the unittest framework itself.
 */

#include "mongo/logger/console.h"
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include "mongo/unittest/unittest.h"

namespace {

    using std::cout;
    using std::endl;
    using std::ostream;
    using std::string;
    using std::stringstream;

    TEST(ConsoleTest, testUtf8) {
        // this constant should match ConsoleStreamBuffer::bufferSize in console.cpp
        const size_t bufferSize = 1024U;

        mongo::Console console;
        ostream& out = console.out();

        // example unicode code points are from:
        //     http://en.wikipedia.org/wiki/UTF-8#Examples
        struct {
            size_t length;
            const char * utf8CodePoint;
        } data[] = {
            { 2U, "\xc2\xa2" }, // U+0024
            { 3U, "\xe2\x82\xac" }, // U+20AC
            { 4U, "\xf0\xa4\xad\xa2" }, // U+24B62
            { 0, "unused" },
        };

        // generate strings with unicode point located near end of buffer
        // to see how console stream handles incomplete unicode multi-byte sequences

        // to see these multibtye code points in the Windows terminal, set the console font
        // to Lucida. Currently the 4-byte sequence used in the test data is still not being
        // displayed properly.

        for (int i = 0; data[i].length > 0; i++) {
            for (size_t prefixLength = 0; prefixLength <= data[i].length; prefixLength++) {
                stringstream descriptionStream;
                descriptionStream << "ConsoleTest::testUtf8 - checking handling of "
                                  << "multi-byte sequence. This line contains "
                                  << prefixLength << " of " << data[i].length << "-byte sequence "
                                  << "before the end of the console's internal "
                                  << bufferSize << "-byte buffer";
                string description = descriptionStream.str();
                size_t padLength = bufferSize - prefixLength;
                string padding(padLength - description.length(), '.');
                string line = description + padding + data[i].utf8CodePoint;
                out << line << endl;
            }
        }

        // check if out is std::cout
        out << "ConsoleTest::testUtf8 - Console::out() is using "
            << (out.rdbuf() == cout.rdbuf() ? "std::cout" : "custom output stream") << endl;
    }

}  // namespace
