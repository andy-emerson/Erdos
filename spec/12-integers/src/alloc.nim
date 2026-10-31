# alloc.nim: malloc and free. In the memory build (-d:mem) every block's requested bytes are accounted: the live total
# and its peak; malloc_usable_size gives the heap figure beside.
proc cMalloc(n: csize_t): pointer {.importc: "malloc", header: "<stdlib.h>".}
proc cFree(p: pointer) {.importc: "free", header: "<stdlib.h>".}

when defined(mem):
  import std/tables
  proc mallocUsableSize(p: pointer): csize_t {.importc: "malloc_usable_size", header: "<malloc.h>".}
  var live*, peak*: int
  var sizes = initTable[pointer, int]()
  proc xalloc*(n: int): pointer =
    result = cMalloc(csize_t(n))
    sizes[result] = n
    live += n
    if live > peak: peak = live
  proc xfree*(p: pointer) =
    if p != nil:
      live -= sizes[p]
      sizes.del(p)
      cFree(p)
  proc acctOf*(p: pointer): int = sizes[p]
  proc heapOf*(p: pointer): int = int(mallocUsableSize(p))
else:
  template xalloc*(n: int): pointer = cMalloc(csize_t(n))
  template xfree*(p: pointer) = cFree(p)
