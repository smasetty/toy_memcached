# Toy Memcached
This hobby project is a simple and no-frills version of the memcached(and hence the name:smile:). Recently I came across a [blog post](https://www.adayinthelifeof.nl/2011/02/06/memcache-internals/) which talks about memcached internals, the algorithms and some of the intricacies built into the design. This reading got me very *inspired* to come up with my own version of memcached. My goal was to implement a functional in-memory caching tool and nothing more. This project is a result of this journey.

### What this project is
- [x] An in-memory Key-Value cache
- [x] A simple Thread Safe and Stateless API(think RESTful)
