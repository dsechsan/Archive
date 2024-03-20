//
//  Archive.hpp
//
//
//
//

#ifndef Archive_hpp
#define Archive_hpp


#include <cstdio>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <memory>
#include <optional>
#include <stdexcept>
#include <map>
//#include "ChunkManager.cpp"
#include <zlib.h>
#include <functional>


namespace ECE141 {

    enum class ActionType {added, extracted, removed, listed, dumped};
    enum class AccessMode {AsNew, AsExisting}; //you can change values (but not names) of this enum

    const size_t MAX_BUFFER_SIZE = 16384;

    struct ArchiveObserver {
        void operator()(ActionType anAction,
                        const std::string &aName, bool status);
    };

    enum class ArchiveErrors {
        noError=0,
        fileNotFound=1, fileExists, fileOpenError, fileReadError, fileWriteError, fileCloseError,
        fileSeekError, fileTellError, fileError, badFilename, badPath, badData, badBlock, badArchive,
        badAction, badMode, badProcessor, badBlockType, badBlockCount, badBlockIndex, badBlockData,
        badBlockHash, badBlockNumber, badBlockLength, badBlockDataLength, badBlockTypeLength
    };

    template<typename T>
    class ArchiveStatus {
    public:
        // Constructor for success case
        explicit ArchiveStatus(const T value)
                : value(value), error(ArchiveErrors::noError) {}

        // Constructor for error case
        explicit ArchiveStatus(ArchiveErrors anError)
                : value(std::nullopt), error(anError) {
            if (anError == ArchiveErrors::noError) {
                throw std::logic_error("Cannot use noError with error constructor");
            }
        }

        // Deleted copy constructor and copy assignment to make ArchiveStatus move-only
        ArchiveStatus(const ArchiveStatus&) = delete;
        ArchiveStatus& operator=(const ArchiveStatus&) = delete;

        // Default move constructor and move assignment
        ArchiveStatus(ArchiveStatus&&) noexcept = default;
        ArchiveStatus& operator=(ArchiveStatus&&) noexcept = default;

        T getValue() const {
            if (!isOK()) {
                throw std::runtime_error("Operation failed with error");
            }
            return *value;
        }

        bool isOK() const { return error == ArchiveErrors::noError && value.has_value(); }
        ArchiveErrors getError() const { return error; }

    private:
        std::optional<T> value;
        ArchiveErrors error;
    };


    class IDataProcessor {
    public:
        virtual std::vector<uint8_t> process(const std::vector<uint8_t>& input) = 0;
        virtual std::vector<uint8_t> reverseProcess(const std::vector<uint8_t>& input) = 0;
        virtual ~IDataProcessor(){};
    };

    /** This is new child class of data processor, use it to compress the if add asks for it*/
    class Compression : public IDataProcessor {
    public:
        std::vector<uint8_t> process(const std::vector<uint8_t>& input) override {
            // write the compress process here

            if (input.empty()) {
                return {};
            }

            z_stream zs{};
            zs.zalloc = Z_NULL;
            zs.zfree = Z_NULL;
            zs.opaque = Z_NULL;

            if(deflateInit(&zs, Z_DEFAULT_COMPRESSION) != Z_OK){
                std::cerr << "Failed to initialize zlib for compression." << std::endl;
            }

            zs.next_in = reinterpret_cast<Bytef*>(const_cast<uint8_t*>(input.data()));
            zs.avail_in = input.size();

            int deflateStatus;
            std::vector<uint8_t> output;
            std::vector<uint8_t> tempBuffer(MAX_BUFFER_SIZE);

            do {
                zs.next_out = reinterpret_cast<Bytef*>(tempBuffer.data());
                zs.avail_out = tempBuffer.size();

                deflateStatus = deflate(&zs, Z_FINISH);

                if (output.size() < zs.total_out) {
                    output.insert(output.end(), tempBuffer.begin(), tempBuffer.begin() + static_cast<long>(tempBuffer.size())- zs.avail_out);
                }

            } while (deflateStatus == Z_OK);

            if (deflateStatus != Z_STREAM_END) {
                deflateEnd(&zs);
                throw std::runtime_error("Failed during compression process.");
            }

            deflateEnd(&zs);
            return output;
        }

        std::vector<uint8_t> reverseProcess(const std::vector<uint8_t>& input) override {
            // write the compress process here

            if (input.empty()) {
                return {};
            }

            z_stream zs{};
            zs.zalloc = Z_NULL;
            zs.zfree = Z_NULL;
            zs.opaque = Z_NULL;

            if (inflateInit(&zs) != Z_OK) {
                std::cerr << "Failed to initialize zlib for decompression" << std::endl;
            }
            zs.next_in = reinterpret_cast<Bytef*>(const_cast<uint8_t*>(input.data()));
            zs.avail_in = input.size();

            int inflateStatus;
            std::vector<uint8_t> output;
            std::vector<uint8_t> tempBuffer(MAX_BUFFER_SIZE);

            do {
                zs.next_out = reinterpret_cast<Bytef *>(tempBuffer.data());
                zs.avail_out = tempBuffer.size();

                inflateStatus = inflate(&zs, Z_NO_FLUSH);

                if (output.size() < zs.total_out) {
                    output.insert(output.end(), tempBuffer.begin(), tempBuffer.begin() + static_cast<long>(tempBuffer.size()) - zs.avail_out);
                }

            } while (inflateStatus == Z_OK);

            if (inflateStatus != Z_STREAM_END) {
                inflateEnd(&zs); // Clean up
                throw std::runtime_error("Failed during decompression process.");
            }

            inflateEnd(&zs); // Clean up
            return output;

        }
        ~Compression() override = default;
    };

    //--------------------------------------------------------------------------------
    //You'll need to define your own classes for Blocks, and other useful types...
    //--------------------------------------------------------------------------------

    const int kChunkSize = 1024;

    struct Header {
        bool occupied;
        size_t ChunkNum;
        size_t partNum;
        size_t nextIdx;
        size_t fileSize;
        size_t dataSize;
        char fileName[15];
        int64_t timeInserted;
        bool compressed;
    };
    const int kAvailableSize = kChunkSize - sizeof(Header);
    struct Chunk {
        Header header;
        char data[kAvailableSize] = {};
        Chunk() : header{false, 0, 0, 0, 0, 0, "null", 0, false} {};
    };

    using ChunkCallback = std::function<bool(Chunk &, size_t)>;

    class ChunkManager {
    public:
        ChunkManager() = default;
        explicit ChunkManager(const std::string &FileName) : archiveFileName(FileName), inputFileSize(0), inputFile("null") {
            archiveFileStream.open(archiveFileName, std::ios::binary | std::ios::out | std::ios::in);
            archiveFileStream.seekp(0, std::ios::end);
            if (!archiveFileStream.is_open()) {
                std::cerr << "Unable to open the archive file\n";
            }
        }

        ~ChunkManager() {
            if (archiveFileStream.is_open()) archiveFileStream.close();
        }

        void addChunks(size_t numChunksToAdd);
        size_t getArchiveChunkCount();
        [[nodiscard]] size_t getInputFileSize() const;
        [[nodiscard]] size_t getInputChunkCount() const;
        bool getChunk(Chunk &theChunk, size_t aPos);
        std::vector<int> getFreeChunks();
        static int64_t getTimeInserted();
        std::string unixTimeToDate(int64_t aUnixTime);
        bool each(ChunkCallback aCallback);
        bool find(const std::string &aName);
        std::map<size_t, size_t> getChunksOfAFile(const std::string &aName);
        void writeChunksToArchive(const std::string &aFileName, IDataProcessor *aProcessor);
        bool retrieve(const std::string &aName, const std::string &aFullPath);
        bool deleteChunksOfFile(const std::string &aName);
        void Compact();
        void listFiles(std::ostream &anOutput, size_t *aNumFiles = nullptr);
        void dump(std::ostream &anOutput);
        void setInputFileName(const std::string &anInputFile);

    protected:
        size_t numberOfChunks = 0;
        std::string archiveFileName;
        std::fstream archiveFileStream;
        std::string inputFile; // for the add operation
        mutable size_t inputFileSize;
        std::vector<size_t> emptyChunkIdx;
    };


    class Archive {
    protected:
        std::vector<std::shared_ptr<IDataProcessor>> processors;
        std::vector<std::shared_ptr<ArchiveObserver>> observers;
        Archive(std::string aFullPath, AccessMode aMode);  //protected on purpose
        std::string archiveFilePath;
        AccessMode accessMode;


    public:

        ~Archive() = default;  //

        static    ArchiveStatus<std::shared_ptr<Archive>> createArchive(const std::string &anArchiveName);
        static    ArchiveStatus<std::shared_ptr<Archive>> openArchive(const std::string &anArchiveName);

        Archive&  addObserver(std::shared_ptr<ArchiveObserver>& anObserver);

//        ArchiveStatus<bool>      add(const std::string &aFullPath);
        ArchiveStatus<bool>      extract(const std::string &aFilename, const std::string &aFullPath);
        ArchiveStatus<bool>      remove(const std::string &aFilename);

        ArchiveStatus<size_t>    list(std::ostream &aStream);
        ArchiveStatus<size_t>    debugDump(std::ostream &aStream);

        ArchiveStatus<size_t>    compact();
        ArchiveStatus<std::string> getFullPath() const; //get archive path (including .arc extension)


        ArchiveStatus<bool>      add(const std::string &aFullpath, IDataProcessor* aProcessor=nullptr);
        //STUDENT: add anything else you want here, (e.g. blocks?)...

        static std::string getFileName(const std::string &aFullPath );

        std::shared_ptr<ChunkManager> chunkManager;

    };

}

#endif /* Archive_hpp */
