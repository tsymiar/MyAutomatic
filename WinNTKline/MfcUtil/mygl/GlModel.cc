#include "GlModel.h"

#define NUM 3

GLuint	texture[NUM];
float _dX, _dY;
int W, H;
House house;

BMP::BMP(const char *FileName) {
	Load(FileName);
	TexSet();
}

BMP::~BMP()
{
	if (Data != NULL)
	{
		free(Data);
		Data = NULL;
	}
}

GLint BMP::getTex()
{
	return texture;
}

bool BMP::Load(const char *FileName) {
	FILE *File;
	unsigned long size;
	unsigned long i;
	unsigned short int planes;			//面数
	unsigned short int bpp;				// 像素数
	char temp;							// 颜色
										//打开图片
	if (fopen_s(&File, FileName, "rb") != 0) {
		std::cout << "图片不存在" << std::endl;
		return false;
	}
	//移动至横向
	fseek(File, 18, SEEK_CUR);
	//读取横向
	if ((i = fread(&horizon, 4, 1, File)) != 1) {
		std::cout << "横向读取失败" << std::endl;
		return false;
	}
	//读取纵向
	if ((i = fread(&vertical, 4, 1, File)) != 1) {
		std::cout << "纵向读取失败" << std::endl;
		return false;
	}
	//计算图像的尺寸
	size = horizon * vertical * 3;
	if ((fread(&planes, 2, 1, File)) != 1) {   //bmp填1
		std::cout << "计算尺寸错误" << std::endl;
		return false;
	}
	if (planes != 1) {
		std::cout << "不是bmp图像" << std::endl;
		return false;
	}
	//读取像素值
	if ((i = fread(&bpp, 2, 1, File)) != 1) {
		std::cout << "读取像素值失败" << std::endl;
		return false;
	}
	if (bpp != 24) {//如果不是24bpp的话失败
		std::cout << "不是24bit图像" << std::endl;
		return false;
	}
	//跳过24bit，监测RGB数据
	fseek(File, 24, SEEK_CUR);    //读取数据
	Data = (char *)malloc(size);
	if (Data == NULL) {
		std::cout << "内存量不能锁定" << std::endl;
		return false;
	}
	if ((i = fread(Data, size, 1, File)) != 1) {
		std::cout << "不能读取数据" << std::endl;
		return false;
	}
	for (i = 0; i<size; i += 3) { //bgr -> rgb
		temp = Data[i];
		Data[i] = Data[i + 2];
		Data[i + 2] = temp;
	}
	return true;
}
void BMP::TexSet()
{
	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, horizon, vertical, 0, GL_RGB, GL_UNSIGNED_BYTE, Data);
	glBindTexture(GL_TEXTURE_2D, 0);
}

AUX_RGBImageRec *LoadBMP(char *Filename)
{
	FILE *File = NULL;                                // File Handle
	if (!Filename)                                  // Make Sure A Filename Was Given
	{
		return NULL;                            // If Not Return NULL
	}
	File = fopen(Filename, "r");                       // Check To See If The File Exists
	if (File)                                       // Does The File Exist?
	{
		fclose(File);                           // Close The Handle
		return auxDIBImageLoad(Filename);
	}
	return NULL;                                    // If Load Failed Return NULL
}

void __outdoor(bool oo)
{
	glBindTexture(GL_TEXTURE_2D, texture[1]);

	glBegin(GL_QUADS);

	glTexCoord2f(1.0f, 1.0f); glVertex3f(0.0f, 10.0f, 0.0f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-36.0f, 10.0f, 0.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-36.0f, 10.0f, 36.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(0.0f, 10.0f, 36.0f);

	glEnd();
}

void __sphere(GLfloat xx, GLfloat yy, GLfloat zz, GLfloat radius, GLfloat M, GLfloat N)
{
	float step_z = float(_PI_) / M;
	float step_xy = 2 * float(_PI_) / N;
	double x[4], y[4], z[4];

	float angle_z = 0.0;
	float angle_xy = 0.0;
	int i = 0, j = 0;
	glBegin(GL_QUADS);
	for (i = 0; i<M; i++)
	{
		angle_z = i * step_z;

		for (j = 0; j<N; j++)
		{
			angle_xy = j * step_xy;

			x[0] = radius * sin(angle_z) * cos(angle_xy);
			y[0] = radius * sin(angle_z) * sin(angle_xy);
			z[0] = radius * cos(angle_z);

			x[1] = radius * sin(angle_z + step_z) * cos(angle_xy);
			y[1] = radius * sin(angle_z + step_z) * sin(angle_xy);
			z[1] = radius * cos(angle_z + step_z);

			x[2] = radius*sin(angle_z + step_z)*cos(angle_xy + step_xy);
			y[2] = radius*sin(angle_z + step_z)*sin(angle_xy + step_xy);
			z[2] = radius*cos(angle_z + step_z);

			x[3] = radius * sin(angle_z) * cos(angle_xy + step_xy);
			y[3] = radius * sin(angle_z) * sin(angle_xy + step_xy);
			z[3] = radius * cos(angle_z);

			for (int k = 0; k<4; k++)
			{
				glVertex3f(xx + float(x[k]), yy + float(y[k]), zz + float(z[k]));
			}
		}
	}
	glEnd();
}

void __ball()
{
	/* 设置材质属性 */
	GLfloat mat_ambient[] = { 0.9f, 0.5f, 0.8f, 1.0f };
	GLfloat mat_diffuse[] = { 0.9f, 0.5f, 0.8f, 1.0f };
	GLfloat mat_shininess[] = { 100.0f };
	glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);

	glBindTexture(GL_TEXTURE_2D, texture[2]);

	glPushMatrix();//保存变换后的位置和角度。
	glTranslatef(-5.0f, -2.0f, -10.0f);
	GLUquadricObj * sphere = gluNewQuadric();//绘制二次曲面
	gluQuadricOrientation(sphere, GLU_OUTSIDE);//法线向外
	gluQuadricNormals(sphere, GLU_SMOOTH);//法线风格
	gluSphere(sphere, 1.0, 50, 50);//二次曲面绘制函数
	gluDeleteQuadric(sphere);
	glPopMatrix();
}

void GlModel::GlTexture(bool oo)
{
	glEnable(GL_TEXTURE_2D);							// Enable Texture Mapping
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);					// Set The Blending Function For Translucency
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);				// This Will Clear The Background Color To Black
	glClearDepth(1.0);									// Enables Clearing Of The Depth Buffer
	glDepthFunc(GL_LESS);								// The Type Of Depth Test To Do
	glEnable(GL_DEPTH_TEST);							// Enables Depth Testing
	glShadeModel(GL_SMOOTH);							// Enables Smooth Color Shading
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// Really Nice Perspective Calculations

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	Load__QDU(W, H);
	__outdoor(oo);
	__ball();
	__sphere(0, 0, 0, 0.2f, 20, 20);
}

int GlModel::LoadGLTexture()                                    // Load Bitmaps And Convert To Textures
{
	int load = FALSE;    									// Status Indicator
	AUX_RGBImageRec *TextureImage[NUM];
	for (int i = 0; i<NUM; i++)
		memset(TextureImage, i, sizeof(void *) * 1);        // Set The Pointer To NULL
															// Load The Bitmap, Check For Errors, If Bitmap's Not Found Quit
	if ((TextureImage[0] = LoadBMP("image/qdu.bmp")) &&
		(TextureImage[1] = LoadBMP("image/outdoor.bmp")) && (TextureImage[2] = LoadBMP("image/bkg.bmp")))
	{
		load = TRUE;                            // Set The Status To TRUE
		for (int k = 0; k<NUM; k++)
		{
			glGenTextures(1, &texture[k]);
			// Create Nearest Filtered Texture
			glBindTexture(GL_TEXTURE_2D, texture[k]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TextureImage[k]->sizeX, TextureImage[k]->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, TextureImage[k]->data);
		}
	}
	else
		return NUM;
	for (int j = 0; j<NUM; j++)
	{
		if (TextureImage[j])                            // If Texture Exists
		{
			if (TextureImage[j]->data)              // If Texture Image Exists
			{
				free(TextureImage[j]->data);    // Free The Texture Image Memory
			}
			free(TextureImage[j]);                  // Free The Image Structure
		}
	}
	return load;                                  // Return The Status
}

void GlModel::Load__QDU(int wide, int tall)
{
	glOrtho(0, wide, tall, 0, -1, 1);
	BMP *bmp = new BMP("image/qdu.bmp");
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glOrtho(0.0, wide, tall, 0.0, -1.0, 1.0);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, bmp->texture);
	glEnable(GL_ALPHA_TEST);//试描画
	glBegin(GL_POLYGON);
	glTexCoord2f(0.0f, 0.0f); glVertex2d(0, 0);
	glTexCoord2f(0.0f, 1.0f); glVertex2d(0, tall);
	glTexCoord2f(1.0f, 1.0f); glVertex2d(wide, tall);
	glTexCoord2f(1.0f, 0.0f); glVertex2d(wide, 0);
	glEnd();
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_TEXTURE_2D);
	delete bmp;
}

void GlModel::Model(int wide, int tall, float deltax, float deltay)
{
	_dX = deltax;
	_dY = deltay;
	AdjustModel();
	BuildF16(W, H);
}

void GlModel::House(int wide, int tall)
{
	house.InitRenderWin();
	house.Render();
}
