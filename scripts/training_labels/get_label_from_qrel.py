import argparse
from subprocess import Popen, PIPE

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("partition_name")
    parser.add_argument("shardmaps_dir")
    parser.add_argument("qrel_file_path")
    args = parser.parse_args()

    base_dir = './output/' + args.partition_name

    # add shard to qrels
    qrels_with_shard_file_path = base_dir + '/train.qrels_with_shard'
    process = Popen(['condor_run', 'ruby shard_to_qrels.rb {0} {1} > {2}'.format(args.qrel_file_path,
                                                                                 args.shardmaps_dir,
                                                                                 qrels_with_shard_file_path)],
                    stdout=PIPE, stderr=PIPE)
    stdout, stderr = process.communicate()

    # count n_rel in each shard
    labels = {}
    for line in open(qrels_with_shard_file_path):
        items = line.split()
        queryID = int(items[0])
        shardID = int(items[-1])
        rel_score = int(items[-2])
        if rel_score <= 0:
            continue
        if queryID not in labels:
            labels[queryID] = {}
        labels[queryID][shardID] = labels[queryID].get(shardID, 0) + 1

