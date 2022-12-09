#include <stdio.h>
#include <assert.h>
#include <vector>
#include <queue>
#include <set>
#include <algorithm>
#include <iostream>
#include <string.h>

using namespace std;

int tabular;
unsigned char package[65536];

/* if the protocol package is binary */
bool bin_proto = true;

/* for http */
string content;
string delims[16] = {" ", "\r\n"};

class alog{
    public:
    int o;
    unsigned char c;
    vector<__uint64_t> s;
};

class tree_node{
    public:
    set<int> range;
    bool parallel_mark;
    bool is_parent_of_parallel;
    vector<tree_node *> child;
    vector<vector<__uint64_t> > history;
    tree_node(){
        parallel_mark = false;
        is_parent_of_parallel = false;
    }
};

class trie_tree_node{
    public:
    int visited;
    vector<__uint64_t> data;
    vector<trie_tree_node *> child;
    trie_tree_node(){visited = 0;}
};

bool comp_node(tree_node *node1, tree_node *node2){
    return *node1->range.begin() < *node2->range.begin();
}

template<class T>
bool subset(set<T> a, set<T> b){
    set<T> c;
    set_difference(a.begin(), a.end(), b.begin(), b.end(), inserter(c, c.begin()));
    return c.empty();
}

tree_node *Find(tree_node *u, tree_node *v){
    int len = u->child.size();
    for(int i = 0; i < len; ++i){
        if(subset(v->range, u->child[i]->range)) return Find(u->child[i], v);
    }
    return u;
}

tree_node *Field_Tree_Creation(vector<alog *> log){
    tree_node *root = new tree_node();
    int N = log.size();
    for(int i = 0; i < N - 1; ++i){
        root->range.insert(log[i]->o);
        package[log[i]->o] = log[i]->c;
    }
    set<int> p;
    p.insert(log[0]->o);
    for(int i = 1; i < N; ++i){
        set<int> q;
        q.insert(log[i]->o);
        set<int> pq;
        if(log[i]->o == log[i - 1]->o + 1 && log[i]->s == log[i - 1]->s){
            set_union(p.begin(), p.end(), q.begin(), q.end(), inserter(pq, pq.begin()));
            p.clear();
            p = pq;
        }
        else{
            tree_node *v = new tree_node();
            v->range = p;
            tree_node *u = Find(root, v);
            int len = u->child.size();
            for(vector<tree_node *>::iterator it = u->child.begin(); it != u->child.end(); ){
                if(subset((*it)->range, v->range)){
                    v->child.push_back(*it);
                    it = u->child.erase(it);
                }
                else ++it;
            }
            u->child.push_back(v);
            p = q;
            sort(u->child.begin(), u->child.end(), comp_node);
            sort(v->child.begin(), v->child.end(), comp_node);
        }
    }
    return root;
}

void merge_node(tree_node *node1, tree_node *node2){
    set<int> new_set;
    vector<tree_node *> new_vec;
    set_union(node1->range.begin(), node1->range.end(), node2->range.begin(), node2->range.end(), inserter(new_set, new_set.begin()));
    node1->range.clear();
    node2->range.clear();
    node1->range = new_set;
    new_vec.insert(new_vec.end(), node1->child.begin(), node1->child.end());
    new_vec.insert(new_vec.end(), node2->child.begin(), node2->child.end());
    node1->child.clear();
    node2->child.clear();
    node1->child = new_vec;
}

void Tokenization(tree_node *root){
    for(vector<tree_node *>::iterator it = root->child.begin(); it != root->child.end(); ){
        if(it == root->child.begin()){
            it++;
            continue;
        }
        bool flag = false;
        for(int j = 0; j < 2; ++j){
            int fix_len = delims[j].length();
            it--;
            if(fix_len > (*it)->range.size()){
                it++;
                continue;
            }
            string postfix = content.substr(*(--(*it)->range.end()) - fix_len + 1, fix_len);
            it++;
            if(fix_len > (*it)->range.size()) continue;
            string prefix = content.substr(*(*it)->range.begin(), fix_len);
            if(postfix == delims[j] || prefix == delims[j]) flag = true;
        }
        if(flag){
            it++;
            continue;
        }
        merge_node(*(--it), *it);
        it++;
        root->child.erase(it);
    }
    for(tree_node *child:root->child) Tokenization(child);
}

void Redundant_Node_Deletion(tree_node *root){
    while(root->child.size() == 1){
        for(tree_node *node:root->child[0]->child){
            root->child.push_back(node);
        }
        root->child.erase(root->child.begin());
    }
    for(tree_node *node:root->child) Redundant_Node_Deletion(node);
}

void Split_By_Delimiter(tree_node *root){
    if(!root->child.size()){
        string data = content.substr(*(root->range.begin()), root->range.size());
    }
}

void New_Node_Insertion(tree_node *root){
    set<int> new_set;
    set<int> tmp_set;
    int child_num = root->child.size();
    if(!child_num) return;
    for(int i = 0; i < child_num; ++i){
        set_union(root->child[i]->range.begin(), root->child[i]->range.end(), new_set.begin(), new_set.end(), inserter(tmp_set, tmp_set.begin()));
        new_set = tmp_set;
        tmp_set.clear();
    }
    set<int>::iterator it1 = root->range.begin();
    set<int>::iterator it2 = new_set.begin();
    tree_node *new_node = new tree_node();
    for( ; it1 != root->range.end() && it2 != new_set.end(); ){
        assert(*it2 >= *it1);
        if(*it2 > *it1){
            new_node->range.insert(*it1);
            ++ it1;
        }
        else{
            if(!new_node->range.empty()){
                root->child.push_back(new_node);
                new_node = new tree_node();
            }
            ++ it1;
            ++ it2;
        }
    }
    new_node = new tree_node();
    for( ; it1 != root->range.end(); ){
        new_node->range.insert(*it1);
        ++ it1;
    }
    if(!new_node->range.empty())
        root->child.push_back(new_node);
    sort(root->child.begin(), root->child.end(), comp_node);
    for(tree_node *node:root->child) New_Node_Insertion(node);
}

void Parrallel_Field_Identification(tree_node *root, vector<alog *> log){
    int N = log.size();
    queue<tree_node *> q;
    q.push(root);
    while(!q.empty()){
        tree_node *v = q.front();
        q.pop();
        for(tree_node *node:v->child) q.push(node);
        int child_num = v->child.size();
        trie_tree_node *trie_root = new trie_tree_node();
        for(int i = 0; i < child_num; ++i){
            int lo = *(v->child[i]->range.begin());
            trie_tree_node *trie_node = trie_root;
            for(int k = 0; k < N; ++k){
                if(log[k]->o == lo){
                    v->child[i]->history.push_back(log[k]->s);
                    bool flag = false;
                    for(trie_tree_node *node:trie_node->child){
                        if(node->data == log[k]->s){
                            flag = true;
                            trie_node = node;
                            node->visited++;
                        }
                    }
                    if(!flag){
                        trie_tree_node *node = new trie_tree_node();
                        node->data = log[k]->s;
                        node->visited++;
                        trie_node->child.push_back(node);
                    }
                }
            }
        }
        int parallel_num = 0;
        for(int i = 0; i < child_num; ++i){
            int parallel_size = v->child[i]->history.size() * 100 / 100; // h%, h = 80;
            trie_tree_node *trie_node = trie_root;
            for(int k = 0; k < parallel_size; ++k){
                for(trie_tree_node *node:trie_node->child){
                    if(node->data == v->child[i]->history[k]) trie_node = node;
                }
            }
            if(trie_node->visited > 1){
                v->child[i]->parallel_mark = true;
                parallel_num++;
            }
        }
        assert(parallel_num != 1 && "impossible for one single parallel field!");
        if(parallel_num){
            tree_node *new_node = new tree_node();
            // notice: there is some difference from the paper, here I did not handle the non-marked node(s) between two marked nodes
            for(vector<tree_node *>::iterator it = v->child.begin(); it != v->child.end(); ){
                if((*it)->parallel_mark){
                    new_node->child.push_back(*it);
                    set<int> new_set;
                    set_union(new_node->range.begin(), new_node->range.end(), (*it)->range.begin(), (*it)->range.end(), inserter(new_set, new_set.begin()));
                    new_node->range.clear();
                    new_node->range = new_set;
                    v->child.erase(it);
                }
                else it++;
            }
            new_node->is_parent_of_parallel = true;
            v->child.push_back(new_node);
            sort(v->child.begin(), v->child.end(), comp_node);
        }
    }
}

void Sequential_Field_Identifier(tree_node *root, vector<tree_node *> &seq_list, queue<tree_node *> &seq_q){
    if(root->child.size() == 0) seq_list.push_back(root);
    else if(root->is_parent_of_parallel){
        seq_list.push_back(root);
        for(tree_node *node:root->child) seq_q.push(node);
    }
    else{
        for(tree_node *node:root->child) Sequential_Field_Identifier(node, seq_list, seq_q);
    }
}

void dump_node(tree_node *node){
    for(set<int>::iterator it = node->range.begin(); it != node->range.end(); ++it){
        if(!bin_proto) switch (package[*it]){
            case '\r':
                cout << "\\r";
            break;
            case '\n':
                cout << "\\n";
            break;
            default:
                cout << package[*it];
            break;
        }
        else{
            printf("%02x ", package[*it]);
        }
    }
}

void dump(tree_node *node){
    for(int i = 0; i < tabular; ++i) cout << " ";
    dump_node(node);
    cout << endl;
    tabular += 4;
    for(tree_node *child:node->child) dump(child);
    tabular -= 4;
}

int main(int argc, char *argv[]){
    assert(argc == 2 && "only one argument");
    vector<alog *> log;
    FILE *fp = fopen(argv[1], "r");
    char af[16];
    int offset;
    int ch;
    while(fscanf(fp, "%s %d, %d, ", af, &offset, &ch)){
        if(af[0] == '-') break;
        alog *l = new alog();
        l->o = offset;
        l->c = (unsigned char) ch;
        uint64_t func;
        while(fscanf(fp, "%lu, ", &func)){
            l->s.push_back(func);
        }
        if(log.empty()
        || (l->o != log.back()->o)
        || (l->s != log.back()->s))
            log.push_back(l);
    }

    alog *l = new alog();
    l->o = -1;
    l->c = 0;
    l->s.push_back(0);
    log.push_back(l);
    
    tree_node *tree = Field_Tree_Creation(log);
    printf("original tree:\n");
    dump(tree);

    if(!bin_proto) content = (char *)package;

    New_Node_Insertion(tree);
    printf("new node tree:\n");
    dump(tree);
    if(!bin_proto) Tokenization(tree);
    Redundant_Node_Deletion(tree);
    printf("reduce tree:\n");
    dump(tree);

    return 0;
    // Stop

    // para fields identifier:
    Parrallel_Field_Identification(tree, log);
    printf("para tree:\n");
    dump(tree);
    Redundant_Node_Deletion(tree);
    printf("final reduce tree:\n");
    dump(tree);

    // seq fields identifier:
    vector<vector<tree_node *> > seq_lists;
    queue<tree_node *> seq_q;
    seq_q.push(tree);
    while(!seq_q.empty()){
        vector<tree_node *> seq_list;
        tree_node *root = seq_q.front();
        seq_q.pop();
        Sequential_Field_Identifier(root, seq_list, seq_q);
        seq_lists.push_back(seq_list);
    }

    for(vector<tree_node *> list:seq_lists){
        for(tree_node *node:list){
            dump_node(node);
            cout << "->";
        }
        cout << endl;
    }
    return 0;
}