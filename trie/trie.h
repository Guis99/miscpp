#ifndef TRIE_HEADER
#define TRIE_HEADER

#include <array>
#include <vector>
#include <iostream>
#include <memory>

#include <cstddef>
#include <cstdint>
#include <cassert>

#include <arena/arena.h>

/* ---------------Basic trie implementation-------------- 
- Uses naive 'new' keyword for managing memory
*/

struct TrieNodeBasic {
    std::array<std::shared_ptr<TrieNodeBasic>, 26> children;
    bool isTerminal;
    
    TrieNodeBasic() : 
        children(),
        isTerminal(false) {}
};

struct TrieNodeRaw {
    std::array<TrieNodeRaw*, 26> children;
    bool isTerminal;
    
    TrieNodeRaw() : 
        children(),
        isTerminal(false) {}
};

template<typename Derived, typename T>
class TrieBase {
    public:
        TrieBase() = default;

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
                    throw std::runtime_error("key not found");                
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
        ~TrieBasic();
    protected:
        T addNode(T node, size_t child_idx);
};

// explicit template instantiations

template<> TrieBasic<TrieNodeRaw*>::TrieBasic();
template<> TrieBasic<TrieNodeRaw*>::~TrieBasic();
template<> TrieNodeRaw* TrieBasic<TrieNodeRaw*>::addNode(TrieNodeRaw*, size_t);

template<> TrieBasic<std::shared_ptr<TrieNodeBasic>>::TrieBasic();
template<> TrieBasic<std::shared_ptr<TrieNodeBasic>>::~TrieBasic();
template<> std::shared_ptr<TrieNodeBasic> TrieBasic<std::shared_ptr<TrieNodeBasic>>::addNode(std::shared_ptr<TrieNodeBasic>, size_t);

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
};

class TrieVec {
    public:
        TrieVec(size_t alloc_size = 400000);

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
    friend class TrieBase<TrieArena, TrieNodeRaw*>; 
    public:
        TrieArena(size_t block_size=400000);

    private:
        Arena memoryPool;
        TrieNodeRaw* addNode(TrieNodeRaw* node, size_t child_idx);
};

#endif