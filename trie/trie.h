#include <array>
#include <cstddef>
#include <cstdint>
#include <cassert>
#include <vector>
#include <iostream>
#include <memory>

#include <arena/arena.h>

#ifndef TRIE_HEADER
#define TRIE_HEADER

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

struct TrieNodeRaw {
    std::array<TrieNodeRaw*, 26> children;
    bool isTerminal;
    
    TrieNodeRaw() : 
        children(),
        isTerminal(false) {}
        
    ~TrieNodeRaw() {}
};

template<typename Derived, typename T>
class TrieBase {
    public:
        TrieBase() {};

        void insert(std::string_view key) {
            T currNode = rootNode;
            for (const auto& ch : key) {
                size_t child_idx = ch - 'a';
                if (currNode->children[child_idx]) {
                    currNode = currNode->children[child_idx];
                } else {
                    currNode = static_cast<Derived*>(this)->addNode(currNode, child_idx);
                }
            }

            currNode->isTerminal = true;
        }

        void remove_key(std::string_view key) {
            T currNode = rootNode;
            for (const auto& ch : key) {
                size_t child_idx = ch - 'a';
                if (currNode->children[child_idx]) {
                    currNode = currNode->children[child_idx];
                } else {
                    std::exception();
                }
            }

            currNode->isTerminal = false;
        }

        bool query(std::string_view key) {
            T currNode = rootNode;
            for (const auto& ch : key) {
                size_t child_idx = ch - 'a';
                if (!currNode->children[child_idx]) {
                    return false;
                }
                currNode = currNode->children[child_idx];
            }

            return currNode->isTerminal;
        }

    protected:
        T rootNode;
};      

template<typename T>
class TrieBasic : public TrieBase<TrieBasic<T>, T> {
    friend class TrieBase<TrieBasic<T>, T>; 
    public:
        TrieBasic();
    protected:
        T addNode(T node, size_t child_idx);
};

template<> inline TrieBasic< std::shared_ptr<TrieNodeBasic>>::TrieBasic() {
    rootNode = std::make_shared<TrieNodeBasic>();
}

template<> inline TrieBasic<TrieNodeRaw*>::TrieBasic() {
    rootNode = new TrieNodeRaw();
}

template<> inline TrieNodeRaw* TrieBasic<TrieNodeRaw*>::addNode(TrieNodeRaw* node, size_t child_idx) {
    node->children[child_idx] = new TrieNodeRaw();
    return node->children[child_idx];
}

template<> inline std::shared_ptr<TrieNodeBasic> TrieBasic< std::shared_ptr<TrieNodeBasic>>::addNode(std::shared_ptr<TrieNodeBasic> node, size_t child_idx) {
    node->children[child_idx] = std::make_shared<TrieNodeBasic>();
    return node->children[child_idx];
}

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

class TrieArena : public TrieBase<TrieArena, TrieNodeRaw*> {
    public:
        TrieArena(size_t block_size=400000, size_t num_blocks=1) : 
            memoryPool(block_size, num_blocks) {
                assert(2 * sizeof(TrieNodeRaw) < block_size);
                void* ptr = memoryPool.request(sizeof(TrieNodeRaw));
                rootNode = new (ptr) TrieNodeRaw();
            }
        ~TrieArena() {};

        TrieNodeRaw* addNode(TrieNodeRaw* node, size_t child_idx);

    private:
        Arena memoryPool;
};

#endif