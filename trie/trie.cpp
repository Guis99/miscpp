#include "trie.h"
#include <exception>

namespace trie_util {

static void delete_trie(TrieNodeRaw* node) {
    if (!node) return;
    for (auto child : node->children) {
        delete_trie(child);
    }

    delete node;
}

}

// Definition of explicit template decls

// TrieBasic with raw pointer
template<> TrieBasic<TrieNodeRaw*>::TrieBasic() {
    rootNode = new TrieNodeRaw();
}

template<> TrieBasic<TrieNodeRaw*>::~TrieBasic() {
    trie_util::delete_trie(this->rootNode);
}

template<> TrieNodeRaw* TrieBasic<TrieNodeRaw*>::addNode(TrieNodeRaw* node, size_t child_idx) {
    node->children[child_idx] = new TrieNodeRaw();
    return node->children[child_idx];
}

// TrieBasic with smart pointer
template<> TrieBasic< std::shared_ptr<TrieNodeBasic>>::TrieBasic() {
    rootNode = std::make_shared<TrieNodeBasic>();
}

template<> TrieBasic<std::shared_ptr<TrieNodeBasic>>::~TrieBasic() = default;

template<> std::shared_ptr<TrieNodeBasic> TrieBasic< std::shared_ptr<TrieNodeBasic>>::addNode(std::shared_ptr<TrieNodeBasic> node, size_t child_idx) {
    node->children[child_idx] = std::make_shared<TrieNodeBasic>();
    return node->children[child_idx];
}

// Vector-based trie implementation
TrieVec::TrieVec(size_t alloc_size) {
    #ifdef VEC_TRIE_SUPEROPT
        nodeVector.resize(alloc_size);
    #else
        nodeVector.reserve(alloc_size);
        nodeVector.emplace_back();
    #endif
}

size_t TrieVec::addNode(size_t node_idx, size_t child_idx) {
    size_t new_idx = nextIdx++;

    #ifdef VEC_TRIE_SUPEROPT
        if (new_idx >= nodeVector.size()) {
            nodeVector.resize(2 * nodeVector.size());
        }
    #else
        nodeVector.emplace_back();
    #endif

    nodeVector[node_idx].children[child_idx] = new_idx;
    return new_idx;
}

void TrieVec::insert(std::string_view key) {
    size_t currNode = 0;
    for (const auto& ch : key) {
        size_t child_idx = ch - 'a';
        if (nodeVector[currNode].children[child_idx] != 0) {
            currNode = nodeVector[currNode].children[child_idx];
        } else {
            currNode = addNode(currNode, child_idx);
        }
    }
    nodeVector[currNode].isTerminal = true;
}

bool TrieVec::query(std::string_view key) {
    size_t currNode = 0;
    for (const auto& ch : key) {
        size_t child_idx = ch - 'a';
        if (!nodeVector[currNode].children[child_idx]) {
            return false;
        }

        currNode = nodeVector[currNode].children[child_idx];
    }
    return nodeVector[currNode].isTerminal;
}

void TrieVec::remove_key(std::string_view key) {
    size_t currNode = 0;
    for (const auto& ch : key) {
        size_t child_idx = ch - 'a';
        if (nodeVector[currNode].children[child_idx] != 0) {
            currNode = nodeVector[currNode].children[child_idx];
        } else {
            throw std::runtime_error("key not found");
        }
    }

    nodeVector[currNode].isTerminal = false;
}

// Arena allocator-based trie

TrieArena::TrieArena(size_t block_size) : 
            memoryPool(block_size) {
    assert(20 * sizeof(TrieNodeRaw) < block_size); // ensure arena has room for at least 20 nodes
    void* ptr = memoryPool.request(sizeof(TrieNodeRaw), alignof(TrieNodeRaw));
    this->rootNode = new (ptr) TrieNodeRaw();
}

TrieNodeRaw* TrieArena::addNode(TrieNodeRaw* node, size_t child_idx) {
    void* ptr = memoryPool.request(sizeof(TrieNodeRaw), alignof(TrieNodeRaw));
    node->children[child_idx] = new (ptr) TrieNodeRaw();

    return node->children[child_idx];
}
