#include "ModuleTrigger.h"


/*
The following function takes some set of trigger candidates as designed for the May 2019 protoDUNE DAQ test to look for horizontal crossing muons, keeping in mind at the time of writing, only APA 1 and 3 (by LArSoft's numbering) were instrumented with Felix.
*/

//int nAPAs_Felix = 2;

//int window_time_gap = 150;
//int window_channel_gap = 30;
//int APA_time_gap = 150;
//int APA_channel_gap = 30;
//uint32_t size_thresh = 350;

//double slope_diff = 0.4;

// std::pairs = (time gap, channel gap)
int ModuleTrigger(std::vector<TC> candidates, uint32_t size_thresh, std::pair <int,int> window_gaps, std::pair <int,int> APA_gaps, double slope_diff, int nAPAs){
  //PERFORM ANY SORTING NECESSARY, RIGHT NOW, NONE DONE SINCE PACKAGE STRUCTURE PRESENTLY UNCLEAR AND ASSUMED TO BE TIME ORDERED.

  uint64_t last_end_time = 0; 
  uint32_t last_end_channel = 10000;
  int last_APA = -1000;
  int trigger;

  std::vector<uint32_t> APA_size(nAPAs_Felix,0);

  double last_slope, slope = -1;

  for (int i=0; i < candidates.size(); ++i){

    /*
    cout << "CANDIDATE" << endl;
    cout << "APA: " << candidates.at(i).at(9) << endl;
    cout << "Window: " << candidates.at(i).at(10) << endl;
    cout << "ADJ: " << candidates.at(i).adjacency << endl;
    //    cout << "First Channel: " << candidates.at(i).first_ch << endl;
    cout << "First Tick: " << candidates.at(i).first_time << endl;
    //cout << "Last Channel: " << candidates.at(i).last_ch << endl;
    cout << "Last Tick: " << candidates.at(i).last_time << endl;
    */

    // The use of unsigned types makes this a bit cumbersome, but this should work assuming the differences don't overflow a double
    slope = uint64abs(candidates.at(i).first_time,candidates.at(i).last_time)/uint32abs(candidates.at(i).first_ch,candidates.at(i).last_ch);
    if ((candidates.at(i).first_time<candidates.at(i).last_time && candidates.at(i).first_ch>candidates.at(i).last_ch)
       || (candidates.at(i).first_time>candidates.at(i).last_time && candidates.at(i).first_ch<candidates.at(i).last_ch)) slope *= -1;

    /*
    cout << "Slope: " << slope << endl;
    cout << "" << endl;
    */

    if (candidates.at(i).apanum == last_APA){
      if (last_slope < 0){
	if (uint32abs(candidates.at(i).last_ch,last_end_channel) <= window_gaps.second && 
	    uint64abs(candidates.at(i).last_time,last_end_time) <= window_gaps.first &&
	    fabs(slope-last_slope) <= slope_diff){
	  APA_size.at(candidates.at(i).apanum)+=candidates.at(i).adjacency;
	}
	else{
	  for (int j=0; j < nAPAs_Felix; ++j){
	    if (APA_size.at(j) >= size_thresh){
	      ++trigger;
	    }
	    APA_size.at(j)=0;
	  }
	  if (trigger == nAPAs_Felix){
	    return 1;
	  }
	  else{
	    trigger=0;
	    APA_size.at(candidates.at(i).apanum)+=candidates.at(i).adjacency;
	  }
	}
      }
      else if (last_slope > 0){
	if (uint32abs(candidates.at(i).first_ch,last_end_channel) <= window_gaps.second && 
	    uint64abs(candidates.at(i).first_time,last_end_time) <= window_gaps.first &&
	    fabs(slope-last_slope) <= slope_diff){
	  APA_size.at(candidates.at(i).apanum)+=candidates.at(i).adjacency;
	}
	else{
	  for (int j=0; j < nAPAs_Felix; ++j){
	    if (APA_size.at(j) >= size_thresh){
	      ++trigger;
	    }
	    APA_size.at(j)=0;
	  }
	  if (trigger == nAPAs_Felix){
	    return 1;
	  }
	  else{
	    trigger=0;
	    APA_size.at(candidates.at(i).apanum)+=candidates.at(i).adjacency;
	  }
	}
      }
      else{
	//TOO LAZY TO CODE ANYTHING FOR 0 SLOPE RN... IT'S TRICKY...	
      }
    }

    else if (fabs(candidates.at(i).apanum-last_APA) == 1){
      if (last_slope < 0){
	if (uint32abs(479,(candidates.at(i).last_ch+last_end_channel)) <= APA_gaps.second && 
	    uint64abs(candidates.at(i).last_time,last_end_time) <= APA_gaps.first &&
	    fabs(slope-last_slope) <= slope_diff){
	  APA_size.at(candidates.at(i).apanum)+=candidates.at(i).adjacency;
	}
	else{
	  for (int j=0; j < nAPAs_Felix; ++j){
	    if (APA_size.at(j) >= size_thresh){
	      ++trigger;
	    }
	    APA_size.at(j)=0;
	  }
	  if (trigger == nAPAs_Felix){
	    return 1;
	  }
	  else{
	    trigger=0;
	    APA_size.at(candidates.at(i).apanum)+=candidates.at(i).adjacency;
	  }
	}
      }
      else if (last_slope > 0){
	if (uint32abs((479+candidates.at(i).first_ch),last_end_channel) <= APA_gaps.second && 
	    uint64abs(candidates.at(i).first_time,last_end_time) <= APA_gaps.first &&
	    fabs(slope-last_slope) <= slope_diff){
	  APA_size.at(candidates.at(i).apanum)+=candidates.at(i).adjacency;
	}
	else{
	  for (int j=0; j < nAPAs_Felix; ++j){
	    if (APA_size.at(j) >= size_thresh){
	      ++trigger;
	    }
	    APA_size.at(j)=0;
	  }
	  if (trigger == nAPAs_Felix){
            return 1;
	  }
	  else{
	    trigger=0;
	    APA_size.at(candidates.at(i).apanum)+=candidates.at(i).adjacency;
	  }
	}
      }
      else{
	//TOO LAZY TO CODE ANYTHING FOR 0 SLOPE RN... IT'S TRICKY...
      }
    }

    else{
      for (int j=0; j < nAPAs_Felix; ++j){
	if (APA_size.at(j) >= size_thresh){
	  ++trigger;
	}
	APA_size.at(j)=0;
      }
      if (trigger == nAPAs_Felix){
	return 1;
      }
      else{
	trigger=0;
	APA_size.at(candidates.at(i).apanum)+=candidates.at(i).adjacency;
      }
    }


    last_slope = slope;
    last_APA = candidates.at(i).apanum;
    if (slope < 0){
      last_end_channel = candidates.at(i).first_ch;
      last_end_time = candidates.at(i).first_time;
    }
    else if (slope > 0){
      last_end_channel = candidates.at(i).last_ch;
      last_end_time = candidates.at(i).last_time;
    }
    else{
      //TOO LAZY TO CODE ANYTHING FOR 0 SLOPE RN... IT'S TRICKY...
    }
  }
  for (int j=0; j < nAPAs_Felix; ++j){
    if (APA_size.at(j) >= size_thresh){
      ++trigger;
    }
    APA_size.at(j)=0;
  }
  if (trigger == nAPAs_Felix){
    return 1;
  }
  else{
    return 0;
  }
}
