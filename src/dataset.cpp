/*
 * Copyright (C) 2007 by
 * 
 * 	Xuan-Hieu Phan
 *	hieuxuan@ecei.tohoku.ac.jp or pxhieu@gmail.com
 * 	Graduate School of Information Sciences
 * 	Tohoku University
 *
 * GibbsLDA++ is a free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * GibbsLDA++ is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GibbsLDA++; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include "constants.h"
#include "strtokenizer.h"
#include "dataset.h"

using namespace std;

int dataset::write_wordmap(string wordmapfile, mapword2id * pword2id) {
    FILE * fout = fopen(wordmapfile.c_str(), "w");
    if (!fout) {
	printf("Cannot open file %s to write!\n", wordmapfile.c_str());
	return 1;
    }    
    
    mapword2id::iterator it;
    fprintf(fout, "%d\n", pword2id->size());
    for (it = pword2id->begin(); it != pword2id->end(); it++) {
	fprintf(fout, "%s %d\n", (it->first).c_str(), it->second);
    }
    
    fclose(fout);
    
    return 0;
}

int dataset::read_wordmap(string wordmapfile, mapword2id * pword2id) {
    pword2id->clear();
    
    FILE * fin = fopen(wordmapfile.c_str(), "r");
    if (!fin) {
	printf("Cannot open file %s to read!\n", wordmapfile.c_str());
	return 1;
    }    
    
    char buff[BUFF_SIZE_SHORT];
    string line;
    
    fgets(buff, BUFF_SIZE_SHORT - 1, fin);
    int nwords = atoi(buff);
    
    for (int i = 0; i < nwords; i++) {
	fgets(buff, BUFF_SIZE_SHORT - 1, fin);
	line = buff;
	
	strtokenizer strtok(line, " \t\r\n");
	if (strtok.count_tokens() != 2) {
	    continue;
	}
	
	pword2id->insert(pair<string, int>(strtok.token(0), atoi(strtok.token(1).c_str())));
    }
    
    fclose(fin);
    
    return 0;
}

int dataset::read_wordmap(string wordmapfile, mapid2word * pid2word) {
    pid2word->clear();
    
    FILE * fin = fopen(wordmapfile.c_str(), "r");
    if (!fin) {
	printf("Cannot open file %s to read!\n", wordmapfile.c_str());
	return 1;
    }    
    
    char buff[BUFF_SIZE_SHORT];
    string line;
    
    fgets(buff, BUFF_SIZE_SHORT - 1, fin);
    int nwords = atoi(buff);
    
    for (int i = 0; i < nwords; i++) {
	fgets(buff, BUFF_SIZE_SHORT - 1, fin);
	line = buff;
	
	strtokenizer strtok(line, " \t\r\n");
	if (strtok.count_tokens() != 2) {
	    continue;
	}
	
	pid2word->insert(pair<int, string>(atoi(strtok.token(1).c_str()), strtok.token(0)));
    }
    
    fclose(fin);
    
    return 0;
}

int dataset::read_trndata(string dfile, string wordmapfile) {
    mapword2id word2id;
//    read_wordmap(dfile, &word2id); 

    FILE * fin = fopen(dfile.c_str(), "r");
    if (!fin) {
	printf("Cannot open file %s to read!\n", dfile.c_str());
	return 1;
    }   
    
    mapword2id::iterator it;    
    char buff[BUFF_SIZE_LONG];
    string line;
    
    // get the number of documents
    fgets(buff, BUFF_SIZE_LONG - 1, fin);
    M = atoi(buff);
    if (M <= 0) {
	printf("No document available!\n");
	return 1;
    }
    
    // allocate memory for corpus
    if (docs) {
	deallocate();
    } else {
	docs = new document*[M];
    }
    
    // set number of words to zero
    V = 0;
    
    for (int i = 0; i < M; i++) {
	fgets(buff, BUFF_SIZE_LONG - 1, fin);
	line = buff;
	strtokenizer strtok(line, " \t\r\n");
	int length = strtok.count_tokens();

	if (length <= 0) {
	    printf("Invalid (empty) document!\n");
	    deallocate();
	    M = V = 0;
	    return 1;
	}
	
	// allocate new document
	document * pdoc = new document(length-1);
	
	pdoc->language = strtok.token(0);
	for (int j = 1; j < length; j++) {
	    strtokenizer termtok(strtok.token(j), ":");
	    it = word2id.find(termtok.token(0));
	    if (it == word2id.end()) {
		// word not found, i.e., new word
		pdoc->words[j-1] = word2id.size();
		word2id.insert(pair<string, int>(termtok.token(0), word2id.size()));
	    } else {
		pdoc->words[j-1] = it->second;
	    }
	    if (pdoc->language == "codeS") {
		    if (termtok.token(1) == "eng")
			    pdoc->tags[j-1] = 0;
		    else
			    pdoc->tags[j-1] = 1;
	    }
	    else if (pdoc->language == "eng") {
	    	pdoc->tags[j-1] = 0;
	    }
	    else {
	    	pdoc->tags[j-1] = 1;
	    }
	}
	
	// add new doc to the corpus
	add_doc(pdoc, i);
    }
    
    fclose(fin);
    
    // write word map to file
    if (write_wordmap(wordmapfile, &word2id)) {
	return 1;
    }
    
    // update number of words
    V = word2id.size();
    
    return 0;
}

int dataset::read_newdata(string dfile, string wordmapfile) {
    mapword2id word2id;
    map<int, int> id2_id;
    //printf("in read new data!!!!\n"); 
    read_wordmap(wordmapfile, &word2id);
    if (word2id.size() <= 0) {
	printf("No word map available!\n");
	return 1;
    }
    //printf("word map size = %d\n", word2id.size());

    FILE * fin = fopen(dfile.c_str(), "r");
    if (!fin) {
	printf("Cannot open file %s to read!\n", dfile.c_str());
	return 1;
    }   

    mapword2id::iterator it;
    map<int, int>::iterator _it;
    char buff[BUFF_SIZE_LONG];
    string line;
    
    // get number of new documents
    fgets(buff, BUFF_SIZE_LONG - 1, fin);
    M = atoi(buff);
    if (M <= 0) {
	printf("No document available!\n");
	return 1;
    }
    
    // allocate memory for corpus
    if (docs) {
	deallocate();
    } else {
	docs = new document*[M];
    }
    _docs = new document*[M];
    
    // set number of words to zero
    V = 0;
    
    for (int i = 0; i < M; i++) {
	fgets(buff, BUFF_SIZE_LONG - 1, fin);
	line = buff;
	strtokenizer strtok(line, " \t\r\n");
	int length = strtok.count_tokens();

	if (length <= 0) {
	    printf("Invalid (empty) document!\n");
	    deallocate();
	    M = V = 0;
	    return 1;
	}
	
	vector<int> doc;
	vector<int> _doc;
	for (int j = 1; j < length; j++) {
	    strtokenizer termtok(strtok.token(j), ":");
	    it = word2id.find(termtok.token(0));
	    if (it == word2id.end()) {
		//printf("the word not in the dictionary!!\n");	
		// word not found, i.e., word unseen in training data
		// do anything? (future decision)
	    } else {
		int _id;
		_it = id2_id.find(it->second);
		if (_it == id2_id.end()) {
		    _id = id2_id.size();
		    id2_id.insert(pair<int, int>(it->second, _id));
		    _id2id.insert(pair<int, int>(_id, it->second));
		} else {
		    _id = _it->second;
		}
	//	printf("word = %d\t", it->second);	
		doc.push_back(it->second);
		_doc.push_back(_id);
	    }
	    //printf("\n");

	}
	
	// allocate memory for new doc
	document * pdoc = new document(doc);
	document * _pdoc = new document(_doc);
	
	pdoc->language = strtok.token(0);
	_pdoc->language = strtok.token(0);
	
	// add new doc
	add_doc(pdoc, i);
	_add_doc(_pdoc, i);
    }
    
    fclose(fin);
    
    // update number of new words
    V = word2id.size();//id2_id.size();
    
    return 0;
}

int dataset::read_newdata_withrawstrs(string dfile, string wordmapfile) {
    mapword2id word2id;
    map<int, int> id2_id;
    
    read_wordmap(wordmapfile, &word2id);
    if (word2id.size() <= 0) {
	printf("No word map available!\n");
	return 1;
    }

    FILE * fin = fopen(dfile.c_str(), "r");
    if (!fin) {
	printf("Cannot open file %s to read!\n", dfile.c_str());
	return 1;
    }   

    mapword2id::iterator it;
    map<int, int>::iterator _it;
    char buff[BUFF_SIZE_LONG];
    string line;
    
    // get number of new documents
    fgets(buff, BUFF_SIZE_LONG - 1, fin);
    M = atoi(buff);
    if (M <= 0) {
	printf("No document available!\n");
	return 1;
    }
    
    // allocate memory for corpus
    if (docs) {
	deallocate();
    } else {
	docs = new document*[M];
    }
    _docs = new document*[M];
    
    // set number of words to zero
    V = 0;
    
    for (int i = 0; i < M; i++) {
	fgets(buff, BUFF_SIZE_LONG - 1, fin);
	line = buff;
	strtokenizer strtok(line, " \t\r\n");
	int length = strtok.count_tokens();
	
	vector<int> doc;
	vector<int> _doc;
	for (int j = 0; j < length - 1; j++) {
	    it = word2id.find(strtok.token(j));
	    if (it == word2id.end()) {
		// word not found, i.e., word unseen in training data
		// do anything? (future decision)
	    } else {
		int _id;
		_it = id2_id.find(it->second);
		if (_it == id2_id.end()) {
		    _id = id2_id.size();
		    id2_id.insert(pair<int, int>(it->second, _id));
		    _id2id.insert(pair<int, int>(_id, it->second));
		} else {
		    _id = _it->second;
		}
		
		doc.push_back(it->second);
		_doc.push_back(_id);
	    }
	}
	
	// allocate memory for new doc
	document * pdoc = new document(doc, line);
	document * _pdoc = new document(_doc, line);
	
	// add new doc
	add_doc(pdoc, i);
	_add_doc(_pdoc, i);
    }
    
    fclose(fin);
    
    // update number of new words
    V = word2id.size();//id2_id.size();
    
    return 0;
}

