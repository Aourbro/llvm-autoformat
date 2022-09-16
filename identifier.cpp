#include <stdio.h>
#include <assert.h>
#include <vector>
#include <set>
#include <algorithm>
#include <iostream>
#include <string.h>

using namespace std;

int tabular;
string content(1024, 0);
string delims[16] = {" ", "\r\n"};

class alog{
    public:
    int o;
    char c;
    vector<__uint64_t> s;
    bool operator==(alog &l){
        return (this->o == l.o);
    }
};

class tree_node{
    public:
    set<int> range;
    vector<tree_node *> child;
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
    for(int i = 0; i < N; ++i){
        root->range.insert(log[i]->o);
        content[log[i]->o] = log[i]->c;
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

void dump(tree_node *node){
    for(int i = 0; i < tabular; ++i) cout << " ";
    for(set<int>::iterator it = node->range.begin(); it != node->range.end(); ++it){
        switch (content[*it]){
            case '\r':
                cout << "\\r";
            break;
            case '\n':
                cout << "\\n";
            break;
            default:
                cout << content[*it];
            break;
        }
    }
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
        l->c = (char) ch;
        uint64_t func;
        while(fscanf(fp, "%lu, ", &func)){
            l->s.push_back(func);
        }
        if(log.empty()
        || (l->o != log.back()->o)
        || (l->s != log.back()->s))
            log.push_back(l);
    }
    tree_node *tree = Field_Tree_Creation(log);
    Tokenization(tree);
    Redundant_Node_Deletion(tree);
    dump(tree);
    return 0;
}