//
// Created by Sandeep Chintada on 3/7/24.
//

#ifndef ECE141_ARCHIVE_CHUNKMANAGER_CPP
#define ECE141_ARCHIVE_CHUNKMANAGER_CPP

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
#include <zlib.h>
#include "Archive.hpp"

namespace ECE141 {


    void ChunkManager::addChunks(size_t numChunksToAdd){
        Chunk theChunk;
        for(int i = 0; i<numChunksToAdd; i++){
            std::memset(&theChunk,0,sizeof(theChunk));
            archiveFileStream.seekp(0,std::ios::end);
            archiveFileStream.write(reinterpret_cast<char*>(&theChunk),sizeof(theChunk));
            numberOfChunks++;
            if(archiveFileStream.fail()){
                std::cerr << "failed to add chunks to the archive file\n";
            }
        }
    }

    size_t ChunkManager::getArchiveChunkCount(){
        if(!archiveFileStream.is_open()) std::cerr << "unable to open archive stream to read\n";
        archiveFileStream.seekg(0,std::ios::end);
        numberOfChunks = archiveFileStream.tellg() / kChunkSize;
        return numberOfChunks;
    }

    [[nodiscard]] size_t ChunkManager::getInputFileSize() const{
        std::ifstream theInputStream(inputFile, std::ios::binary | std::ios::in);
        if (!theInputStream.is_open()) std::cerr << "unable to open file\n";
        theInputStream.seekg(0, std::ios::end);
        inputFileSize = theInputStream.tellg();
        theInputStream.seekg(0,std::ios::beg);

        return inputFileSize;
    }

    [[nodiscard]] size_t ChunkManager::getInputChunkCount() const{
        auto theSize = getInputFileSize();
        return (theSize/kAvailableSize) + (theSize % kAvailableSize? 1: 0);
    }

    bool ChunkManager::getChunk(Chunk &theChunk , size_t aPos){
        if(aPos < numberOfChunks) {
            size_t theChunkPos = kChunkSize*aPos;
            archiveFileStream.seekg(static_cast<int>(theChunkPos),std::ios::beg);
            archiveFileStream.read(reinterpret_cast<char *>(&theChunk), sizeof(theChunk));
            return true;
        }else{
            return false;
        }
    }

    std::vector<int> ChunkManager::getFreeChunks(){
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

     int64_t ChunkManager::getTimeInserted(){
        auto now = std::chrono::system_clock::now();
        auto now_secs = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
        int64_t now_secs_64 = static_cast<int64_t>(now_secs);

        return now_secs_64;
    }
    std::string ChunkManager::unixTimeToDate(int64_t aUnixTime) {
        auto t = static_cast<std::time_t>(aUnixTime);
        std::tm *localTime = std::localtime(&t);
        char buffer[80];
        std::strftime(buffer, sizeof(buffer), "%d-%m-%Y %H:%M:%S", localTime);
        return {buffer};
    }

    bool ChunkManager::each(ChunkCallback aCallback){
        size_t thePos{0};
        bool theResult{true};
        Chunk theChunk;
        std::memset(&theChunk,0,sizeof(theChunk));
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

    bool ChunkManager::find(const std::string &aName){
        bool found = false;
        if (numberOfChunks) {
            each([&](Chunk& theChunk, size_t aPos) -> bool {
                if (std::strcmp(theChunk.header.fileName, aName.c_str()) == 0 && theChunk.header.partNum == 1) {
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

    std::map<size_t, size_t> ChunkManager::getChunksOfAFile(const std::string &aName){
        std::map<size_t,size_t> theFileMap;

        each([&](Chunk& theChunk, size_t aPos){
            if(std::strcmp(theChunk.header.fileName, aName.c_str()) == 0){
                theFileMap[theChunk.header.partNum] = aPos;
            }
            return true;
        });
        return  theFileMap;
    };
    void ChunkManager::writeChunksToArchive(const std::string& aFileName, IDataProcessor* aProcessor) {
        std::ifstream theInputStream(inputFile, std::ios::binary | std::ios::in);
        size_t theFileSize = getInputFileSize();
        std::vector<uint8_t> fileData;

        bool isCompressed = false;
        std::vector<uint8_t> theCompressedData;

        if (aProcessor != nullptr) {
            // Read the entire file into memory (consider streaming for large files)
            fileData.resize(theFileSize);
            theInputStream.read(reinterpret_cast<char*>(fileData.data()), theFileSize);

            // Compress the file data
            theCompressedData = aProcessor->process(fileData);
            isCompressed = true;
            theFileSize = theCompressedData.size(); // Update fileSize to reflect compressed size
            std::cout << "filesize" << theFileSize << std::endl;
        }

        size_t theStreamPos = 0;
        size_t theReqdNumOfChunks = theFileSize/kAvailableSize + (theFileSize % kAvailableSize? 1: 0); ; // Adjusted to use a function for clarity
        auto theFreeIdx = getFreeChunks();

        if (theFreeIdx.size() < theReqdNumOfChunks) {
            addChunks(theReqdNumOfChunks - theFreeIdx.size());
        }
        theFreeIdx = getFreeChunks();

        for (size_t i = 0; i < theReqdNumOfChunks; i++) {
            archiveFileStream.seekp(kChunkSize * theFreeIdx[i], std::ios::beg);

            Chunk theChunk;
            std::memset(&theChunk, 0, sizeof(theChunk));

            size_t chunkDataSize = (theStreamPos + kChunkSize - sizeof(theChunk.header) > theFileSize) ? theFileSize - theStreamPos : kChunkSize - sizeof(theChunk.header);

            if (isCompressed) {
                std::memcpy(theChunk.data, theCompressedData.data() + theStreamPos, chunkDataSize);
            } else {
                // If not compressed, read directly from the file
                theInputStream.seekg(static_cast<int>(theStreamPos), std::ios::beg);
                theInputStream.read(theChunk.data, static_cast<long>(chunkDataSize));
            }

            theStreamPos += chunkDataSize;

            // Setup chunk header here
            theChunk.header.timeInserted = getTimeInserted();
            std::strcpy(theChunk.header.fileName, aFileName.c_str());
            theChunk.header.occupied = true;
            theChunk.header.partNum = i + 1;
            theChunk.header.ChunkNum = theFreeIdx[i];
            theChunk.header.dataSize = chunkDataSize;
            theChunk.header.fileSize = theFileSize; // This might reflect the compressed file size
            theChunk.header.compressed = isCompressed;
            if (i != theReqdNumOfChunks - 1) {
                theChunk.header.nextIdx = theFreeIdx[i + 1];
            } else {
                theChunk.header.nextIdx = 0;
            }

            // Write the chunk to the archive
            archiveFileStream.write(reinterpret_cast<const char*>(&theChunk), sizeof(theChunk));
        }
    }

    bool ChunkManager::retrieve(const std::string &aName, const std::string &aFullPath){
//        auto aProcessor = std::make_unique<IDataProcessor>();
        std::unique_ptr<IDataProcessor> aProcessor = std::make_unique<Compression>();
        auto theFileMap = getChunksOfAFile(aName);
        if(theFileMap.empty()) return false;

        Chunk theChunk;
        std::vector<uint8_t> theCompressedData;
        std::ofstream theOutputFileStream(aFullPath, std::ios::binary | std::ios::out | std::ios::trunc);

        for (const auto& pair : theFileMap){
            std::memset(&theChunk,0,sizeof(theChunk));

            archiveFileStream.seekg(static_cast<int>((pair.second)*kChunkSize), std::ios::beg);
            archiveFileStream.read(reinterpret_cast<char*>(&theChunk),sizeof(theChunk));

            std::streamsize bytesRead = archiveFileStream.gcount();

            if(theChunk.header.compressed){
                theCompressedData.insert(theCompressedData.end(),theChunk.data, theChunk.data + std::min(theChunk.header.dataSize, static_cast<size_t>(bytesRead) - sizeof(theChunk.header)));
            } else {
                theOutputFileStream.write(theChunk.data, static_cast<long>(theChunk.header.dataSize));
            }
            theOutputFileStream.seekp(0,std::ios::end);
        }

        if(theOutputFileStream.fail()){
            std::cerr << "failed to write the file data\n";
            return false;
        }

        if (!theCompressedData.empty()) {
            std::vector<uint8_t> theUncompressedData = aProcessor->reverseProcess(theCompressedData);
            theOutputFileStream.write(reinterpret_cast<const char*>(theUncompressedData.data()), static_cast<std::streamsize>(theUncompressedData.size()));
            if (theOutputFileStream.fail()) {
                std::cerr << "Failed to write decompressed file data.\n";
                return false;
            }
        }
        return true;
    };

    bool ChunkManager::deleteChunksOfFile(const std::string &aName){
        auto theFileMap = getChunksOfAFile(aName);
        if(theFileMap.empty()) return false;

        Chunk theEmptyChunk;
        for(const auto& pair : theFileMap){
            archiveFileStream.seekp(static_cast<int>((pair.second)*kChunkSize),std::ios::beg);
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

    void ChunkManager::Compact(){
        std::string tempfile = "/tmp/temp.arc";
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

    void ChunkManager::listFiles(std::ostream& anOutput, size_t* aNumFiles){
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
                anOutput << std::right << std::setw(20) << unixTimeToDate(aChunk.header.timeInserted) << "\n";
            }
            return true;
        });
    }

    void ChunkManager::dump(std::ostream& anOutput){
        anOutput << "###    status       name\n";
        anOutput << "------------------------\n";
        each([&](Chunk& theChunk, size_t aPos){
            std::string status = theChunk.header.occupied ? "used": "empty";
            std::string name = theChunk.header.occupied ? theChunk.header.fileName : "";
            anOutput << (aPos + 1) << ".   " << status << "     " << name <<  "    " << theChunk.header.dataSize <<  "\n";
            return true;
        });
    }

    void ChunkManager::setInputFileName(const std::string& anInputFile) {
        inputFile = anInputFile;
    }
};

#endif //ECE141_ARCHIVE_CHUNKMANAGER_CPP
