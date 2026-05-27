#include "trie.h"
#include <cassert>
#include <vector>

void test_trie_naive() {
    std::cout<<"b4 inst"<<std::endl;
    TrieBasic trie = TrieBasic();
    std::cout<<"after inst"<<std::endl;
    
    trie.insert("hello");
    trie.insert("c");
    trie.insert("word");
    trie.insert("words");

    assert(trie.query("hello"));
    assert(trie.query("c"));
    assert(trie.query("word"));
    assert(trie.query("words"));

    assert(!trie.query("wordss"));
}

void test_trie_vector() {
    std::cout<<"b4 inst"<<std::endl;
    TrieVec trie = TrieVec();
    std::cout<<"after inst"<<std::endl;

    trie.insert("hello");
    trie.insert("c");
    trie.insert("word");
    trie.insert("words");

    assert(trie.query("hello"));
    assert(trie.query("c"));
    assert(trie.query("word"));
    assert(trie.query("words"));

    assert(!trie.query("wordss"));
}

int main() {
    test_trie_naive();
    test_trie_vector();
}