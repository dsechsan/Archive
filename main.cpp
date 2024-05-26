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

//template<typename T>
//bool isPointer(T* value){
//    if(*value) return true;
//    else return false;
//}

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

    int p1{100};
    int *p2 = new int{200};
    int **p3 = &p2;
    **p3 += 100;

    std::cout << p1 << *p2 << **p3 << "\n";

//    std::cout << isPointer(&p1) << std::endl;

    return 0;
}

/*Write a C++ class named FileHandler that demonstrates the RAII idiom for managing a stream in 20 lines of code or less. Requirements:
The constructor of FileHandler should accept a string representing the file path and open the file for writing.
Use std::ofstream to manage the file stream. Implement a member function named writeToFile that accepts a string and writes it
to the stream. Ensure that the file is properly closed when an instance of FileHandler is destroyed by implementing a suitable destructor.
 */

//class FileHandler{
//    FileHandler(const std::string& aFilename){
//        fileStream.open(aFilename,std::ios::binary | std::ios::out);
//    };
//    ~FileHandler(){
//        if(fileStream.is_open()) fileStream.close();
//    };
//    void writeToFile(std::string &aString){
//        fileStream.seekp(0,std::ios::end);
//        fileStream.write(aString.c_str(),std::strlen(aString.c_str()));
//    };
//    std::ofstream fileStream;
//};


