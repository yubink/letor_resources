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

struct distance_scores {
  distance_scores(int rank_, double kl_, double cosine_): rank(rank_), kl(kl_), cosine(cosine_){};
  int rank;
  double kl;
  double cosine;
};

char* getOption(char ** begin, char ** end, const std::string & option) {
  char ** itr = std::find(begin, end, option);
  if (itr != end && ++itr != end) {
    return *itr;
  }
  return 0;
}

bool hasOption(char** begin, char** end, const std::string& option) {
  return std::find(begin, end, option) != end;
}

void readParams(const char* paramFile, map<string, string> *params) {
  ifstream file;
  file.open(paramFile);

  string line;
  if (file.is_open()) {

    while (getline(file, line)) {
      char mutableLine[line.size() + 1];
      std::strcpy(mutableLine, line.c_str());

      char* key = std::strtok(mutableLine, "=");
      char* value = std::strtok(NULL, "=");
      (*params)[key] = value;
    }
    file.close();
  }
}


// kl diverence
double KLDiverence(map<lemur::api::TERMID_T, double>& centroid, 
    map<lemur::api::TERMID_T, double>& featVector) {

  double kl = 0;
  for (auto& iter : featVector) {
    auto citer = centroid.find(iter.first);
    if (citer != centroid.end()) {
      double q = citer->second;
      double p = iter.second;
      kl += p * (log(p)-log(q));
    } else {
      // this should never happen?!
      //assert(false);
      //cout << iter.first << " in doc but not in centroid" << endl;
    }
  }

  return kl;
}

// dot product; input vectors should be unit sized
double cosineSimilarity(map<lemur::api::TERMID_T, double>& centroid, 
    map<lemur::api::TERMID_T, double>& featVector) {

  double cosine = 0;
  for (auto& iter : featVector) {
    auto citer = centroid.find(iter.first);
    if (citer != centroid.end()) {
      cosine += citer->second*iter.second;
    }
  }

  return cosine;
}

// use inverse distance for kl, so that 0 has meaning.
// returns: inverse kl, cosine
pair<double, double> topDoc(vector<distance_scores>& shardScores) {
  if (shardScores.size() == 0) {
    return make_pair(0.0, 0.0);
  }
  return make_pair(1.0/shardScores[0].kl, shardScores[0].cosine);
}

pair<double, double> avgTop10(vector<distance_scores>& shardScores) {
  double klTot = 0;
  double cosineTot = 0;
  double cnt = 0;
  for (auto& score : shardScores) {
    if (score.rank <= 10) {
      cnt += 1;
      klTot += score.kl;
      cosineTot += score.cosine;
    }
  }
  if (cnt == 0) {
    return make_pair(0.0, 0.0);
  }

  return make_pair(cnt/klTot, cosineTot/cnt);
}

pair<double, double> avgTop100(vector<distance_scores>& shardScores) {
  double klTot = 0;
  double cosineTot = 0;
  double cnt = 0;
  for (auto& score : shardScores) {
    if (score.rank <= 100) {
      cnt += 1;
      klTot += score.kl;
      cosineTot += score.cosine;
    }
  }
  if (cnt == 0) {
    return make_pair(0.0, 0.0);
  }
  return make_pair(cnt/klTot, cosineTot/cnt);
}

int main(int argc, char * argv[]) {
  //const char* centroidsDir = "/bos/tmp11/zhuyund/partition/ShardCentroids/output/cent1-qw160-split-new/centroids/";
  //const char* centroidsDir = "/bos/usr0/yubink/tmpcent/";
  //const char* centroidsDir = "/bos/usr0/zhuyund/partition/ShardCentroids/output/gov2-qw80-cent1-s1-2/centroids";

  // Centroids dir is a dictory that contains files, one per shard, where the
  // file content is the centroid for that shard. Each line is in format: termid,weight
  // The name of each file should be its shardname with no extensions, and
  // should correspond to what is listed in the shardlist file (-l option).
  const char* centroidsDir = getOption(argv, argv + argc, "-c");

  // Location of the Indri index that was used to generate the centroids. 
  // This index is what the centroid files' termids refer to. 
  const char* indriIndex = getOption(argv, argv + argc, "-i");

  // Sample dir contains a list of files named <query num>.filtered; each file
  // contains the TREC retrieval result for that query from a CSI (the CSI
  // itself does not need to be supplied)
  //const char* sampleDir = "/bos/usr0/zhuyund/fedsearch/output/csi/cent1-qw160-split-new/1/sample1/";
  const char* sampleDir = getOption(argv, argv + argc, "-s");

  // Directory to output feature files. Output files will end in ".features"
  const char* outDir = getOption(argv, argv + argc, "-o");

  // File that contains a listing of all shard names, one per line.
  const char* shardList = getOption(argv, argv + argc, "-l");

  vector<string> allShards;
  ifstream shardListFile(shardList);
  if (shardListFile.is_open()) {
    string shardno;
    while (shardListFile >> shardno) {
      allShards.push_back(shardno);
    }
  } else {
    cerr << "Can't open shardlist file."<< endl;
    return 1;
  }


  fs::path outPath(outDir);
  if (!fs::is_directory(outPath)) {
    cerr << "Invalid output dir." << endl;
    return 1;
  }

  // open index
  Repository* repo = new Repository();
  //repo->openRead("/bos/tmp9/indexes/ClueWeb09_Indexes/ClueWeb09_English_01");
  //repo->openRead("/bos/tmp12/javalent/out");
  repo->openRead(indriIndex);

  indri::collection::Repository::index_state indexes = repo->indexes();
  if(indexes->size() > 1) {
    cerr << "Index had more than 1 parts. Quitting." << endl;
    return 1;
  }
  indri::index::Index* index = (*indexes)[0];
  indri::collection::CompressedCollection* collection = repo->collection();

  cout << "Opened index" << endl ;

  // read in the centroids
  map<string, map<lemur::api::TERMID_T, double> > centroidMap;

  fs::path centroidsPath(centroidsDir);
  fs::directory_iterator end_itr;
  for (fs::directory_iterator itr(centroidsPath); itr != end_itr; ++itr) {
    if (fs::is_regular_file(itr->path())) {
      fs::ifstream centroidFile(itr->path());
      //cout << "Processing: " << itr->path() << endl ;
      if (centroidFile.is_open()) {
        string shardno = itr->path().stem().string();

        lemur::api::TERMID_T termId;
        char comma;
        float weight;

        while (centroidFile >> termId >> comma >> weight) {
          centroidMap[shardno][termId] = weight;
        }
      }
    }
  }

  cout << "Read centroids" << endl ;
  for (auto& iter : centroidMap) {
    cout << iter.first << endl;
  }

  // read each query's CSI retrieval result
  fs::path samplePath(sampleDir);

  for (fs::directory_iterator itr(samplePath); itr != end_itr; ++itr) {
    const fs::path * currFile = &itr->path();
    cout << "Processing: " << currFile->string() << endl ;

    if (fs::is_regular_file(*currFile)) {
      if (currFile->extension().string().compare(".filtered") == 0) {
        // TODO read 100 lines, get docno, shard no, do shit, store it

        fs::ifstream retFile(*currFile);
        if (!retFile.is_open()) {
          cerr << "File couldn't be opened: " << currFile->string();
          return 1;
        }

        map<string, vector<distance_scores> > shardScores;
        fs::path qPath = (*itr).path().stem();

        int qnum;
        string q0;
        string extDocId;
        int oldRank;
        double score;
        string indri;
        string shardno;

        int lineCnt = 0;
        while (retFile >> qnum >> q0 >> extDocId >> oldRank >> score >> indri >> shardno) {
          ++lineCnt;
          if (lineCnt > 100) {
            break;
          }

          // for each line in the filtered file, ca
          // get docno, get shardno
          std::vector<lemur::api::DOCID_T> documentIDs;
          documentIDs = collection->retrieveIDByMetadatum( "docno" , extDocId );

          assert(documentIDs.size() == 1);

          const indri::index::TermList* termList = index->termList( documentIDs[0] );
          const indri::utility::greedy_vector<int>& terms = termList->terms();
          std::map<lemur::api::TERMID_T, double> featVector;

          double docLen = 0;
          for( size_t i=0; i<terms.size(); i++ ) {
            docLen += 1;
            if( terms[i] == 0 ) {
              //OOV
            } else {
              auto iter = featVector.find(terms[i]);
              if( iter == featVector.end() ) {
                featVector[terms[i]] = 1;
              } else {
                featVector[terms[i]] += 1;
              }
            }
          }
          delete termList;

          // FIXME double check this works normalize vector
          for (auto& iter : featVector) {
            iter.second /= docLen;
          }


          double klScore = KLDiverence(centroidMap[shardno], featVector);
          double cosineScore = cosineSimilarity(centroidMap[shardno], featVector);
          shardScores[shardno].push_back(distance_scores(lineCnt, klScore, cosineScore));
        }
        retFile.close();

        cout << "read return filtered file." << endl;

        // generate features for this query in output file
        fs::path outFilePath = outPath;
        qPath += ".features";
        outFilePath /= qPath;

        fs::fstream outf(outFilePath, ios::out);

        for (auto& shard : allShards) {
          outf << shard << " ";
          auto siter = shardScores.find(shard);
          if (siter != shardScores.end()) {
            pair<double, double> topDocScores = topDoc(siter->second);
            pair<double, double> avgTop10Scores = avgTop10(siter->second);
            pair<double, double> avgTop100Scores = avgTop100(siter->second);

            outf << topDocScores.first << " " << topDocScores.second << " "
              << avgTop10Scores.first << " " << avgTop10Scores.second << " "
              << avgTop100Scores.first << " " << avgTop100Scores.second;
          } else {
            outf << "0 0 0";
          }
          outf << endl;
        }
        outf.close();
      }
    }
  }

  //char* paramFile = getOption(argv, argv + argc, "-p");
  // read parameter file
  //std::map<string, string> params;
  //readParams(paramFile, &params);


  return 0;
}

