#include <stdio.h>
#include <assert.h>
#include <vector>
#include <set>
#include <algorithm>
#include <iostream>
#include <string.h>

using namespace std;

int table;
char content[1024];

class alog{
    public:
    int o;
    char c;
    vector<__uint64_t> s;
};

class tree_node{
    public:
    set<int> range;
    vector<tree_node *> children;
};

template<class T>
bool subset(set<T> a, set<T> b){
    set<T> c;
    set_difference(a.begin(), a.end(), b.begin(), b.end(), inserter(c, c.begin()));
    return c.empty();
}

tree_node *Find(tree_node *u, tree_node *v){
    int len = u->children.size();
    for(int i = 0; i < len; ++i){
        if(subset(v->range, u->children[i]->range)) return Find(u->children[i], v);
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
            int len = u->children.size();
            for(vector<tree_node *>::iterator it = u->children.begin(); it != u->children.end(); ){
                if(subset((*it)->range, v->range)){
                    v->children.push_back(*it);
                    it = u->children.erase(it);
                }
                else ++it;
            }
            u->children.push_back(v);
            p = q;
        }
    }
    return root;
}

void dump(tree_node *node){
    for(int i = 0; i < table; ++i) cout << " ";
    for(set<int>::iterator it = node->range.begin(); it != node->range.end(); ++it){
        cout << content[*it];
    }
    cout << endl;
    table += 4;
    for(int i = 0; i < node->children.size(); ++i){
        dump(node->children[i]);
    }
    table -= 4;
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
        log.push_back(l);
    }
    tree_node *tree = Field_Tree_Creation(log);
    dump(tree);
    return 0;
}