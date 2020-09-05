#ifndef VIEW_LEVEL2_H_
#define VIEW_LEVEL2_H_

#include "MYGL/OGLKview.h"

class Level2View
{
private:
    float Py = .0f;
    float Pm = .0f;
    int hour = 0;
    int column = 0;
    int MAX_COL = 17;
    OGLKview Okv;
    OGLKview::Color4f color = { 1,1,1,1 };
private:
    void fillTable(bool unfurl, float x);
    void fillList(float x);
    void fillItem(float x);
    void fillAskBid(OGLKview::Point pnt);
public:
    OGLKview::Item item;
public:
    Level2View();
    Level2View* getDepth();
    void FillChart(bool unfurl);
    void SetBackground();
    void DrawItem(OGLKview::Item item, int col);
    int DrawItem(OGLKview::Item item, bool mode = false);
    int DelItem(int item);
    virtual ~Level2View();
};
#endif // !VIEW_LEVEL2_H_
