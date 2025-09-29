#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <string>
#include <cstdlib>
#include <map>
#include <iomanip>

#include "toy_memory.hpp"

void merge(std::vector<double> &buffer, int B, int left, int mid, int right)
{
    int left_length = mid - left + 1;
    int right_length = right - mid;
    for (int i = 0; i < left_length; ++i)
    {
        buffer[(B / 2) + i] = buffer[left + i];
        // temporarily copying data to second half of buffer
    }
    for (int i = 0; i < right_length; ++i)
    {
        buffer[(3 * B / 4) + i] = buffer[mid + 1 + i];
    }

    int i = 0, j = 0, k = left;
    while ((i < left_length) && (j < right_length))
    {
        if (buffer[(B / 2) + i] <= buffer[(3 * B / 4) + j])
        {
            buffer[k] = buffer[(B / 2) + i];
            k++;
            i++;
            // sorting the elements
        }
        else
        {
            buffer[k] = buffer[(3 * B / 4) + j];
            k++;
            j++;
        }
    }
    while (i < left_length)
    {
        buffer[k++] = buffer[(B / 2) + i];
        i++;
    }

    while (j < right_length)
    {
        buffer[k++] = buffer[(3 * B / 4) + j];
        j++;
    }
}
void merge_sort(std::vector<double> &buffer, int B, int left, int right)
{
    if (left >= right)
    {
        return;
        // base case reached when sorting done
    }
    int mid = left + (right - left) / 2;
    merge_sort(buffer, B, left, mid);
    merge_sort(buffer, B, mid + 1, right);
    merge(buffer, B, left, mid, right);
}
void merge_sorted_sublists(int B, int b, int N, int T, BufferPool &buffer_pool, BufferPoolManager &buffer_pool_manager, SecStoreManager &sec_store_manager, const std::vector<std::string> &sublist_keys, const std::string &input_file, const std::string &output_file)
{
    int sorting_size = B / 2;
    int no_of_sublists = N / sorting_size;
    int no_of_blocks = B / b;
    int no_of_blocks_per_sublist = (no_of_blocks) / (no_of_sublists + 1);
    int no_of_blocks_for_output = no_of_blocks - (no_of_sublists * no_of_blocks_per_sublist);
    int output_buffer_size = B - ((no_of_sublists * no_of_blocks_per_sublist * b) + (2 * no_of_sublists));
    bool sorted = false;
    buffer_pool_manager.free(0, B);
    for (int load = 0; load < no_of_sublists; ++load)
    {
        int start = 0;
        std::string index = "sublist_" + std::to_string(load);
        int bump = 0;
        for (int repeat = 0; repeat < no_of_blocks_per_sublist; ++repeat)
        {
            bool initialise = sec_store_manager.read(index, start, b, ((load * no_of_blocks_per_sublist * b) + bump));
            if (initialise == true)
            {
                std::cout << "Initialised data for sublist " << load << " in address " << (load * no_of_blocks_per_sublist * b) + bump << std::endl;
            }
            bump = bump + b;
            start = start + b;
        }
    }
    // create a pointer array within buffer for pointer
    for (int pointer = 0; pointer < no_of_sublists; ++pointer)
    {
        buffer_pool.buffer[(no_of_sublists * no_of_blocks_per_sublist * b) + pointer] = 0;
    }
    // create a pointer array that points to next address of sublist when new data gets called
    for (int pointer_sublist = 0; pointer_sublist < no_of_sublists; ++pointer_sublist)
    {
        buffer_pool.buffer[(no_of_sublists * no_of_blocks_per_sublist * b) + no_of_sublists + pointer_sublist] = no_of_blocks_per_sublist * b;
    }
    int buffer_index = ((no_of_sublists * no_of_blocks_per_sublist * b) + (2 * no_of_sublists)); // starting address for the buffer
    do
    {
        double element_index = 999;
        int sublist_index = -1;
        for (int sublist = 0; sublist < no_of_sublists; ++sublist)
        {
            if (buffer_pool.buffer[(no_of_sublists * no_of_blocks_per_sublist * b) + sublist] != -1)
            { // pointer -1 means that the sublist is empty since sorting is done!
                int element_address = (sublist * no_of_blocks_per_sublist * b) + buffer_pool.buffer[(no_of_sublists * no_of_blocks_per_sublist * b) + sublist];
                if (buffer_pool.buffer[element_address] < element_index)
                {
                    // new min element found
                    element_index = buffer_pool.buffer[element_address];
                    sublist_index = sublist;
                }
            }
        }
        if ((element_index == 999) && (sublist_index == -1))
        {
            // all elmenents sorted
            sorted = true;
            // flush the current elements in output buffer even if they are not full
            int output_size = buffer_index - ((no_of_sublists * no_of_blocks_per_sublist * b) + (2 * no_of_sublists));
            std::string str = "sublist_" + std::to_string(-1);
            bool sec_write_success = sec_store_manager.write(str, output_size, (buffer_index - output_size));
            if (sec_write_success == true)
            {
                std::cout << "Successfully finished sorting all data!" << std::endl;
            }
            return;
        }
        int pointer_update = buffer_pool.buffer[(no_of_sublists * no_of_blocks_per_sublist * b) + sublist_index];
        pointer_update++; // update pointer to point to next element
        if (pointer_update == (no_of_blocks_per_sublist * b))
        {
            // end of sublist reached and need to call new data in
            pointer_update = 0; // reset pointer to 0
            std::string str = "sublist_" + std::to_string(sublist_index);
            int increment = 0;
            for (int call = 0; call < no_of_blocks_per_sublist; ++call)
            {
                int sublist_address = buffer_pool.buffer[(no_of_sublists * no_of_blocks_per_sublist * b) + no_of_sublists + sublist_index];
                // Next address to get data from sublist
                buffer_pool_manager.free((sublist_index * no_of_blocks_per_sublist * b) + increment, b);
                // make the buffer_pool sublist available again before writing data to buffer_pool again
                int temp_buffer_address = ((sublist_index * no_of_blocks_per_sublist * b) + increment);
                bool call_new_data = sec_store_manager.read(str, sublist_address, b, temp_buffer_address);
                if (call_new_data == true)
                {
                    // std::cout << "Called new data from sublist " << sublist_index << " to buffer for " << call << " times!" << std::endl;
                }
                else
                {
                    // write a very large value so it doesn't get used in the min comparations
                    pointer_update = 0;
                    buffer_pool.buffer[temp_buffer_address] = 99999;
                }
                sublist_address = sublist_address + b;
                buffer_pool.buffer[(no_of_sublists * no_of_blocks_per_sublist * b) + no_of_sublists + sublist_index] = sublist_address;
                increment = increment + b;
            }
        }
        buffer_pool.buffer[(no_of_sublists * no_of_blocks_per_sublist * b) + sublist_index] = pointer_update;
        // move min element to output buffer
        buffer_pool.buffer[buffer_index] = element_index;
        buffer_index++;
        if (buffer_index == (B))
        {
            // buffer is full
            std::string str = "sublist_" + std::to_string(-1);
            bool sec_write_success = sec_store_manager.write(str, output_buffer_size, (buffer_index - output_buffer_size));
            if (sec_write_success == true)
            {
                // std::cout << "Successfully wrote data to sorted data from output buffer to secondary storage!" << std::endl;
            }
            buffer_index = ((no_of_sublists * no_of_blocks_per_sublist * b) + (2 * no_of_sublists)); // resetting buffer address
        }
    } while (sorted == false);
    return;
}
int external_merge_sort(int B, int b, int N, int T, const std::string &input_file, const std::string &output_file)
{
    BufferPool buffer_pool(B, b);
    BufferPoolManager buffer_pool_manager(buffer_pool);
    SecStore sec_store;
    SecStoreManager sec_store_manager(sec_store, buffer_pool, buffer_pool_manager, b, T);

    // read the input file from disk to sec store
    sec_store.read_file(input_file, "input");
    int sorting_size = (B / 2) / b;
    int no_of_sublists = N / (B / 2);
    std::vector<std::string> sublist_keys(no_of_sublists);
    for (int i = 0; i < no_of_sublists; ++i)
    {
        sublist_keys[i] = "sublist_" + std::to_string(i);
    }
    int start = 0;
    for (int k = 0; k < no_of_sublists; ++k)
    {
        int address = -b;
        int block = 0;
        do
        {
            sec_store_manager.read("input", start, b, address + b);
            address = address + b;
            start = start + b;
            block++;
        } while (block < sorting_size);
        merge_sort(buffer_pool.buffer, B, 0, (B / 2) - 1);
        std::string str = "sublist_" + std::to_string(k);
        bool sublist_write = sec_store_manager.write(str, B / 2, 0);
        if (sublist_write == true)
        {
            std::cout << "sorted sublist " << k << " has been written successfully to Secondary Storage!" << std::endl;
        }
        else
        {
            std::cout << "Sorting of sublist has caused an error!" << std::endl;
        }
        // freeing the contents of the buffer pool once sorting is done so new data can be called again
        buffer_pool_manager.free(0, (B / 2) / b);
    }
    merge_sorted_sublists(B, b, N, T, buffer_pool, buffer_pool_manager, sec_store_manager, sublist_keys, input_file, output_file);
    std::vector<double> result = sec_store.get_data("sublist_-1");
    std::cout << std::fixed << std::setprecision(15);
    sec_store.write_file(output_file, result);
    std::cout << "H: " << sec_store_manager.H << std::endl;
    return 0;
}

int main()
{
    // for testing
    int B;
    int b;
    int T;
    int N = 200000;
    std::string input_file, output_file;
    std::cout << "Enter value for B (10,000 / 20,000 / 40,000 / 80,000): ";
    std::cin >> B;
    std::cout << "Enter value for b (100 / 200): ";
    std::cin >> b;
    std::cout << "Enter positive value for T: ";
    std::cin >> T;
    std::cout << "Enter input file path (full path of the input file): ";
    std::cin >> input_file;
    std::cout << "Enter output file path (full path of the output file): ";
    std::cin >> output_file;
    // std::string input_file = "/Users/lakindulehanlithpuramuhandirumge/Documents/CSCI2100A_24P2/inputs/inputs.txt";
    // std::string output_file = "/Users/lakindulehanlithpuramuhandirumge/Documents/CSCI2100A_24P2/outputs/sorted.txt";
    external_merge_sort(B, b, N, T, input_file, output_file);
}
