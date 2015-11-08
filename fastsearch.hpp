/* stringlib: fastsearch implementation */
#pragma once

#include <cstdint>
#include <cstring>
#include <iterator>
#include <limits.h>

/* fast search/count implementation, based on a mix between boyer-
   moore and horspool, with a few more bells and whistles on the top.
   for some more background, see: http://effbot.org/zone/stringlib.htm */

/* note: fastsearch may access s[n], which isn't a problem when using
   Python's ordinary string types, but may cause problems if you're
   using this code in other contexts.  also, the count mode returns -1
   if there cannot possible be a match in the target string, and 0 if
   it has actually checked for matches, but didn't find any.  callers
   beware! */

namespace stringlib {

#if LONG_BIT >= 128
#define STRINGLIB_BLOOM_WIDTH 128
#elif LONG_BIT >= 64
#define STRINGLIB_BLOOM_WIDTH 64
#elif LONG_BIT >= 32
#define STRINGLIB_BLOOM_WIDTH 32
#else
#error "LONG_BIT is smaller than 32"
#endif

#define STRINGLIB_BLOOM_ADD(mask, ch)                                          \
  ((mask |= (1UL << ((ch) & (STRINGLIB_BLOOM_WIDTH - 1)))))
#define STRINGLIB_BLOOM(mask, ch)                                              \
  ((mask & (1UL << ((ch) & (STRINGLIB_BLOOM_WIDTH - 1)))))

#define STRINGLIB_ALIGN_DOWN(p, a)                                             \
  ((void *)((uintptr_t)(p) & ~(uintptr_t)((a)-1)))

ssize_t fastsearch_memchr_1char(const char *s, ssize_t n, char ch,
                                unsigned char needle) {
  const char *ptr = s;
  const char *e = s + n;
  while (ptr < e) {
    const void *candidate =
        memchr((const void *)ptr, needle, (e - ptr) * sizeof(char));
    if (candidate == nullptr)
      return -1;
    ptr = (const char *)STRINGLIB_ALIGN_DOWN(candidate, sizeof(char));
    if (sizeof(char) == 1 || *ptr == ch)
      return (ptr - s);
    /* False positive */
    ptr++;
  }
  return -1;
}

class fastsearch {
public:
  constexpr fastsearch() : p(nullptr), m(0), mlast(0), skip(0), mask(0) {}
  fastsearch(const char *pattern_begin, const char *pattern_end)
      : p(pattern_begin), m(std::distance(pattern_begin, pattern_end)) {
    if (m > 1) {
      mlast = m - 1;
      skip = mlast - 1;
      mask = 0;
      build_table();
    }
  }

  template<typename Range>
  const char *operator()(const Range& corpus) {
    return operator()(corpus.data(), corpus.data() + corpus.size());
  }

  const char *operator()(const char *corpus_begin, const char *corpus_end) {
    if (corpus_begin == corpus_end)
      return corpus_end;
    if (m == 0)
      return corpus_begin;

    const ssize_t pos =
        m <= 1
            ? do_search_1char(corpus_begin,
                              std::distance(corpus_begin, corpus_end))
            : do_search(corpus_begin, std::distance(corpus_begin, corpus_end));
    return pos < 0 ? corpus_end : corpus_begin + pos;
  }

private:
  void build_table() {
    /* create compressed boyer-moore delta 1 table */

    /* process pattern[:-1] */
    for (ssize_t i = 0; i < mlast; i++) {
      STRINGLIB_BLOOM_ADD(mask, p[i]);
      if (p[i] == p[mlast])
        skip = mlast - i - 1;
    }
    /* process pattern[-1] outside the loop */
    STRINGLIB_BLOOM_ADD(mask, p[mlast]);
  }

  ssize_t do_search_1char(const char *s, ssize_t n) {
    ssize_t w = n - m;

    if (w < 0)
      return -1;

    /* look for special cases */
    if (m <= 0)
      return -1;
    /* use special case for 1-character strings */
    if (n > 10) {
      /* use memchr if we can choose a needle without two many likely
         false positives */
      unsigned char needle = p[0] & 0xff;
      return fastsearch_memchr_1char(s, n, p[0], needle);
    }
    for (ssize_t i = 0; i < n; i++)
      if (s[i] == p[0])
        return i;

    return -1;
  }

  ssize_t do_search(const char *s, ssize_t n) {

    ssize_t w = n - m;

    if (w < 0)
      return -1;

    ssize_t j;
    const char *ss = s + m - 1;
    const char *pp = p + m - 1;

    for (ssize_t i = 0; i <= w; i++) {
      /* note: using mlast in the skip path slows things down on x86 */
      if (ss[i] == pp[0]) {
        /* candidate match */
        for (j = 0; j < mlast; j++)
          if (s[i + j] != p[j])
            break;
        if (j == mlast) {
          /* got a match! */
          return i;
        }
        /* miss: check if next character is part of pattern */
        if (!STRINGLIB_BLOOM(mask, ss[i + 1]))
          i = i + m;
        else
          i = i + skip;
      } else {
        /* skip: check if next character is part of pattern */
        if (!STRINGLIB_BLOOM(mask, ss[i + 1]))
          i = i + m;
      }
    }

    return -1;
  }

  const char *p;
  ssize_t m;
  ssize_t mlast;
  ssize_t skip;
  unsigned long mask;
};

const char *fast_search(const char *corpus_first, const char *corpus_last,
                        const char *pat_first, const char *pat_last) {
  fastsearch fs(pat_first, pat_last);
  return fs(corpus_first, corpus_last);
}

template<typename Range>
const char *fast_search(const char *corpus_first, const char *corpus_last,
                        const Range &pattern) {
  fastsearch fs(pattern.data(), pattern.data() + pattern.size());
  return fs(corpus_first, corpus_last);
}

template<typename CorpusRange, typename PatternRange>
const char *fast_search(const CorpusRange &corpus, const PatternRange &pattern) {
  fastsearch fs(pattern.data(), pattern.data() + pattern.size());
  return fs(corpus);
}

template<typename Range>
fastsearch make_fast_search(const Range &pattern) {
  return fastsearch(pattern.data(), pattern.data() + pattern.size());
}

} // namespace stringlib
