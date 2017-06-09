# letor_resources

## Features
- CSI-based features
    - Rank-S and ReDDE: use fedsearch software
    - Distance to shard Centroid: TODO Yubin
- Shard Popularity
    - Todo Zhuyun.
- Term-based features
    - Taily: provided by Yubin's Taily implementation
    - Champion list: TODO Yubin
    - Query Likelihood, Query term statistics, Bigram features: see instructions below.

## LearningToRnk Model
TODO Zhuyun

## Query Likelihood, Query statistics, Bigram counts
0. Go to ./scripts/raw_features.
1. Compile shardFeature.cpp
    1. open Makefile.app. Change line2: `APP=shardFeature`. Save.
    2. `make -f Makefile.app`. 
3. Extract raw features (tf, df, etc.) from indexes.
    1.  Go back to root dir.
    2.  Execute:
        ```
         python ./scripts/raw_features/genShardFeatureJobs.py
         <partition_name>  # any name
         <shardmaps_dir>   # shard map dir
         <query_file>      # e.g. ./data/cw.queries
         -r <repo_dirs>    # a list of indexes. 
        ``` 
        (see sample_raw_feat_script.sh)
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
    1. Features will be written into `./output/<partition_name>/high_level_features/{unigram, bigram}/<i>.features`. i is query ID (e.g. 0 to 200). 
    1. Each line of unigram feature file has 16 items. shard_id {ql_score query_stats*4} * {body, title, inlink}
    2. Each line of bigram feature file has 2 items: shard_id bigram_log_count
