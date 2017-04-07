#ifndef BINARYTREE_H
#define BINARYTREE_H
#include <iostream>
#include <stack>
#include <queue>

using namespace std;

/****************链式存储****************/
typedef struct BinaryTreeNode {
	char data;//数据
	struct BinaryTreeNode* m_Lsub, *m_Rsub;//左右子指针
}BinNode, *BinTree;
typedef struct BiTNodePost {
	BinTree binTree;
	char tag;
}BiTNodePost, *BiTPost;
/****************二叉树的创建****************/
//按先序序列创建二叉树
int CreateBinTree(BinTree &T)
{
	char data;
	//按先序次序输入二叉树中结点的值（一个字符），“#”表示空树
	scanf("%c", &data);
	if (data == '#')
	{
		T = NULL;
	}
	else
	{
		T = (BinTree)malloc(sizeof(BinNode));
		//生成根节点
		T->data = data;
		//构造左右子树
		CreateBinTree(T->m_Lsub);
		CreateBinTree(T->m_Rsub);
	}
	return 0;
}
//二叉树结点个数
int GetNodeNum(BinNode *pRoot)
{
	if (pRoot == NULL)//递归出口
		return 0;
	return GetNodeNum(pRoot->m_Lsub) + GetNodeNum(pRoot->m_Rsub) + 1;
}
//二叉树深度
int GetDepth(BinNode *pRoot)
{
	if (pRoot == NULL)
		return 0;
	int depthL = GetDepth(pRoot->m_Lsub);
	int depthR = GetDepth(pRoot->m_Rsub);
	return depthL > depthR ? (depthL + 1) : (depthR + 1);
}
//二叉树第K层的节点个数
int GetNodeNumKthLevel(BinaryTreeNode * pRoot, int k)
{
	if (pRoot == NULL || k < 1)
		return 0;
	if (k == 1)
		return 1;
	int numLeft = GetNodeNumKthLevel(pRoot->m_Lsub, k - 1); // 左子树中k-1层的节点个数  
	int numRight = GetNodeNumKthLevel(pRoot->m_Rsub, k - 1); // 右子树中k-1层的节点个数  
	return (numLeft + numRight);
}
//二叉树中叶子节点的个数
int GetLeafNodeNum(BinaryTreeNode * pRoot)
{
	if (pRoot == NULL)
		return 0;
	if (pRoot->m_Lsub == NULL && pRoot->m_Rsub == NULL)
		return 1;
	int numLeft = GetLeafNodeNum(pRoot->m_Lsub); // 左子树中叶节点的个数  
	int numRight = GetLeafNodeNum(pRoot->m_Rsub); // 右子树中叶节点的个数  
	return (numLeft + numRight);
}
//判断两棵二叉树是否结构相同
bool StructureCmp(BinaryTreeNode * pRoot1, BinaryTreeNode * pRoot2)
{
	if (pRoot1 == NULL && pRoot2 == NULL) // 都为空，返回真  
		return true;
	else if (pRoot1 == NULL || pRoot2 == NULL) // 有一个为空，一个不为空，返回假  
		return false;
	bool resultLeft = StructureCmp(pRoot1->m_Lsub, pRoot2->m_Lsub); // 比较对应左子树   
	bool resultRight = StructureCmp(pRoot1->m_Rsub, pRoot2->m_Rsub); // 比较对应右子树  
	return (resultLeft && resultRight);
}
//二叉树的镜像
BinaryTreeNode * Mirror(BinaryTreeNode * pRoot)
{
	if (pRoot == NULL) // 返回NULL  
		return NULL;
	BinaryTreeNode * pLeft = Mirror(pRoot->m_Lsub); // 求左子树镜像  
	BinaryTreeNode * pRight = Mirror(pRoot->m_Rsub); // 求右子树镜像  
													 // 交换左子树和右子树  
	pRoot->m_Lsub = pRight;
	pRoot->m_Rsub = pLeft;
	return pRoot;
}
//判断二叉树是不是平衡二叉树
bool IsAVL(BinaryTreeNode * pRoot, int & height)
{
	if (pRoot == NULL) // 空树，返回真  
	{
		height = 0;
		return true;
	}
	int heightLeft;
	bool resultLeft = IsAVL(pRoot->m_Lsub, heightLeft);
	int heightRight;
	bool resultRight = IsAVL(pRoot->m_Rsub, heightRight);
	// 左子树和右子树都是AVL，并且高度相差不大于1，返回真  
	if (resultLeft && resultRight && abs(heightLeft - heightRight) <= 1) 
	{
		height = max(heightLeft, heightRight) + 1;
		return true;
	}
	else
	{
		height = max(heightLeft, heightRight) + 1;
		return false;
	}
}
/******************************************************************************
将二叉查找树变为有序的双向链表
参数：	pRoot:		二叉查找树根节点指针；
		pFirstNode:	转换后双向有序链表的第一个节点指针；
		pLastNode:	转换后双向有序链表最后一个节点指针
******************************************************************************/
void Convert(BinaryTreeNode * pRoot,
	BinaryTreeNode * & pFirstNode, BinaryTreeNode * & pLastNode)
{
	BinaryTreeNode *pFirstLeft, *pLastLeft, *pFirstRight, *pLastRight;
	if (pRoot == NULL)
	{
		pFirstNode = NULL;
		pLastNode = NULL;
		return;
	}
	if (pRoot->m_Lsub == NULL)
	{
		// 如果左子树为空，对应双向有序链表的第一个节点是根节点  
		pFirstNode = pRoot;
	}
	else
	{
		Convert(pRoot->m_Lsub, pFirstLeft, pLastLeft);
		// 二叉查找树对应双向有序链表的第一个节点就是左子树转换后双向有序链表的第一个节点  
		pFirstNode = pFirstLeft;
		// 将根节点和左子树转换后的双向有序链表的最后一个节点连接  
		pRoot->m_Lsub = pLastLeft;
		pLastLeft->m_Rsub = pRoot;
	}

	if (pRoot->m_Rsub == NULL)
	{
		// 对应双向有序链表的最后一个节点是根节点  
		pLastNode = pRoot;
	}
	else
	{
		Convert(pRoot->m_Rsub, pFirstRight, pLastRight);
		// 对应双向有序链表的最后一个节点就是右子树转换后双向有序链表的最后一个节点  
		pLastNode = pLastRight;
		// 将根节点和右子树转换后的双向有序链表的第一个节点连接  
		pRoot->m_Rsub = pFirstRight;
		pFirstRight->m_Lsub = pRoot;
	}
	return;
}

void Visit(BinTree T)
{
	if (T->data != '#')
	{
		printf("%c", T->data);
	}
}
/****************二叉树遍历****************/
//分层遍历
//按层次从上往下、从左到右
void LevelOrder(BinTree T) {
	BinTree p = T;
	//队列  
	queue<BinTree> queue;
	//根节点入队  
	queue.push(p);
	//队列非空循环  
	while (!queue.empty()) {
		//队头元素出队  
		p = queue.front();
		//访问p指向的结点  
		printf("%c ", p->data);
		//退出队列  
		queue.pop();
		//左子树不空，将左子树入队  
		if (p->m_Lsub != NULL) {
			queue.push(p->m_Lsub);
		}
		//右子树不空，将右子树入队  
		if (p->m_Rsub != NULL) {
			queue.push(p->m_Rsub);
		}
	}
}
/*：先序遍历(非递归)
  *访问T->data后，将T入栈，遍历左子树；遍历完左子树返回时，栈顶元素应为T，出栈，再先序遍历T的右子树。
  */
void OnrecPreorder(BinTree T) {
	stack<BinTree> stack;
	//p是遍历指针  
	BinTree p = T;
	//栈不空或者p不空时循环  
	while (p || !stack.empty()) {
		if (p != NULL) {
			//存入栈中  
			stack.push(p);
			//访问根节点  
			printf("%c ", p->data);
			//遍历左子树  
			p = p->m_Lsub;
		}
		else {
			//退栈  
			p = stack.top();
			stack.pop();
			//访问右子树  
			p = p->m_Rsub;
		}
	}//while  
}
/*：中序(非递归)
  *T是要遍历树的根指针，要求在遍历完左子树后访问根，再遍历右子树。
  *先将T入栈，遍历左子树；遍历完返回时，栈顶元素应为T，出栈，访问T->data，再中序遍历T的右子树。
  */
void OnrecInorder(BinTree T) {
	stack<BinTree> stack;
	//p是遍历指针  
	BinTree p = T;
	//栈不空或者p不空时循环  
	while (p || !stack.empty()) {
		if (p != NULL) {
			//存入栈中  
			stack.push(p);
			//遍历左子树  
			p = p->m_Lsub;
		}
		else {
			//退栈，访问根节点  
			p = stack.top();
			printf("%c ", p->data);
			stack.pop();
			//访问右子树  
			p = p->m_Rsub;
		}
	}//while  
}
//后序遍历(非递归)  
void OnrecPostorder(BinTree T) {
	stack<BiTPost> stack;
	//p是遍历指针  
	BinTree p = T;
	BiTPost BT;
	//栈不空或者p不空时循环  
	while (p != NULL || !stack.empty()) {
		//遍历左子树  
		while (p != NULL) {
			BT = (BiTPost)malloc(sizeof(BiTNodePost));
			BT->binTree = p;
			//访问过左子树  
			BT->tag = 'L';
			stack.push(BT);
			p = p->m_Lsub;
		}
		//左右子树访问完毕访问根节点  
		while (!stack.empty() && (stack.top())->tag == 'R') {
			BT = stack.top();
			//退栈  
			stack.pop();
			BT->binTree;
			printf("%c ", BT->binTree->data);
		}
		//遍历右子树  
		if (!stack.empty()) {
			BT = stack.top();
			//访问过右子树  
			BT->tag = 'R';
			p = BT->binTree;
			p = p->m_Rsub;
		}
	}//while  
}
//先序遍历(递归)
void RecurPreorder(BinTree T)
{
	if (T != NULL)
	{
		//访问根节点
		Visit(T);
		//访问左子节点
		RecurPreorder(T->m_Lsub);
		//访问右子节点
		RecurPreorder(T->m_Rsub);
	}
}
//中序遍历(递归)
void RecurInorder(BinTree T)
{
	if (T = NULL)
	{
		//访问子节点
		RecurInorder(T->m_Lsub);
		Visit(T);
		RecurInorder(T->m_Rsub);
	}
}
//后序遍历(递归)
void RecurPostorder(BinTree T)
{
	if (T != NULL)
	{
		RecurPostorder(T->m_Lsub);
		RecurPostorder(T->m_Rsub);
		Visit(T);
	}
}
#endif // !BINARYTREE_H
