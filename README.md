# hasher - shorten keys in an ArangoDB graph

This program takes a file with ArangoDB vertices (smart or not) or a
file with ArangoDB edges (smart or not) and does the following:

 - For a vertex `_key`, the smart graph attribute (everything up to the
   first colon `:` is kept, the rest of the key is hashed to a 64bit
   value and replaced by the base64 encoded hash.
 - Likewise, the `_from` and `_to` entries of the edges are rewritten
   accordingly.
 - The `_key` of an edge is hashed, too, but the smart graph attributes
   (everything before the first colon `:` and everything after the
   second colon `:`) are kept.
 - The `_id` and `_rev` attributes of both vertices and edges are
   discarded.

In the non-smart case in which no colons occur in the keys, the
corresponding manipulation of the keys is done.

For the vertices, a lookup table is written out (one JSON document per
line, JSONL) to later retrieve the old keys from the new hashed keys.

This allows to decrease the memory usage of a graph in the case of long
keys.

This program should work without collisions for up to 2^30 vertices or
so. For larger numbers the hash function must be modified to use more
than 64 bits.

Usage:

```
    hasher vertices vert_out table_out
    hasher edges edge_out

      both read vertices and edges resp. from stdin
```

In this, `vert_out` is the name of the vertex output file, and
`table_out` is the name of the table output file.

For edges, `edge_out` is the name of the edge output file.
