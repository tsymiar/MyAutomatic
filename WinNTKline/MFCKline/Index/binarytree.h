#ifndef BITREE_H
#define BITREE_H
#include <iostream>
/****************链式存储****************/
typedef struct BinaryTreeNode {
	char data;//数据
	struct BinaryTreeNode* m_Lsub, *m_Rsub;//左右子指针
}BinNode, *BinTree;
/****************二叉树的创建****************/
//按先序序列创建二叉树
int CreateBiTree(BinTree &T)
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
		CreateBiTree(T->m_Lsub);
		CreateBiTree(T->m_Rsub);
	}
	return 0;
}
/****************二叉树遍历****************/
//递归
void Visit(BinTree T)
{
	if (T->data != '#')
	{
		printf("%c", T->data);
	}
}
//先序遍历
void PreOrder(BinTree T)
{
	if (T != NULL)
	{
		//访问根节点
		Visit(T);
		//访问左子节点
		PreOrder(T->m_Lsub);
		//访问右子节点
		PreOrder(T->m_Rsub);
	}
}
//中序遍历

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
//
#endif // !BITREE_H
