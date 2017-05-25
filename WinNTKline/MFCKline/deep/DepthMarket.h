#ifndef CHART_DEPTH_H_
#define CHART_DEPTH_H_

#include "MYGL/OGLKview.h"

class DepthMarket
{
private:
	float Py, Pm;
	int hour = 0;
	int column = 0;
	int MAX_COL = 17;
	OGLKview Okv;
	OGLKview::Color4f color = { 1,1,1,1 };
private:
	void fillItem();
	void fillList();
	void fillAskBid(OGLKview::Point pt);
public:
	OGLKview::Item item;
public:
	DepthMarket();
	void FillChart(bool unfurl);
	void DrawItem(OGLKview::Item item, int col);
	int DrawItem(OGLKview::Item item, bool mode = false);
	void DelItem(int item);
	DepthMarket* getDepth();
	virtual ~DepthMarket();
};
#endif // !CHART_DEPTH_H_