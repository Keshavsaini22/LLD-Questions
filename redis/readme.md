Redis is a high-performance, in-memory data-structure server that stores key-value data and provides atomic operations, expiration, persistence, replication, transactions, Lua/scripts, streams, and clustering.Redis moves frequently accessed state into memory and gives you specialized data structures with extremely cheap operations.
Application
     |
     v
   Redis
     |
     +---- String
     +---- Hash
     +---- List
     +---- Set
     +---- Sorted Set
     +---- Stream
     +---- Bitmap
     +---- HyperLogLog
     +---- Geo   //all above are  redis data structure
     |
     +---- Persistence
     +---- Replication
     +---- Pub/Sub
     +---- Transactions
     +---- Cluster
An atomic operation in Redis means that a command or a group of commands executes as a single, indivisible unit of work. When an operation is atomic, it is guaranteed that no other client can interfere or inject a command in the middle of its execution.It either succeeds completely or fails completely, and other clients will only ever see the data before the operation started or after it finished—never in an intermediate, half-done state.
Redis is single-threaded (at its core execution level), meaning it processes commands one after another. Because of this design, any individual command you send to Redis is naturally atomic.

-----------------------------------------------------------------------------------------

Why is Redis so fast?-
In-memory data.RAM is dramatically faster than disk.
Efficient data structures. not just hashmap.It has specialized representations for different workloads.

-----------------------------------------------------------------------------------------

Redis historically uses a largely single-threaded command execution model for data manipulation.
Conceptually:

Client 1 ──┐
Client 2 ──┼──> Event Loop ──> Execute command
Client 3 ──┘

This has a huge benefit:You don't need a lock around every modification of every data structure.

Redis uses an event loop.
Conceptually:

while (running) {
    events = wait_for_events();

    for (event : events) {
        process(event);
    }
}

Rather than: thread per connection

-----------------------------------------------------------------------------------------

Redis uses a compact binary protocol(0 or 1) internally for many structures and optimized encodings for memory efficiency.

-----------------------------------------------------------------------------------------

Redis Optimized for:
very low latency
high throughput
simple key-based access
atomic operations
temporary state
counters
distributed coordination
caching
queues
ranking
sessions

-----------------------------------------------------------------------------------------

When should I use Redis?
Use Redis when I need extremely low-latency access to data that can be represented efficiently with Redis's data structures, particularly when the workload is read/write heavy, frequently accessed, temporary, or requires atomic operations.
For example:
leaderboard    ZADD leaderboard score user
session store  session:<id> → user/session data
rate limiter   rate_limit:<user>:<minute> → counter
distributed lock  SET lock:resource value NX EX 30
real-time counters  INCR page_views
Queues      LPUSH queue job and RPOP queue
Pub sub
Cache

-----------------------------------------------------------------------------------------

Redis string-
example-
SET user:100:name "Amit"
GET user:100:name

It can hold: "hello" ,"123", JSON, binary data, serialized object

Strings also support atomic numeric operations.
SET counter 10
INCR counter

TC-
GET       O(1)
SET       O(1)
INCR      O(1)
APPEND    O(n)

-----------------------------------------------------------------------------------------

Redis Hash

Instead of:
user:123:name
user:123:age
user:123:city
user:123:email

you can use: HSET user:123 name "Amit" age 25 city "Delhi"

Retrieve: HGET user:123 name or: HGETALL user:123

-----------------------------------------------------------------------------------------

Redis SET
A Set stores unique values.

SADD users:online user1
SADD users:online user2
SADD users:online user1

The second user1 isn't duplicated.

Operations:
SADD
SREM
SISMEMBER
SMEMBERS
SINTER
SUNION
SDIFF

Typical membership:
SISMEMBER → O(1)

-----------------------------------------------------------------------------------------

Sorted Set

Imagine:
Alice → 100
Bob   → 500
Charlie → 300

Leaderboard:
ZADD leaderboard 100 Alice
ZADD leaderboard 500 Bob
ZADD leaderboard 300 Charlie

Now: ZREVRANGE leaderboard 0 9 WITHSCORES gives top users.

Conceptually Redis maintains: 1.member → score 2.plus an ordered structure.

It uses skip lisst DS.
Suppose you have:
10
20
30
40
50
60
70
80

A linked list requires: 10 → 20 → 30 → ... → 80
Search can be: O(n)

A skiplist adds multiple levels:

        10 -------- 40 -------- 70
        |           |           |
        10 -- 20 -- 40 -- 50 -- 70
        |     |     |     |     |
        10 20 30 40 50 60 70 80
Expected complexity:
Search     O(log n)
Insert     O(log n)
Delete     O(log n)

-----------------------------------------------------------------------------------------

Redis List

Useful for:
queue
stack
recent items
work lists

Commands:
LPUSH queue job1
RPUSH queue job2

LPOP queue
RPOP queue

Conceptually: job1 ← job2 ← job3 ← job4

-----------------------------------------------------------------------------------------

Blocking queues - Redis can block until data becomes available.

BLPOP queue 0

-----------------------------------------------------------------------------------------

Redis Streams-Streams are designed for event/log-like workloads

XADD orders * user 123 product 456

Conceptually-
Stream
------------------------------------------------
ID          Event
------------------------------------------------
1-0         order created
2-0         payment completed
3-0         order shipped

They support:
append-only events
consumer groups
message IDs
replay
acknowledgements
pending messages

What Problem It SolvesMissed messages: Standard Redis Pub/Sub drops messages if a client disconnects. Streams save messages in memory so clients can read them later.Streams let a group of workers process different messages at the same time without doing the same work twice.

When to Use ItActivity streams: Tracking user actions or site clicks in order.Event sourcing: Storing every state change in an application as an event.Message queues: Handling background tasks or job processing between microservices.IoT data ingestion: Collecting data streams from smart devices or sensors.
When NOT to Use ItComplex routing: If you need advanced routing rules or multi-exchange patterns, use RabbitMQ.Massive big-data storage: If you need to store petabytes of historical logs for months or years, use Apache Kafka because Redis keeps data primarily in RAM.

Pub/Sub
Publisher
   |
 Redis
  / \
 C1  C2
If a consumer is offline, it generally doesn't get the missed messages.

Streams
Producer
    |
    v
 Stream
    |
    +--> Consumer group A
    |
    +--> Consumer group B
Messages remain in the stream according to retention policy.

-----------------------------------------------------------------------------------------

TTL
You can attach expiration to keys.

SET session:123 abc EX 3600
Meaning: expire after 3600 seconds

This is extremely useful for:
cache entries
sessions
OTP state
temporary locks
rate-limit windows
temporary tokens

How does expiration actually work? because we dont want redis to scan all the data to 
delete expired entries.
So redis uses combination of passive expiration and active expiration
Passive expiration means-When a key is accessed:
GET key
 ↓
Is expired?
 ↓
yes → delete
Active expiration - Redis periodically samples keys with expiration metadata and removes expired keys.

-----------------------------------------------------------------------------------------

Persistence- 
This is where many candidates incorrectly say: "Redis is an in-memory cache, so it doesn't persist."
Redis can persist data by RDB means Periodic snapshots.
RDB (Redis Database), which is one of the primary mechanisms Redis uses to persist data from your RAM onto a permanent hard drive.
Conceptually:
RAM
 |
 | periodically
 v
snapshot.rdb

RDB vs. AOF: How to Prevent Data LossIf losing a few minutes of data is unacceptable for your project, RDB alone isn't enough. Redis offers an alternative persistence method called AOF (Append Only File).
AOF logs every single write command (SET, INCR, etc.) to a text file as it happens.
Conceptually AOF:
SET a 10
SET b 20
INCR a
Redis records write operations in an append-only log.
On restart:
AOF
 ↓
Replay commands
 ↓
Reconstruct memory
Pro Tip: Most production environments use both RDB and AOF together. AOF ensures you don't lose data, while RDB gives you a clean, fast backup file to reboot with.

-----------------------------------------------------------------------------------------

Replication
Redis supports primary/replica architecture.
             ┌── Replica 1
Primary ─────┼── Replica 2
             └── Replica 3


Redis Replication creates copies of your entire database to protect against crashes, Redis Clustering splits your database into smaller pieces across multiple servers to handle massive amounts of data.

-----------------------------------------------------------------------------------------

Redis Sentinel

Sentinel helps with:
monitoring
failure detection
automatic failover
configuration discovery

Conceptually:
           Sentinel
          /   |   \
         /    |    \
     Primary Replica Replica

If primary dies:
Primary
   X
   |
Sentinel
   |
elect/promote
   |
Replica → New Primary

Sentinel is mainly about high availability, not horizontal sharding.

-----------------------------------------------------------------------------------------

Atomicity

Redis commands are generally atomic at the command level.
For example: INCR counter is atomic.

-----------------------------------------------------------------------------------------

MULTI / EXEC

Redis supports transactions.
Conceptually:
MULTI
SET a 10
SET b 20
EXEC

Commands are queued and then executed sequentially.Unlike traditional databases (like MySQL or PostgreSQL), Redis transactions do not support rollbacks. If a command in the middle of a transaction fails because of a data mismatch (like trying to run INCR on a text string), Redis will not undo the previous commands. It will simply keep going and execute the rest of the block.

-----------------------------------------------------------------------------------------

WATCH- Optimistic concurrency.

Conceptually:
WATCH balance
GET balance
MULTI
SET balance new_value
EXEC

If another client modifies the watched key before EXEC, the transaction can fail. This resembles optimistic locking.

-----------------------------------------------------------------------------------------

Lua/scripts - Can contain complex if/else logic, loops, and math variables.
Useful for:
rate limiters
locks
atomic workflows
multi-key update

-----------------------------------------------------------------------------------------

Caching introduces another question: What happens when memory is full?
Redis supports eviction policies. Ex LRU, LFU, ttl

-----------------------------------------------------------------------------------------

Redis caching patterns-

Cache-aside-
Application
    |
    v
 Redis
   / \
 hit  miss
       |
       v
      DB
       |
       v
     Redis

Write-through cache-
Application
    |
    v
 Redis
    |
    v
 Database

Write-behind / write-back
Application writes: App → Redis
and Redis/system asynchronously persists: Redis → DB
Very fast, but more complicated.
Risk:Redis crashes before persistence

Read-through-Similar to Cache-Aside, but the application treats the cache as the main data store. The application never talks directly to the database for reads. Instead, the caching layer itself automatically fetches data from the database on a miss.
App
 |
 v
Cache
 |
 +-- hit → return
 |
 +-- miss → DB → cache → return

-----------------------------------------------------------------------------------------

Cache stampede-
Suppose: 100,000 requests all request: product:123 and the cache expires.
All 100,000 hit DB.

             Redis MISS
                 |
      +----------+----------+
      |          |          |
     DB         DB         DB
      |          |          |
     ...        ...        ...

DB gets crushed.
Solution- Acquire lock at first operation and cache the db response then other will get the responsefrom redis

-----------------------------------------------------------------------------------------

Bloom filter

-----------------------------------------------------------------------------------------

Cache avalanche-Suppose millions of keys expire simultaneously.

Redis
 ↓
millions of misses
 ↓
DB overloaded

Solutions:
TTL jitter
staggered expiration
prewarming
multiple cache layers
rate limiting
fallback/degradation

-----------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------
