/*################################################################ 
## MODULE: MSA_Convex.cpp
## VERSION: 1.0 
## SINCE 2015-09-01
##      
#################################################################
## Edited by MacVim
## Class Info auto-generated by Snippet 
################################################################*/

#include<iostream>
#include "MSA_Convex.h"

/* Debugging option */
// #define RECURSION_TRACE
// #define FIRST_SUBPROBLEM_DEBUG
// #define SECOND_SUBPROBLEM_DEBUG

void usage () { cout << "./MSA_Convex (options) [seq_file]" << endl;
    cout << "seq_file should contain one or more DNA sequence. " << endl;
    cout << "Options: " << endl;
    cout << "\t-l Set the maximum length of model sequence. (default 0)";
    cout << "\t-m Set step size (\\mu) for updating the ADMM coordinate variables. (default 0.1)";
    cout << "\t-p Set maximum pertubation of penalty to break ties. (default 0)";
    cout << "\t-s Set ADMM early stop toggle: early stop (on) if > 0. (default on)";
    cout << "\t-r Set whether reinitialize W_1 and W_2 at each ADMM iteration. (default off)";
}

void parse_cmd_line (int argn, char** argv) {
	int i;
    for(i = 1; i < argn; i++){
        if ( argv[i][0] != '-' ) break;
        if ( ++i >= argn ) usage();
        switch(argv[i-1][1]){
            case 'e': ADMM_EARLY_STOP_TOGGLE = (atoi(argv[i])>0); break;
            case 'r': REINIT_W_ZERO_TOGGLE = (atoi(argv[i])>0); break;
            case 'l': LENGTH_OFFSET = atoi(argv[i]); break;
            case 'm': MU = atof(argv[i]); break;
            case 'p': PERB_EPS = atof(argv[i]); break;
            default:
                      cerr << "unknown option: -" << argv[i-1][1] << endl;
                      exit(0);
        }
    }
    if (i >= argn) usage();
    trainFname = argv[i];
}

void parse_seqs_file (SequenceSet& allSeqs, int& numSeq, char* fname) {
    ifstream seq_file(fname);
    string tmp_str;
    while (getline(seq_file, tmp_str)) {
        int seq_len = tmp_str.size();
        Sequence ht_tmp_seq (seq_len+1+1, 0);
        ht_tmp_seq[0] = '*';
        for(int i = 0; i < seq_len; i ++) 
            ht_tmp_seq[i+1] = tmp_str.at(i);
        ht_tmp_seq[seq_len+1] = '#';
        allSeqs.push_back(ht_tmp_seq);
        ++ numSeq;
    }
    seq_file.close();
}

int get_init_model_length (vector<int>& lenSeqs) {
    int max_seq_length = -1;
    int numSeq = lenSeqs.size(); 
    for (int i = 0; i < numSeq; i ++)
        if (lenSeqs[i] > max_seq_length) 
            max_seq_length = lenSeqs[i];
    return max_seq_length;
}

double get_sub1_cost (Tensor5D& W, Tensor5D& Z, Tensor5D& Y, Tensor5D& C, double& mu, SequenceSet& allSeqs) {
    int numSeq = W.size();
    int T2 = W[0].size();
    double lin_term = 0.0, qua_term = 0.0;
    for (int n = 0; n < numSeq; n ++) {
        int T1 = W[n].size();
        for (int i = 0; i < T1; i ++) 
            for (int j = 0; j < T2; j ++) 
                for (int d = 0; d < NUM_DNA_TYPE; d ++) 
                    for (int m = 0; m < NUM_MOVEMENT; m ++) {
                        double sterm = W[n][i][j][d][m] - Z[n][i][j][d][m] + 1.0/mu*Y[n][i][j][d][m];
                        lin_term += C[n][i][j][d][m] * W[n][i][j][d][m];
                        qua_term += 0.5 * mu * sterm * sterm;
                    }
    }
    return lin_term + qua_term;
}

double get_sub2_cost (Tensor5D& W, Tensor5D& Z, Tensor5D& Y, double& mu, SequenceSet& allSeqs) {
    int numSeq = W.size();
    int T2 = W[0].size();
    double qua_term = 0.0;
    for (int n = 0; n < numSeq; n ++) {
        int T1 = W[n].size();
        for (int i = 0; i < T1; i ++) 
            for (int j = 0; j < T2; j ++) 
                for (int d = 0; d < NUM_DNA_TYPE; d ++) 
                    for (int m = 0; m < NUM_MOVEMENT; m ++) {
                        double sterm = W[n][i][j][d][m] - Z[n][i][j][d][m] + 1.0/mu*Y[n][i][j][d][m];
                        qua_term += sterm * sterm;
                    }
    }
    return qua_term;
}

double first_subproblem_log (int fw_iter, Tensor4D& W, Tensor4D& Z, Tensor4D& Y, Tensor4D& C, double mu) {
    double cost = 0.0, lin_term = 0.0, qua_term = 0.0;
    double Ws = tensor4D_frob_prod (W, W); 
    int T1 = W.size();
    int T2 = W[0].size();
    for (int i = 0; i < T1; i ++) 
        for (int j = 0; j < T2; j ++) 
            for (int d = 0; d < NUM_DNA_TYPE; d ++) 
                for (int m = 0; m < NUM_MOVEMENT; m ++) {
                    double sterm =  (W[i][j][d][m] - Z[i][j][d][m] + 1.0/mu*Y[i][j][d][m]);
                    lin_term += C[i][j][d][m] * W[i][j][d][m];
                    qua_term += 0.5*mu *sterm * sterm;
                }
    cost = lin_term + qua_term;
    cout << "[FW1] iter=" << fw_iter
         << ", ||W||^2: " << Ws 
         << ", lin_term: " << lin_term 
         << ", qua_sterm: " << qua_term
         << ", cost=" << cost 
         << endl;
}

double second_subproblem_log (int fw_iter, Tensor5D& W, Tensor5D& Z, Tensor5D& Y, double mu) {
    double cost = 0.0,  qua_term = 0.0;
    int numSeq = W.size();
    double Ws = 0.0;
    for (int n = 0; n < numSeq; n ++) 
        Ws += tensor4D_frob_prod (W[n], W[n]); 
    for (int n = 0; n < numSeq; n ++) {
        int T1 = W[n].size();
        int T2 = W[n][0].size();
        for (int i = 0; i < T1; i ++) 
            for (int j = 0; j < T2; j ++) 
                for (int d = 0; d < NUM_DNA_TYPE; d ++) 
                    for (int m = 0; m < NUM_MOVEMENT; m ++) {
                        double sterm =  (W[n][i][j][d][m] - Z[n][i][j][d][m] + 1.0/mu*Y[n][i][j][d][m]);
                        qua_term += 0.5*mu *sterm * sterm;
                    }
    }
    cost = qua_term;
    cout << "[FW2] iter=" << fw_iter 
         << ", ||W||^2: " << Ws  
         << ", qua_sterm: " << qua_term
         << ", cost=" << cost  
         << endl;
}

/* We resolve the first subproblem through the frank-wolfe algorithm */
void first_subproblem (Tensor4D& W_1, Tensor4D& W_2, Tensor4D& Y, Tensor4D& C, double& mu, Sequence data_seq) {
    /*{{{*/
    // 1. Find the update direction
    int T1 = W_1.size();
    int T2 = W_1[0].size();
    Tensor4D M (T1, Tensor(T2, Matrix(NUM_DNA_TYPE, vector<double>(NUM_MOVEMENT, 0.0)))); 
    // reinitialize to all-zero matrix
    for (int i = 0; i < T1 and REINIT_W_ZERO_TOGGLE; i ++) 
        for (int j = 0; j < T2; j ++) 
            for (int d = 0; d < NUM_DNA_TYPE; d ++) 
                for (int m = 0; m < NUM_MOVEMENT; m ++)
                    W_1[i][j][d][m] = 0.0;
                    
    int fw_iter = -1;
    // first_subproblem_log(fw_iter, W_1, W_2, Y, C, mu);
    while (fw_iter < MAX_1st_FW_ITER) {
        fw_iter ++;
#ifdef PARRALLEL_COMPUTING
//#pragma omp parallel for
#endif
        for (int i = 0; i < T1; i ++) 
            for (int j = 0; j < T2; j ++) 
                for (int d = 0; d < NUM_DNA_TYPE; d ++) 
                    for (int m = 0; m < NUM_MOVEMENT; m ++)
                        M[i][j][d][m] = mu*(W_1[i][j][d][m] - W_2[i][j][d][m]) + Y[i][j][d][m]; 
        Tensor4D S (T1, Tensor(T2, Matrix(NUM_DNA_TYPE, vector<double>(NUM_MOVEMENT, 0.0)))); 
        Trace trace (0, Cell(3));
        cube_smith_waterman (S, trace, M, C, data_seq);

        // 2. Exact Line search: determine the optimal step size \gamma
        // gamma = [ ( C + Y_1 + mu*W_1 - mu*Z ) dot (W_1 - S) ] / (mu* || W_1 - S ||^2)
        double numerator = 0.0, denominator = 0.0;
#ifdef PARRALLEL_COMPUTING
//#pragma omp parallel for
#endif
        for (int i = 0; i < T1; i ++) 
            for (int j = 0; j < T2; j ++) 
                for (int d = 0; d < NUM_DNA_TYPE; d ++) 
                    for (int m = 0; m < NUM_MOVEMENT; m ++) {
                        double wms = W_1[i][j][d][m] - S[i][j][d][m];
                        numerator += (C[i][j][d][m] + Y[i][j][d][m] + mu*W_1[i][j][d][m] - mu*W_2[i][j][d][m]) * wms;
                        denominator += mu * wms * wms;
                    }
        // 3a. early stop condition: neglible denominator
        if (denominator < 1e-6) break; // early stop
        double gamma = numerator / denominator;
        // initially pre-set to Conv(A)
        if (fw_iter == 0) gamma = 1.0;
        // Gamma should be bounded by [0,1]
        gamma = max(gamma, 0.0);
        gamma = min(gamma, 1.0);
        // 3b. early stop condition: neglible gamma
        if (fabs(gamma) < EPS_1st_FW) break;  // early stop condition
       // cout << "gamma: " << gamma << ", mu*||W-S||^2: " << denominator << endl;

        // 4. update W_1
        for (int i = 0; i < T1; i ++) 
            for (int j = 0; j < T2; j ++) 
                for (int d = 0; d < NUM_DNA_TYPE; d ++) 
                    for (int m = 0; m < NUM_MOVEMENT; m ++)
                        W_1[i][j][d][m] = (1-gamma) * W_1[i][j][d][m] + gamma* S[i][j][d][m];

        // 5. output iteration tracking info
        // first_subproblem_log(fw_iter, W_1, W_2, Y, C, mu);
    }
    return; 
    /*}}}*/
}

/* We resolve the second subproblem through sky-plane projection */
void second_subproblem (Tensor5D& W_1, Tensor5D& W_2, Tensor5D& Y, double& mu, SequenceSet& allSeqs, vector<int> lenSeqs) {
/*{{{*/
    int numSeq = allSeqs.size();
    int T2 = W_2[0][0].size();
    // reinitialize W_2 to all-zero matrix
    for (int n = 0; REINIT_W_ZERO_TOGGLE and n < numSeq; n ++) {
        int T1 = W_2[n].size();
        for (int i = 0; i < T1; i ++)  
            for (int j = 0; j < T2; j ++) 
                for (int d = 0; d < NUM_DNA_TYPE; d ++) 
                    for (int m = 0; m < NUM_MOVEMENT; m ++) 
                        W_2[n][i][j][d][m] = 0.0;
    }

    vector<Tensor4D> delta (numSeq, Tensor4D(0, Tensor(T2, Matrix(NUM_DNA_TYPE,
                        vector<double>(NUM_MOVEMENT, 0.0)))));  
    tensor5D_init (delta, allSeqs, lenSeqs, T2);
    Tensor tensor (T2, Matrix (NUM_DNA_TYPE, vector<double>(NUM_DNA_TYPE, 0.0)));
    Matrix mat_insertion (T2, vector<double>(NUM_DNA_TYPE, 0.0));

    int fw_iter = -1;
    while (fw_iter < MAX_2nd_FW_ITER) {
        fw_iter ++;
        // 1. compute delta
#ifdef PARRALLEL_COMPUTING
//#pragma omp parallel for
#endif
        for (int n = 0; n < numSeq; n ++) {
            int T1 = W_2[n].size();
            for (int i = 0; i < T1; i ++) { 
                for (int j = 0; j < T2; j ++) 
                    for (int d = 0; d < NUM_DNA_TYPE; d ++) 
                        for (int m = 0; m < NUM_MOVEMENT; m ++) {
                            delta[n][i][j][d][m] = -1.0* mu * (W_2[n][i][j][d][m] - W_1[n][i][j][d][m]) + Y[n][i][j][d][m];
#ifdef SECOND_SUBPROBLEM_DEBUG
                            if (delta[n][i][j][d][m] > 0)
                                cout <<"delta: " << n << "," << i << "," << j << "," << d  << "," << m << ": "
                                     << delta[n][i][j][d][m] << endl;
#endif
                            if (m == DELETION_A or m == MATCH_A)
                                tensor[j][d][dna2T3idx('A')] += max(0.0, delta[n][i][j][d][m]);
                            else if (m == DELETION_T or m == MATCH_T)
                                tensor[j][d][dna2T3idx('T')] += max(0.0, delta[n][i][j][d][m]);
                            else if (m == DELETION_C or m == MATCH_C)
                                tensor[j][d][dna2T3idx('C')] += max(0.0, delta[n][i][j][d][m]);
                            else if (m == DELETION_G or m == MATCH_G)
                                tensor[j][d][dna2T3idx('G')] += max(0.0, delta[n][i][j][d][m]);
                            else if (m == DELETION_START or m == MATCH_START)
                                tensor[j][d][dna2T3idx('*')] += max(0.0, delta[n][i][j][d][m]);
                            else if (m == DELETION_END or m == MATCH_END)
                                tensor[j][d][dna2T3idx('#')] += max(0.0, delta[n][i][j][d][m]);
                            else if (m == INSERTION) {
                                mat_insertion[j][d] += max(0.0, delta[n][i][j][d][m]);
                            }
                        }
            }
        }
#ifdef SECOND_SUBPROBLEM_DEBUG
        cout << "tensor transition input list:" << endl;
        for (int j = 0; j < T2; j ++) 
            for (int d = 0; d < NUM_DNA_TYPE; d ++) 
                for (int k = 0; k < NUM_DNA_TYPE; k ++) {
                    if (tensor[j][d][k] > 0)
                    cout << "(" << j << ", " << d << ", " << k << ")=" << tensor[j][d][k] << endl;
                }
#endif

        double delta_square = 0.0;
        for (int n = 0; n < numSeq; n ++) 
            delta_square += tensor4D_frob_prod (delta[n], delta[n]);
        //cout << "delta_square: " << delta_square << endl;
        if ( delta_square < 1e-12 ) {
            //cout << "small delta. early stop." << endl;
            break;
        }

        // 2. determine the trace: run viterbi algorithm
        Trace trace (0, Cell(2)); // 1d: j, 2d: ATCG
        refined_viterbi_algo (trace, tensor, mat_insertion);
        Tensor5D S (numSeq, Tensor4D(0, Tensor(T2, Matrix(NUM_DNA_TYPE, vector<double>(NUM_MOVEMENT, 0.0))))); 
        tensor5D_init (S, allSeqs, lenSeqs, T2);

        // 3. recover values for S 
        // 3b. set a number of selected elements to 1
        for (int t = 0; t < trace.size(); t++) {
            int sj = trace[t].location[0];
            int sd = trace[t].location[1];
            int sm = dna2T3idx(trace[t].acidB);
            for (int n = 0; n < numSeq; n ++) {
                int T1 = S[n].size();
                for (int i = 0; i < T1; i ++) {
                    for (int m = 0; m < NUM_MOVEMENT; m ++)
                        if (delta[n][i][sj][sd][m] > 0.0) { 
                            if (m == DEL_BASE_IDX + sm or m == MTH_BASE_IDX + sm)
                                S[n][i][sj][sd][m] = 1.0;
                            else if (m == INSERTION and trace[t].action == INSERTION) {
                                S[n][i][sj][sd][m] = 1.0;
                            }
                        }
                }
            }
        }

#ifdef SECOND_SUBPROBLEM_DEBUG
        cout << "Result of viterbi:" << endl;
        for (int t = 0; t < trace.size(); t++) 
            cout << "(" <<  trace[t].location[0] << ", " << trace[t].acidA << ", "<< trace[t].acidB << ")=" << trace[t].score << endl;
        double S_s = 0.0;
        for (int n = 0; n < numSeq; n ++) 
            S_s += tensor4D_frob_prod (S[n], S[n]);
        cout << "S_s: " << S_s << endl;
        for (int n = 0; n < numSeq; n ++) 
            tensor4D_dump(S[n]);
#endif

        // 4. Exact Line search: determine the optimal step size \gamma
        // gamma = [ ( Y_2 + mu*W - mu*Z ) dot (W_2 - S) ] / || W_2 - S ||^2
        //           ---------------combo------------------
        double numerator = 0.0, denominator = 0.0;
        for (int n = 0; n < numSeq; n ++) {
            int T1 = S[n].size();
            for (int i = 0; i < T1; i ++) 
                for (int j = 0; j < T2; j ++) 
                    for (int d = 0; d < NUM_DNA_TYPE; d ++) 
                        for (int m = 0; m < NUM_MOVEMENT; m ++) {
                            double wms = W_2[n][i][j][d][m] - S[n][i][j][d][m];
                            numerator += (-1.0*Y[n][i][j][d][m] + mu*W_2[n][i][j][d][m] - mu*W_1[n][i][j][d][m]) * wms;
                            denominator += mu * wms * wms;
                        }
        }
#ifdef SECOND_SUBPROBLEM_DEBUG
        cout << "numerator: " << numerator << ", denominator: " << denominator << endl;
#endif
        if ( denominator < 10e-6) {
            //cout << "small denominator: " << denominator << endl;
            break;
        }
        double gamma = numerator / denominator;
        // if (fw_iter == 0) gamma = 1.0;
        gamma = max(gamma, 0.0);
        gamma = min(gamma, 1.0);
        // cout << "gamma: " << gamma << ", mu*||W-S||^2: " << denominator << endl;

        // 3. update W
        for (int n = 0; n < numSeq; n ++) {
            int T1 = S[n].size();
            for (int i = 0; i < T1; i ++) 
                for (int j = 0; j < T2; j ++) 
                    for (int d = 0; d < NUM_DNA_TYPE; d ++) 
                        for (int m = 0; m < NUM_MOVEMENT; m ++)
                            W_2[n][i][j][d][m] = (1-gamma) * W_2[n][i][j][d][m] + gamma* S[n][i][j][d][m];
        }

        // 4. output iteration tracking info
        // second_subproblem_log(fw_iter, W, Z, Y, mu);
        // 5. early stop condition
        if (fabs(gamma) < EPS_2nd_FW) break; 
    }
    return;
/*}}}*/
}

Tensor5D CVX_ADMM_MSA (SequenceSet& allSeqs, vector<int>& lenSeqs, int T2) {
    /*{{{*/
    // 1. initialization
    int numSeq = allSeqs.size();
    vector<Tensor4D> C (numSeq, Tensor4D(0, Tensor(T2, Matrix(NUM_DNA_TYPE,
                        vector<double>(NUM_MOVEMENT, 0.0)))));  
    vector<Tensor4D> W_1 (numSeq, Tensor4D(0, Tensor(T2, Matrix(NUM_DNA_TYPE,
                        vector<double>(NUM_MOVEMENT, 0.0)))));  
    vector<Tensor4D> W_2 (numSeq, Tensor4D(0, Tensor(T2, Matrix(NUM_DNA_TYPE,
                        vector<double>(NUM_MOVEMENT, 0.0)))));  
    vector<Tensor4D> Y (numSeq, Tensor4D(0, Tensor(T2, Matrix(NUM_DNA_TYPE,
                        vector<double>(NUM_MOVEMENT, 0.0)))));  
    tensor5D_init (C, allSeqs, lenSeqs, T2);
    tensor5D_init (W_1, allSeqs, lenSeqs, T2);
    tensor5D_init (W_2, allSeqs, lenSeqs, T2);
    tensor5D_init (Y, allSeqs, lenSeqs, T2);
    set_C (C, allSeqs);

    // 2. ADMM iteration
    int iter = 0;
    double mu = MU;
    double prev_CoZ = MAX_DOUBLE;
    while (iter < MAX_ADMM_ITER) {
        // 2a. Subprogram: FrankWolf Algorithm
        // NOTE: parallelize this for to enable parallelism
#ifdef PARRALLEL_COMPUTING
#pragma omp parallel for
#endif
        for (int n = 0; n < numSeq; n++) 
            first_subproblem (W_1[n], W_2[n], Y[n], C[n], mu, allSeqs[n]);

        // 2b. Subprogram: 
        second_subproblem (W_1, W_2, Y, mu, allSeqs, lenSeqs);

        // 2d. update Y: Y += mu * (W_1 - W_2)
        for (int n = 0; n < numSeq; n ++)
            tensor4D_lin_update (Y[n], W_1[n], W_2[n], mu);

        // 2e. print out tracking info
        double CoZ = 0.0;
        for (int n = 0; n < numSeq; n++) 
            CoZ += tensor4D_frob_prod(C[n], W_2[n]);
        double W1mW2 = 0.0;
        for (int n = 0; n < numSeq; n ++) {
            int T1 = W_1[n].size();
            for (int i = 0; i < T1; i ++) 
                for (int j = 0; j < T2; j ++) 
                    for (int d = 0; d < NUM_DNA_TYPE; d ++) 
                        for (int m = 0; m < NUM_MOVEMENT; m ++) {
                            double value = (W_1[n][i][j][d][m] - W_2[n][i][j][d][m]);
                            W1mW2 = max( fabs(value), W1mW2 ) ;
                        }
        }
        // cerr << "=============================================================================" << endl;
        char COZ_val [50], w1mw2_val [50]; 
        sprintf(COZ_val, "%6f", CoZ);
        sprintf(w1mw2_val, "%6f", W1mW2);
        cerr << "ADMM_iter = " << iter 
            << ", C o Z = " << COZ_val
            << ", || W_1 - W_2 ||_{max} = " << w1mw2_val
            << endl;
        // cerr << "sub1_Obj = CoW_1+0.5*mu*||W_1-Z+1/mu*Y_1||^2 = " << sub1_cost << endl;
        // cerr << "sub2_Obj = ||W_2-Z+1/mu*Y_2||^2 = " << sub2_cost << endl;

        // 2f. stopping conditions
        if (ADMM_EARLY_STOP_TOGGLE and iter > MIN_ADMM_ITER)
            if ( W1mW2 < EPS_Wdiff ) {
                cerr << "CoZ Converges. ADMM early stop!" << endl;
                break;
            }
        prev_CoZ = CoZ;
        iter ++;
    }
    cout << "W_1: " << endl;
    for (int i = 0; i < numSeq; i ++) tensor4D_dump(W_1[i]);
    cout << "W_2: " << endl;
    for (int i = 0; i < numSeq; i ++) tensor4D_dump(W_2[i]);
    return W_2;
    /*}}}*/
}

void sequence_dump (SequenceSet& allSeqs, int n) {
    char buffer [50];
    sprintf (buffer, "Seq%5d", n);
    cout << buffer << ": ";
    for (int j = 0; j < allSeqs[n].size(); j ++) 
        cout << allSeqs[n][j];
    cout << endl;
}

int main (int argn, char** argv) {
    // 1. parse cmd 
    parse_cmd_line(argn, argv);

    // 2. input DNA sequence file
    int numSeq = 0;
    SequenceSet allSeqs (0, Sequence());
    parse_seqs_file(allSeqs, numSeq, trainFname);
    vector<int> lenSeqs (numSeq, 0);
    for (int n = 0; n < numSeq; n ++) 
        lenSeqs[n] = allSeqs[n].size();

    // pre-info
    cout << "#########################################################" << endl;
    cout << "ScoreMatch: " << C_M;
    cout << ", ScoreInsertion: " << C_I;
    cout << ", ScoreDeletion: " << C_D;
    cout << ", ScoreMismatch: " << C_MM << endl;
    for (int n = 0; n < numSeq; n ++) 
        sequence_dump(allSeqs, n);

    // 3. relaxed convex program: ADMM-based algorithm
    // omp_set_num_threads(NUM_THREADS);
    int T2 = get_init_model_length (lenSeqs) + LENGTH_OFFSET; // model_seq_length
    time_t begin = time(NULL);
    vector<Tensor4D> W = CVX_ADMM_MSA (allSeqs, lenSeqs, T2);
    time_t end = time(NULL);

    // 4. output the result
    // a. tuple view
    cout << ">>>>>>>>>>>>>>>>>>>>>>>TupleView<<<<<<<<<<<<<<<<<<<<<<<<" << endl;
    for (int n = 0; n < numSeq; n ++) {
        cout << "n = " << n << endl;
        tensor4D_dump(W[n]);
    }
    // b. sequence view
    cout << ">>>>>>>>>>>>>>>>>>>>>>>SequenceView<<<<<<<<<<<<<<<<<<<<<<<<" << endl;
    int T2m = T2;
    Tensor tensor (T2m, Matrix (NUM_DNA_TYPE, vector<double>(NUM_DNA_TYPE, 0.0)));
    Matrix mat_insertion (T2m, vector<double> (NUM_DNA_TYPE, 0.0));
    for (int n = 0; n < numSeq; n ++) {
        int T1 = W[n].size();
        for (int i = 0; i < T1; i ++) { 
            for (int j = 0; j < T2m; j ++) {
                for (int d = 0; d < NUM_DNA_TYPE; d ++) {
                    for (int m = 0; m < NUM_MOVEMENT; m ++) {
                        if (m == DELETION_A or m == MATCH_A)
                            tensor[j][d][dna2T3idx('A')] += max(0.0, W[n][i][j][d][m]);
                        else if (m == DELETION_T or m == MATCH_T)
                            tensor[j][d][dna2T3idx('T')] += max(0.0, W[n][i][j][d][m]);
                        else if (m == DELETION_C or m == MATCH_C)
                            tensor[j][d][dna2T3idx('C')] += max(0.0, W[n][i][j][d][m]);
                        else if (m == DELETION_G or m == MATCH_G)
                            tensor[j][d][dna2T3idx('G')] += max(0.0, W[n][i][j][d][m]);
                        else if (m == DELETION_START or m == MATCH_START)
                            tensor[j][d][dna2T3idx('*')] += max(0.0, W[n][i][j][d][m]);
                        else if (m == DELETION_END or m == MATCH_END)
                            tensor[j][d][dna2T3idx('#')] += max(0.0, W[n][i][j][d][m]);
                        else if (m == INSERTION) 
                            mat_insertion[j][d] += max(0.0, W[n][i][j][d][m]);
                    }
                }
            }
        }
    }
    Trace trace (0, Cell(2)); // 1d: j, 2d: ATCG
    refined_viterbi_algo (trace, tensor, mat_insertion);
    // for (int i = 0; i < trace.size(); i ++) 
    //    cout << trace[i].toString() << endl;
    for (int n = 0; n < numSeq; n ++) {
        char buffer [50];
        sprintf (buffer, "Seq%5d", n);
        cout << buffer << ": ";
        for (int j = 0; j < allSeqs[n].size(); j ++) 
            cout << allSeqs[n][j];
        cout << endl;
    }
    Sequence recSeq;
    cout << "SeqRecov: ";
    for (int i = 0; i < trace.size(); i ++) 
        if (trace[i].action != INSERTION) {
            cout << trace[i].acidB;
            recSeq.push_back(trace[i].acidB);
            if (trace[i].acidB == '#') break;
        }
    cout << endl;
    cout << ">>>>>>>>>>>>>>>>>>>>>>>MatchingView<<<<<<<<<<<<<<<<<<<<<<<<" << endl;
    SequenceSet allModelSeqs, allDataSeqs;
    for (int n = 0; n < numSeq; n ++) {
        Sequence model_seq;
        Sequence data_seq;
        int T1 = W[n].size();
        for (int i = 0; i < T1; i ++) { 
            for (int j = 0; j < T2m; j ++) {
                for (int d = 0; d < NUM_DNA_TYPE; d ++) {
                    for (int m = 0; m < NUM_MOVEMENT; m ++) {
                        if (W[n][i][j][d][m] > 0.95) {
                            if (m == INSERTION) {
                                data_seq.push_back(allSeqs[n][i]);
                                model_seq.push_back(GAP_NOTATION);
                            } else if (DEL_BASE_IDX <= m and m < MTH_BASE_IDX) { 
                                data_seq.push_back(GAP_NOTATION);
                                model_seq.push_back(T3idx2dna(m-DEL_BASE_IDX));
                            } else if (MTH_BASE_IDX <= m and m < NUM_MOVEMENT) {
                                data_seq.push_back(allSeqs[n][i]);
                                model_seq.push_back(T3idx2dna(m-MTH_BASE_IDX));
                            }
                        }
                    }
                }
            }
        }
        cout << "SeqRecov: ";
        for (int i = 0; i < model_seq.size(); i ++) 
            cout << model_seq[i];
        cout << endl;
        char buffer [50];
        sprintf (buffer, "Seq%5d", n);
        cout << buffer << ": ";
        for (int i = 0; i < data_seq.size(); i ++) 
            cout << data_seq[i];
        cout << endl;
        data_seq.erase(data_seq.begin());
        model_seq.erase(model_seq.begin());
        data_seq.erase(data_seq.end()-1);
        model_seq.erase(model_seq.end()-1);
        allModelSeqs.push_back(model_seq);
        allDataSeqs.push_back(data_seq);
    }
    cout << ">>>>>>>>>>>>>>>>>>>>>ClustalOmegaView<<<<<<<<<<<<<<<<<<<<<<" << endl;
    SequenceSet allCOSeqs (numSeq, Sequence(0));
    vector<int> pos(numSeq, 0);
    while (true) {
        set<int> insertion_ids;
        for (int i = 0; i < numSeq; i ++) {
            if (pos[i] >= allModelSeqs[i].size()) continue;
            if (allModelSeqs[i][pos[i]] == '-') 
                insertion_ids.insert(i);
        }
        if (insertion_ids.size() != 0) {
            // insertion exists
            for (int i = 0; i < numSeq; i ++) {
                if (insertion_ids.find(i)==insertion_ids.end()) // not in set
                    allCOSeqs[i].push_back('-');
                else { // in set
                    allCOSeqs[i].push_back(allDataSeqs[i][pos[i]++]);
                }
            }
        } else { // no insertion
            for (int i = 0; i < numSeq; i ++) 
                allCOSeqs[i].push_back(allDataSeqs[i][pos[i]++]);
        }
        // terminating
        bool terminated = true;
        for (int i = 0; i < numSeq; i ++) 
            if (pos[i] != allModelSeqs[i].size()) {
                terminated = false; 
                break;
            }
        if (terminated) break;
    }
    string fname (trainFname);
    fname = fname + ".co";
    ofstream co_out (fname.c_str());
    for (int i = 0; i < numSeq; i ++) {
        for (int j = 0; j < allCOSeqs[i].size(); j++)  {
            co_out << allCOSeqs[i][j];
            cout << allCOSeqs[i][j];
        }
        co_out << endl;
        cout << endl;
    }
    co_out.close();
    cout << "#########################################################" << endl;
    cout << "Time Spent: " << end - begin << " seconds" << endl;
    return 0;
}
