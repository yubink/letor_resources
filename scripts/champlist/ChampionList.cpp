#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <boost/math/distributions/gamma.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/unordered_map.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include "indri/QueryEnvironment.hpp"
#include "indri/Repository.hpp"
#include "indri/CompressedCollection.hpp"
#include "indri/ScopedLock.hpp"

using namespace indri::index;
using namespace indri::collection;
namespace fs = boost::filesystem;

using namespace std;

struct doc_scores {
  string docno;
  double indri;
  double bm25;
  doc_scores(string docno_, double indri_, double bm25_): docno(docno_), indri(indri_), bm25(bm25_) {};
};

double calcIndriFeature(double tf, double ctf, double totalTermCount, double docLength, int mu = 2500) {
  return log( (tf + mu*(ctf/totalTermCount)) / (docLength + mu) );
}

double calcBM25Feature(double tf, double df, double avgDocLen, double docLength, int totDocs, double b=0.4, double k1=0.9) {
  // make sure it's not negative
  double w_qt = std::max(1e-6, log((totDocs - df + 0.5) / (df+0.5)));

  double K_d = k1*((1-b) + (b*(docLength/avgDocLen)));
  double w_dt = ((k1+1)*tf) / (K_d + tf);
  return w_dt*w_qt;
}

char* getOption(char ** begin, char ** end, const std::string & option) {
  char ** itr = std::find(begin, end, option);
  if (itr != end && ++itr != end) {
    return *itr;
  }
  return 0;
}

void tokenize(string line, const char * delim, vector<string>* output) {
  char mutableLine[line.size() + 1];
  std::strcpy(mutableLine, line.c_str());
  for (char* value = std::strtok(mutableLine, delim);
      value != NULL;
      value = std::strtok(NULL, delim)) {
    output->push_back(value);
  }
}

void readParams(const char* paramFile, map<string, string> *params) {
  ifstream file;
  file.open(paramFile);

  string line;
  if (file.is_open()) {
    while (getline(file, line)) {
      if (line.size() < 2)
        continue;
      char mutableLine[line.size() + 1];
      std::strcpy(mutableLine, line.c_str());

      char* key = std::strtok(mutableLine, "=");
      char* value = std::strtok(NULL, "=");
      (*params)[key] = value;
    }
    file.close();
  }
}

int main(int argc, char * argv[]) {

  // what i need
  // index
  // term list
  // output term:


  // Centroids dir is a dictory that contains files, one per shard, where the
  // file content is the centroid for that shard. Each line is in format: termid,weight
  // The name of each file should be its shardname with no extensions, and
  // should correspond to what is listed in the shardlist file (-l option).
  const char* outDir = getOption(argv, argv + argc, "-o");

  // parameter file. contains list of indexes and list of terms
  char* paramFile = getOption(argv, argv + argc, "-p");

  // read in parameter file
  std::map<string, string> params;
  readParams(paramFile, &params);

  vector<string> terms;
  if (params.find("terms") != params.end()) {
    tokenize(params["terms"], ":", &terms);
  }

  fs::path outPath(outDir);
  if (!fs::is_directory(outPath)) {
    cerr << "Invalid output dir." << endl;
    return 1;
  }

  // open output files
  fs::path bm25File = "champion_bm25.out";
  fs::path indriFile = "champion_indri.out";

  fs::fstream bm25Out;
  bm25Out.open (outPath / bm25File, ios::out);

  fs::fstream indriOut ;
  indriOut.open (outPath / indriFile, ios::out);

  // open all indri indexes; the 10/20 part indexes used
  string indexstr = params["index"];
  vector<Repository*> indexes;
  char mutableLine[indexstr.size() + 1];
  std::strcpy(mutableLine, indexstr.c_str());

  // get all shard indexes and add to vector
  for (char* value = std::strtok(mutableLine, ":");
      value != NULL;
      value = std::strtok(NULL, ":")) {
    Repository* repo = new Repository();
    repo->openRead(value);
    indexes.push_back(repo);
  }

  cerr << "Opened indexes" << endl ;

  // get the total term length of the collection (for Indri scoring)
  // and total docs in the entire collection (for bm25)
  double totalTermCount = 0;
  double totalDocCount = 0;
  vector<Repository*>::iterator rit;
  for (rit = indexes.begin(); rit != indexes.end(); ++rit) {
    // if it has more than one index, quit
    Repository::index_state state = (*rit)->indexes();
    if (state->size() > 1) {
      cout << "Index has more than 1 part. Can't deal with this, man.";
      exit(EXIT_FAILURE);
    }
    Index* index = (*state)[0];
    totalTermCount += index->termCount();
    totalDocCount += index->documentCount();
  }
  double avgDocLength = totalTermCount/totalDocCount;

  // only create shard statistics for specified terms
  int termCnt = 0;
  set<string> stemsSeen;
  vector<string>::iterator it;
  for (it = terms.begin(); it != terms.end(); ++it) {

    termCnt++;
    if (termCnt % 100 == 0) {
      cerr << "  Finished " << termCnt << " terms" << endl;
    }

    string term = *it;
    // stemify term
    string stem = (indexes[0])->processTerm(*it);
    if (stemsSeen.find(stem) != stemsSeen.end()) {
      continue;
    }
    stemsSeen.insert(stem);
    cerr << "Processing: " << (*it) << " (" << stem << ")" << endl;

    // if this is a stopword, skip
    if (stem.size() == 0)
      continue;

    // get collection wide term statistics
    double ctf = 0;
    double df = 0;
    for (rit = indexes.begin(); rit != indexes.end(); ++rit) {
      // if it has more than one index, quit
      Repository::index_state state = (*rit)->indexes();
      if (state->size() > 1) {
        cerr << "Index has more than 1 part. Can't deal with this, man.";
        exit(EXIT_FAILURE);
      }
      Index* index = (*state)[0];
      DocListIterator* docIter = index->docListIterator(stem);

      // term not found
      if (docIter == NULL) continue;

      docIter->startIteration();
      TermData* termData = docIter->termData();
      ctf += termData->corpus.totalCount;
      df += termData->corpus.documentCount;
      delete docIter;
    }

    // for each index
    vector<doc_scores> all_docs;
    int idxCnt = 0;
    for (rit = indexes.begin(); rit != indexes.end(); ++rit) {
      indri::collection::Repository::index_state state = (*rit)->indexes();
      if (state->size() > 1) {
        cerr << "Index has more than 1 part. Can't deal with this, man.";
        exit(EXIT_FAILURE);
      }
      Index* index = (*state)[0];

      // collection pointer used for fetching metadata
      indri::collection::CompressedCollection* collection = (*rit)->collection();

      // get inverted list iterator for this index
      DocListIterator* docIter = index->docListIterator(stem);

      // term not found
      if (docIter == NULL)
        continue;

      // go through each doc in index containing the current term
      // calculate features
      for (docIter->startIteration(); !docIter->finished(); docIter->nextEntry()) {
        DocListIterator::DocumentData* doc = docIter->currentEntry();

        double length = index->documentLength(doc->document);
        double tf = doc->positions.size();

        // calulate Indri score feature and sum it up
        double feat = calcIndriFeature(tf, ctf, totalTermCount, length);
        double bm25feat = calcBM25Feature(tf, df, avgDocLength, length, totalDocCount);

        all_docs.push_back(doc_scores(collection->retrieveMetadatum(doc->document, 
                "docno"), feat, bm25feat));
      } // end doc iter
      // free iterator to save RAM!
      delete docIter;

      cerr << "  Index #" << idxCnt << endl;
      ++idxCnt;
    } // end index iter


    // output top 1000 docs per term

    // first by indri
    sort(all_docs.begin(), all_docs.end(), [](const doc_scores & a, const doc_scores & b) -> bool
        { 
        return a.indri > b.indri; 
        });

    bm25Out << term << endl;
    indriOut << term << endl;

    for (unsigned int i = 0; i < min((unsigned int)all_docs.size(), (unsigned int)1000); ++i) {
      bm25Out << all_docs[i].docno << ":" << all_docs[i].bm25 << endl;
      indriOut << all_docs[i].docno << ":" << all_docs[i].indri << endl;
    }
    bm25Out << endl;
    indriOut << endl;
  } //end term  for

  bm25Out.close();
  indriOut.close();

  // second by bm25


  //char* paramFile = getOption(argv, argv + argc, "-p");
  // read parameter file
  //std::map<string, string> params;
  //readParams(paramFile, &params);


  return 0;
}

