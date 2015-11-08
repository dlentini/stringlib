stringlib
=========

  CPython stringlib fastsearch as standalone library.

How to use:
```c++
  const std::string pattern = "fast";
  const std::string corpus = "my very long text to search for fast";

  // Simple case, just execute the search:
  const char* i = stringlib::fast_search(corpus, pattern);

  // Make a search object that can be reused:
  stringlib::fastsearch search = stringlib::make_fast_search(pattern);
  const char* i = search(corpus);
```
References:

  http://effbot.org/zone/stringlib.htm

  https://github.com/python/cpython/tree/master/Objects/stringlib
