# letor_resources

## Features
Put features under `./output/<name>/ltr_features/`
- CSI-based features
    - Rank-S and ReDDE: use fedsearch software
    - Distance to shard Centroid: see instructions below
- Shard Popularity: *Todo Zhuyun.*
- Term-based features
    - Taily: provided by Yubin's Taily implementation
    - Champion list: *TODO Yubin*
    - Query Likelihood, Query term statistics, Bigram features
        - see instructions below.
        
## LearningToRnk Model
*TODO Zhuyun*

## Champion list
This is a pre-calculated feature. It keeps track of the best N documents for
each query term. At query time, it looks at how many of the best N documents
each shard has and uses this as a feature. If there are multiple terms in a
query, the scores of the terms are simply summed.

This feature requires running two steps. First step is a C++ program that will
extract and calculate the Indri/BM25 scores for the best N documents for every
term in the index.  Then, a Python program reads the output of the first step
and generates the actual feature file.

Note, N is hardcoded as 1000 in the C++ file. Python file generates features
for N=10,100 for both Indri and BM25.

Scripts are located under ./scripts/champlist.  You need Indri installed in
your home directory in order for compile the C++ code.  (i.e. Indri was
installed with --prefix=/bos/usr0/YOUR_USERNAME) You also need the C++ Boost
library.

1. Build C++ program by running `make`
2. Run program `./ChampionList -p paramFile -o outputDir`
  * outputDir: Directory you want the term information to be dumped to.
  * paramFile: Contains list of terms and indexes to dump them from in the
    following format.
    ```
    index=path_of_index1:path_of_index2
    terms=list:of:terms
    ```
    You can
    list multiple indexes, if they are multiple random parititions of a
    large collection like ClueWeb A. Note that they should be *random*
    parititions for the output scores to be somewhat reliable! If they have
    skewed language models like selective search shards, then the scores
    dumped from each index is not comparable. (Don't use ~ in your index path.
    It doesn't work with Indri.)
    The terms listed should NOT be stemmed or stopped. The program will
    process the term based on how the index you provided have stopped and
    stemmed the terms. The output file will list terms in their original form. 
    Indexes and terms should be colon (:) separated.
    There is an example parameter file in `example_param.txt`
3. Run Python script `./make_feats.py queryfile shardmap outdir [dumpfiles*]`
  * queryfile: File that contains the queries, one per line in the format of
    `qnum:all the query terms`
  * shardmap: Directory that contains shard map files. Each file should be
    named with the shard name (no extensions) and the shard name should contain at least
    one numeral. Each file lists all the documents that belong to that shard
    (should be external document number), one per line.
  * outputDir: directory where the .features file should be created
  * dumpfile: output of ChampionList. You can supply multiple .out files (e.g.
    `outputDir/*.out`)


## Distance to shard centroid
This is a CSI feature. It calculates how representative the documents
retrieved from a CSI is of their shards where they came from. In concrete
terms, it calculates the distance between the averaged language model of the
retrieved documents and their origin shard's centroid.

Necessary script is located under ./scripts/centroid. You need Indri installed
in your home directory in order for this to work. (i.e. Indri was installed
with --prefix=/bos/usr0/YOUR_USERNAME) You also need the C++ Boost library.

1. Build program by running `make`
2. Run program `./CentroidDistances -c centroidsDir -i indriIndex -s sampleDir -o outDir -l shardList`
  * centroidsDir: A directory that contains files, one per shard, where the
    file content is the centroid for that shard. Each line is in format: termid,weight
    The name of each file should be its shardname with no extensions, and
    should correspond to what is listed in the shardlist file (-l option).
  * indriIndex: Location of the Indri index that was used to generate the centroids. 
    This index is what the centroid files' termids refer to. 
  * sampleDir: Directory contains a list of files named <queryNum>.filtered; each file
    contains the TREC retrieval result for that query from a CSI (the CSI
    itself does not need to be supplied)
  * outDir: Directory to output feature files. Output files will end in ".features"
  * shardList: File that contains a listing of all shard names, one per line.

## Query Likelihood, Query statistics, Bigram counts
Make sure your indexes has **body**, **inlink** and **title** fields.
1. Compile shardFeature.cpp
    1. Go to ./scripts/raw_features.
    2. Copy Pre-compiled binary file (with indri-5.2): `/bos/usr0/zhuyund/partition/letor_resources/scripts/raw_features/shardFeature`
    3. Or open Makefile.app. Change line2: `APP=shardFeature`. Save. `make -f Makefile.app`. 
3. Extract **raw** features (tf, df, etc.) from indexes.
    1.  Go back to root dir.
    2.  (See `sample_raw_feat_script.sh`) Execute:
        ```
         python ./scripts/raw_features/genShardFeatureJobs.py
         <partition_name>  # any name
         <shardmaps_dir>   # shard map dir
         <query_file>      # e.g. ./data/cw.queries
         -r <repo_dirs>    # a list of indexes. 
        ``` 
    3.  A bunch of condor jobs will be written into `./output/<partition_name>/jobs/`.  One job per shard. 
    4.  Submit jobs (Don't submit all jobs at one time! Submit in batches!). You can use:
        ```
        python ./scripts/raw_features/job_submitter.py <partition_name>
        ```
    5. Raw features of shard s will be written into `./output/<parititon_name>/features/<s>.feat` and `./output/<parititon_name>/features/<s>.feat_bigram`
3.  Generate high level features (likelihood score, statistics, bigram log count)
    1. Make sure raw features are extracted.
    2. In root dir, do
       ```
       scripts/high_level_features/gen_features.py
       <parition_name>
       <stemmed_query_file>  # e.g. ./data/cw.queries.stemmed
       ```
4. Output
    1. Features will be written into `./output/<partition_name>/ltr_features/{unigram, bigram}/<i>.features`. i is query ID (e.g. 1 to 200). 
    1. Each line of **unigram** feature file has 16 items. shard_id {ql_score query_stats*4} * {body, title, inlink}
    2. Each line of **bigram** feature file has 2 items: shard_id bigram_log_count
