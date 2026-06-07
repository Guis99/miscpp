#include <iostream>
#include <memory>
#include <new>

#include "arena.h"

Arena::Arena(size_t block_size) :
    block_size(block_size),
    num_blocks(1),
    offset(0) {

    curr_block = malloc(block_size);
    blocks.push_back(curr_block);
}

Arena::~Arena() {
    #ifdef DUMP_ARENA_STATS
        size_t mem = (num_blocks - 1) * block_size + offset;
        printf("====Arena allocator teardown summary====\n");
        printf("Total blocks allocated: %zu\n", num_blocks);
        printf("Total memory used: %zu\n", mem);
        printf("=========================================\n");
        std::cout << std::endl;
    #endif
    for (auto block : blocks) {
        free(block);
    }
}

void* Arena::request(size_t mem_size, size_t alignment) {
    void* ptr = static_cast<char*>(curr_block) + offset;
    size_t space = block_size - offset;

    if (!std::align(alignment, mem_size, ptr, space)) {
        alloc_new_block();
        ptr = curr_block;
        space = block_size;
        std::align(alignment, mem_size, ptr, space);
    }

    offset = static_cast<char*>(ptr) - static_cast<char*>(curr_block) + mem_size;
    return ptr;
}

void Arena::alloc_new_block() {
    curr_block = malloc(block_size);
    if (!curr_block) throw std::bad_alloc();

    blocks.push_back(curr_block);
    num_blocks++;
}