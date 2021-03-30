#ifndef TRIE_H
#define TRIE_H

#include <vector>
#include <iostream>
#include "rtable_entry.h"

using namespace std;

class trieNode {
public:
    pair<unsigned int,rTableEntry> data;
    trieNode* children[2];

    trieNode() {
        data = make_pair(0, rTableEntry());
        children[0] = nullptr;
        children[1] = nullptr;
    }

    ~trieNode() {
        if(children[0]) {
            delete children[0];
        }

        if(children[1]) {
            delete children[1];
        }
    }

    trieNode(int bit, rTableEntry v) {
        data = make_pair(bit ,v);
        children[0] = nullptr;
        children[1] = nullptr;
    }
};

class Trie {
public: 
    trieNode* root;

    Trie() {
        root = new trieNode();
    }

    void insert(rTableEntry entry) {
        trieNode* currentNode = root;
        unsigned int elem = entry.prefix;

        int cont = 0;
        for(int i = 0; i < entry.maskLen; ++i) {
            // get current bit
            int bitValue = (elem>>i) & 1;

            if(!currentNode->children[bitValue]) {
                 // remember bit and initialize with empty entry
                 currentNode->children[bitValue] = new trieNode(bitValue, rTableEntry());
            }

            if(i == entry.maskLen - 1) {
                // initialize end of prefix with the actual entry
                currentNode->children[bitValue]->data.second = entry;
            }

            currentNode = currentNode->children[bitValue];
            cont++;
        } 
    }

    void next(trieNode* node, unsigned int elem, int pos, rTableEntry& match) {
        if(node && pos < 32) {
            int bitValue = (elem>>pos) & 1;

            match = node->data.second;

            // treat case with same prefix but longer mask
            if(bitValue == 1 && !node->children[1] && node->children[0]) {
                next(node->children[0], elem, pos+1, match);
            } else {
                next(node->children[bitValue], elem, pos+1, match);
            }
        }
    }

    rTableEntry getPrefix(unsigned int elem) {
        rTableEntry match;

        if(root && elem > 0) {
            next(root->children[elem & 1], elem, 1, match);
        }

        return match;
    }
};

#endif