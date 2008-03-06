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

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include "wildcard/wildcards.cpp"
#include "ai/tokens.h"
#include "ai/ai.hpp"
#include <cctype>
#include <iostream>

using namespace std;

AI tai;

unsigned long int defmodel=2;

bool shouldtalk;


/* ---------------- */



/* ****************************************
    The code.
 * ************************************* */

int main(int argc, char** argv) {
	Wildcard wildmatch;
	string theline, matcher, aireply;
    vector<string> tokens;
	vector<string> tuplets;
    vector<string> tupletsubtokens;
    unsigned long int x;
	unsigned long int llen;
    matcher="!*";
    theline="";
	/* Displaying a banner with info... */
	cout << "Triple AI bot started" << endl
	     << "Words@'word.dat' relations@'rels.dat'" << endl;

	//* this might need another seed in WIN32 *//
	srand(time(0));
	tai.readalldata("botdata");
	cout << tai.countwords() << " words, ";
	cout << tai.countrels() << " relations known." << endl;
    cout << "Waiting for input. Commands: !save, !quit" << endl << endl;


	shouldtalk=true;
	tai.setpermute(1);
	while (shouldtalk) {
		getline(cin,theline);
        for(x=0;x<theline.size();x++) { theline[x]=tolower(theline[x]); }
		tokens.clear();
        tokenize(theline,tokens," ,:"); llen= tokens.size();
        if (llen>0) {
            if (wildmatch.wildcardfit(matcher.c_str(),tokens[0].c_str())) {
                //commands
                if (tokens[0] == "!quit") { shouldtalk = false; }
                else if (tokens[0] == "!save") {
                	tai.savealldata("botdata");
                    cout << "!saved words and relations" << endl << endl;
                }
				else if (llen>1) {
					if (tokens[0] == "!keys") {
						tai.setdatastring(subtokstring(tokens,1,100," "));
						tai.extractkeywords();
						cout << tai.getdatastring() << endl << endl;
					}
					if ((tokens[0] == "!conn") && (llen>2)) {
						tai.setdatastring(subtokstring(tokens,1,100," "));
						tai.extractkeywords();
						cout << "#KEYWORDS: " << tai.getdatastring() << endl;
						tai.connectkeywords(0);
						aireply=tai.getdatastring();
						cout << "!## " << aireply << "." << endl;
					}
					if ((tokens[0] == "!dijks") && (llen>2)) {
						tai.setdatastring(subtokstring(tokens,1,100," "));
						tai.extractkeywords();
						cout << "#KEYWORDS: " << tai.getdatastring() << endl;
						tai.connectkeywords(1);
						aireply=tai.getdatastring();
						cout << "!## " << tokens[1] << " "
						     << aireply << "." << endl;
					}
					if ((tokens[0] == "!sconn") && (llen>2)) {
						tai.setdatastring(subtokstring(tokens,1,100," "));
						tai.extractkeywords();
						cout << "#KEYWORDS: " << tai.getdatastring() << endl;
						tai.connectkeywords(2);
						aireply=tai.getdatastring();
						cout << "!## " << aireply << "." << endl;
						//tai.cleankeywords();
						cout << "! " << tai.getdatastring() << "." << endl << endl;
					}
					if ((tokens[0] == "!ai") && (llen>2)) {
						if (tokens[1] == "model") {
							defmodel = atol(tokens[2].c_str());
							cout << "!ai model set to " << defmodel << endl << endl;
						}
						if (tokens[1] == "permute") {
							int defmodel = atol(tokens[2].c_str());
							tai.setpermute(defmodel);
							cout << "!ai model set to " << defmodel << endl << endl;
						}
						if (tokens[1] == "follow") {
							tai.setdatastring(subtokstring(tokens,2,100," "));
							tai.extractkeywords();
							cout << "!You talk about: " << tai.getdatastring() << endl << endl;
						}
						if (tokens[1] == "expand") {
							tai.setdatastring(subtokstring(tokens,2,100," "));
							tai.extractkeywords();
							cout << "!You talk about: " << tai.getdatastring();					
							tai.expandkeywords();
							cout << "!Response about: " << tai.getdatastring() << endl << endl;
						}
					}
				}
            } // end of commands
            else {
                //reply and learn
                //reply
				tai.setdatastring(subtokstring(tokens,0,100," "));
				tai.extractkeywords();
				cout << "#Keys  : " << tai.getdatastring() << endl;
				tai.expandkeywords();
				cout << "#Expand: " << tai.getdatastring() << endl;
				tai.connectkeywords(defmodel); //dijkstra method = 1
				aireply=tai.getdatastring();
				if (aireply == "") { aireply = "*shrug*"; }
				cout << "> "
				     << aireply << endl;


                //learn
				tai.setdatastring(theline);
				tai.learndatastring("(other)");
				 // end of learn

            } // end of chat
        } // end of non-empty imput
	}
	tai.savealldata("botdata");
	cout << endl << "Bye bye." << endl;
    return 0;
}