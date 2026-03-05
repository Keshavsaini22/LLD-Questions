Generic key and value

Get, set, put, remove - key value pair
Cache eviction policies- LRU, LFU etc etc
Eviction can be of manual where during my insertion cache will get evicted or using background job which is automatic method
Time based expiration (TTL) - optional
Cache hit/cache miss tracking - optional

System must manage limited memory using eviction policies
Supports 2 core operations - accessData(get), update(write)
System should be thread-safe and handle concurrent operations
Custom write policies- write back policy, write around, write through
We need to ensure "read your own writes" consistency meaning - After a client writes data, any subsequent read by the same client must return the updated value.
We will keep our operations on one thread means one key operation will be in one thread i.e different users can do operations on same key but all operation will be in one thread. This makes sure that we have latest value of key at any time (VVVIMP 28th min)



For this problem go for write through and LRU cache