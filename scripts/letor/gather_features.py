"""
Gather all features and write into ranksvm format
"""
import argparse
import os, sys

N_FIELD = 3
FIELD_NAMES = ['all', 'title', 'inlink']

def init_feat_names():
    feat_names = ["ranks_score", "ranks_r", "all_score", "all_r", "NA", "NA",
                  "NA", "NA",
                  "ranks_hist", "all_hist", "NA", "redde_score", "redde_r", "redde_hist",
                  "bigram_score", "bigram_r", "bigram_hist", "prior", "title_score", "title_r", "title_hist",
                  "NA", "NA", "NA", "inlink_score", "inlink_r", "inlink_hist",
                  "NA", "NA", "NA", "NA", "NA", "NA", "NA"]
    tmp = ["NA", "NA", "NA", "NA", "ctfmax", "ctfmin", "ctfidf_max", "ctfidf_min"]

    feat_names += ['all_' + s for s in tmp]
    feat_names += ['NA' for s in tmp]
    feat_names += ['title_' + s for s in tmp]
    feat_names += ['inlink_' + s for s in tmp]
    feat_names += ["NA", "NA", "NA", "NA", "NA", "taily_score", "taily_rank"]
    feat_names += ["champion_top1000", "champion_top100", "NA"]
    feat_names += ["cent_{0}".format(i) for i in range(6)]

    return feat_names

def init_feat():
    a = [0, 0, 0, 0, 0, 0, 0, 0, 100, 100, 100, 0, 0, 100, 0, 0, 100, 0, 0, 0, 100, 0, 0, 100, 0, 0, 100,
     0, 0, 0,
     0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
    return a


def read_unigram_features(qid, feat_file_path, feats, feat_names):
    if qid not in feats:
        feats[qid] = {}

    for line in open(feat_file_path):
        items = line.strip().split(' ')
        shardid = items[0]
        if shardid not in feats[qid]:
            feats[qid][shardid] = init_feat()

        for i in range(N_FIELD):

            field_name = FIELD_NAMES[i]

            lm_score, ctfmax, ctfmin, ctfidf_max, ctfidf_min = items[1 + i * 5 : 1 + (i + 1) * 5]

            lm_index = feat_names.index("{0}_score".format(field_name))
            feats[qid][shardid][lm_index] = lm_score

            ctfmax_index = feat_names.index("{0}_ctfmax".format(field_name))
            feats[qid][shardid][ctfmax_index] = ctfmax

            ctfmin_index = feat_names.index("{0}_ctfmin".format(field_name))
            feats[qid][shardid][ctfmin_index] = ctfmin

            ctfidf_max_index = feat_names.index("{0}_ctfidf_max".format(field_name))
            feats[qid][shardid][ctfidf_max_index] = ctfidf_max

            ctfidf_min_index = feat_names.index("{0}_ctfidf_min".format(field_name))
            feats[qid][shardid][ctfidf_min_index] = ctfidf_min

    # sort the lm score on each field to get r and hist
    for i in range(N_FIELD):
        lm_scores = []

        field_name = FIELD_NAMES[i]
        lm_index = feat_names.index("{0}_score".format(field_name))
        lm_r_index = feat_names.index("{0}_r".format(field_name))
        lm_hist_index = feat_names.index("{0}_hist".format(field_name))

        for shardid in feats[qid].keys():
            lm_scores.append((feats[qid][shardid][lm_index], shardid))
        tmp = sorted(lm_scores, reverse=True)
        r = 0
        for lm_score, shardid in tmp:
            r += 1
            feats[qid][shardid][lm_r_index] = 1.0/r
            feats[qid][shardid][lm_hist_index] = r/10


def read_bigram_features(qid, feat_file_path, feats, feat_names):
    if qid not in feats:
        feats[qid] = {}

    bigram_score_index = feat_names.index("bigram_score")
    for line in open(feat_file_path):
        items = line.strip().split(' ')
        shardid = int(items[0])
        if shardid not in feats[qid]:
            feats[qid][shardid] = init_feat()

        bigram_score = float(items[1])

        feats[qid][shardid][bigram_score_index] = bigram_score

    bigram_scores = []
    bigram_r_index = feat_names.index("bigram_r")
    bigram_hist_index = feat_names.index("bigram_hist")

    for shardid in feats[qid].keys():
        bigram_scores.append((feats[qid][shardid][bigram_score_index], shardid))
    tmp = sorted(bigram_scores, reverse=True)
    r = 0
    for score, shardid in tmp:
        r += 1
        feats[qid][shardid][bigram_r_index] = 1.0 / r
        feats[qid][shardid][bigram_hist_index] = r / 10


def read_taily_score(taily_file_path, feats, feat_names):
    # TODO: read taily
    taily_score_index = feat_names.index("taily_score")
    # read taily score for each (qid, shardid)
    # put it into feats[qid][shardid][taily_score_index]

    # TODO: get taily rank
    raise NotImplementedError


def read_ranks_score():
    raise NotImplementedError

def read_redde_score():
    raise NotImplementedError

def read_centroid_dist_score():
    raise NotImplementedError

def read_champion_score():
    # TODO read champion features: top1000 and top100
    raise NotImplementedError

def to_ranksvm_format(feats, out_file_path):
    out_file = open(out_file_path, 'w')
    for qid in feats:
        for shardid in feats[qid]:
            feat_vec = feats[qid][shardid]
            feat_str = ' '.join(['{0}:{1}'.format(i + 1, s) for i,s in enumerate(feat_vec)])
            outstr = "0 qid:{0} {1} # {2}\n".format(qid, feat_str, shardid)
            out_file.write(outstr)
    out_file.close()

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("partition_name")
    parser.add_argument("n_queries", type=int, default=200)
    args = parser.parse_args()

    base_dir = "./output/" + args.partition_name

    res_dir = base_dir + "/ltr_features/"

    unigram_dir = res_dir + "/unigram/"

    bigram_dir = res_dir + "/bigram/"

    ranksvm_feat_file = res_dir + "/ranksvm.feat"
    ranksvm_feat_name_file = res_dir + "/ranksvm.feat.name"

    feat_names = init_feat_names()
    feats = {}

    for qid in range(1, args.n_queries + 1):

        # read unigram features...
        unigram_file = "{0}/{1}.features".format(unigram_dir, qid)
        bigram_file = "{0}/{1}.features".format(bigram_dir, qid)

        # read bigram features...
        read_unigram_features(qid, unigram_file, feats, feat_names)
        read_bigram_features(qid, bigram_file, feats, feat_names)

        # read other features...
        # TODO: read taily score, get reversed ranking (taily_score, taily_r)
        # TODO: read champion features

        # SLOW features
        # read ranks score, get reversed ranking and bin_ranking (ranks_score, ranks_r, ranks_hist)
        # read redde score, get reversed ranking and bin_ranking (redde_score, redde_r, redde_hist)
        # read cent dist features

    to_ranksvm_format(feats, ranksvm_feat_file)

