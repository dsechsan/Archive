//
// Created by Sandeep Chintada on 3/7/24.
//

#ifndef ECE141_ARCHIVE_CHUNKMANAGER_H
#define ECE141_ARCHIVE_CHUNKMANAGER_H

#include <cstdio>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <memory>
#include <optional>
#include <stdexcept>
#include <map>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <functional>
#include <cstring>

const int kChunkSize = 1024;
#pragma pack(push, 1)
struct Header{
    bool occupied;
    size_t ChunkNum;
    size_t partNum;
    size_t nextIdx;
    size_t fileSize;
    size_t dataSize;
    std::string fileName;
    std::string timeInserted;
};
#pragma pack(pop)

const int kAvailableSize = kChunkSize -sizeof(Header);
struct Chunk{
    Header header;
    char data[kAvailableSize] = {};
    Chunk(): header{false,0,0,0,0,0, "null", "null"} {};
};

using ChunkCallback = std::function<bool(Chunk &,size_t)>;
class ChunkManager{
public:
    ChunkManager()= default;
    explicit ChunkManager(const std::string &FileName): archiveFileName(FileName),inputFileSize(0), inputFile("null"){
        archiveFileStream.open(archiveFileName, std::ios::binary | std ::ios::out | std::ios::in);
        archiveFileStream.seekp(0, std::ios::end);
//        archiveFileStream << "This is a test\n";
        if(!archiveFileStream.is_open()){
            std::cerr << "Unable to open the archive file\n";
        }
    }

    ~ChunkManager(){
        if(archiveFileStream.is_open()) archiveFileStream.close();
    }

    void addChunks(size_t numChunksToAdd){
        for(int i = 0; i<numChunksToAdd; i++){
            Chunk theChunk;
//            std::memset(&theChunk,0,sizeof(theChunk));
            archiveFileStream.seekp(0,std::ios::end);
            archiveFileStream.write(reinterpret_cast<char*>(&theChunk),sizeof(theChunk));
            numberOfChunks++;
            if(archiveFileStream.fail()){
                std::cerr << "failed to add chunks to the archive file\n";
            }
        }
    }

    size_t getArchiveChunkCount(){
        if(!archiveFileStream.is_open()) std::cerr << "unable to open archive stream to read\n";
        archiveFileStream.seekg(0,std::ios::end);
        numberOfChunks = archiveFileStream.tellg() / kChunkSize;
        return numberOfChunks;
    }

    [[nodiscard]] size_t getInputFileSize() const{
        std::ifstream theInputStream(inputFile, std::ios::binary | std::ios::in);
        if (!theInputStream.is_open()) std::cerr << "unable to open file\n";
        theInputStream.seekg(0, std::ios::end);
        inputFileSize = theInputStream.tellg();
        theInputStream.seekg(0,std::ios::beg);

        return inputFileSize;
    }

    [[nodiscard]] size_t getInputChunkCount() const{
        auto theSize = getInputFileSize();
        return (theSize/kChunkSize) + (theSize % kChunkSize? 1: 0);
    }

    bool getChunk(Chunk &theChunk , size_t aPos){
        if(aPos < numberOfChunks) {
            size_t theChunkPos = kChunkSize*aPos;
            archiveFileStream.seekg(static_cast<int>(theChunkPos),std::ios::beg);
            archiveFileStream.read(reinterpret_cast<char *>(&theChunk), sizeof(theChunk));
            return true;
        }else{
            return false;
        }
    }

    std::vector<int> getFreeChunks(){
        std::vector<int> theList;
        each([&](Chunk& theChunk, size_t aPos){
            if (!theChunk.header.occupied) {
                theList.push_back(static_cast<int>(aPos));
                return true;
            }
            return true;
        });
        return theList;
    }

    static std::string getTimeInserted(){
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm* local_time = std::localtime(&now_c);

        char buffer[20]; // Adjust the size as needed
        std::strftime(buffer, sizeof(buffer), "%d-%m-%Y %H:%M:%S", local_time);
        buffer[sizeof(buffer) - 1] = '\0';
        return std::string(buffer);
//        std::stringstream ss ;
//        ss << std::put_time(local_time, "%d-%m-%Y  %H:%M:%S");
//        return ss.str();
    }

    bool each(ChunkCallback aCallback){
        size_t thePos{0};
        bool theResult{true};
        Chunk theChunk;
//        std::memset(&theChunk,0,sizeof(theChunk));
        while(theResult){
            theResult = getChunk(theChunk,thePos);
            if(theResult){
                theResult = aCallback(theChunk,thePos++);
            }
        }
        return true;
    }

//    size_t getNumberOfChunks(){
//        if(!theChunks.empty()) return theChunks.size();
//        else return 0;
//    }

    bool find(const std::string &aName){
        bool found = false;
        if (numberOfChunks) {
            each([&](Chunk& theChunk, size_t aPos) -> bool {
                if (theChunk.header.fileName == aName && theChunk.header.partNum == 1) {
                    found = true;
                    return false;
                }
                return true;
            });
            return found;
        } else {
            return false;
        }
    };

    std::map<size_t, size_t> getChunksOfAFile(const std::string &aName){
        std::map<size_t,size_t> theFileMap;

        each([&](Chunk& theChunk, size_t aPos){
            if(theChunk.header.fileName == aName){
                theFileMap[theChunk.header.partNum] = aPos;
            }
            return true;
        });
        return  theFileMap;
    };

    // Chunkify a normal file
    void writeChunksToArchive(const std::string& aFileName){
        numberOfChunks = getArchiveChunkCount();

        size_t theFileSize = getInputFileSize();
        size_t theReqdNumOfChunks = getInputChunkCount();
        size_t theStreamPos{0};
        size_t theDataSize{0};

        auto theFreeIdx = getFreeChunks();

        if(theFreeIdx.size() < theReqdNumOfChunks){
            addChunks(theReqdNumOfChunks - theFreeIdx.size());
        }

        theFreeIdx = getFreeChunks();

//        Chunk theChunk;
        std::ifstream theInputStream(inputFile,std::ios::binary | std::ios::in);
        for(auto i = 0;i< theReqdNumOfChunks; i++) {
            archiveFileStream.seekp(kChunkSize*theFreeIdx[i],std::ios::beg);

            Chunk theChunk;
//            std::memset(&theChunk,0,sizeof(theChunk));
            theDataSize = (theStreamPos + kChunkSize - sizeof(theChunk.header) > theFileSize) ? theFileSize - theStreamPos : kChunkSize - sizeof(theChunk.header);
            theInputStream.seekg(static_cast<int>(theStreamPos), std::ios::beg);
            theInputStream.read(theChunk.data, static_cast<long>(theDataSize));
            theStreamPos += theDataSize;

            theChunk.header.timeInserted = getTimeInserted();
            theChunk.header.fileName = aFileName;
            theChunk.header.occupied = true;
            theChunk.header.partNum = i + 1;
            theChunk.header.ChunkNum = theFreeIdx[i];
            theChunk.header.dataSize = theDataSize;
            theChunk.header.fileSize = theFileSize;
            if (i != theReqdNumOfChunks - 1) {
                theChunk.header.nextIdx = theFreeIdx[i + 1];
            }
            else theChunk.header.nextIdx = 0;

            archiveFileStream.write(reinterpret_cast<const char*>(&theChunk),sizeof(theChunk));

        }
    }

    bool retrieve(const std::string &aName, const std::string &aFullPath){
        auto theFileMap = getChunksOfAFile(aName);
        if(theFileMap.empty()) return false;

        std::ofstream theOutputFileStream(aFullPath, std::ios::binary | std::ios::out);
        for (const auto& pair : theFileMap){
//            std::memset(&theChunk,0,sizeof(theChunk));
            Chunk theChunk;

            archiveFileStream.seekg(static_cast<int>((pair.second)*kChunkSize), std::ios::beg);
            archiveFileStream.read(reinterpret_cast<char*>(&theChunk.header),sizeof(Header));

            archiveFileStream.seekg(static_cast<int>((pair.second)*kChunkSize + sizeof(Header)), std::ios::beg);
            archiveFileStream.read(reinterpret_cast<char*>(&theChunk.data),kAvailableSize);
            theOutputFileStream.write(theChunk.data,static_cast<long>(theChunk.header.dataSize));
            if(theOutputFileStream.fail()){
                std::cerr << "failed to write the file data\n";
                return false;
            }
        }
        return true;
    };

    bool deleteChunksOfFile(const std::string &aName){
        auto theFileMap = getChunksOfAFile(aName);
        if(theFileMap.empty()) return false;

        for(const auto& pair : theFileMap){
            archiveFileStream.seekp(static_cast<int>((pair.second)*kChunkSize),std::ios::beg);
            Chunk theEmptyChunk;
            std::memset(&theEmptyChunk,0,sizeof(theEmptyChunk));
            archiveFileStream.write(reinterpret_cast<const char*>(&theEmptyChunk),sizeof(theEmptyChunk));
            emptyChunkIdx.push_back(pair.second);
            if(archiveFileStream.fail()){
                archiveFileStream.clear();
                return false;
            }
        }
        return true;
    }

    void Compact(){

        std::string tempfile = "/Users/dsechs/Library/CloudStorage/OneDrive-UCSanDiego/Desktop/ECE 141A/PA4/tmp/temp.arc";
       std::fstream theTempFileStream(tempfile, std::ios::binary | std::ios::out | std::ios::in | std::ios::trunc);
       if(!theTempFileStream.is_open()) std::cerr << "Failed to open temporary file\n";

       int theNewIdx = 0;
       std::map<size_t, int> theIndexMap;
       each([&](Chunk &theChunk, size_t aPos){
           if(theChunk.header.occupied) {
               theIndexMap[theChunk.header.ChunkNum] = theNewIdx;
               theChunk.header.ChunkNum = theNewIdx;
               theTempFileStream.seekp(0,std::ios::end);
               theTempFileStream.write(reinterpret_cast<const char*>(&theChunk),kChunkSize);
               theNewIdx++;
           }
           return true;
       });

       each([&](Chunk &theChunk, size_t aPos){
           if(theChunk.header.nextIdx){
               theChunk.header.nextIdx = theIndexMap[theChunk.header.nextIdx];
           }
           return true;
       });

       theTempFileStream.flush();
       theTempFileStream.seekg(0,std::ios::beg);

       archiveFileStream.close();
       archiveFileStream.open(archiveFileName, std::ios::binary | std ::ios::out | std::ios::in | std::ios::trunc);

       if(theTempFileStream && archiveFileStream) archiveFileStream << theTempFileStream.rdbuf();

        if (std::remove(tempfile.c_str()) != 0) {
            std::perror("Error deleting temporary file");
        } else {
            std::cout << "Temporary file deleted successfully." << std::endl;
        }

    }

    void listFiles(std::ostream& anOutput, size_t* aNumFiles = nullptr){
        anOutput << std::left << std::setw(2) << "###" << " ";
        anOutput << std::setw(15) << "name" << " ";
        anOutput << std::setw(10) <<  "size" << " ";
        anOutput << std::setw(20) << "date added" << "\n";
        anOutput << "-----------------------------------------------------\n";

        each([&](Chunk& aChunk, size_t aPos) {
            if (aChunk.header.occupied && aChunk.header.partNum == 1) {
                anOutput << std::right << std::setw(2) << ++(*aNumFiles) << ". ";
                anOutput << std::left << std::setw(15) << aChunk.header.fileName << " ";
                anOutput << std::setw(10) << aChunk.header.fileSize << " ";
                // Assuming `timeInserted` is a string that fits in 20 characters
                anOutput << std::right << std::setw(20) << aChunk.header.timeInserted << "\n";
            }
            return true;
        });
    }

    void dump(std::ostream& anOutput){
        anOutput << "###    status       name\n";
        anOutput << "------------------------\n";
        each([&](Chunk& theChunk, size_t aPos){
            std::string status = theChunk.header.occupied ? "used": "empty";
            std::string name = theChunk.header.occupied ? theChunk.header.fileName : "";
            anOutput << (aPos + 1) << ".   " << status << "     " << name << "\n";
            return true;
        });
    }

    void setInputFileName(const std::string& anInputFile) {
        inputFile = anInputFile;
    }

private:
    std::vector<Chunk> theChunks;
    size_t numberOfChunks = 0;
    std::string archiveFileName;
    std::fstream archiveFileStream;
    std::string inputFile; // for the add operation
    mutable size_t inputFileSize;
    std::vector<size_t> emptyChunkIdx;
};

#endif //ECE141_ARCHIVE_CHUNKMANAGER_H
