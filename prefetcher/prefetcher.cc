/*
 * A sample prefetcher which does sequential one-block lookahead.
 * This means that the prefetcher fetches the next block _after_ the one that
 * was just accessed. It also ignores requests to blocks already in the cache.
 */

#include "interface.hh"
#include<iostream>
#include<list>
#include<deque>
#include<stdint.h>

using namespace std;

list<Addr> candidates;
list<Addr> prefetches;
deque<Addr> inFlight;

struct TableEntry{

    Addr PC;
    Addr LastAddress;
    Addr LastPrefetch;
    deque<Addr> ArrayOfDeltas;
};

struct DCPT_Table {
    DCPT_Table();
    TableEntry *FindEntry (Addr PC);
    list<TableEntry *> EntryList;
    
};


static DCPT_Table *TableLookUp;
void prefetch_filter (TableEntry *entry){

    prefetches.clear();
    list<Addr>::iterator candidates_;

    for (candidates_ = candidates.begin(); candidates_!= candidates.end(); ++candidates_){

        if(!inFlight(candidates_) && !in_mshr_queue(candidates_) && !in_cache(candidates_)){
            prefetches.push_back(*candidates_);
            inFlight.push_back(*candidates_);

            entry->LastPrefetch = *candidates_;
        }

        if(candidates_ == entry->LastPrefetch){
            prefetches.clear();
        }
    }

    //return prefetches;
}

list<Addr> Delta_Correlation(TableEntry *entry){
    candidates.clear();
    
    deque<Addr>::iterator array_ = entry->ArrayOfDeltas.end();
    array_ -=1;
    Addr d1 = *array_;

    array_ -=1;
    Addr d2 = *array_;

    Addr address = entry->LastAddress;

    array_ = entry->ArrayOfDeltas.begin();

    while( array_ != entry->ArrayOfDeltas.end()){

        Addr u= *array_;
        array_++;
        Addr v= *array_;

        if ( u == d1 && v == d2){
            for (array_ ++; array_!= entry ->ArrayOfDeltas.end();++array_)
            address = address + *array_;
            candidates.push_back(address);
        }
    }

    return candidates;

}

void prefetch_init(void)
{
    /* Called before any calls to prefetch_access. */
    /* This is the place to initialize data structures. */
    TableLookUp = new DCPT_Table;

    //DPRINTF(HWPrefetch, "Initialized sequential-on-access prefetcher\n");
}

void prefetch_access(AccessStat stat)
{
    // /* pf_addr is now an address within the _next_ cache block */
    // Addr pf_addr = stat.mem_addr + BLOCK_SIZE;

    // /*
    //  * Issue a prefetch request if a demand miss occured,
    //  * and the block is not already in cache.
    //  */
    // if (stat.miss && !in_cache(pf_addr)) {
    //     issue_prefetch(pf_addr);
    // }
    TableEntry *CurrentEntry = TableLookUp -> FindEntry(stat.pc);
    uint64_t Delta ;
    Delta = stat.mem_addr - CurrentEntry->LastAddress;
    if (CurrentEntry == 0){
        TableEntry * NewEntry = new TableEntry(stat.pc);
        NewEntry ->PC = stat.pc;
        NewEntry ->LastAddress = stat.mem_addr;
        NewEntry ->ArrayOfDeltas.push_front(1);
        NewEntry ->LastPrefetch = 0;
        TableLookUp ->EntryList.push_front(NewEntry);
        CurrentEntry = NewEntry;

    }
    else if( Delta !=0){
        CurrentEntry ->ArrayOfDeltas.push_back(Delta);
        CurrentEntry ->LastAddress = stat.mem_addr;
    candidates = Delta_Correlation(CurrentEntry);
    prefetches = prefetch_filter(CurrentEntry, candidates);
    issue_prefetch(prefetches);
    }

}

void prefetch_complete(Addr addr) {
    /*
     * Called when a block requested by the prefetcher has been loaded.
     */
}




