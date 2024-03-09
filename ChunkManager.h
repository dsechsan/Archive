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

const int kChunkSize = 1024;
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
    explicit ChunkManager(std::istream& anInput) : inputStream(&anInput), outputStream(nullptr){};
    explicit ChunkManager(std::ostream& anOutput) : inputStream(nullptr), outputStream(&anOutput){};
    ~ChunkManager() = default;

    void addChunks(size_t numberOfChunks){
        for(int i = 0; i<numberOfChunks; i++){
            Chunk theChunk;
            theChunks.push_back(theChunk);
        }
    }

    [[nodiscard]] size_t getFileSize() const{
        if(inputStream) {
            inputStream->seekg(0, std::ios::end);
            return inputStream->tellg();
        }
        return 0;
    }

    [[nodiscard]] size_t getInputChunkCount() const{
        auto theSize = getFileSize();
        return (theSize/kChunkSize) + (theSize % kChunkSize? 1: 0);
    }

    bool getChunk(Chunk &theChunk , size_t aPos){
        if(aPos < theChunks.size()) {
            theChunk = theChunks[aPos];
            return true;
        }else{
            return false;
        }
    }

    std::vector<int> getFreeChunks(){
        std::vector<int> theList;
        each([&](Chunk& theChunk, size_t aPos){
            if (!theChunk.header.occupied) {
                theList.push_back(aPos);
                return true;
            }
            return true;
        });
        return theList;
    }

    std::string getTimeInserted(){
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm* local_time = std::localtime(&now_c);
        std::stringstream ss ;
        ss << std::put_time(local_time, "%d-%m-%Y %H;%M;%s");
        return ss.str();
    }

    bool each(ChunkCallback aCallback){
        size_t thePos{0};
        bool theResult{true};
        Chunk theChunk;
        while(theResult){
            theResult = getChunk(theChunk,thePos);
            if(theResult){
                theResult = aCallback(theChunk,thePos++);
            }
        }
        return true;
    }

    size_t getNumberOfChunks(){
        return theChunks.size();
    }

    bool find(const std::string &aName, int* aFindIndex = nullptr ){
        bool found = false;
        if(getNumberOfChunks()) {
            each([&](Chunk &theChunk, size_t aPos) {
                if (theChunk.header.fileName == aName && theChunk.header.partNum == 1) {
                    if (aFindIndex != nullptr) {
                        *aFindIndex = static_cast<int>(theChunk.header.ChunkNum);
                    }
                    found = true;
                    return true;
                }
                return false;
            });
            return found;
        }else return false;
    };

    std::map<size_t, size_t> getChunksOfAFile(const std::string &aName){
        std::map<size_t,size_t> theFileMap;

        each([&](Chunk& theChunk, size_t aPos){
            if(theChunk.header.fileName == aName){
                theFileMap[theChunk.header.partNum] = aPos;
            }
            return true;
        });
//        int theChunkIdx{-1};
//        find(aName,&theChunkIdx);
//        while(theChunkIdx!= -1) {
//            Chunk theChunk;
//            if(getChunk(theChunk, theChunkIdx)){
//                if(theChunk.header.fileName == aName && theChunk.header.partNum){
//                    theFileMap[theChunk.header.partNum] = theChunkIdx;
//                }
//                theChunkIdx = static_cast<int>(theChunk.header.nextIdx);
//            } else break;
//
//        }
        return  theFileMap;
    };

    // Chunkify a normal file
    void writeDataToChunks(const std::string& aFileName){
        size_t theFileSize = getFileSize();
        size_t theReqdNumOfChunks = getInputChunkCount();
        size_t theStreamPos{0};
        size_t theDataSize{0};

        auto theFreeIdx = getFreeChunks();

        if(theFreeIdx.size() < theReqdNumOfChunks){
            addChunks(theReqdNumOfChunks - theFreeIdx.size());
        }

        theFreeIdx = getFreeChunks();


        for(auto i = 0;i< theReqdNumOfChunks; i++) {
            theDataSize = (theStreamPos + kAvailableSize > theFileSize) ? theFileSize - theStreamPos : kAvailableSize;
            inputStream->seekg(static_cast<int>(theStreamPos), std::ios::beg);
            inputStream->read(theChunks[theFreeIdx[i]].data, static_cast<long>(theDataSize));
            theStreamPos += theDataSize;

            theChunks[theFreeIdx[i]].header.timeInserted = getTimeInserted();
            theChunks[theFreeIdx[i]].header.fileName = aFileName;
            theChunks[theFreeIdx[i]].header.occupied = true;
            theChunks[theFreeIdx[i]].header.partNum = i + 1;
            theChunks[theFreeIdx[i]].header.ChunkNum = theFreeIdx[i];
            theChunks[theFreeIdx[i]].header.dataSize = theDataSize;
            theChunks[theFreeIdx[i]].header.fileSize = theFileSize;
            if (i != theReqdNumOfChunks - 1) {
                theChunks[theFreeIdx[i]].header.nextIdx = theFreeIdx[i + 1];
            }
            else theChunks[theFreeIdx[i]].header.nextIdx = -1;
        }
    }

    bool writeChunksToFile(){
        for(auto &theChunk : theChunks) {
            outputStream->write(reinterpret_cast<const char *>(&theChunk), sizeof(theChunk));
            if(outputStream->fail()){
                std::cerr << "failed to write to the archive file\n";
                return false;
            }
        }
        return true;
    };

    bool readChunksFromFile(){
        bool theStatus{false};
        if(inputStream) {
            inputStream->seekg(0, std::ios::end);
            size_t theInputChunks = (inputStream->tellg()) / kChunkSize;
            inputStream->seekg(0); // place the pointer back at the beginning of the stream
            if (!theChunks.empty()) {
                theChunks.clear();
            }
            addChunks(theInputChunks);
            each([&](Chunk &theChunk, size_t aPos) {
                inputStream->read(reinterpret_cast<char *>(&theChunks[aPos].header), sizeof(Header));
                inputStream->read(theChunks[aPos].data, kAvailableSize);
                theStatus = true;
                return true;
            });
        }
        return theStatus;
    }

    bool retrieve(const std::string &aName){
        auto theFileMap = getChunksOfAFile(aName);
        for (const auto& pair : theFileMap){
            auto theChunk = theChunks[pair.second];
            outputStream->write(theChunk.data,static_cast<long>(theChunk.header.dataSize));
            if(outputStream->fail()){
                std::cerr << "failed to write the file data\n";
                return false;
            }
        }
        return true;
    };

    bool deleteChunksOfFile(const std::string &aName){
        auto theFileMap = getChunksOfAFile(aName);
        for(const auto& pair : theFileMap){
            auto theChunk = theChunks[pair.second];
            std::memset(theChunk.data, '\0', theChunk.header.dataSize);
            theChunk.header = {false,0,0,0,0,0, "null", "null"};
        }
        return true;
    }

    void removeEmptyChunks(){
        auto newEnd = std::remove_if(theChunks.begin(),theChunks.end(),
                                     [](const Chunk& chunk) { return !chunk.header.occupied; });
        theChunks.erase(newEnd, theChunks.end());
    }

    void updateAfterCompact(){
       std::map<std::string, std::vector<size_t>> theFileIdxMap;
       each([&](Chunk &theChunk, size_t aPos){
           if(theChunk.header.occupied) {
               theFileIdxMap[theChunk.header.fileName].push_back(aPos);
           }
           return true;
       });

        // arrange the chunknum values in the order of the partnums
        for (auto& pair : theFileIdxMap) {
            std::sort(pair.second.begin(), pair.second.end(), [&](size_t a, size_t b) {
                return theChunks[a].header.partNum < theChunks[b].header.partNum;
            });
        }

        //update the next_idx
        for(auto& pair : theFileIdxMap){
            size_t index = 0;
            for(auto current = pair.second.begin(); current != pair.second.end(); ++current, ++index){
                if(std::next(current) != pair.second.end()){
                    theChunks[*current].header.nextIdx = *(std::next(current));
                }else{
                    theChunks[*current].header.nextIdx = -1;
                }
            }
        }

    }

    void listFiles(std::ostream& anOutput, size_t* aNumFiles = nullptr){
        anOutput << "###   name               size               date added\n";
        anOutput << "------------------------------------------------------\n";

        each([&](Chunk& aChunk, size_t aPos) {
            if (aChunk.header.occupied && aChunk.header.partNum == 1) {
                anOutput << std::setw(5) << std::right << ++(*aNumFiles) << ".  ";
                anOutput << std::setw(12) << std::left << aChunk.header.fileName << " ";
                anOutput << std::setw(12) << std::right << aChunk.header.fileSize << "    ";
                anOutput << aChunk.header.timeInserted << "\n";
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
            anOutput << std::setw(3) << (aPos + 1) << ".   " << std::setw(8) << status << "     " << name << "\n";
            return true;
        });
    }

    void setInputStream(std::istream* anInputStream) {
        inputStream = anInputStream;
    }

    void setOutputStream(std::ostream* anOutputStream) {
        outputStream = anOutputStream;
    }

private:
    std::vector<Chunk> theChunks;
    std::istream* inputStream;
    std::ostream* outputStream;
};

#endif //ECE141_ARCHIVE_CHUNKMANAGER_H
