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

#include <fstream>
#include <algorithm>

#include "ai.hpp"

#include <limits>
#include <numeric>

#define MINIMUM_INFO_BITS 10.0
#define TRIP_DISTRUBUTED

using std::numeric_limits;

/* Initializations */

AI::AI(string dbf): db(dbf) {
	db.Query("PRAGMA cache_size = 50000; PRAGMA temp_store = MEMORY;");
	db.Query("PRAGMA read_uncommited = True;");
	markov.CMarkovInit(&db);
	vertical.CGraphInit(&db, "assoc");
	dictionary.CDictionaryInit(&db);
	useRandom = true;
	Distributed = false;
	keywords.clear();
	relcount=0;
	markov.setOrder(6);
	setpermute(TRIP_MAXKEY_DEFAULT);
	vertCount=0;
	maxpermutecount = TRIP_AI_MAXPERMUTATIONS;
}

/* Not implemented */
void AI::prune_vertical_nonkeywords()
{

}


void AI::learnonlymarkov()
{
	
	for (unsigned k = 0; k < keywords.size(); ++k)
	{
		dictionary.AddWord(keywords[k]); //non-text learn possible.
	}
	markov.BeginTransaction();
	markov.remember(keywords);
	markov.EndTransaction();
		
}

void AI::connect_to_workers(string file)
{
	Distributed = false;
	slaves.clear();
	
	vector<string> hosts;
	vector<int> ports;
	
	ifstream f(file.c_str());
	while (f && !f.eof())
	{
		string host; int port;
		f >> host >> port;
		if (host.size() > 1) {
			Distributed = true;
			hosts.push_back(host);
			ports.push_back(port);
		}
	}
	slaves.resize(hosts.size());
	for (unsigned k = 0; k < slaves.size(); ++k)
	{
		//cout << k << " connect: " << hosts[k] << ":" << ports[k] << endl;
		while (!(slaves[k].Connect(hosts[k], ports[k]))) { sleep(1); }
		//cout << "done." << endl;
	}
}

vector<vector<unsigned> > AI::GetRepliesFromAll()
{
	string request = TripMaster::PrepareReplyRequest(keywords);
	SendAllSlavesAndWait(request);
	vector<vector<unsigned> > replies;
	//cout << "Getting replies" << endl;
	for (unsigned i = 0; i < slaves.size(); ++i)
	{
		//cout << i << endl;
		string reply = slaves[i].ReplyData();
		if (TripMaster::VerifyAnswer(reply))
		{
			//cout << "RemoteReply: " << reply << endl;
			replies.push_back(TripMaster::ConvertAnswerToVector(reply));
		}
	}
	return replies;
}

void AI::SendLearnKeywords()
{
	string request = TripMaster::PrepareLearnRequest(keywords);
	SendAllSlavesAndWait(request);
}

void AI::SendAllSlavesAndWait(const string& request)
{
	for (unsigned i = 0; i < slaves.size(); ++i)
	{
		//cout << "Sendto:" << i << " " << request << endl;
		slaves[i].SendRequest(request);
	}
	bool complete = false;
	unsigned timeoutcounter = 0;
	while (!complete)
	{
		complete = true;
		for (unsigned i = 0; i < slaves.size(); ++i)
		{
			if (!slaves[i].RequestComplete())
			{
				complete = false;
			}
		}
		usleep(5000); //5ms
		if (++timeoutcounter > 5000) { break; } // break after 25 seconds.
	}
	//cout << "Complete." << endl;
}


//REFACTOR: will continue to reside in AI
void AI::readalldata(const string& datafolder) {
	relcount=0;
	dictionary.clear();
	relcount+=dictionary.readwords(datafolder + "/words.dat");
	vertCount+=vertical.ReadLinks(datafolder + "/relsv.dat");
}


//REFACTOR: will continue to reside in AI
void AI::savealldata(const string& datafolder) {

}


void AI::learndatastring(const string& bywho, const string& where, const time_t& when) {
	context[where].setVertical(&vertical, &dictionary);
	vector<unsigned> markov_words;
	if  (   (keywords.size() > 1) 
		 && (context[where].isNick(dictionary.GetWord(keywords[0]))) 
		)
	{
		markov_words.insert(markov_words.begin(), keywords.begin() + 1, keywords.end());
	}
	else
	{
		markov_words = keywords;
	}
	markov.BeginTransaction();
	markov.remember(markov_words);
	markov.EndTransaction();
	//cout << "Okay okay" << endl;
	// this might cause bugs.
	//cout << "->OK0.1" << endl;
	if (Distributed)
	{
		SendLearnKeywords();
	}
	if (where != "(null)")
	{
		extractkeywords(); 
		context[where].push(bywho, keywords, when);
	}
	//cout << "OkayXokay" << endl;


}

void AI::expandkeywords()
{
	map<unsigned, double> kcontext;
	map<unsigned, double> kcontextCount;
	map<unsigned, double>::iterator z;
	TNodeLinks::iterator reply_keywrd;
	vector<unsigned>::iterator keywrd;
	vector<CMContext> results;
	vector<CMContext>::iterator rit;
	map<unsigned, double> modifiers;
	
	//cout << "Levenstein expanding..." << endl;
	
	
	map<unsigned, map<unsigned, string> > expansionmap
		= dictionary.FindSimilarWords(keywords);
	for (map<unsigned, map<unsigned, string> >::iterator similars =
		expansionmap.begin(); similars != expansionmap.end(); ++similars)
	{
		for (map<unsigned,string>::iterator x = 
			 	similars->second.begin();
				 x != similars->second.end(); ++x)
		{
			modifiers[x->first] = similars->second.size();
			keywords.push_back(x->first);
		}
		modifiers[similars->first] = similars->second.size();
	}
	// End of levenstein expanding
	
	//sort(keywords.begin(), keywords.end());
	for (keywrd = keywords.begin(); keywrd != keywords.end(); ++keywrd)
	{
		TNodeLinks req_keywrd[2];
		req_keywrd[0] = vertical.GetFwdLinks(*keywrd, TRIP_MAXKEY*2);
		req_keywrd[1] = vertical.GetBckLinks(*keywrd, TRIP_MAXKEY*2);
		double req_keywrd_occurances = dictionary.occurances(*keywrd);
		for (unsigned mapInd = 0; mapInd <= 1; ++mapInd)
		{
		if (req_keywrd[mapInd].size() > 0) {
			for (reply_keywrd = req_keywrd[mapInd].begin(); 
				 reply_keywrd != req_keywrd[mapInd].end(); 
				 ++reply_keywrd)
			{
				double reply_keywrd_occurances 
						= dictionary.occurances(reply_keywrd->first);
				double candidate_score;
				if ((candidate_score = scorekeyword_bycount(reply_keywrd->first) > 0.0))
				{
					double co_occur = reply_keywrd->second;
					double minimumf = min(reply_keywrd_occurances,req_keywrd_occurances);
					if (minimumf < co_occur) { minimumf = co_occur; }
					
					/*
					double key_value = 2.0 * (candidate_score + 1.0)
											/ (candidate_score + 10.0);
					*/
					/*double assoc_value = 2.0 * (x + 0.3) / (x + 4.0);*/
					double divider_one = 2.0 * (2.0 + req_keywrd_occurances)
										/ (20.0 + req_keywrd_occurances);
					
					double divider_two = 2.0 * (modifiers[*keywrd] + 1.0) 
											/ (modifiers[*keywrd] + 3.0);
					double divider_tree = 
						2.0 * (2.0 + reply_keywrd_occurances )
										/ (20.0 + reply_keywrd_occurances);
				 	
					kcontext[reply_keywrd->first] += (co_occur / minimumf)
												/ divider_one 
												/ divider_two
												/ divider_tree;
					kcontextCount[reply_keywrd->first] += 1.0;
					
					/*
					log(1.0 + reply_keywrd->second) / req_keywrd_occurances
						/ req_keywrd.size(); // (1.0 + modifiers[*keywrd]);
						*/
				}
			}
		}
		} // for both maps
	}
	CMContext element;
	for (z = kcontext.begin(); z != kcontext.end(); ++z)
	{
			element.wrd = z->first;
			double cAssociators = kcontextCount[z->first];
			element.cnt = z->second * cAssociators;
			results.push_back(element);
	}
	sort(results.begin(), results.end());
	reverse(results.begin(), results.end());
	unsigned keycnt = 1;
	keywords.clear();
	for (rit = results.begin(); rit != results.end(); ++rit)
	{
		if (keycnt > TRIP_MAXKEY) break;
		keywords.push_back(rit->wrd);
		++keycnt;
	}
}

/* public */

const long int AI::countrels() {
	return markov.count();
}

void AI::setdatastring(const string& datastring) {
	vector<string> strkeywords;
	strkeywords.clear();
	string theline = datastring; unsigned long int x;
	for(x=1;x<theline.size();x++)
	{
		switch (theline[x])
		{
			case '.':
			case ',':
			case '!':
			case '?':
			case ':':
			case '(':
			case ')': theline.insert(x," "); ++x; theline.insert(x+1," "); ++x; 
					  break; 
			default : break;
		}
	}
	lowercase(theline);
	if (theline!="") {
		tokenize(theline,strkeywords," \t\n");
	}
	unsigned long int litmp = strkeywords.size(); unsigned long int i;
	for (i=0;i<litmp;i++) {
		dictionary.AddWord(strkeywords[i]);
	}
	keywords.clear();
	for (i=0; i<strkeywords.size(); i++) 
	{
		keywords.push_back(dictionary.GetKey(strkeywords[i]));
	}
	//cout << endl;
}

void AI::setnumericdatastring(const vector<string>& numeric)
{
	keywords.clear();
	for (unsigned i=0; i < numeric.size(); ++i)
	{
		keywords.push_back(convert<unsigned>(numeric[i]));
	}
}

const string AI::getdatastring(const string& where, const time_t& when) {
	string theline="";
	unsigned int i;
	if (keywords.size())
	{
		for (i=0;i<keywords.size();i++)
		{
			string kwrd = dictionary.GetWord(keywords[i]);
			string::size_type loc = kwrd.find_first_of(".,!?)",0); // -:
			if (loc != string::npos) //interpunction char.
				{ theline=theline+kwrd; }
			else  //normal word
				{ theline=theline+" "+kwrd; }
		}
	}
	if (where != "(null)")
	{
		extractkeywords(); //this might cause bugs
		context[where].my_dellayed_context = keywords;
		context[where].my_dellayed_context_time = when;
	}
	return theline;
}

const string AI::getnumericdatastring()
{
	string theline = "";
	for (unsigned i = 0; i < keywords.size(); ++i)
	{
		theline = theline + " " + convert<string>(keywords[i]);
	}
	return theline;
}

void AI::extractkeywords() {
	vector<unsigned> sorted;
	vector<unsigned>::iterator it;//,ix;l
	vector<unsigned>::iterator imax;
	sorted.clear();
	double infinity = numeric_limits<double>::infinity();

	for (it=keywords.begin(); it!=keywords.end(); it++)
	{
		double scorekeywordresult = scorekeyword(*it);
		if ((scorekeywordresult > 0.0) && (scorekeywordresult != infinity))
		{
			sorted.push_back(*it);
		}
	}
	keywords.clear();
	keywords.swap(sorted);
}

//REFACTOR: Move to CAIStatistics
const float AI::scorekeywords(const map<unsigned, bool>& keymap) {
	double curscore = 0.01;
	map<unsigned,bool> usedword;
	unsigned int it; unsigned twrd;
	for (it=0;it<keywords.size();it++)
	{
		twrd = keywords[it];
		if (twrd)
		{
			if (usedword.find(twrd) == usedword.end())
			{
				double keyscore = scorekeyword(twrd);
				curscore+= (keyscore > 0)?1:-1;
				// Reward the usage of more specified keywords
				if (keymap.find(twrd) != keymap.end()) { // is in keymap
					curscore += 2.0 * MINIMUM_INFO_BITS;
				}
				usedword[twrd] = true;
			}
			else
			{
				curscore -= MINIMUM_INFO_BITS; // repetition penalty
			}
		}
		//cout << (keywords[it] != 0 ? dictionary.GetWord(keywords[it]): "<0>");
		//cout << " ";
			
	}
	//cout << "(" << (curscore / (keymap.size() + 0.1)) <<  ")";
	//cout << endl;
	return curscore / (keymap.size() + 0.01);
}

void AI::scoreshuffles(int method)
{
	map<unsigned, bool> keyword_map;
	for (unsigned i = 0; i < keywords.size(); ++i)
	{
		keyword_map[keywords[i]] = true;
	}
	vector<vector<unsigned> >::iterator it;
	scores.clear();
	for (it=shuffles.begin();it!=shuffles.end();it++)
	{
		keywords.clear();
		keywords = *it;
		scores.push_back(scorekeywords(keyword_map));
	}
}

void AI::keywordsbestshuffle()
{
	list<float>::iterator scoreit = scores.begin();
	vector<vector<unsigned> >::iterator it = shuffles.begin();
	float bestscore = -10000.0; // needs 1000 words in reply to get below.
	while ( (it != shuffles.end()) && (scoreit!=scores.end()) )
	{
		if (*scoreit > bestscore)
		{
			bestscore=*scoreit;
			keywords.clear();
			keywords=*it;
		}
		++it;++scoreit;
	}
	
	//cout << "## answer score (" << bestscore << ")" << endl;
}


/* * * * * * * * * * * * * * *
 * AI Answering
 * * * * * * * * * * * * * * */

void AI::buildcleanup() {
	vector<string> strkeywords;
	for (unsigned i = 0; i < keywords.size(); ++i)
	{
		strkeywords.push_back(dictionary.GetWord(keywords[i]));
	}
	string bclean;
	unsigned int rpsz = strkeywords.size();
	unsigned int il;
	bool inced;
	il=0;bclean="";inced = false;
	while (il<rpsz) {
		inced = false;
		if (il+1 < rpsz) {
			if (
				(strkeywords[il] == strkeywords[il+1])
			   )
				{ il+=1; inced=true; }
			else if (il+3 < rpsz) {
				if ((strkeywords[il] == strkeywords[il+2]) && 
					(strkeywords[il+1] == strkeywords[il+3]) 
					) {
					il+=2;
					inced=true;
				}
				else if (il+5 < rpsz) {
					if (  (strkeywords[il] == strkeywords[il+3])
			   		&& (strkeywords[il+1] == strkeywords[il+4])
					&& (strkeywords[il+2] == strkeywords[il+5])
			   		) {
				   		il+=3;
						inced=true;
			   		}
				} // il+5=rpsz
			} // il+3=rpsz
		}
		if (!(inced)) {
			if (il<rpsz) { bclean=bclean+" "+strkeywords[il]; }
			il+=1;
		}
	}
	strkeywords.clear();
	tokenize(bclean,strkeywords," ");
	unsigned i = 0;
	while ((i < strkeywords.size()) && (strkeywords[i] == ","))
		++i;
	keywords.clear();
	for ( ; i < strkeywords.size(); ++i)
	{
		if ((i != (strkeywords.size() - 1))
			|| (strkeywords[i] != ","))
			keywords.push_back(dictionary.GetKey(strkeywords[i]));
	}
	
}


void AI::connectkeywords(int method, int nopermute)
{
	string answerstr = ""; string tmpstr = "";
	if (Distributed) {
		shuffles = GetRepliesFromAll();
	}
	else {
		if (useRandom)
		{
			shuffles = markov.dconnect(keywords, method, maxpermutecount);
		}
		else 
		{
			shuffles = markov.connect(keywords, method);
		}	
	}
	scoreshuffles(0);
	keywordsbestshuffle();
	replace(keywords.begin(),keywords.end(),
			(unsigned)0,dictionary.GetKey(","));
	buildcleanup();
}

//REFACTOR: move to CAIStatistics.
const float AI::scorekeyword(unsigned wrd)
{
	//cout << "0.0 - log(" << dictionary.occurances(wrd) << "/"
	//				   << dictionary.occurances() << ") / log(2) - 10.0" << endl;
	/*
	return 0.0 - log(1.0 * dictionary.occurances(wrd) 
					  / dictionary.occurances()) / log(2)
					  - MINIMUM_INFO_BITS; 
	*/
	return scorekeyword_bycount(dictionary.occurances(wrd));
	// at least MINIMUM_INFO_BITS bits needed to get positive value
}

const float AI::scorekeyword_bycount(unsigned wcnt)
{
	return 0.0 - log(1.0 * wcnt 
					  / dictionary.occurances()) / log(2)
					  - MINIMUM_INFO_BITS;
}

unsigned AI::countwords() { return dictionary.count(); }

const unsigned AI::countvrels() {
	return vertical.count;
}

