#include <array>
#include <cstdint>
#include <vector>
#include <iostream>

/* ---------------Basic trie implementation-------------- 
- Uses naive 'new' keyword for managing memory
*/
struct TrieNodeBasic {
    std::array<TrieNodeBasic*, 26> children;
    bool isLeaf;
    
    TrieNodeBasic() : 
        children(),
        isLeaf(true) {}
        
    ~TrieNodeBasic() {}
};

class TrieBasic {
    public:
        TrieBasic() {
            rootNode = new TrieNodeBasic();
        };
        void insert(std::string_view key);
        void remove_key(std::string_view key);
        bool query(std::string_view key);

    private:
        TrieNodeBasic* rootNode;
        TrieNodeBasic* addNode(TrieNodeBasic* node, size_t child_idx);
};

/* ---------------Container-based implementation-------------- 
- Nodes kept in contiguous std::vector, uses indexes to track children 
*/

struct TrieNodeVec {
    std::array<uint32_t, 26> children;
    bool isLeaf;
    char _pad[128 - 26*4 - 1];  // pad to 128 bytes; power-of-2 stride removes imul in traversal

    TrieNodeVec() :
        children(),
        isLeaf(true) {}

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

class TrieArena {
    public:
        TrieArena();
        ~TrieArena();

        void insert(std::string_view key);
        void remove_key(std::string_view key);
        bool query(std::string_view key);

    private:
        TrieNodeBasic* rootNode;
        void addNode(TrieNodeBasic* node, size_t child_idx);
};