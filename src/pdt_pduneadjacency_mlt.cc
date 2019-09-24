
#include "pdt_engines.h"
#include "ptmp/data.h"
#include "ptmp/filter.h"
#include "json.hpp"

#include <czmq.h>
#include <vector>

using json = nlohmann::json;

struct TC{
  uint32_t adjacency;
  uint32_t adcpeak;
  uint32_t adcsum;
  uint32_t tspan;
  uint32_t first_ch;
  uint32_t last_ch;
  uint64_t tstart;
  uint64_t first_time;
  uint64_t last_time;
  int apanum;
};

// #include "pdt/ModuleTrigger.h"
int ModuleTrigger(std::vector<TC> candidates, uint32_t size_thresh, std::pair <int,int> window_gaps, std::pair <int,int> APA_gaps, double slope_diff, int nAPAs);

/* 
From David:

It holds an APA number where I've set it such that 0 is the Upstream
Apa instrumented with Felix, that 1 is the midstream and so on. It can
be calculated from the channel so either we don't change the detid and
calculate it later, or we add a little functionality to calculate it
using the channel of any hit and set it when creating the output TPSet
that represents the TC.


From Phil:

 detid set inside FELIX_BR:

 (m_fiber_no << 16) | (m_slot_no << 8) | m_crate_no
    
m_crate_no is the same as the "online" APA number ("Installation numbering" below):

 Installation numbering:
             APA5  APA6  APA4
  beam -->
             APA3  APA2  APA1

  The Offline numbering:
             APA1  APA3  APA5
  beam -->
             APA0  APA2  APA4

So, based on David's message, the mapping is:

m_crate_no	candidate[8]
5		0
6		1
4		2

(m_crate_no=4 will never occur in the current system, as APA 4 isn't
instrumented with FELIX yet, but the plan is that it will be
eventually).

*/
static
int detid_to_offline_apa_number(int detid)
{
    //                installation APA number  1   2   3   4   5   6
    const std::vector<int> i2o_apa_map = {-1, -1, -1, -1,  2,  0,  1};
    const size_t inst_apa = detid & 0x000000FF;
    if (inst_apa < 0 or inst_apa > 6) { return -1; }
    return i2o_apa_map[inst_apa];
}

// Fixme: TPSet holds 50MHz clocks, PDT assumes 2MHz
const uint32_t hwtick_per_internal = 25;

class pdt_td_engine : public ptmp::filter::engine_t {
    std::vector<TC> candidates;
    ptmp::data::TPSet outbound;
   // ptmp::data::data_time_t twindow{150000}; // 3ms at 50MHz ticks
public:
    pdt_td_engine(const std::string& config) {
      auto jcfg = json::parse(config);

      int nAPAs = 0;
      double slope_diff = 0;
      uint32_t size_thresh = UINT_MAX;
      std::pair <int,int> APA_gaps = (0,0);
      std::pair <int,int> window_gaps = (0,0);
      ptmp::data::data_time_t twindow;
      if (jcfg["module_adj_thresh"].is_number())      { size_thresh = jcfg["module_adj_thresh"]; }
      if (jcfg["slope_diff"].is_number())             { slope_diff = jcfg["slope_diff"]; }
      if (jcfg["module_twindow"].is_number())         { twindow = jcfg["module_twindow"]; }
      if (jcfg["number_APAs"].is_number())            { nAPAs = jcfg["number_APAs"]; }
      if (jcfg["window_gap_pair"].first.is_number() &&
          jcfg["window_gap_pair"].second.is_number()) { window_gaps = jcfg["window_gap_pair"]; }
      if (jcfg["APA_gap_pair"].first.is_number() &&
          jcfg["APA_gap_pair"].second.is_number())    { APA_gaps = jcfg["APA_gap_pair"]; }

    }

    virtual ~pdt_td_engine() {
    }


    void ingest(const ptmp::data::TPSet& fresh) {
        const ptmp::data::TrigPrim* special = nullptr;
        for (const auto& tp : fresh.tps()) {
            if (tp.flags() == PDT_SPECIAL_TP) {
                special = &tp;
                break;
            }
            if (tp.tspan() == 0) {
                continue;
            }
        }
        if (!special) {
            zsys_error("PDUNEAdjacencyMLT_engine: failed to find special TP from %d", fresh.detid());
            return;
        }

        if (candidates.empty()) {   // first in batch
            // warning: This relies on input to be properly synchronous.  
            outbound.set_tstart(fresh.tstart());
            outbound.set_detid(fresh.detid());
        }

        // ModuleTrigger() wants a 9-tuple of ints.  First eight entries
        // are what TriggerCommand() returns.  The ninth is the *offline*
        // APA number (not installation number).
        // 
        //     0         1       2       3      4      5     6    7   8
        // (adjacency, adcpeak, adcsum, tspan, chan1, chan2, t1, t2, apanum)
        TC candidate;

        // Adjacency value is outside the TPSet/TrigPrim model and is
        // special to the PDT TriggerCandidate() algorithm.  We've stashed
        // it in TrigPrim.adcpeak in a final TrigPrim with .flag set to
        // 0xdeadbeaf which is collected above.
        candidate.tstart = fresh.tstart();
        candidate.adjacency = special->adcpeak();
        // 1: not used
        // 2: not used
        // 3: not used
        candidate.first_ch = fresh.chanbeg();
        candidate.last_ch = fresh.chanend();
        // PDT works in 2MHz clock ticks, not 50MHz
        candidate.first_time = special->tstart() / hwtick_per_internal;
        candidate.last_time = (special->tstart() + special->tspan())/hwtick_per_internal;
        candidate.apanum = detid_to_offline_apa_number(fresh.detid());
        candidates.push_back(candidate);

        outbound.set_chanbeg(std::min(outbound.chanbeg(), fresh.chanbeg()));
        outbound.set_chanend(std::min(outbound.chanend(), fresh.chanend()));
    }

    virtual void operator()(const ptmp::data::TPSet& input_tpset,
                            std::vector<ptmp::data::TPSet>& output_tpsets) {


        // We need a sliding window so we can stitch TCs across time windows.
                                                                                   
        const ptmp::data::data_time_t this_tstart = input_tpset.tstart();
        if (candidates.empty()) {
            ingest(input_tpset);
            return;
        }
        if (this_tstart < candidates.back().tstart) {
            zsys_warning("PDUNEAdjacencyMLT_engine: dropping tardy at %ld by %ld",
                         this_tstart, outbound.tstart()-this_tstart);
            return;
        } 

        ingest(input_tpset);
        // Make a simple sliding window, with width = twindow
        while ((candidates.back().tstart - candidates.front().tstart) > twindow ) {
          candidates.erase(candidates.begin());
          // Saves a little time
          if (candidates.size() < 2) return;
        }
         
        // ready to process ( *_gaps are pairs (time,channel) )
        int ModuleTrigger(candidates, size_thresh, window_gaps, APA_gaps, slope_diff, nAPAs);

        // Print some messages for ml debugging
        if(trigger) {
          for(auto cand : candidates) {
            zsys_info("Adj: %ld | APA: %ld  | FT: %ld | FCh: %ld |  LT: %ld |  LCh: %ld", \ 
                      cand.adjacency, cand.apanum, cand.first_time, cand.first_ch, cand.last_time, cand.last_ch);
          }
       }

        if (trigger) {                   // got one
            outbound.set_tstart( candidates.front().tstart );
            outbound.set_count(1+outbound.count());
            outbound.set_created(ptmp::data::now());
            zsys_info("Trigger TS %ld", outbound.tstart());
            output_tpsets.push_back(outbound);
            clear();
        }

    }

    void clear() {
        // don't clear detid
        outbound.set_tstart(0);
        outbound.set_tspan(0);
        outbound.set_chanbeg(0);
        outbound.set_chanend(0);
        outbound.set_totaladc(0);
        outbound.clear_tps();
        candidates.clear();
    }

}; // class pdt_td_engine

PTMP_FILTER(pdt_td_engine, pdune_adjacency_td)
