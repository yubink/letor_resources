#! /usr/bin/env python

import os
import os.path as path
import argparse
from collections import defaultdict
import math
import re

import pprint

# Correlates AUC results for queries and Qeval results for multiple runs?


if __name__ == '__main__':
    pp = pprint.PrettyPrinter(indent=2)
    parser = argparse.ArgumentParser()
    parser.add_argument('queryfile', help='Query file. qnum:query terms here')
    parser.add_argument('shardmap', help='Directory containing shardmaps')
    parser.add_argument('outdir', help='Output directory to dump the\
            generated features into.')
    parser.add_argument('dumpfiles', nargs='*', help='Output of ChampionList.\
    As many .out files as necessary. Features will be outputted in supplied\
    file order.')
    args = parser.parse_args()

    # read queries
    queries = dict()
    for line in open(args.queryfile):
        qnum, query = line.strip().split(':')
        queries[qnum] = query.split(' ')

    # Gather shard files; non dot files that has at least one numeral in
    # its name. (it filters out open .*.swp files and size files)
    (_, _, shardfiles) = os.walk(args.shardmap).next()
    shardlist = sorted([x for x in shardfiles if not x.startswith('.') and re.match('\d', x) is not None])

    # read in shard map in the direction of doc->shard
    doc_to_shard = dict()
    for shard in shardlist:
        print(shard)
        for line in open(path.join(args.shardmap, shard)):
            doc_to_shard[line.strip()] = shard 

    print("Read shard map")

    for dumpfile in args.dumpfiles:
        term_topdocs = dict()
        # read in output of ChampionList into dictionary sorted by term
        with open(dumpfile) as f:
            line = f.readline()
            while len(line) > 0:
                term = line.strip()
                line = f.readline().strip()

                topdocs = []
                while len(line.strip()) > 0:
                    docno, score = line.split(':')
                    score = float(score)
                    topdocs.append((docno, score))
                    line = f.readline()

                term_topdocs[term] = topdocs
                line = f.readline()
                print(line)


        # for each query, generate a feature file containing feature values for
        # each shard
        all_feats_rows = defaultdict(lambda: [])
        for qnum, qterms in queries.items():
            doc_scores = defaultdict(int)
            for term in qterms:
                for doc, score in term_topdocs[term]:
                    doc_scores[doc] += score

            sorted_docs = sorted(doc_scores.items(), key=lambda pair: pair[1], reverse=True)
            shard_counts = defaultdict(int)

            feat_rows = dict()
            for shard in shardlist:
                feat_rows[shard] = [0, 0]
                
            for idx, doc_score in enumerate(sorted_docs[0:100]):
                if idx == 10:
                    for shard, count in shard_counts.items():
                        # track the top 10 doc feature
                        feat_rows[shard][0] = count

                doc, score = doc_score
                shard_counts[doc_to_shard[doc]] += 1

            # top 100 doc feature
            for shard, count in shard_counts.items():
                feat_rows[shard][1] = count

            # add the created feature pair to the final output
            for shard in shardlist:
                all_feats_rows[shard].extend(feat_rows[shard])

    # output all features
    for qnum in queries.keys():
        with open(path.join(args.outdir, qnum+'.features'), 'w') as outf:
            for shard, row in sorted(all_feats_rows.items(), key=lambda pair: 
                    int(pair[0])):
                outf.write('%s %s\n'%(shard, ' '.join([str(x) for x in row])))

    print('done!')
