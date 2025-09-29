# External Merge Sort Implementation  for Limited Memoery 

## Project Overview

This project implements an External Merge Sort algorithm for sorting large datasets that cannot fit entirely into main memory. The implementation demonstrates efficient external sorting techniques by managing buffer pools and secondary storage operations, as required for the Data Structures course project. The goal is to sort 200,000(N) double-precision floating-point numbers (each 8 bytes) from an input file using limited main memory (BufferPool of size B words), while minimizing accesses to slower secondary storage (SecStore, T times slower than BufferPool). The space in BufferPool is divided into blocks of b words each.

The algorithm follows the external merge sort process described in the project specification, including creating sorted runs in the first phase and performing a k-way merge in the second phase.

## Implementation Details

### Algorithm Description

The implementation uses a two-phase external merge sort:

1. **First Phase (Run Creation)**: 
   - Read B/2 records (in blocks of b words) from SecStore into BufferPool.
   - Sort each chunk using an out-of-place merge sort (recursively dividing into smaller elements and merging while sorting, using the second half of the buffer for temporary storage).
   - Write sorted sublists back to SecStore as key-value pairs (e.g., "sublist_k" where k = 0 to N/(B/2) - 1, with N=200,000).

2. **Second Phase (k-Way Merge)**: 
   - Divide BufferPool into B/(k+1) blocks for a k-way merge.
   - Allocate space for input buffers (one per sublist) and an output buffer.
   - Use two pointer arrays: one for the next element in the buffer per sublist, and another for the next address in SecStore to fetch data when a buffer empties.
   - Find the smallest element across sublists, write it to the output buffer, and flush to SecStore ("sublist_-1") when full.
   - Continue until all data is merged and sorted.

The merge sort is out-of-place for simplicity, though in-place could improve efficiency. The k-way merge uses direct comparisons (O(k) per element), with a suggestion for min-heap optimization (O(log k)) for better performance in larger buffers. The reason for the lack of the min-heap optimization was due to the constraints preventing the use of memory outside the buffer.

### Key Components

1. **BufferPool**: Manages a pool of memory buffers (size B words, divided into blocks of b words) to store data temporarily during sorting. Uses a vector for data and a boolean vector for block availability.
2. **BufferPoolManager**: Handles allocation and freeing of consecutive memory blocks for reading/writing data.
3. **SecStore**: Simulates secondary storage using a std::map<std::string, std::vector<double>> for key-value pairs of sorted/unsorted data. Includes file read/write functions.
4. **SecStoreManager**: Manages data flow between BufferPool and SecStore, tracking overhead H (H += T per block read). Supports read (from SecStore to buffer) and write (from buffer to SecStore, appending for "sublist_-1").
5. **Other Data Structures**: 
   - Vectors for dynamic data storage (treated as fixed-size arrays here).
   - std::map for efficient key-based lookup in SecStore.

### Performance Metrics

The total overhead H represents the cost of SecStore accesses (H = number of block reads * T). Key observations from analysis:
- H is independent of B but proportional to T and inversely proportional to b (doubling b halves H).
- Higher B reduces sublists (k = N/(B/2)), minimizing SecStore reads/writes but requiring more memory.
- Higher b reduces read operations but may worsen memory allocation.
- Best combination: B=80,000, b=200, T=1 for minimal H.

Example tables (from report, for b=100 and varying T/B):

| T   | H (B=10000) | H (B=20000) | H (B=40000) | H (B=80000) |
|-----|-------------|-------------|-------------|-------------|
| 1   | 4000        | 4000        | 4000        | 4000        |
| 4   | 16000       | 16000       | 16000       | 16000       |
| 16  | 64000       | 64000       | 64000       | 64000       |
| 64  | 256000      | 256000      | 256000      | 256000      |
| 128 | 512000      | 512000      | 512000      | 512000      |
| 256 | 1024000     | 1024000     | 1024000     | 1024000     |

For b=200, H values are halved. Buffer size B has no effect on H when T is fixed (e.g., T=64).

### Tradeoffs and Improvements

1. **Out-of-Place Merge Sort**: Easier to implement but less memory-efficient (sorts B/2 elements at once, increasing SecStore accesses). In-place sort could handle B elements, reducing overhead, but is harder to implement.
2. **K-Way Merge**: Efficient for small k, but O(k) comparisons per element can slow large buffers. Use a min-heap (size k) for O(log k) complexity, trading O(k) space for performance.

## Code Structure

### C++ Implementation

- **main.cpp**: Entry point. Implements merge_sort, merge, merge_sorted_sublists, and external_merge_sort. Prompts for B, b, T, input/output paths, then sorts and outputs H.
- **toy_memory.hpp**: Defines BufferPool, BufferPoolManager, SecStore, and SecStoreManager classes.

## How to Run the Code

### C++ Implementation

1. Ensure you have a C++ compiler (e.g., g++ with C++17 support).

2. Compile the code (Ensure you are in the src/cpp directory):
   ```bash
   g++ main.cpp -o external_sort -std=c++17
   ```

3. Run the executable:
   ```bash
   ./external_sort
   ```

4. When prompted, enter the following parameters:
   - **B**: Buffer pool size in words (e.g., 10000, 20000, 40000, or 80000)
   - **b**: Block size in words (e.g., 100 or 200)
   - **T**: Relative time for secondary storage access (positive integer, e.g., 1, 4, 16, 64, 128, 256)
   - **Input file path**: Full path to the input file (e.g., `/path/to/inputs.txt` containing 200,000 doubles)
   - **Output file path**: Full path to the output file (e.g., `/path/to/sorted.txt`)
   - You should be able to see the following:
   ```bash
    Successfully finished sorting all data!
    H: xxxx
    ```

The program will sort the data, write to the output file, and print the overhead H.

## Notes on Parameters

- **B**: Larger values reduce sublists and SecStore accesses but increase memory use.
- **b**: Larger values reduce read operations (lower H) but may fragment memory.
- **T**: Directly scales H; lower is better.
- **N**: Fixed at 200,000 records.

## Troubleshooting

If you encounter any issues running the code, please ensure:
1. You have the correct file paths for input and output (input must contain exactly 200,000 valid doubles, one per line).
2. You have sufficient permissions to read/write to the specified locations.
3. Memory allocation fails if no consecutive blocks are available—check B and b values.
4. For debugging, reduce N or use smaller inputs.

If problems persist, please contact: lakindumuhandirumge@gmail.com
