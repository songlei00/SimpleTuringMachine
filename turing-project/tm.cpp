#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <assert.h>
#include "tm.h"
using namespace std;

void Turing::build_tm(const string &filename) {
    // Read the tm file, parse the file and build corresponding tm
    ifstream in(filename);
    string s;

    if(!in.is_open()) {
        fprintf(stderr, "Open file failed\n");
        exit(-1);
    }

    while(getline(in, s)) {
        // delete the comment and empty string
        int comment_pos = s.find(";");
        if(comment_pos != -1) 
            s.erase(comment_pos);

        int is_all_blank = 1;
        for(int i = 0; i < s.size(); i++) 
            if(s[i] != ' ') {
                is_all_blank = 0;
                break;
            }
        if(is_all_blank) // string like: "", "    " and so on
            continue;

        // add tm information
        if(s[0] == '#') {
            char line_type = s[1];

            int parenthesis_pos = s.find("{");
            if(parenthesis_pos != -1)
                s[parenthesis_pos] = ' ';
            parenthesis_pos = s.find("}");
            if(parenthesis_pos != -1)
                s[parenthesis_pos] = ' ';

            int start_pos = s.find(" = ");
            s = s.substr(start_pos+3);
            
            replace_all(s, ",", " ");
            
            insert_state_tm(line_type, s);
        } else {
            insert_delta(s);
        }
    }

    // check after finish building the tm
    for(map<key, value>::iterator iter=delta.begin(); iter != delta.end(); iter++) {
        key k = iter->first;
        value v = iter->second;
        string error_message;
        int is_illegal = is_delta_illegal(k, v, error_message);
        if(is_illegal) {
            print_delta_illegal_info(k, v, error_message);
            
            fprintf(stderr, "syntax error\n");
            exit(-1);
        }
    }
}

void Turing::insert_state_tm(const char type, const string &state_str) {
    istringstream istr(state_str);
    string tmp_str;
    set<string> *pt = NULL;

    switch(type) {
        case 'Q': pt = &Q; break;
        case 'S': pt = &S; break;
        case 'G': pt = &G; break;
        case 'q': istr >> q0; break;
        case 'B': istr >> B; break;
        case 'F': pt = &F; break;
        case 'N': istr >> n_tape; break;
        default:  exit(-1);
    }
    
    if(pt != NULL)
        while(istr >> tmp_str)
            pt->insert(tmp_str);
}

int Turing::is_delta_illegal(const key &k, const value &v, string &error_message) const {
    /**
     * 1. check whether the state in set Q
     * 2. check whether the number of tape state and direction is N 
     * 3. check whether the tape symbol is in set G
     */

    int is_illegal = 0;
     
    if(Q.find(k.state) == Q.end()) {
        error_message = "Invalid old state";
        is_illegal = 1;
    } else if (k.tape_state.size() != n_tape) {
        error_message = "The number of old tape state is incorrect";
        is_illegal = 1;
    } else if (v.new_tape_state.size() != n_tape) {
        error_message = "The number of new tape state is incorrect";
        is_illegal = 1;
    } else if (v.direction.size() != n_tape) {
        error_message = "The number of tape head direction is incorrect";
        is_illegal = 1;
    } else if (Q.find(v.next_state) == Q.end()) {
        error_message = "Invalid new state";
        is_illegal = 1;
    } else {
        for(int i = 0; i < k.tape_state.size(); i++) {
            if(G.find(string(1, k.tape_state[i])) == G.end()) {
                error_message = "Incorrect tape symbol in old tape state";
                is_illegal = 1;
                break;
            }
        }
        for(int i = 0; i < v.new_tape_state.size(); i++) {
            if(G.find(string(1, v.new_tape_state[i])) == G.end()) {
                error_message = "Incorrect tape symbol in new tape state";
                is_illegal = 1;
                break;
            }
        }
        for(int i = 0; i < v.direction.size(); i++) {
            if(v.direction[i] != 'l' && v.direction[i] != 'r' && v.direction[i] != '*') {
                error_message = "Incorrect direction symbol";
                is_illegal = 1;
                break;
            }
        }
    }

    return is_illegal;
}

void Turing::print_delta_illegal_info(const key &k, const value &v, string &error_message) const {
    if(mode == VERBOSE) {
        fprintf(stderr, "%s: %s %s %s %s %s\n", 
            error_message.c_str(), k.state.c_str(), k.tape_state.c_str(),
            v.new_tape_state.c_str(), v.direction.c_str(), v.next_state.c_str());
    }
}

void Turing::insert_delta(const string &delta_str) {
    // read the delta transition
    istringstream istr(delta_str);
    key k;
    value v;
    istr >> k.state >> k.tape_state >> v.new_tape_state >> v.direction >> v.next_state;
    // cout << k.state << " " << k.tape_state << " " << v.new_tape_state << " " << v.direction << " " << v.next_state << endl;

    // check non deterministic delta transition
    string error_message;
    if(delta.find(k) != delta.end()) {
        error_message = "Non deterministic transition";
        print_delta_illegal_info(k, v, error_message);
            
        fprintf(stderr, "syntax error\n");
        exit(-1);
    }
    
    delta[k] = v;
}

void Turing::replace_all(string &s, const string &old_value, const string &new_value) const {
    int pos;
    while((pos = s.find(old_value)) != -1)
        s = s.replace(pos, old_value.size(), new_value);
}

void Turing::print_set(const set<string> &s) const {
    set<string>::iterator i;
    for(i = s.begin(); i != s.end(); i++) 
        cout << *i << " ";
    cout << endl;
}

Turing::Turing(const string &filename, const tm_mode_t m) {
    mode = m;
    build_tm(filename);        
}

void Turing::init() {
    // init the 1st tape
    tape_t tmp_tape;
    if(input_string == "") {  // empty input
        tmp_tape.tape[0] = '_';
        tmp_tape.range.second = 1;
    } else {
        for(int i = 0; i < input_string.size(); i++)
            tmp_tape.tape[i] = input_string[i];
        tmp_tape.range.second = input_string.size();
    }
    tapes.push_back(tmp_tape);
    
    // init the other tapes
    for(int i = 1; i < n_tape; i++) {
        tape_t tmp_tape;
        tapes.push_back(tmp_tape);
    }

    // init the state
    cur_state = q0;
    cur_step = 0;
}

int Turing::step() {
    string tape_state;
    for(int i = 0; i < n_tape; i++)
        tape_state += string(1, get_tape_state(i));

    key k(cur_state, tape_state);
    if(delta.find(k) == delta.end()) {
        Log("No transition\n");
        return -1;
    } else {
        value v = delta[k];
        if(v.new_tape_state == tape_state && v.next_state == cur_state) {
            int inmove_flag = 1;
            for(int i = 0; i < v.direction.size(); i++) {
                if(v.direction[i] != '*') {
                    inmove_flag = 0;
                    break;
                }
            }
            if(inmove_flag) {
                Log("Dead loop\n");
                return -1;
            }
        }
        // cout << v.new_tape_state << " " << v.direction << " " << v.next_state << endl;
        for(int i = 0; i < n_tape; i++) {
            tapes[i].tape[tapes[i].head] = v.new_tape_state[i];

            if(v.direction[i] == 'l') {
                tapes[i].head --;
                // exceed the leftmost position
                if(tapes[i].head < tapes[i].range.first) {
                    tapes[i].range.first --;
                    tapes[i].tape[tapes[i].head] = '_';
                }
            } else if (v.direction[i] == 'r') {
                tapes[i].head ++;
                // exceed the rightmote position
                if(tapes[i].head >= tapes[i].range.second) {
                    tapes[i].range.second ++;
                    tapes[i].tape[tapes[i].head] = '_';
                }
            } else {
                ;  // * means no move
            }

            // there is a block in the leftmost position
            for(; 
                tapes[i].range.second - tapes[i].range.first > 1 && 
                tapes[i].range.first < tapes[i].head && 
                tapes[i].tape[tapes[i].range.first] == '_';
                tapes[i].range.first ++
            );
            // there is a block in the rightmost position
            for(;
                tapes[i].range.second - tapes[i].range.first > 1 &&
                tapes[i].range.second > tapes[i].head + 1 && 
                tapes[i].tape[tapes[i].range.second-1] == '_';
                tapes[i].range.second --
            );
        }
        cur_state = v.next_state;
        cur_step += 1;
        return 0;
    }
}

void Turing::simulate(const string &input_str) {
    input_string = input_str;

    // check input string
    int is_illegal = is_input_illegal(input_str);
    if(is_illegal != -1) {
        print_input_illegal_info(is_illegal);
        exit(-1);
    }
    if(is_illegal == -1 && mode == VERBOSE) {
        cout << "Input: " << input_string << endl;;
        cout << "==================== RUN ====================" << endl;
    }
    
    // init the tm tapes
    init();
    
    // simulate
    do {
        if(mode == VERBOSE) 
            print_cur_state();
    } while (step() != -1);

    // print result
    string result = "";
    for(int i = tapes[0].range.first; i < tapes[0].range.second; i++)
        result += string(1, tapes[0].tape[i]);
    if(mode == VERBOSE)
        cout << "Result: " << result << endl <<
             "==================== END ====================" << endl;
    else
        cout << result << endl;
}

void Turing::info() const {
    cout << "------------ TM -------------" << endl;
    cout << "N tape: " << n_tape << endl;

    cout << "Input set Q: ";
    print_set(Q);

    cout << "Input set S: ";
    print_set(S);

    cout << "Tape set G: ";
    print_set(G);

    cout << "Start state q0: " << q0 << endl;
    cout << "Block B: " << B << endl;

    cout << "Final state set F: ";
    print_set(F);
    cout << "-----------------------------" << endl;
}

void Turing::print_cur_state() {
    cout << "Step   : " << cur_step << endl;
    for(int i = 0; i < tapes.size(); i++) {
        int *aligned_offset = new int[tapes[i].range.second - tapes[i].range.first + 1]; 
        aligned_offset[0] = 0;

        cout << "Index" << i << " : ";
        for(int j = tapes[i].range.first; j < tapes[i].range.second; j++) {
            ostringstream ostr;
            ostr << j;
            aligned_offset[j - tapes[i].range.first + 1] = ostr.str().size() + 1;

            cout << j << " ";
        }
            
        cout << endl << "Tape" << i << "  : ";
        for(int j = tapes[i].range.first; j < tapes[i].range.second; j++) {
            for(int k = 0; k < aligned_offset[j - tapes[i].range.first]-2; k++) // print blank to left aligned
                cout << " ";
            cout << tapes[i].tape[j] << " ";
        }
        
        cout << endl << "Head" << i << "  : ";
        for(int j = tapes[i].range.first; j < tapes[i].head+1; j++) {
            for(int k = 0; k < aligned_offset[j - tapes[i].range.first]-2; k++) // print blank to left aligned
                cout << " ";
        }
        for(int j = 0; j < tapes[i].head - tapes[i].range.first; j++) {
            cout << "  ";
        }
            
        cout << "^" << endl;

        delete []aligned_offset;
    }
    cout << "State  : " << cur_state << endl;
    cout << "---------------------------------------------" << endl;
}

int Turing::is_input_illegal(const string &input_str) const {
    int illegal_idx = -1;
    for(int i = 0; i < input_str.size(); i++) {
        string tmp(1, input_str[i]);
        if(S.find(tmp) == S.end()) {
            illegal_idx = i;
            break;
        }
    }
    return illegal_idx;
}

void Turing::print_input_illegal_info(const int illegal_idx) const {
    if(mode == NORMAL) {
        fprintf(stderr, "illegal input\n");
    } else if (mode == VERBOSE) {
        fprintf(stderr, "Input: %s\n", input_string.c_str());
        fprintf(stderr, "==================== ERR ====================\n");
        fprintf(stderr, "error: '%c' was not declared in the set of input symbols\n", input_string[illegal_idx]);
        fprintf(stderr, "Input: %s\n", input_string.c_str());
        for(int i = 0; i < string("Input: ").size() + illegal_idx; i++)
            fprintf(stderr, " ");
        fprintf(stderr, "^\n");
        fprintf(stderr, "==================== END ====================\n");
    } else {
        assert(0);
    }
}
