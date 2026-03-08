#include "AVLtree.h"
#include "Command.h"

#include <cassert>
#include <set>

void avl_init(AVLNode* node) {
    node->left = nullptr;
    node->right = nullptr;
    node->parent = nullptr;
    node->depth = 1;
    node->cnt = 1;
}

/*----辅助函数----*/
uint32_t avl_depth(AVLNode * node)
{
    return node ?node->depth:0;
}

uint32_t avl_cnt(AVLNode * node)
{
    return node ?node->cnt:0;
}

uint32_t max(uint32_t lhs, uint32_t rhs)  {
    return lhs < rhs? rhs : lhs;
}

//维护depth和cnt字段
static void avl_update(AVLNode *node){
    node -> depth = 1 + max(avl_depth(node->left), avl_depth(node->right));
    node -> cnt = avl_cnt(node->left) + avl_cnt(node->right) + 1;
}

static AVLNode * rot_left(AVLNode * node)
{
    AVLNode * new_node = node->right;
    if(new_node->left){
        new_node->left->parent = node;
    }
    node->right = new_node->left;
    new_node -> left = node;
    new_node -> parent = node->parent;
    node ->parent = new_node;
    avl_update(node);
    avl_update(new_node);
    return new_node;
}

static AVLNode * rot_right(AVLNode * node)
{
    AVLNode * new_node = node->left;
    if(new_node->right){
        new_node->right->parent = node;
    }
    node->left = new_node->right;
    new_node -> right = node;
    new_node -> parent = node->parent;
    node ->parent = new_node;
    avl_update(node);
    avl_update(new_node);
    return new_node;
}

//左子树太深，需要先左旋转再右旋转
static AVLNode *avl_fix_left(AVLNode * root)  {
    if (avl_depth(root->left->left) < avl_depth(root->left->right))  {
        root->left = rot_left(root->left);
    }
    return rot_right(root);
}


//右子树太深，需要先右旋转再左旋转
static AVLNode *avl_fix_right(AVLNode * root)  {
    if (avl_depth(root->right->right) < avl_depth(root->right->left))  {
        root->right = rot_right(root->right);
    }
    return rot_left(root);
}

//avl——fix函数用于在插入或删除操作后修正所有问题,它从最初受影响的节点开始,一直处理到根节点,
//因为旋转可能会改变树的根节点,所以这个函数会返回根节点
AVLNode * avl_fix(AVLNode * node)
{
    while(true){
        avl_update(node);
        uint32_t l = avl_depth(node->left);
        uint32_t r = avl_depth(node->right);
        AVLNode ** from = nullptr;
        if(node->parent){
            from = (node->parent->left == node)? &node->parent->left : &node->parent->right;
        }
        //通过from判断是否处理到了root
        if(l-r==2){
            node = avl_fix_left(node);
        } else if(r-l==2){
            node = avl_fix_right(node);
        } 
        if(from){
            *from = node;
            node = node->parent;//处理当前处理过后的节点的父节点
        } else{
            return node;
        }
    }
}

/* avl删除:
 * 1. 要是目标节点没有子树,直接删除
 * 2. 要是目标节点只有一个子树,直接用子树替换目标节点
 * 3. 要是目标节点有两个子树，删掉其右子树里的兄弟(中序后继，即比当前节点node大的最小节点)节点,然后将兄弟节点与目标节点交换
*/
AVLNode * avl_del(AVLNode *node)
{
    if(node->right == nullptr){
        //没有右子树,用左子树替换该节点
        //将左子树连接到父节点
        AVLNode * parent = node->parent;
        if(node->left){
            node->left->parent = parent;
        }
        if(parent){
            (parent->left == node? parent->left : parent->right) = node->left;
            return avl_fix(parent);
        }else{
            //删除根节点
            return node->left;
        }
    }else{
        // 用它的下一个兄弟节点交换该节点
        AVLNode *victim = node ->right;
        while(victim->left){
            victim = victim->left;
        }
        AVLNode * root = avl_del(victim);

        *victim = *node;//让victim的left，right，parent指向原先node的位置
        if (victim->left)  {
            //让node的左节点的parent指向victim，相当于丢弃node
            victim->left->parent = victim;
        }
        if (victim->right)  {
            //让node的右节点的parent指向victim，相当于丢弃node
            victim->right->parent = victim;
        }
        AVLNode *parent = node->parent;
        if (parent)  {
            (parent->left == node? parent->left : parent->right) = victim;
            return root;
        } else  {
            // 正在删除根节点？
            return victim;
        }
    }
}
 
//插入节点的代码
void add(Container &c,uint32_t val)
{
    Data *data = new Data();
    avl_init(&data->node);
    data->val = val;

    if(!c.root){//如果根节点为空
        c.root = &data->node;
        return;
    }
    AVLNode *cur = c.root;
    while (true)
    {
        AVLNode ** from =(val < container_of(cur, Data, node)->val)? &cur->left : &cur->right;
        if(!*from){
            *from = &data->node;
            data->node.parent = cur;
            c.root = avl_fix(&data->node);
            break;
        }
        cur = *from;
    }
    
}


bool del(Container &c, uint32_t val)  {
    AVLNode *cur = c.root;
    while (cur)  {
        uint32_t cur_val = container_of(cur , Data, node)->val;
        if (val == cur_val)  {
            break;
        }
        cur = (val < cur_val)? cur->left : cur->right;
    }
    if (!cur)  {
        return false;
    }
    c.root = avl_del(cur);
    delete container_of(cur, Data, node);
    return true;
}

// 偏移到后继或前驱节点
// 注意：无论偏移量有多长，最坏情况下时间复杂度都是O(log(n))
AVLNode *avl_offset(AVLNode *node,  int64_t offset)  {
    int64_t pos =  0;       // 相对于起始节点
    while  (offset !=  pos)  {
        if  (pos <  offset &&  pos +  avl_cnt(node->right)  >=  offset)  {
            // 目标节点在右子树内
            node =  node->right;
            pos +=  avl_cnt(node->left)  +  1;//要进入node->right，就偏移了原始node的node->right->left的节点数和node本身
        }  else  if  (pos >  offset &&  pos -  avl_cnt(node->left)  <=  offset)  {
            // 目标节点在左子树内
            node =  node->left;
            pos -=  avl_cnt(node->right)  +  1;//要进入node->left，就反向偏移了原始node的node->left->right的节点数和node本身
        }  else  {
            // 转到父节点
            AVLNode *parent =  node->parent;
            if  (! parent)  {
                return  NULL;
            }
            if  (parent->right ==  node)  {
                pos -=  avl_cnt(node->left)  +  1;//上面的逆过程
            }  else  {
                pos +=  avl_cnt(node->right)  +  1;//上面的逆过程
            }
            node =  parent;
        }
    }
    return  node;
}


