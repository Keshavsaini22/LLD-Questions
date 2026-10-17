Functional requirements
1. Register and remove backend servers at runtime.
2. Receive a request and select a backend server.
3. Support multiple load-balancing algorithms:
   - Round Robin
   - Random
   - Least Connections
   - Weighted Round Robin
   - IP Hash (optional extension)
4. Mark servers healthy or unhealthy.
5. Never route new requests to unhealthy servers.
6. Track in-flight requests and release them when processing finishes.
7. Handle concurrent incoming requests safely.
8. Return a clear failure when no healthy backend is available.


Non-functional requirements
- Thread safety during concurrent selection and server updates.
- Extensibility: add an algorithm without rewriting the dispatcher.
- Fault tolerance: skip failed backends.
- Testability: algorithms should be independently testable.
- Efficient selection, ideally O(1) for Round Robin and O(n) for basic Least Connections.