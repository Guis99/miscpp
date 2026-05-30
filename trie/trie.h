#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>
#include <iostream>
#include <memory>

/* ---------------Basic trie implementation-------------- 
- Uses naive 'new' keyword for managing memory
*/
struct TrieNodeBasic {
    std::array<std::shared_ptr<TrieNodeBasic>, 26> children;
    bool isTerminal;
    
    TrieNodeBasic() : 
        children(),
        isTerminal(false) {}
        
    ~TrieNodeBasic() {}
};

class TrieBasic {
    public:
        TrieBasic() {
            rootNode = std::make_shared<TrieNodeBasic>();
        };
        void insert(std::string_view key);
        void remove_key(std::string_view key);
        bool query(std::string_view key);

    private:
        std::shared_ptr<TrieNodeBasic> rootNode;
        std::shared_ptr<TrieNodeBasic> addNode(std::shared_ptr<TrieNodeBasic> node, size_t child_idx);
};

/* ---------------Container-based implementation-------------- 
- Nodes kept in contiguous std::vector, uses indexes to track children 
*/

struct TrieNodeVec {
    std::array<uint32_t, 26> children;
    bool isTerminal;
    char _pad[128 - 26*4 - 1];  // pad to 128 bytes; power-of-2 stride removes imul in traversal

    TrieNodeVec() :
        children(),
        isTerminal(false) {}

    ~TrieNodeVec() {}
};

class TrieVec {
    public:
        TrieVec(size_t alloc_size = 400000) {
            nodeVector.resize(alloc_size);
        }

        void insert(std::string_view key);
        void remove_key(std::string_view key);
        bool query(std::string_view key);

    private:
        size_t nextIdx = 1;
        std::vector<TrieNodeVec> nodeVector;
        size_t addNode(size_t node_idx, size_t child_idx);
};

/* ---------------Arena-based implementation-------------- 
- Nodes kept in contiguous memory managed by arena
*/

class Arena {
    public:
        Arena(size_t block_size, size_t num_blocks);
        void* request(size_t mem_size);
    private:
        size_t block_size;
        size_t num_blocks;
        std::vector<void*> blocks;
        size_t offset;
        void* curr_block;   
};

class TrieArena : TrieBasic {
    public:
        TrieArena(size_t block_size=400000, size_t num_blocks=1) : 
            memoryPool(block_size, num_blocks) {}
        ~TrieArena();

        void insert(std::string_view key);
        void remove_key(std::string_view key);
        bool query(std::string_view key);

    private:
        TrieNodeBasic* rootNode;
        Arena memoryPool;
        void addNode(TrieNodeBasic* node, size_t child_idx);
};