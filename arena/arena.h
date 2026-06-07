#ifndef ARENA_HEADER
#define ARENA_HEADER

#include <cstddef>
#include <cstdint>
#include <vector>

class Arena {
    public:
        Arena(size_t block_size);
        ~Arena();
        void* request(size_t mem_size, size_t alignment);

        Arena(const Arena&) = delete;
        Arena& operator=(const Arena&) = delete;

    private:
        size_t block_size; // bytes
        size_t num_blocks;
        std::vector<void*> blocks;
        size_t offset; // bytes
        void* curr_block; 

        void alloc_new_block();
};

#endif