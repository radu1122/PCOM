#ifndef RTABLE_H
#define RTABLE_H

#include "trie.h"
#include "rtable_entry.h"

#include <vector>
#include <string>
#include <fstream>
#include <arpa/inet.h>
#include <algorithm>

struct rTable {
    Trie entries;
    std::string fileName;

    rTable(std::string fileName) {
        this->fileName = fileName;
    }

    rTableEntry findBestEntry(unsigned int ip) {
        return entries.getPrefix(ip);
    }

    void parseRTable() {
        std::ifstream f(fileName);

        std::string pString, nhString, maskString;
        int interface;

        while(f>>pString>>nhString>>maskString>>interface) {
            struct in_addr prefix, nextHop, mask;
            inet_aton(pString.c_str(), &prefix);
            inet_aton(nhString.c_str(), &nextHop);
            inet_aton(maskString.c_str(), &mask);

            rTableEntry entry(prefix.s_addr, nextHop.s_addr, ntohl(mask.s_addr), interface);

            entries.insert(entry);
        }

        f.close();
    }
};

#endif