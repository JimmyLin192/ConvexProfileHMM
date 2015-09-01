/*###############################################################
## MODULE: PSA_NAIVE.cpp
## VERSION: 1.0 
## SINCE 2015-08-29
##      
#################################################################
## Edited by MacVim
## Class Info auto-generated by Snippet 
################################################################*/

#include "stdio.h"
#include "stdlib.h"
#include <vector>
#include <sstream> 
#include <iostream>
#include <fstream>

using namespace std;

/* Debugging option */
//#define RECURSION_TRACE

/* Define penalities of each case */
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

/* 
    The first sequence is observed. 
    The second sequence is the one to be aligned with the observed one.
*/
void usage () {
    cout << "./PSA_NAIVE [seq_file]" << endl;
    cout << "seq_file should contain two DNA sequence in its first line and second line. " << endl;
    cout << "The first sequence is observed. " << endl;
    cout << "The second sequence is the one to be aligned with the observed one." << endl;
}

class Tracker {
    public:
        double score;
        vector<char> aligned_seqA;
        vector<char> aligned_seqB;
        int numInsertion, numDeletion, numMatch, numMismatch;
        Tracker (double score) {
            this->score = score;
            this->numInsertion = 0;
            this->numDeletion = 0;
            this->numMatch = 0;
            this->numMismatch = 0;
        }
        Tracker (Tracker& tr) {
            this->score = tr.score;
            this->aligned_seqA = tr.aligned_seqA;
            this->aligned_seqB = tr.aligned_seqB;
            this->numInsertion = tr.numInsertion;
            this->numDeletion = tr.numDeletion;
            this->numMatch = tr.numMatch;
            this->numMismatch = tr.numMismatch;
        }
        Tracker (double score, vector<char>& aligned_seqA, vector<char>& aligned_seqB) {
            this->score = score;
            this->aligned_seqA = aligned_seqA;
            this->aligned_seqB = aligned_seqB;
            this->numInsertion = 0;
            this->numDeletion = 0;
            this->numMatch = 0;
            this->numMismatch = 0;
        }
        void operator=(const Tracker& tr) {
            this->score = tr.score;
            this->aligned_seqA = tr.aligned_seqA;
            this->aligned_seqB = tr.aligned_seqB;
            this->numInsertion = tr.numInsertion;
            this->numDeletion = tr.numDeletion;
            this->numMatch = tr.numMatch;
            this->numMismatch = tr.numMismatch;
        }
        string toString () {
            ostringstream s;
            s << "Score: " << score << ", ";
            s << "Aligned_SeqA: ";
            for (int i = 0; i < aligned_seqA.size(); i ++) 
                s << aligned_seqA[i];
            s << ", ";
            s << "Aligned_SeqB: ";
            for (int i = 0; i < aligned_seqB.size(); i ++) 
                s << aligned_seqB[i];
            s << ". ";
            return s.str();
        }
};

Tracker align (vector<char>& seqA, vector<char>& seqB, int posA, int posB, Tracker tr) {
    int lenA = seqA.size();
    int lenB = seqB.size();
    int remlenA = lenA - posA - 1;
    int remlenB = lenB - posB - 1;
    // 1. termination criteria
    if (remlenA < 0 and remlenB < 0) {
        return tr;
    } else if (remlenA < 0) {
        // deletion
        tr.score += DELETION_SCORE * (remlenB+1);
        tr.numDeletion += (remlenB+1);
        for (int i = 0; i < remlenB+1; i++) {
            tr.aligned_seqA.push_back(GAP_NOTATION);
            tr.aligned_seqB.push_back(seqB[posB+i]);
        }
        return tr;
    } else if (remlenB < 0) {
        // insertion
        tr.score += INSERTION_SCORE * (remlenA+1);
        tr.numInsertion += (remlenA+1);
        for (int i = 0; i < remlenA+1; i++) {
            tr.aligned_seqA.push_back(seqA[posA]);
            tr.aligned_seqB.push_back(GAP_NOTATION);
        }
        return tr;   
    }
    // 2. invoke recursion to achieve dynamic programming
    Tracker insertion_tr (tr);  
    Tracker deletion_tr (tr);
    Tracker match_tr (tr);
    if (remlenA >= 0 and remlenB >= 0) {
        // trigger match or mismatch
        char seqA_acid = seqA[posA];
        char seqB_acid = seqB[posB];
        if (isMatch2(seqA_acid, seqB_acid)) {
            match_tr.score += MATCH_SCORE;
            match_tr.numMatch += 1;
        } else {
            match_tr.score += MISMATCH_SCORE;
            match_tr.numMismatch += 1;
        }
        match_tr.aligned_seqA.push_back(seqA_acid);
        match_tr.aligned_seqB.push_back(seqB_acid);
        match_tr = align(seqA, seqB, posA+1, posB+1, match_tr);
    }
    if (remlenA >= 0) {
        // trigger insertion
        char seqA_acid = seqA[posA];
        insertion_tr.score += INSERTION_SCORE;
        insertion_tr.aligned_seqA.push_back(seqA_acid);
        insertion_tr.aligned_seqB.push_back(GAP_NOTATION);
        insertion_tr.numInsertion += 1;
        insertion_tr = align(seqA, seqB, posA+1, posB, insertion_tr);
    } 
    if (remlenB >= 0) {
        // trigger deletion
        char seqB_acid = seqB[posB];
        deletion_tr.score += DELETION_SCORE;
        deletion_tr.aligned_seqA.push_back(GAP_NOTATION);
        deletion_tr.aligned_seqB.push_back(seqB_acid);
        deletion_tr.numDeletion += 1;
        deletion_tr = align(seqA, seqB, posA, posB+1, deletion_tr);
    }

    // 3. compare trackers and return the best one
    Tracker opt_tr(0.0);
    if (match_tr.score >= max(deletion_tr.score, insertion_tr.score)) 
        opt_tr = match_tr;
    else if (insertion_tr.score >= max(deletion_tr.score, match_tr.score))
        opt_tr = insertion_tr;
    else if (deletion_tr.score <= max(match_tr.score, insertion_tr.score)) 
        opt_tr = deletion_tr;
#ifdef RECURSION_TRACE
    cout << opt_tr.toString() << endl;
#endif
    return opt_tr;
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
    cout << "###############################################" << endl;
    cout << "ScoreInsertion: " << INSERTION_SCORE;
    cout << ", ScoreDeletion: " << DELETION_SCORE;
    cout << ", ScoreMismatch: " << MISMATCH_SCORE;
    cout << ", ScoreMatch: " << MISMATCH_SCORE << endl;
    cout << "1st_DNA: " << primary_DNA << endl;
    cout << "2nd_DNA: " << secondary_DNA << endl;
    cout << ">>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<" << endl;
    // 3. process Depth-first Search
    vector<char> seqA (primary_DNA.begin(), primary_DNA.end());
    vector<char> seqB (secondary_DNA.begin(), secondary_DNA.end());
    Tracker init_tr (0.0); 
    Tracker out_tr (0.0);
    out_tr = align(seqA, seqB, 0, 0, init_tr);
    // 4. output the result
    cout << "numInsertion: " << out_tr.numInsertion;
    cout << ", numDeletion: " << out_tr.numDeletion;
    cout << ", numMismatch: " << out_tr.numMismatch;
    cout << ", numMatch: " << out_tr.numMatch;
    cout << ", Score: " << out_tr.score << endl;
    cout << "1st_aligned_DNA: ";
    for (int i = 0; i < out_tr.aligned_seqA.size(); i++) 
        cout << out_tr.aligned_seqA[i];
    cout << endl;
    cout << "2nd_aligned_DNA: ";
    for (int i = 0; i < out_tr.aligned_seqB.size(); i++) 
        cout << out_tr.aligned_seqB[i];
    cout << endl;
    cout << "###############################################";
    return 0;
}
