#include "DepthChart.h"

DepthChart* DepthChart::depth = NULL;

DepthChart* DepthChart::getDepth()
{
	if (depth == NULL)
		depth = new DepthChart();
	return depth;
}

DepthChart::DepthChart()
{
	item.time.hour = item.time.min = 0;
	item.pc_ = .0f;
	item.mode = 0;
	item.bs = NULL;
}

void DepthChart::FillChart(bool unfurl)
{
	OGLKview::Point t_chart{ 0.72f,1.111f };
	Okv.SwitchViewport(0);
	if (unfurl)
	{
		Py = 0.57f;
		MAX_COL = 17;
		fillAskbid(t_chart);
	}
	else
	{
		Py = 1.111f;
		MAX_COL = 25;
	}
	Okv.SetColor({ 1,0,0 });
	glBegin(GL_LINES);
	{
		glVertex2f(0.7f, (Py + 1.09F) / 2);
		glVertex2f(1.7f, (Py + 1.09F) / 2);

		glVertex2f(0.7f, Py - 0.07f);
		glVertex2f(1.7f, Py - 0.07f);

		glVertex2f(0.7f, Py - 0.55f);
		glVertex2f(1.7f, Py - 0.55f);
	}
	glEnd();
	drawList();
	fillitem();
	Okv.SetColor({0,0,0});
	glBegin(GL_QUADS);
	{
		glTexCoord2f(0.f, 0.f);
		glVertex2f(0.7f, -1.17f);
		glTexCoord2f(1.f, 0.f);
		glVertex2f(1.7f, -1.17f);
		glTexCoord2f(1.f, 1.f);
		glVertex2f(1.7f, 1.2417f);
		glTexCoord2f(0.f, 1.f);
		glVertex2f(0.7f, 1.2417f);
	}
	glEnd();
}

void DepthChart::drawList(void)
{
	OGLKview::Point pt = { 0.72f,Py - 0.6f };
	OGLKview::Color4f color = { 0.7f,0.7f,0.7f };
	Okv.DrawKtext("北京时间", pt, 17, color);
}

void DepthChart::DrawItem(OGLKview::Item item, int col)
{
	char time0[8] = " ";
	OGLKview::Color4f color = { 1,1,1,1 };
	OGLKview::Point pt = { 0.72f,Py - 0.666f*(col+1) };
	if (item.time.hour > hour)
	{
		sprintf(time0, "%d:%d", item.time.hour, item.time.min);
		hour = item.time.hour;
		Okv.DrawKtext(time0, pt, 17, color, "", false);
	}
	else {
		pt.x = 0.752f;
		sprintf(time0, ":%d", item.time.min);
		Okv.DrawKtext(time0, pt, 14, color);
	}
}

int DepthChart::DrawItem(OGLKview::Item item,bool mode)
{
	char ti[8], time[8] = " ";
	OGLKview::Color4f color = { 1,1,1,1 };
	OGLKview::Point pt = { 0.72f,Py - 0.666f };
	item.time.min <= 9 ? sprintf(time, "0%d", item.time.min) : sprintf(time, "%d", item.time.min);
	if ((item.time.min >= 60) || (item.time.min < 0) || (item.time.hour > 24) || (item.time.hour < 0))
		return -1;
	if (item.time.hour > hour)
	{
		sprintf(ti, "%d:%s", item.time.hour, time);
		hour = item.time.hour;
		Okv.DrawKtext(ti, pt, 17, color, "", false);
	}
	else {
		pt.x = 0.752f;
		sprintf(ti, ":%s", time);
		Okv.DrawKtext(ti, pt, 14, color);
	}
	column++;
	if (column >= MAX_COL)
		column--;
	else
		pt.y -= 0.066f*column;
	return column;
}

void DepthChart::DelItem(int item)
{

}

void DepthChart::fillAskbid(OGLKview::Point pt)
{
	Okv.DrawKtext("卖5", pt, 14, color, "宋体");
	pt.y -= 0.066f;
	Okv.DrawKtext("卖4", pt, 14, color, "宋体");
	pt.y -= 0.066f;
	Okv.DrawKtext("卖3", pt, 14, color, "宋体");
	pt.y -= 0.066f;
	Okv.DrawKtext("卖2", pt, 14, color, "宋体");
	pt.y -= 0.190f;
	Okv.DrawKtext("买2", pt, 14, color, "宋体");
	pt.y -= 0.066f;
	Okv.DrawKtext("买3", pt, 14, color, "宋体");
	pt.y -= 0.066f;
	Okv.DrawKtext("买4", pt, 14, color, "宋体");
	pt.y -= 0.066f;
	Okv.DrawKtext("买5", pt, 14, color, "宋体");
}

void DepthChart::fillitem()
{
	OGLKview::Point pt;
	pt = { 0.72f, (Py + 1.09F) / 2 + 0.011F };
	Okv.DrawKtext("卖1", pt, 17, color, " ", false);
	pt.y = (Py + 1.09F) / 2 - 0.05F;
	Okv.DrawKtext("买1", pt, 17, color, " ", false);
	pt.y = Py - 0.123f;
	Okv.DrawKtext("最新", pt, 14, color, "黑体");
	pt.y -= 0.066f;
	Okv.DrawKtext("涨跌", pt, 14, color, "黑体");
	pt.y -= 0.066f;
	Okv.DrawKtext("幅度", pt, 14, color, "黑体");
	pt.y -= 0.066f;
	Okv.DrawKtext("总手", pt, 14, color, "黑体");
	pt.y -= 0.066f;
	Okv.DrawKtext("现手", pt, 14, color, "黑体");
	pt.y -= 0.066f;
	Okv.DrawKtext("涨停", pt, 14, color, "黑体");
	pt.y -= 0.066f;
	Okv.DrawKtext("持仓", pt, 14, color, "黑体");
	pt = { 0.98f, Py - 0.123f };
	Okv.DrawKtext("均价", pt, 14, color, "黑体");
	pt.y -= 0.066f;
	Okv.DrawKtext("昨结", pt, 14, color, "黑体");
	pt.y -= 0.066f;
	Okv.DrawKtext("开盘", pt, 14, color, "黑体");
	pt.y -= 0.066f;
	Okv.DrawKtext("最高", pt, 14, color, "黑体");
	pt.y -= 0.066f;
	Okv.DrawKtext("最低", pt, 14, color, "黑体");
	pt.y -= 0.066f;
	Okv.DrawKtext("跌停", pt, 14, color, "黑体");
	pt.y -= 0.066f;
	Okv.DrawKtext("仓差", pt, 14, color, "黑体");
}

DepthChart::~DepthChart()
{
	if (depth)
	{
		depth = NULL;
		delete depth;
	}
}
