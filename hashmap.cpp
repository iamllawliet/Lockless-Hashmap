#include <iostream> 
#include <atomic> 
#include <memory> 
#include <vector> 
#include <functional>
using namespace std;

template <typename K, typename V>
class LockFreeHashTable
{
private:
    //Node structure to hold key value pairs and a pointer to the next node
    struct Node
    {
        K key;
        std:: atomic <V> value;//Atomic is used to make the value object thread safe
        std:: shared_ptr <Node> next;
        Node(K key, V value): key(key), value(value), next(NULL){}
    };
    std::vector <std::atomic <std::shared_ptr<Node> > > table;//Hash table
    size_t capacity;//Capacity of the hash table

    size_t hash(const K& key) const//Hash function to map keys to indices in the table using the standard hash function
    {
        return std::hash <K> {} (key) %capacity;
    }
public:
    LockFreeHashTable(size_t capacity):capacity(capacity)
    {
        table.resize(capacity);
        for(size_t i=0;i<capacity;i++)
        {
            table[i] = std::atomic<std::shared_ptr<Node> > (NULL);
        }
    }
    bool put(const K& key, const V& value)
    {
        size_t index = hash(key);
        auto newNode = std::make_shared <Node> (key,value);
        while(true)//Keep attempting to perform the transaction
        {
            std::shared_ptr <Node> head = table[index].load();
            newNode->next = head;
            if(table[index].compare_exchange_weak(head,newNode))//Using the compare and swap loop
            {
                return true;
            }
        }
    }
    std::shared_ptr <V> get(const K& key) const
    {
        size_t index = hash(key);
        std::shared_ptr <Node> node = table[index].load();
        while(node)//Iterating through all the nodes
        {
            if(node->key==key)
            {
                return std::make_shared<V> (node->value.load());
            }
            node = node->next;
        }
        return NULL;
    }
    bool remove(const K& key)
    {
        size_t index = hash(key);
        while(true)//Keep attempting to perform the remove transaction
        {
            std::shared_ptr <Node> head = table[index].load();
            if(!head)
            {
                return false;
            }
            if(head->key==key)//When the first node at the index has the key
            {
                std::shared_ptr <Node> newHead = head->next;
                if(table[index].compare_exchange_weak(head,newHead))
                {
                    return true;
                }
            }
            else
            {
                std::shared_ptr <Node> prev = head;
                std::shared_ptr <Node> curr = head->next;
                while(curr)//Iterating through all the key value pairs at that index
                {
                    if(curr->key==key)
                    {
                        std::shared_ptr <Node> newNext = curr->next;
                        if(prev->next.compare_exchange_weak(curr,newNext))//Using CAS
                        {
                            return true;
                        }
                    }
                    prev = curr;
                    curr = curr->next;
                }
                return false;
            }
        }
    }
};
