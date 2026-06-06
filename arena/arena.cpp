#include "arena.h"

// Arena allocator-based trie
Arena::Arena(size_t block_size, size_t num_blocks) :
    block_size(block_size),
    num_blocks(num_blocks),
    offset(0),
    block_idx(0) {

    curr_block = malloc(block_size);
    blocks.push_back(curr_block);

    for (size_t blk_idx = 1; blk_idx < num_blocks; blk_idx++) {
        blocks.push_back(malloc(block_size));
    }
}

Arena::~Arena() {
    #ifdef DUMP_ARENA_STATS
        size_t mem = (blocks.size() - 1) * block_size + offset;
        printf("====Arena allocator teardown summary====\n");
        printf("Total blocks allocated: %zu\n", blocks.size());
        printf("Total memory used: %zu\n", mem);
        printf("=========================================\n");
        std::cout << std::endl;
    #endif
    for (auto block : blocks) {
        free(block);
    }
}

void* Arena::request(size_t mem_size) {
    if (offset + mem_size > block_size) {
        if (block_idx >= num_blocks - 1 ) {
            curr_block = malloc(block_size);
            blocks.push_back(curr_block);
        } else {
            curr_block = blocks[block_idx+1];
        }
        
        offset = 0;
        block_idx++;
    }

    char* place_loc = reinterpret_cast<char*>(curr_block) + offset;
    offset += mem_size;

    return place_loc;
}
