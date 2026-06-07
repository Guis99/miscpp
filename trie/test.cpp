#include "trie.h"
#include <cassert>

void test_trie_naive_shared_ptr() {
    std::cout<<"b4 inst"<<std::endl;
    TrieBasic<std::shared_ptr<TrieNodeBasic>> trie = TrieBasic<std::shared_ptr<TrieNodeBasic>>();
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
    assert(!trie.query("w"));
    assert(!trie.query("wo"));
    assert(!trie.query("wor"));

    trie.remove_key("word");
    trie.insert("wo");

    assert(!trie.query("word"));
    assert(trie.query("wo"));
    assert(trie.query("words"));
}

void test_trie_naive() {
    std::cout<<"b4 inst"<<std::endl;
    TrieBasic<TrieNodeRaw*> trie = TrieBasic<TrieNodeRaw*>();
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
    assert(!trie.query("w"));
    assert(!trie.query("wo"));
    assert(!trie.query("wor"));

    trie.remove_key("word");
    trie.insert("wo");

    assert(!trie.query("word"));
    assert(trie.query("wo"));
    assert(trie.query("words"));
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
    assert(!trie.query("w"));
    assert(!trie.query("wo"));
    assert(!trie.query("wor"));

    trie.remove_key("word");
    trie.insert("wo");

    assert(!trie.query("word"));
    assert(trie.query("wo"));
    assert(trie.query("words"));
}

void test_trie_arena() {
    std::cout<<"b4 inst"<<std::endl;
    TrieArena trie = TrieArena(500);
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
    assert(!trie.query("w"));
    assert(!trie.query("wo"));
    assert(!trie.query("wor"));

    trie.remove_key("word");
    trie.insert("wo");

    assert(!trie.query("word"));
    assert(trie.query("wo"));
    assert(trie.query("words"));
}

int main() {
    test_trie_naive_shared_ptr();
    test_trie_naive();
    test_trie_vector();
    test_trie_arena();
}