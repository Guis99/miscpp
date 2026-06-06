#include <cstddef>
#include <cstdint>
#include <vector>
#include <iostream>

#ifndef ARENA_HEADER
#define ARENA_HEADER

class Arena {
    public:
        Arena(size_t block_size, size_t num_blocks);
        ~Arena();
        void* request(size_t mem_size);

        Arena(const Arena&) = delete;
        Arena& operator=(const Arena&) = delete;

    private:
        size_t block_size; // bytes
        size_t num_blocks;
        std::vector<void*> blocks;
        size_t offset; // bytes
        void* curr_block; 
        size_t block_idx;
};

#endif