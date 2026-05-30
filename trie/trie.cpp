#include "trie.h"
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <memory>

// Naive trie implementation
std::shared_ptr<TrieNodeBasic> TrieBasic::addNode(std::shared_ptr<TrieNodeBasic> node, size_t child_idx) {
    // std::cout<<"add"<<std::endl;
    node->children[child_idx] = std::make_shared<TrieNodeBasic>();

    return node->children[child_idx];
}

void TrieBasic::insert(std::string_view key) {
    std::shared_ptr<TrieNodeBasic> currNode = rootNode;
    for (const auto& ch : key) {
        // std::cout << ch << std::endl;
        size_t child_idx = ch - 'a';
        if (currNode->children[child_idx]) {
            currNode = currNode->children[child_idx];
        } else {
            currNode = addNode(currNode, child_idx);
        }
    }

    currNode->isTerminal = true;
}

bool TrieBasic::query(std::string_view key) {
    std::shared_ptr<TrieNodeBasic> currNode = rootNode;
    for (const auto& ch : key) {
        size_t child_idx = ch - 'a';
        if (!currNode->children[child_idx]) {
            return false;
        }
        currNode = currNode->children[child_idx];
    }

    return currNode->isTerminal;
}

void TrieBasic::remove_key(std::string_view key) {
    std::shared_ptr<TrieNodeBasic>currNode = rootNode;
    for (const auto& ch : key) {
        // std::cout << ch << std::endl;
        size_t child_idx = ch - 'a';
        if (currNode->children[child_idx]) {
            currNode = currNode->children[child_idx];
        } else {
            std::exception();
        }
    }

    currNode->isTerminal = false;
}

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

// Arena allocator-based trie
Arena::Arena(size_t block_size, size_t num_blocks) :
    block_size(block_size),
    num_blocks(num_blocks),
    offset(0) {
    curr_block = malloc(block_size);
    blocks.push_back(curr_block);
}

void* Arena::request(size_t mem_size) {

}


