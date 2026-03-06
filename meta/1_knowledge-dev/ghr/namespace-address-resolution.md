# namespace & address resolution


## essence

what is namespace resolution?

a namespace is a bijective (or surjective) mapping between 2 domains:
- symbolic domain: human-meaningful names, paths, identifiers
- physical domain: machine-addressable entities (handles, addresses,
  locations)

resolution is the computational act of traversing this mapping:
`name ↦ entity`

not "lookup" - it's a crossing between ontological levels:
from meaning-space to address-space.


## the fundamental problem

why does this exist at all?

because computation requires dual representation of identity:

1. human cognition operates in semantic/symbolic space
   - hierarchical paths: `/etc/nginx/nginx.conf`
   - meaningful names: `localhost`, `www.example.com`
   - protocol identifiers: `"tcp"`, `"udp"`

2. machine operation requires concrete addresses
   - inode numbers, file descriptors
   - IP addresses: `127.0.0.1`, `::1`
   - protocol numbers: `6` (tcp), `17` (udp)

resolution is the morphism between these spaces.


## telos

maintain the illusion of persistence and meaning in a system built on
ephemeral numerical handles.

without namespaces:
- you'd reference files by inode number (but inodes change across mounts,
  filesystems)
- network targets by IP (but IPs change, are not human-memorable)
- no hierarchy, no semantic organization

namespaces provide:
- stability: name remains even if underlying entity moves
- meaning: `/home/user/documents` conveys purpose
- hierarchy: structure that mirrors human understanding


## the functions - ontological roles

file namespace operations:
- `stat`, `access` - query entity properties through name
- `chdir` - change reference frame (current working directory)
- `opendir`, `readdir`, `closedir` - traverse namespace structure

network namespace operations:
- `getaddrinfo` - resolve symbolic network name → address(es)
- `freeaddrinfo` - release resolution results
- `getprotobyname` - resolve protocol name → number
- `getsockname` - inverse: given socket, what is its address?

binding operation:
- `bind` - create the mapping itself: 
associate name/address with socket entity


## key questions to explore

1. why separate file and network namespaces?
   - plan 9 unified these (everything is a file). unix keeps them
     distinct. why? historical accident or necessary difference?

2. what is `bind` ontologically?
   - not resolution (name→entity) but instantiation of the mapping
   - it says: "this socket is reachable at this address"
   - inverse of resolution?

3. why does `getaddrinfo` return multiple addresses?
   - one name can map to many entities (load balancing, IPv4+IPv6)
   - but one file path maps to one inode (usually)
   - what determines cardinality of the mapping?

4. what about `chdir`?
   - it changes the reference frame for all subsequent resolutions
   - like changing coordinate system origin
   - is this namespace manipulation or merely convenience?