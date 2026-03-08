## basic.conf

parser development artifact. the smallest config that satisfies every
mandatory validator constraint and nothing more. not a real runnable
scenario — used to verify the parser accepts valid minimal input and
rejects nothing it shouldn't.

mandatory constraints exercised:
. at least 1 listen address per server
. at least 1 location per server
. root present in every location

path dependencies: none (not intended to be run).