#include "trie.h"
#include <exception>

// Vector-based trie implementation
size_t TrieVec::addNode(size_t node_idx, size_t child_idx) {
    size_t new_idx = nextIdx++;
    if (new_idx >= nodeVector.size()) nodeVector.resize(2 * nodeVector.size());
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
            std::exception();
        }
    }

    nodeVector[currNode].isTerminal = false;
}

TrieNodeRaw* TrieArena::addNode(TrieNodeRaw* node, size_t child_idx) {
    void* ptr = memoryPool.request(sizeof(TrieNodeRaw));
    node->children[child_idx] = new (ptr) TrieNodeRaw();

    return node->children[child_idx];
}
