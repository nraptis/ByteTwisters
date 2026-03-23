Standalone runner bundle

Expected layout:
- ./practrand_a_34_passers
- ./shards_cpp
- ./src
- ./tools
- ./34_ladder
- ./34_ladder_passwords.txt
- ./run_practrand_34blk_alphabet_gauntlet.sh
- ./run_practrand_passwords_and_list.sh

Typical run:
MAX_FAIL=0 MAX_VERY_SUSPICIOUS=1000 NO_RESULT_FILES=0 \
./34_ladder \
./practrand_a_34_passers \
./34_ladder_passwords.txt \
34blk \
./secret_34_outputs

Alphabet gauntlet:
MAX_FAIL=0 MAX_VERY_SUSPICIOUS=1000 \
./run_practrand_34blk_alphabet_gauntlet.sh \
./practrand_a_34_passers \
./alphabet_gauntlet_34blk_passers

Notes:
- Metadata may still point at old absolute shard paths. The bundled tooling will
  fall back to ./shards_cpp/<source_shard_name>.cpp automatically.
- PractRand's RNG_test still needs to be available on PATH, or at ./build/practrand/RNG_test.
