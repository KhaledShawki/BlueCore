# mimalloc

Place mimalloc here only when building with:

```text
--memory-backend=mimalloc
```

Expected layout:

```text
third_party/mimalloc/include/mimalloc.h
third_party/mimalloc/lib/<system>/<architecture>/<configuration>/mimalloc.lib
third_party/mimalloc/lib/<system>/<architecture>/<configuration>/libmimalloc.a
```

Blue does not redistribute mimalloc binaries.
