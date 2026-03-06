// =====================================================
// DOUBLY LINKED LIST
// =====================================================

class DoublyLinkedListNode<K> {
    constructor(
        public key: K,
        public prev?: DoublyLinkedListNode<K>,
        public next?: DoublyLinkedListNode<K>
    ) {}
}

class DoublyLinkedList<K> {

    private head?: DoublyLinkedListNode<K>
    private tail?: DoublyLinkedListNode<K>

    moveToFront(node: DoublyLinkedListNode<K>) {

        if (this.head === node) return

        this.remove(node)
        this.addFront(node)
    }

    addFront(node: DoublyLinkedListNode<K>) {

        node.prev = undefined
        node.next = this.head

        if (this.head)
            this.head.prev = node

        this.head = node

        if (!this.tail)
            this.tail = node
    }

    remove(node: DoublyLinkedListNode<K>) {

        if (node.prev)
            node.prev.next = node.next
        else
            this.head = node.next

        if (node.next)
            node.next.prev = node.prev
        else
            this.tail = node.prev
    }

    removeLast(): DoublyLinkedListNode<K> | undefined {

        if (!this.tail) return undefined

        const node = this.tail
        this.remove(node)

        return node
    }
}

//
// =====================================================
// STORAGE
// =====================================================
//

interface CacheStorage<K,V> {

    put(key:K,value:V,ttl?:number):void

    get(key:K):V | undefined

    remove(key:K):void

    containsKey(key:K):boolean

    size():number

    capacity():number
}

class InMemoryCacheStorage<K,V> implements CacheStorage<K,V>{

    private store = new Map<K,{value:V,expiry?:number}>()

    constructor(private cap:number){}

    put(key:K,value:V,ttl?:number){

        const expiry = ttl ? Date.now()+ttl : undefined

        this.store.set(key,{value,expiry})
    }

    get(key:K){

        const entry = this.store.get(key)

        if(!entry) return undefined

        if(entry.expiry && entry.expiry < Date.now()){

            this.store.delete(key)
            return undefined
        }

        return entry.value
    }

    remove(key:K){
        this.store.delete(key)
    }

    containsKey(key:K){
        return this.store.has(key)
    }

    size(){
        return this.store.size
    }

    capacity(){
        return this.cap
    }
}

//
// =====================================================
// DATABASE STORAGE
// =====================================================
//

interface DBStorage<K,V>{

    write(key:K,value:V):void

    read(key:K):V | undefined

    delete(key:K):void
}

class SimpleDBStorage<K,V> implements DBStorage<K,V>{

    private db = new Map<K,V>()

    write(key:K,value:V){
        this.db.set(key,value)
    }

    read(key:K){
        return this.db.get(key)
    }

    delete(key:K){
        this.db.delete(key)
    }
}

//
// =====================================================
// EVICTION POLICY
// =====================================================
//

interface EvictionPolicy<K>{

    keyAccessed(key:K):void

    evictKey():K | null

    removeKey(key:K):void
}

class LRUEvictionPolicy<K> implements EvictionPolicy<K>{

    private list = new DoublyLinkedList<K>()
    private map = new Map<K,DoublyLinkedListNode<K>>()

    keyAccessed(key:K){

        let node = this.map.get(key)

        if(!node){

            node = new DoublyLinkedListNode(key)
            this.map.set(key,node)
            this.list.addFront(node)

        }else{

            this.list.moveToFront(node)
        }
    }

    evictKey(){

        const node = this.list.removeLast()

        if(!node) return null

        this.map.delete(node.key)

        return node.key
    }

    removeKey(key:K){

        const node = this.map.get(key)

        if(!node) return

        this.list.remove(node)
        this.map.delete(key)
    }
}

//
// =====================================================
// WRITE POLICY
// =====================================================
//

interface WritePolicy<K,V>{

    write(key:K,value:V,db:DBStorage<K,V>):void
}

class WriteThroughPolicy<K,V> implements WritePolicy<K,V>{

    write(key:K,value:V,db:DBStorage<K,V>){

        db.write(key,value)
    }
}

//
// =====================================================
// KEY BASED EXECUTOR
// =====================================================
//

class KeyBasedExecutor{

    private static instance:KeyBasedExecutor

    private queues = new Map<string,Promise<any>>()

    private constructor(){}

    static getInstance(){

        if(!this.instance)
            this.instance = new KeyBasedExecutor()

        return this.instance
    }

    execute<K>(key:K,task:()=>Promise<any>):Promise<any>{

        const k = String(key)

        const prev = this.queues.get(k) || Promise.resolve()

        const next = prev.then(task)

        this.queues.set(k,next.catch(()=>{}))

        return next
    }
}

//
// =====================================================
// CACHE CORE
// =====================================================
//

class Cache<K,V>{

    private hits=0
    private misses=0

    private executor = KeyBasedExecutor.getInstance()

    constructor(
        private storage:CacheStorage<K,V>,
        private db:DBStorage<K,V>,
        private evictionPolicy:EvictionPolicy<K>,
        private writePolicy:WritePolicy<K,V>
    ){}

    async accessData(key:K):Promise<V | undefined>{

        return this.executor.execute(key,async()=>{

            const value = this.storage.get(key)

            if(value!==undefined){

                this.hits++
                this.evictionPolicy.keyAccessed(key)

                return value
            }

            this.misses++

            const dbValue = this.db.read(key)

            if(dbValue===undefined)
                return undefined

            await this.insert(key,dbValue)

            return dbValue
        })
    }

    async updateData(key:K,value:V,ttl?:number){

        return this.executor.execute(key,async()=>{

            await this.insert(key,value,ttl)

            this.writePolicy.write(key,value,this.db)
        })
    }

    private async insert(key:K,value:V,ttl?:number){

        while(this.storage.size() >= this.storage.capacity()){

            const evictKey = this.evictionPolicy.evictKey()

            if(!evictKey) break

            this.storage.remove(evictKey)
        }

        this.storage.put(key,value,ttl)

        this.evictionPolicy.keyAccessed(key)
    }

    remove(key:K){

        this.storage.remove(key)
        this.evictionPolicy.removeKey(key)
        this.db.delete(key)
    }

    getHitRate(){

        const total = this.hits + this.misses

        if(total===0) return 0

        return this.hits/total
    }
}

//
// =====================================================
// MAIN
// =====================================================
//

async function main(){

    const cache = new Cache<string,string>(

        new InMemoryCacheStorage(3),
        new SimpleDBStorage(),
        new LRUEvictionPolicy(),
        new WriteThroughPolicy()
    )

    await cache.updateData("A","Apple",5000)
    await cache.updateData("B","Banana")
    await cache.updateData("C","Cat")

    console.log(await cache.accessData("A"))

    await cache.updateData("D","Dog")

    console.log(await cache.accessData("B"))

    console.log("Hit rate:",cache.getHitRate())
}

main()