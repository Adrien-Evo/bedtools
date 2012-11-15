/*****************************************************************************
  absdist.cpp

  (c) 2009 - Aaron Quinlan
  Hall Laboratory
  Department of Biochemistry and Molecular Genetics
  University of Virginia
  aaronquinlan@gmail.com

  Licenced under the GNU General Public License 2.0 license.
******************************************************************************/
#include "lineFileUtilities.h"
#include "absdist.h"

/*
    Constructor
*/
AbsoluteDistance::AbsoluteDistance(string bedAFile, 
                                   string bedBFile,
                                   bool summary)
{
    _bedAFile  = bedAFile;
    _bedBFile  = bedBFile;
    _summary   = summary;
    _tot_queries = 0;
    CalculateAbsoluteDistance();
}


/*
    Destructor
*/
AbsoluteDistance::~AbsoluteDistance(void) {
}


void AbsoluteDistance::LoadMidpoints() {

    _bedB = new BedFile(_bedBFile);

    BED bed;    
    _bedB->Open();
    while (_bedB->GetNextBed(bed)) {
        CHRPOS midpoint = (int) (bed.end + bed.start) / 2;
        _db_midpoints[bed.chrom].push_back(midpoint);
    }
    
    map<string, vector<CHRPOS> >::const_iterator midItr = _db_midpoints.begin();
    map<string, vector<CHRPOS> >::const_iterator midEnd = _db_midpoints.end();
    for (; midItr != midEnd; ++midItr)
    {
        sort(_db_midpoints[midItr->first].begin(), 
             _db_midpoints[midItr->first].end());
    }
}


void AbsoluteDistance::ReportDistanceSummary()
{
    cout << "absdist\t"
         << "count\t"
         << "total\t"
         << "fraction\n";
    
    map<int, size_t>::const_iterator freqItr = _absdists.begin();
    map<int, size_t>::const_iterator freqEnd = _absdists.end();
    for (; freqItr != freqEnd; ++freqItr)
    {
        printf("%d\t%lu\t%lu\t%.3f\n", 
               freqItr->first, 
               freqItr->second,
               _tot_queries,
               (float) freqItr->second / (float) _tot_queries);
    }
}


void AbsoluteDistance::UpdateDistanceSummary(float rel_dist)
{
    _tot_queries++;
    // round the relative distance to two decimal places.
    int log_dist = (int) log(rel_dist);
    _absdists[log_dist]++;
}


void AbsoluteDistance::CalculateAbsoluteDistance() 
{
    LoadMidpoints();
    
    vector<CHRPOS>::iterator low;
    size_t low_idx, high_idx;
    float abs_dist;
    
    _bedA = new BedFile(_bedAFile);

    BED bed;    
    _bedA->Open();
    while (_bedA->GetNextBed(bed)) {
        
        if (_bedA->_status != BED_VALID)
            continue;

        vector<CHRPOS> *chrom_mids = &_db_midpoints[bed.chrom];
        // binary search the current query's midpoint among
        // the database midpoints
        int midpoint = (int) (bed.end + bed.start) / 2;
        low = lower_bound(chrom_mids->begin(), 
                          chrom_mids->end(), 
                          midpoint);
        
        // grab the indicies for the database midpoints that are left and
        // right of the query's midpoint.
        low_idx = low - chrom_mids->begin() - 1;
        high_idx = low_idx + 1;
        
        // make sure we don't run off the boundaries of the database's
        // midpoint vector
        if (low_idx != chrom_mids->size() - 1) 
        {
            // grab the database midpoints that are left and right of
            // the query's midpoint.
            int left = (*chrom_mids)[low_idx];
            int right = (*chrom_mids)[high_idx];
            
            // ?
            if (left > midpoint)
                continue;
            
            // calculate the relative distance between the query's midpoint
            // and the two nearest database midpoints.
            size_t left_dist = abs(midpoint-left);
            size_t right_dist = abs(midpoint-right);            
            abs_dist = min(left_dist, right_dist);

            if (!_summary)
            {
                _bedA->reportBedTab(bed);
                printf("%.3f\n", abs_dist);
            }
            else { 
                UpdateDistanceSummary(abs_dist);
            }
        }
    }

    // report the "histogram" of distances.
    if (_summary)
        ReportDistanceSummary();
}


