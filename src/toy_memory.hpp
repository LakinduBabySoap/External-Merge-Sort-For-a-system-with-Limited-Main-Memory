#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <string>
#include <map>
#include <iomanip>


struct BufferPool {
    // TODO: Implement Buffer Pool
    int B; // Buffer pool size in words
    int b; // Block size in words
    // Simulate Buffer Pool with a vector
    // Each block availability.(free or not)
    std::vector<double> buffer;
    std::vector<bool> block_availability;
    BufferPool(int buffer_size, int block_size)
	:B(buffer_size), b(block_size), buffer(buffer_size), block_availability(buffer_size/block_size, true){}
};

struct BufferPoolManager {
    BufferPool& buffer_pool;
    BufferPoolManager(BufferPool& buffer_pool) : buffer_pool(buffer_pool) {}

    int allocate(int num_blocks) {
	// num_words refers to the number of words that are allocated or freed
	int num_words = buffer_pool.b * num_blocks;
        for(size_t i=0 ; i < buffer_pool.block_availability.size(); ++i){
            if (buffer_pool.block_availability[i] == true){
                bool block_free = true;
                //check if the block has enough free blocks
                for (size_t j = 0 ; j < num_blocks ; j++){
                    if (buffer_pool.block_availability.size() <= (i+j) || (buffer_pool.block_availability[i+j] == false)){
                        block_free = false;
                        break;
                        // no consecutive blocks of space available to store the data
                    }
                }
                if (block_free == true){
                    //have space for allocation
                    for (size_t j = 0 ; j < num_blocks ; ++j){
                        buffer_pool.block_availability[i+j] = false;
                    }
                    return i*(buffer_pool.b);
                }
            }
        }
        std::cout << "Memory allocation failed due to not enough consecutive memory blocks!" << std::endl;
        return -1;
    }

    void free(int start_address, int num_blocks) {
      	// we assume the address is an address returned by the allocate function
        int block_index = start_address/buffer_pool.b;
        for (int i = 0 ; i < num_blocks; ++i){
            if (block_index + i < buffer_pool.block_availability.size()){
                buffer_pool.block_availability[block_index + i] = true;
            }
        }
    }
};

struct SecStore {
    std::map<std::string, std::vector<double> > symbols;
    void write_file(std::string file_name, std::vector<double> data) {
        std::ofstream outfile(file_name);
        if (!outfile){
            std::cerr << "Error opening file for writing!" << std::endl;
            return;
        }
        outfile << std::fixed << std::setprecision(15);
        for (const double& value : data){
            outfile << value << std::endl;
        }
        outfile.close();
    }
     
    std::vector<double> read_file(const std::string& file_name, const std::string& key) {
        std::ifstream infile(file_name);
        std::vector<double> data;
        if (!infile) {
            std::cerr << "Error opening input file!" << std::endl;
            return {}; // Exit if the file cannot be opened
        }
        double value;
        while (infile >> value){
            data.push_back(value);
        }
        infile.close();
        symbols[key] = data;
        return data;
    }
    //this function for seeing if the data is written properly and to get sorted data and end of the sorting
    std::vector<double> get_data(const std::string& key){
        if (symbols.find(key) != symbols.end()){
            return symbols[key];
        } else{
            std::cerr << "No data found for the key: " << key << std::endl;
            return {};
        }
    }

};

struct SecStoreManager {
    SecStore& sec_store;
    BufferPool& buffer_pool;
    BufferPoolManager& buffer_pool_manager;
    int b;
    int T;
    int H = 0;

    SecStoreManager(SecStore& sec_store, BufferPool& buffer_pool, BufferPoolManager& buffer_pool_manager, int b, int T) : sec_store(sec_store), buffer_pool(buffer_pool), buffer_pool_manager(buffer_pool_manager), b(b), T(T) {}

    bool read(const std::string& name, int start, int size , int buff_address) {
        // read data from secondary storage to buffer pool
        if (sec_store.symbols.find(name) == sec_store.symbols.end()){
            std::cerr << "Invalid file name in secondary storage!" << std::endl;
            return false;
        }
        if (buff_address < 0 || size < 0 || (buff_address + size) > buffer_pool.B){
            std::cerr << "Invalid starting buffer address or size!" << std::endl;
            return false;
        }
        std::vector<double> data = sec_store.get_data(name);
        if (data.empty()){
            std::cerr << "No data found for the key: " << name << std::endl;
            return false;
        }
        if (start >= data.size()){
            std::cerr << "All data in the list have been parsed to buffer (read function)!" << std::endl;
            return false;
        }
        int num_blocks = size/ b;
        for (int i = 0 ; i < num_blocks ; ++i){
            H = H + T;
            //main function is written so that read function is reads only 1 block of words a time
        }
        int allocated_address = buffer_pool_manager.allocate(num_blocks);
        if (allocated_address == -1){
            std::cerr << "Memory allocation failed in buffer pool!" << std::endl;
            return false;
        }
        for (int i = 0 ; i < size ; ++i){
            buffer_pool.buffer[buff_address + i] = data[start + i];
        }
        return true;
    }

    bool write(std::string& key, int size, int buff_address) {
        // write data from buffer pool to secondary storage
        std::vector<double> data(size);
        if (key == "sublist_-1") {
            if (sec_store.symbols.find(key) != sec_store.symbols.end()) {
                // Key exists, append data
                for (int i = 0; i < size; ++i){
                    data[i] = buffer_pool.buffer[buff_address + i];
                }
                sec_store.symbols[key].insert(sec_store.symbols[key].end(), data.begin(), data.end());
                //Data appended successfully to the existing entry for key '-1' in secondary storage!
            } else {
                // Key does not exist, create a new entry
                for (int i = 0; i < size; ++i){
                    data[i] = buffer_pool.buffer[buff_address + i];
                }
                sec_store.symbols[key] = data;
                //Data written successfully as a new entry for key '-1' in secondary storage!
            }
            return true;
        }
        if (buff_address < 0){
            std::cerr << "Invalid buffer address" << std::endl;
            return false;
        }
        for (int i = 0; i < size; ++i){
            data[i] = buffer_pool.buffer[buff_address + i];
        }
        sec_store.symbols[key] = data;
        return true;
    }

};
