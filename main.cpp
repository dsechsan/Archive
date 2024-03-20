//
//  main.cpp
//
//
//
//

#include <iostream>
#include <sstream>
#include <cstdlib>
#include <functional>
#include <string>
#include <map>
#include "Testing.hpp"
#include "Archive.hpp"

std::string getLocalFolder() {
  //return std::string("c:/xxx/yyy");
  return std::string("/Users/dsechs/Library/CloudStorage/OneDrive-UCSanDiego/Desktop/ECE 141A/PA4/tmp"); //SET PATH TO LOCAL ARCHIVE FOLDER
}

int main(int argc, const char * argv[]) {
    srand(time(NULL));
    if(argc>1) {
        std::string temp(argv[1]);
        std::stringstream theOutput;

        std::string theFolder(getLocalFolder());
        if(3==argc) theFolder=argv[2];
        ECE141::Testing theTester(theFolder);
                
        using TestCall = std::function<bool()>;
        static std::map<std::string, TestCall> theCalls {
          {"Compile", [&](){return true;}  },
          {"Create",  [&](){return theTester.doCreateTests(theOutput);}  },
          {"Open",    [&](){return theTester.doOpenTests(theOutput);}  },
          {"Add",     [&](){return theTester.doAddTests(theOutput);}  },
          {"Extract", [&](){return theTester.doExtractTests(theOutput);}  },
          {"Remove",  [&](){return theTester.doRemoveTests(theOutput);}  },
          {"List",    [&](){return theTester.doListTests(theOutput);}  },
          {"Dump",    [&](){return theTester.doDumpTests(theOutput);}  },
          {"Stress",  [&](){return theTester.doStressTests(theOutput);}  },
          {"Compress",  [&](){return theTester.doCompressTests(theOutput);}  },
          {"All",     [&](){return theTester.doAllTests(theOutput);}  },
        };
        
        std::string theCmd(argv[1]);
        if(theCalls.count(theCmd)) {
          bool theResult = theCalls[theCmd]();
          const char* theStatus[]={"FAIL","PASS"};
          std::cout << theCmd << " test " << theStatus[theResult] << "\n";
          std::cout << "------------------------------\n"
            << theOutput.str() << "\n";
        }
        else std::cout << "Unknown test\n";        
        
    }
    std::string createpath = getLocalFolder()+ "/testing";

//    ECE141::ArchiveStatus<std::shared_ptr<ECE141::Archive>> theArchive = ECE141::Archive::createArchive(createpath);
//    std::string theFilePath = getLocalFolder() + "/XlargeA.txt";
//
//    std::unique_ptr<ECE141::IDataProcessor> aProcessor = std::make_unique<ECE141::Compression>();
//    theArchive.getValue()->add(theFilePath,aProcessor.get());
//    theArchive.getValue()->debugDump(std::cout);
//    std::string newfilePath = getLocalFolder() + "/ext.txt";
//    theArchive.getValue()->extract("XlargeA.txt",newfilePath);

    return 0;
}
