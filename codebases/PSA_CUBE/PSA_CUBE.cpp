/*###############################################################
## MODULE: PSA_CUBE.cpp
## VERSION: 1.0 
## SINCE 2015-09-01
## AUTHOR Jimmy Lin (xl5224) - JimmyLin@utexas.edu  
## DESCRIPTION: 
##      
#################################################################
## Edited by MacVim
## Class Info auto-generated by Snippet 
################################################################*/

#include "stdio.h"
#include "stdlib.h"
#include <limits>
#include <vector>
#include <sstream> 
#include <iostream>
#include <fstream>

using namespace std;

/* Debugging option */
//#define RECURSION_TRACE

/* Constants */
const double MIN_DOUBLE = -1*numeric_limits<double>::max();

/* Define scores of each case */
const double MATCH_SCORE = +2;
const double MISMATCH_SCORE = -1;
const double INSERTION_SCORE = -1;
const double DELETION_SCORE = -1;
const char GAP_NOTATION = '-';

/* Define match identification function */
bool isMatch1 (char DNA1, char DNA2) {
    if (DNA1 > DNA2) {
        char temp = DNA1;
        DNA1 = DNA2;
        DNA2 = temp;
    }
    if (DNA1 == 'A' and DNA2 == 'T' or DNA1 == 'C' and DNA2 == 'G') 
        return true;
    else return false;
}
bool isMatch2 (char DNA1, char DNA2) {
    return DNA1==DNA2;
}

/* Data Structure */
enum Action {
    INSERTION, DELETION, MATCH, MISMATCH, UNDEFINED
};
string action2str (Action action) {
    switch (action) {
        case INSERTION: return "Insertion";
        case DELETION: return "Deletion";
        case MATCH: return "Match";
        case MISMATCH: return "Mismatch";
        case UNDEFINED: return "Undefined";
    }
}
class Cell {
    public:
        double score; 
        Action action;

        int row_index, col_index; 
        char acidA, acidB;
        Cell () {
            this->score = 0;
            this->action = UNDEFINED;
            this->row_index = -1;
            this->col_index = -1;
            this->acidA = '?';
            this->acidB = '?';
        }
        string toString () {
            stringstream s;
            s << "(" << this->row_index << ", " << this->col_index << ", ";
            s << action2str(this->action) << ", ";
            s << this->acidA << ", " << this->acidB;
            s << ", " << this->score << ") ";
            return s.str();
        }
        char toActSymbol () {
            switch (this->action) {
                case INSERTION: return '-';
                case DELETION: return '|';
                case MATCH: return '\\';
                case MISMATCH: return '@';
                case UNDEFINED: return '*';
            }
        }
};
typedef vector<char> Sequence;
typedef vector<vector<vector<Cell> > > Cube;
typedef vector<vector<Cell> > Plane;
typedef vector<Cell> Trace;

/* 
   The first sequence is observed. 
   The second sequence is the one to be aligned with the observed one.
   */
void usage () {
    cout << "./PSA_CUBE [seq_file]" << endl;
    cout << "seq_file should contain two DNA sequence in its first line and second line. " << endl;
    cout << "The first sequence is observed. " << endl;
    cout << "The second sequence is the one to be aligned with the observed one." << endl;
}


void cubic_smith_waterman (Sequence seqA, Sequence seqB, Plane& plane, Trace& trace) {
    int nRow = plane.size();
    int nCol = plane[0].size();
    // 1. fill in the plane
    double max_score = MIN_DOUBLE;
    int max_i = -1, max_j = -1;
    for (int i = 0; i < nRow; i ++) {
        for (int j = 0; j < nCol; j ++) {
            plane[i][j].row_index = i;
            plane[i][j].col_index = j;
            if (i == 0 or j == 0) continue;
            char acidA = seqA[j-1];
            char acidB = seqB[i-1];
            double mscore = isMatch2(acidA,acidB)?MATCH_SCORE:MISMATCH_SCORE;
            double mm_score = plane[i-1][j-1].score + mscore;
            double ins_score = MIN_DOUBLE;
            for (int l = 1; j - l > 0 ; l ++) 
                ins_score = max(ins_score, plane[i][j-l].score + l * INSERTION_SCORE);
            double del_score = MIN_DOUBLE;
            for (int l = 1; i - l > 0 ; l ++) 
                del_score = max(del_score, plane[i-l][j].score + l * DELETION_SCORE);
            double opt_score = MIN_DOUBLE;
            Action opt_action;
            char opt_acidA, opt_acidB;
            if (ins_score >= max(mm_score, del_score)) {
                opt_score = ins_score;
                opt_action = INSERTION;
                opt_acidA = acidA;
                opt_acidB = GAP_NOTATION;
            } else if (del_score >= max(mm_score, ins_score)) {
                opt_score = del_score;
                opt_action = DELETION;
                opt_acidA = GAP_NOTATION;
                opt_acidB = acidB;
            } else if (mm_score >= max(ins_score, del_score)) {
                opt_score = mm_score;
                opt_action = isMatch2(acidA,acidB)?MATCH:MISMATCH;
                opt_acidA = acidA;
                opt_acidB = acidB;
            }
            plane[i][j].score = opt_score;
            plane[i][j].action = opt_action;
            plane[i][j].acidA = opt_acidA;
            plane[i][j].acidB = opt_acidB;

            if (opt_score >= max_score) {
                max_score = opt_score;
                max_i = i;
                max_j = j;
            }
        }
    }
    // 2. trace back
    cout << "max_i: " << max_i << ", max_j: " << max_j << endl;
    if (max_i == 0 or max_j == 0) {
        trace.push_back(plane[max_i][max_j]);
        return; 
    }
    int i,j;
    for (i = max_i, j = max_j; i > 0 and j > 0; ) {
        trace.insert(trace.begin(), plane[i][j]);
        switch (plane[i][j].action) {
            case MATCH: i--; j--; break;
            case MISMATCH: i--; j--; break;
            case INSERTION: j--; break;
            case DELETION: i--; break;
            case UNDEFINED: cerr << "uncatched action." << endl; break;
        }
    }
    if (i == 0 and j == 0) return;
    // special cases
    else 
        trace.insert(trace.begin(), plane[1][1]);
    
}

int main (int argn, char** argv) {
    // 1. usage
    if (argn < 2) {
        usage();
        exit(1);
    }

    // 2. input DNA sequence file
    ifstream seq_file(argv[1]);
    string primary_DNA, secondary_DNA;
    getline(seq_file, primary_DNA);
    getline(seq_file, secondary_DNA);
    seq_file.close();
    cout << "#########################################################" << endl;
    cout << "ScoreMatch: " << MATCH_SCORE;
    cout << ", ScoreInsertion: " << INSERTION_SCORE;
    cout << ", ScoreDeletion: " << DELETION_SCORE;
    cout << ", ScoreMismatch: " << MISMATCH_SCORE << endl;
    cout << "1st_DNA: " << primary_DNA << endl;
    cout << "2nd_DNA: " << secondary_DNA << endl;

    // 3. process Dynamic Programming
    Sequence seqA (primary_DNA.begin(), primary_DNA.end());
    Sequence seqB (secondary_DNA.begin(), secondary_DNA.end());
    int lenSeqA = seqA.size();
    int lenSeqB = seqB.size();
    Cube cube (lenSeqB+1, Plane(lenSeqA+1, Trace(5, Cell())));
    Trace trace (0, Cell());
    // cubic_smith_waterman (seqA, seqB, plane, trace);

    // 4. output the result
    /*
    cout << ">>>>>>>>>>>>>>>>>>>>>>>Summary<<<<<<<<<<<<<<<<<<<<<<<<" << endl;
    cout << "Length of Trace: " << trace.size();
    cout << ", Score: " << trace.back().score;
    cout << endl;
    int numInsertion = 0, numDeletion = 0, numMatch = 0, numMismatch = 0, numUndefined = 0;
    for (int i = 0; i < trace.size(); i ++) {
        switch (trace[i].action) {
            case MATCH: ++numMatch; break;
            case INSERTION: ++numInsertion; break;
            case DELETION: ++numDeletion; break;
            case MISMATCH: ++numMismatch; break;
            case UNDEFINED: ++numUndefined; break;
        }
    }
    cout << "numMatch: " << numMatch;
    cout << ", numInsertion: " << numInsertion;
    cout << ", numDeletion: " << numDeletion;
    cout << ", numMismatch: " << numMismatch;
    cout << ", numUndefined: " << numUndefined;
    cout << endl;
    // a. tuple view
    cout << ">>>>>>>>>>>>>>>>>>>>>>>TupleView<<<<<<<<<<<<<<<<<<<<<<<<" << endl;
    for (int i = 0; i < trace.size(); i ++) 
        cout << trace[i].toString() << endl;
    // b. sequence view
    cout << ">>>>>>>>>>>>>>>>>>>>>>>SequenceView<<<<<<<<<<<<<<<<<<<<<<<<" << endl;
    cout << "1st_aligned_DNA: ";
    for (int i = 0; i < trace.size(); i ++) 
        cout << trace[i].acidA;
    cout << endl;
    cout << "2nd_aligned_DNA: ";
    for (int i = 0; i < trace.size(); i ++) 
        cout << trace[i].acidB;
    cout << endl;
    // c. plane view
    cout << ">>>>>>>>>>>>>>>>>>>>>>>PlaneView<<<<<<<<<<<<<<<<<<<<<<<<" << endl;
    cout << "* - ";
    for (int i = 0; i < seqA.size(); i ++) 
        cout << seqA[i] << " ";
    cout << endl;
    for (int i = 0; i < plane.size(); i ++) {
        if (i == 0) cout << "- ";
        else cout << seqB[i-1] << " ";
        for (int j = 0; j < plane[i].size(); j++) {
            cout << plane[i][j].score;
            cout << " ";
        }
        cout << endl;
    }
    cout << endl;
    cout << "* * ";
    for (int i = 0; i < seqA.size(); i ++) 
        cout << seqA[i] << " ";
    cout << endl;
    for (int i = 0; i < plane.size(); i ++) {
        if (i == 0) cout << "* ";
        else cout << seqB[i-1] << " ";
        for (int j = 0; j < plane[i].size(); j++) {
            cout << plane[i][j].toActSymbol();
            cout << " ";
        }
        cout << endl;
    }
    */
    cout << "#########################################################" << endl;
    return 0;
}
