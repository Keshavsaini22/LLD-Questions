
#include <unordered_map>
#include <mutex>
using namespace std;

class Node {
public:
    int val;
    int key;
    Node* next;
    Node* prev;

    Node(int val, int key) {
        this->val = val;
        this->key = key;
        next = NULL;
        prev = NULL;
    }
};

class DDL {
public:
    Node* head;
    Node* tail;
    int size;

    DDL(int size) {
        this->size = size;
        head = NULL;
        tail = NULL;
    }

    int deleteLast() {
        if (head == NULL && tail == NULL)
            return 0;

        if (head == tail) {
            Node* todelete = head;
            head = NULL;
            tail = NULL;

            int key = todelete->key;
            delete todelete;
            return key;
        }

        Node* todelete = tail;
        tail = tail->prev;
        tail->next = NULL;

        int key = todelete->key;
        delete todelete;
        return key;
    }

    void insertAtHead(Node* node) {
        if (head == node)
            return;

        if (head == NULL) {
            head = node;
            tail = node;
            node->prev = NULL;
            node->next = NULL;
            return;
        }

        // Detach node from its current position, if already in list
        if (node->prev != NULL)
            node->prev->next = node->next;

        if (node->next != NULL)
            node->next->prev = node->prev;

        if (tail == node)
            tail = node->prev;

        node->next = head;
        node->prev = NULL;
        head->prev = node;
        head = node;
    }
};

class LRUCache {
public:
    unordered_map<int, Node*> mapi;
    DDL* list;
    int size;
    int curr;

private:
    mutex mtx;

public:
    LRUCache(int capacity) {
        list = new DDL(capacity);
        size = capacity;
        curr = 0;
    }

    // Do not copy an LRUCache containing a mutex and owned pointers.
    LRUCache(const LRUCache&) = delete;
    LRUCache& operator=(const LRUCache&) = delete;

    int get(int key) {
        lock_guard<mutex> lock(mtx);

        auto it = mapi.find(key);
        if (it == mapi.end()) {
            return -1;
        }

        Node* node = it->second;
        int val = node->val;

        list->insertAtHead(node);

        return val;
    }

    void put(int key, int value) {
        lock_guard<mutex> lock(mtx);

        // Capacity zero means nothing can be stored.
        if (size == 0)
            return;

        auto it = mapi.find(key);

        if (it != mapi.end()) {
            Node* node = it->second;
            node->val = value;
            list->insertAtHead(node);
            return;
        }

        if (curr == size) {
            int keyToDelete = list->deleteLast();
            mapi.erase(keyToDelete);
            curr--;
        }

        Node* node = new Node(value, key);
        list->insertAtHead(node);
        mapi[key] = node;
        curr++;
    }

    ~LRUCache() {
        // The caller must ensure no other thread is using the cache
        // when its lifetime ends.
        for (auto& entry : mapi) {
            delete entry.second;
        }

        delete list;
    }
};
