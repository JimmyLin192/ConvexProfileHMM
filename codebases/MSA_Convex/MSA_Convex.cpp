/*################################################################ MODULE: MSA_Convex.cpp
## VERSION: 1.0 
## SINCE 2015-09-01
## DESCRIPTION: 
##      
#################################################################
## Edited by MacVim
## Class Info auto-generated by Snippet 
################################################################*/

#include "MSA_Convex.h"

/* Debugging option */
//#define RECURSION_TRACE
#define CUBE_SMITH_WATERMAN_DEBUG
/* 
   The first sequence is observed. 
   The second sequence is the one to be aligned with the observed one.
   */
void usage () { cout << "./PSA_CUBE [seq_file]" << endl;
    cout << "seq_file should contain two DNA sequence in its first line and second line. " << endl;
    cout << "The first sequence is observed. " << endl;
    cout << "The second sequence is the one to be aligned with the observed one." << endl;
}

int get_init_model_length (vector<int>& lenSeqs) {
    int max_seq_length = -1;
    int numSeq = lenSeqs.size(); 
    for (int i = 0; i < numSeq; i ++)
        if (lenSeqs[i] > max_seq_length) 
            max_seq_length = lenSeqs[i];
    return max_seq_length;
}

double first_subproblem_log (int fw_iter, Tensor4D& W, Tensor4D& Z, Tensor4D& Y, Tensor4D& C, double& mu) {
    double cost = 0.0, lin_term = 0.0, qua_term = 0.0;
    double Ws = tensor4D_frob_prod (W, W); 
    int T1 = W.size();
    int T2 = W[0].size();
    for (int i = 0; i < T1; i ++) 
        for (int j = 0; j < T2; j ++) 
            for (int d = 0; d < NUM_DNA_TYPE; d ++) 
                for (int m = 0; m < NUM_MOVEMENT; m ++) {
                    double sterm = 0.5*mu * (W[i][j][d][m] - Z[i][j][d][m] + 1.0/mu*Y[i][j][d][m]);
                    lin_term += C[i][j][d][m] * W[i][j][d][m];
                    qua_term += sterm * sterm;
                }
    cost = lin_term + qua_term;
    cout << "iter=" << fw_iter << ", cost=" << cost ; 
    cout << ", ||W||^2: " << Ws << ", lin_term: " << lin_term <<  ", qua_sterm: " << qua_term<< endl;
}

/* We resolve the first subproblem through the frank-wolfe algorithm */
void first_subproblem (Tensor4D& W, Tensor4D& Z, Tensor4D& Y, Tensor4D& C, double& mu, Sequence data_seq) {
    /*{{{*/
    // 1. Find the update direction
    int T1 = W.size();
    int T2 = W[0].size();
    Tensor4D M (T1, Tensor(T2, Matrix(NUM_DNA_TYPE, vector<double>(NUM_MOVEMENT, 0.0)))); 
    int fw_iter = -1;
    first_subproblem_log(fw_iter, W, Z, Y, C, mu);
    while (fw_iter < MAX_FW_ITER) {
        fw_iter ++;
        for (int i = 0; i < T1; i ++) 
            for (int j = 0; j < T2; j ++) 
                for (int d = 0; d < NUM_DNA_TYPE; d ++) 
                    for (int m = 0; m < NUM_MOVEMENT; m ++)
                        M[i][j][d][m] = mu*(W[i][j][d][m] - Z[i][j][d][m]) + Y[i][j][d][m]; 
        Tensor4D S (T1, Tensor(T2, Matrix(NUM_DNA_TYPE, vector<double>(NUM_MOVEMENT, 0.0)))); 
        Trace trace (0, Cell(3));
        cube_smith_waterman (S, trace, M, C, data_seq);

        // 2. Exact Line search: determine the optimal step size \alpha
        // alpha = { [ -(2/mu)*C - W + Z + (1/mu)*Y ] dot S } / || S ||^2
        //           ---------------combo------------------
        double s_square = tensor4D_frob_prod (S, S);
        double combo = 0.0;
        for (int i = 0; i < T1; i ++) 
            for (int j = 0; j < T2; j ++) 
                for (int d = 0; d < NUM_DNA_TYPE; d ++) 
                    for (int m = 0; m < NUM_MOVEMENT; m ++)
                        combo += ((-2.0/mu)*C[i][j][d][m] - W[i][j][d][m] + Z[i][j][d][m] + (1/mu)*Y[i][j][d][m]) * S[i][j][d][m];
        double alpha;
        // FIXME: why first iteration we need to manually set alpha to 1.0
        if (fw_iter == 0) 
            alpha = 1;
        else
            alpha = combo / s_square;
        cout << "combo: " << combo << ", s_square: " << s_square << endl;

        // 3. update W
        for (int i = 0; i < T1; i ++) 
            for (int j = 0; j < T2; j ++) 
                for (int d = 0; d < NUM_DNA_TYPE; d ++) 
                    for (int m = 0; m < NUM_MOVEMENT; m ++)
                        W[i][j][d][m] += alpha * S[i][j][d][m];

        // 4. output iteration tracking info
        first_subproblem_log(fw_iter, W, Z, Y, C, mu);
        // 5. early stop condition
        if (alpha < 10e-6) {
            cout << "alpha=" << alpha << ", early stop!" << endl;
            break; 
        }
    }
    return; 
    /*}}}*/
}

/* We resolve the second subproblem through sky-plane projection */
void second_subproblem (vector<Tensor4D>& W, vector<Tensor4D>& Z, vector<Tensor4D>& Y, double& mu, SequenceSet& allSeqs, vector<int> lenSeqs) {
    int numSeq = allSeqs.size();
    int T2 = W[0][0].size();
    // 1. compute delta
    vector<Tensor4D> delta (numSeq, Tensor4D(0, Tensor(T2, Matrix(NUM_DNA_TYPE,
                        vector<double>(NUM_MOVEMENT, 0.0)))));  
    tensor5D_init (delta, allSeqs, lenSeqs, T2);
    Tensor tensor (T2, Matrix (NUM_DNA_TYPE, vector<double>(NUM_DNA_TYPE, 0.0)));
    cerr << "Compute delta..." << endl;
    for (int n = 0; n < numSeq; n ++) {
        int T1 = W[n].size();
        for (int i = 0; i < T1; i ++) { 
            for (int j = 0; j < T2; j ++) 
                for (int d = 0; d < NUM_DNA_TYPE; d ++) 
                    for (int m = 0; m < NUM_MOVEMENT; m ++) {
                        delta[n][i][j][d][m] = -1.0 * mu * (W[n][i][j][d][m] - Z[n][i][j][d][m] + (1.0/mu)*Y[n][i][j][d][m]);
                        if (m == DELETION_A or m == MATCH_A)
                            tensor[j][d][dna2T3idx('A')] += delta[n][i][j][d][m];
                        else if (m == DELETION_T or m == MATCH_T)
                            tensor[j][d][dna2T3idx('T')] += delta[n][i][j][d][m];
                        else if (m == DELETION_C or m == MATCH_C)
                            tensor[j][d][dna2T3idx('C')] += delta[n][i][j][d][m];
                        else if (m == DELETION_G or m == MATCH_G)
                            tensor[j][d][dna2T3idx('G')] += delta[n][i][j][d][m];
                    }
        }
    }
    // 2. determine the trace: run viterbi algorithm
    cerr << "Go to viterbi..." << endl;
    Trace trace (0, Cell(2)); // 1d: j, 2d: ATCG
    viterbi_algo (trace, tensor);

    // 3. recover values for W 
    cerr << "Recover Values of W.." << endl;
    for (int n = 0; n < numSeq; n ++) {
        int T1 = W[n].size();
        for (int i = 0; i < T1; i ++) { 
            // 3a. all elements clear to zero
            for (int j = 0; j < T2; j ++) 
                for (int d = 0; d < NUM_DNA_TYPE; d ++) 
                    for (int m = 0; m < NUM_MOVEMENT; m ++)
                        W[n][i][j][d][m] = 0.0;
            // 3b. set a number of selected elements to 1
            for (int t = 0; t < trace.size(); t++) {
                int sj = trace[t].location[0];
                int sd = trace[t].location[1];
                for (int m = 0; m < NUM_MOVEMENT; m ++)
                    // FIXME: delta_n_t1_t2 > 0 then set 1
                    if (delta[n][i][sj][sd][m] > 0.0) 
                        W[n][i][sj][sd][m] = 1.0;
            }
        }
    }   
    return;
}

vector<Tensor4D> CVX_ADMM_MSA (SequenceSet& allSeqs, vector<int>& lenSeqs) {
    /*{{{*/
    // 1. initialization
    int numSeq = allSeqs.size();
    int T2 = get_init_model_length (lenSeqs); // model_seq_length
    vector<Tensor4D> Z (numSeq, Tensor4D(0, Tensor(T2, Matrix(NUM_DNA_TYPE,
                        vector<double>(NUM_MOVEMENT, 0.0)))));  
    vector<Tensor4D> C (numSeq, Tensor4D(0, Tensor(T2, Matrix(NUM_DNA_TYPE,
                        vector<double>(NUM_MOVEMENT, 0.0)))));  
    vector<Tensor4D> W_1 (numSeq, Tensor4D(0, Tensor(T2, Matrix(NUM_DNA_TYPE,
                        vector<double>(NUM_MOVEMENT, 0.0)))));  
    vector<Tensor4D> W_2 (numSeq, Tensor4D(0, Tensor(T2, Matrix(NUM_DNA_TYPE,
                        vector<double>(NUM_MOVEMENT, 0.0)))));  
    vector<Tensor4D> Y_1 (numSeq, Tensor4D(0, Tensor(T2, Matrix(NUM_DNA_TYPE,
                        vector<double>(NUM_MOVEMENT, 0.0)))));  
    vector<Tensor4D> Y_2 (numSeq, Tensor4D(0, Tensor(T2, Matrix(NUM_DNA_TYPE,
                        vector<double>(NUM_MOVEMENT, 0.0)))));  
    tensor5D_init (Z, allSeqs, lenSeqs, T2);
    tensor5D_init (C, allSeqs, lenSeqs, T2);
    tensor5D_init (W_1, allSeqs, lenSeqs, T2);
    tensor5D_init (W_2, allSeqs, lenSeqs, T2);
    tensor5D_init (Y_1, allSeqs, lenSeqs, T2);
    tensor5D_init (Y_2, allSeqs, lenSeqs, T2);
    set_C (C, allSeqs);

    // 2. ADMM iteration
    int iter = 0;
    double mu = 1.0;
    while (iter < MAX_ADMM_ITER) {
        // 2a. Subprogram: FrankWolf Algorithm
        // NOTE: parallelize this for to enable parallelism
        cerr << "first subproblem" << endl;
        for (int n = 0; n < numSeq; n++) {
            cout << "----------------------n=" << n <<"-----------------------------------------" << endl;
            first_subproblem (W_1[n], Z[n], Y_1[n], C[n], mu, allSeqs[n]);
        }
        cout << "=============================================================================";
        cerr << "second subproblem" << endl;
        // 2b. Subprogram: 
        second_subproblem (W_2, Z, Y_2, mu, allSeqs, lenSeqs);
        cout << "=============================================================================";
        // 2c. update Z: Z = (W_1 + W_2) / 2
        // NOTE: parallelize this for to enable parallelism
        for (int n = 0; n < numSeq; n ++) 
            tensor4D_average (Z[n], W_1[n], W_2[n]);
        // 2d. update Y_1 and Y_2: Y_1 += 1/mu * (W_1 - Z)
        // NOTE: parallelize this for to enable parallelism
        for (int n = 0; n < numSeq; n ++)
            tensor4D_lin_update (Y_1[n], W_1[n], Z[n], (1.0/mu));
        for (int n = 0; n < numSeq; n ++)
            tensor4D_lin_update (Y_2[n], W_2[n], Z[n], 1.0/mu);
    }
    return Z;
    /*}}}*/
}

int main (int argn, char** argv) {
    // 1. usage
    if (argn < 2) {
        usage();
        exit(1);
    }

    // 2. input DNA sequence file
    SequenceSet allSeqs (0, Sequence());
    ifstream seq_file(argv[1]);
    string tmp_str;
    int numSeq = 0;
    while (getline(seq_file, tmp_str)) {
        Sequence tmp_seq (tmp_str.begin(), tmp_str.end());
        allSeqs.push_back(tmp_seq);
        ++ numSeq;
    }
    seq_file.close();
    cout << "#########################################################" << endl;
    cout << "ScoreMatch: " << C_M;
    cout << ", ScoreInsertion: " << C_I;
    cout << ", ScoreDeletion: " << C_D;
    cout << ", ScoreMismatch: " << C_MM << endl;
    for (int n = 0; n < numSeq; n ++) {
        char buffer [50];
        sprintf (buffer, "Seq%5d", n);
        cout << buffer << ": ";
        for (int j = 0; j < allSeqs[n].size(); j ++) 
            cout << allSeqs[n][j];
        cout << endl;
    }
    vector<int> lenSeqs (numSeq, 0);
    for (int n = 0; n < numSeq; n ++) 
        lenSeqs[n] = allSeqs[n].size();

    // 3. relaxed convex program: ADMM-based algorithm
    vector<Tensor4D> W = CVX_ADMM_MSA (allSeqs, lenSeqs);

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
    */
    cout << "#########################################################" << endl;
    return 0;
}
