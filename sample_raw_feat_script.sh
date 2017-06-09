# generate jobs for raw feature extracting
partition_name=cwa-test
shardmaps_dir=/bos/usr0/zhuyund/fedsearch/data/cw09catA_s1c1-ext/

python ./scripts/raw_features/genShardFeatureJobs.py $partition_name $shardmaps_dir data/cw.queries -r  /bos/tmp0/indexes/ClueWeb09_Full_Part1/ClueWeb09_English_01 /bos/tmp0/indexes/ClueWeb09_Full_Part1/ClueWeb09_English_02 /bos/tmp0/indexes/ClueWeb09_Full_Part1/ClueWeb09_English_03 /bos/tmp0/indexes/ClueWeb09_Full_Part1/ClueWeb09_English_04 /bos/tmp0/indexes/ClueWeb09_Full_Part1/ClueWeb09_English_05 /bos/tmp1/indexes/ClueWeb09_Full_Part2/ClueWeb09_English_06 /bos/tmp1/indexes/ClueWeb09_Full_Part2/ClueWeb09_English_07 /bos/tmp1/indexes/ClueWeb09_Full_Part2/ClueWeb09_English_08 /bos/tmp1/indexes/ClueWeb09_Full_Part2/ClueWeb09_English_09 /bos/tmp1/indexes/ClueWeb09_Full_Part2/ClueWeb09_English_10
