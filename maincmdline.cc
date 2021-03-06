/*
 *  Copyright (C) Gorgi Kosev a.k.a. Spion
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include <cctype>
#include <fcntl.h>
#include <iostream>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <unistd.h>
#include "ai/ai.hpp"
#include "ai/tokens.h"
#include "common.h"
#include "wildcard/wildcards.cpp"
using namespace std;

AI* tai;
unsigned long int defmodel = 2;
bool shouldtalk;

int main() {
    tai = new AI("botdata/triplie.db");
    setlocale(LC_ALL, "en_US.utf8");
    Wildcard wildmatch;
    string theline, matcher, aireply;
    vector<string> tokens;
    vector<string> tuplets;
    vector<string> tupletsubtokens;
    unsigned long int llen;
    matcher = "!*";
    theline = "";
    /* Displaying a banner with info... */
    cout << "Triple AI bot started" << endl
            << "Words@'word.dat' relations@'rels.dat'" << endl;

    //* this might need another seed in WIN32 *//
    srand(time(0));
    tai->readalldata();
    cout << tai->countwords() << " words, ";
    cout << tai->countrels() << " relations known." << endl;
    cout << "Waiting for input. Commands: !save, !quit" << endl << endl;

    shouldtalk = true;
    tai->connect_to_workers("workers.dat");
    while (shouldtalk) {
        getline(cin, theline);
        lowercase(theline);
        //for(x=0;x<theline.size();x++) { theline[x]=to//lower(theline[x]); }
        tokens.clear();
        tokenize(theline, tokens, " ,:");
        llen = tokens.size();
        if (llen > 0) {
            if (wildmatch.wildcardfit(matcher.c_str(), tokens[0].c_str())) {
                //commands
                if (tokens[0] == "!quit") {
                    shouldtalk = false;
                } else if (tokens[0] == "!save") {
                    tai->savealldata();
                    cout << "!saved words and relations" << endl << endl;
                } else if (llen > 1) {
                    if ((tokens[0] == "!ai") && (llen > 2)) {
                        if (tokens[1] == "order") {
                            defmodel = atol(tokens[2].c_str());
                            cout << "!ai model set to " << defmodel << endl << endl;
                        }
                        if (tokens[1] == "permute") {
                            int permutesize = atol(tokens[2].c_str());
                            tai->setpermute(permutesize);
                            cout << "!ai permuting " << permutesize << endl << endl;
                        }
                        if (tokens[1] == "permutations") {
                            unsigned pcount = atol(tokens[2].c_str());
                            if (!pcount) {
                                pcount = TRIP_AI_MAXPERMUTATIONS;
                            }
                            tai->maxpermute(pcount);
                            cout << "Maximum permutations now " << pcount << endl << endl;
                        }
                        if (tokens[1] == "random") {
                            if (tokens[2] == "on") {
                                tai->useRandom = true;
                                cout << "!Using random." << endl << endl;
                            } else {
                                tai->useRandom = false;
                                cout << "!Using all." << endl << endl;
                            }
                        }
                    }
                }
            }// end of commands
            else {
                //reply and learn
                //reply
                double clockstart = seconds();
                tai->setdatastring(subtokstring(tokens, 0, 100, " "));
                tai->extractkeywords();
                tai->expandkeywords();
                cout << "- " << tai->getdatastring() << endl;
                tai->connectkeywords(defmodel);
                aireply = tai->getdatastring("(console)", time(0));
                double tsec = (seconds() - clockstart);
                if (aireply == "") {
                    aireply = "*shrug*";
                }
                cout << "> "
                        << aireply
                        << " (" << tsec << ")" << endl;

                tai->setdatastring(theline);
                tai->learndatastring("(other)", "(console)", time(0));

            } // end of chat
        } // end of non-empty imput
    }
    cout << endl << "Bye bye." << endl;
    return 0;
}
