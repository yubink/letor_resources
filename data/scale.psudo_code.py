from sklearn.preprocessing import StandardScaler
import cPickle

scaler = cPickle.load(open("scaler.pkl", 'rb'))

# read from ranksvm formt
all_feat = {}
for line in open(feat_file):
    # TODO: qid, feat_vec, shardid <= line
    all_feat[qid].append((feat_vec, shardid))
                
    # scale each query's features
    for qid in range(200):
        feat_q = []
        shard_ids = []
        for feat_vec, shardid in all_feat[qid]:
            feat_q.append(feat_vec)
            shard_ids.append(shardid)
                                                        
            feat_q = scaler.transform(feat_q) # feat_q: |nshards * nfeat|
                                                                
            # write back to ranksvm format
            for feat_vec, shardid in zip(feat_q, shard_ids):
                # TODO: put qid, feat_vec and shardid into ranksvm format
                # TODO: write to new_feat_file
