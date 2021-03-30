#ifndef RTABLE_ENTRY_H
#define RTABLE_ENTRY_H
struct rTableEntry {
    unsigned int prefix;
    unsigned int nextHop;
    unsigned int mask;
    int maskLen;
    int interface;

    rTableEntry() {
        prefix=nextHop=mask=maskLen=interface=0;
    }

    rTableEntry(unsigned int prefix, unsigned int nextHop, unsigned int mask, int interface) {
        this->prefix = prefix;
        this->nextHop = nextHop;
        this->mask = mask;
        this->interface = interface;

        maskLen = getMaskLength();
    }

    bool operator<(const rTableEntry& other) const{
        if(prefix == other.prefix) {
            return mask > other.mask; // descending after mask
        }
        
        // ascending after prefix
        return prefix < other.prefix; 
    }

    int getMaskLength() {
        int cont = 0;
        for(int i = 31; i > 0; --i) {
            if(((mask>>i) & 1) == 0) {
                break;
            }
            cont++;
        }
        return cont;
    }
};
#endif