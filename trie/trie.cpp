#include "trie.h"

TrieNodeBasic* TrieBasic::addNode(TrieNodeBasic* node, size_t child_idx) {
    // std::cout<<"add"<<std::endl;
    node->children[child_idx] = new TrieNodeBasic();
    node->isLeaf = false;

    return node->children[child_idx];
}

void TrieBasic::insert(std::string_view key) {
    TrieNodeBasic* currNode = rootNode;
    for (const auto& ch : key) {
        // std::cout << ch << std::endl;
        size_t child_idx = ch - 'a';
        if (currNode->children[child_idx]) {
            currNode = currNode->children[child_idx];
        } else {
            currNode = addNode(currNode, child_idx);
        }
    }
}

bool TrieBasic::query(std::string_view key) {
    TrieNodeBasic* currNode = rootNode;
    for (const auto& ch : key) {
        size_t child_idx = ch - 'a';
        if (!currNode->children[child_idx]) {
            return false;
        }

        currNode = currNode->children[child_idx];
    }

    return true;
}

size_t TrieVec::addNode(size_t node_idx, size_t child_idx) {
    size_t new_idx = nextIdx++;
    if (new_idx >= nodeVector.size()) nodeVector.resize(2 * new_idx);
    nodeVector[node_idx].children[child_idx] = new_idx;
    nodeVector[node_idx].isLeaf = false;
    return new_idx;
}

void TrieVec::insert(std::string_view key) {
    size_t currNode = 0;
    for (const auto& ch : key) {
        size_t child_idx = ch - 'a';
        if (nodeVector[currNode].children[child_idx] == 0) {
            currNode = addNode(currNode, child_idx);
        } else {
            currNode = nodeVector[currNode].children[child_idx];
        }
    }
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

    return true;
}