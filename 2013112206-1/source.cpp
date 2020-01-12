#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <cmath>
#include <vector>
#include <gl/glut.h>

using namespace std;

int Width = 800, Height = 800;
int ManipulateMode = 0;
int RenderMode = 3;

// 관측변환을 위한 변수
int StartPt[2];

// ROTATE를 위한 변수
double Axis[3] = { 1.0, 0.0, 0.0 };
float Angle = 0.0;


// PAN 을 위한 변수
float RotMat[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
double Distance = -30.0;

struct Vertex
{
	double P[3];
	double N[3];
};

struct Face
{
	int vIdx[3];
};

struct Object
{
	vector<Vertex> VertList;
	vector<Face> FaceList;
};

Object MyModel;

// Forward declaration of functions
void Reshape(int w, int h);
void Keyboard(unsigned char key, int x, int y);
void Mouse(int button, int state, int x, int y);
void MouseMove(int x, int y);

void InitOpenGL();
void LoadModel(char *fname);
void GetSphereCoord(int x, int y, double *p);
void Render();
void RenderFloor();
void RenderModelAsPoint();
void RenderModelAsWire();
void RenderModelAsTri();

void Cross(double out[3], double a[3], double b[3]);
void Sub(double out[3], double a[3], double b[3]);
void Add(double out[3], double a[3], double b[3]);

void CalcNormal();
void SmoothMesh();

int main(int argc, char **argv)
{
	// Initialize OpenGL.
	glutInit(&argc, argv);

	// Initialize window size and its position when it is created.
	glutInitWindowSize(Width, Height);
	glutInitWindowPosition(0, 0);

	// Initialize OpenGL display modes for double buffering, RGB color, depth buffer enabling.
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

	// Create OpenGL window.
	glutCreateWindow("A SimpleGL Program");

	InitOpenGL();
	LoadModel("./model/bunny.txt");
	//LoadModel("model/dragon.txt");
	//LoadModel("../model/armadilo.txt");

	// Register call back functions.
	glutDisplayFunc(Render);
	glutReshapeFunc(Reshape);
	glutMouseFunc(Mouse);
	glutMotionFunc(MouseMove);
	glutKeyboardFunc(Keyboard);

	// Enter the message loop.
	glutMainLoop();
	return 0;
}

void InitOpenGL()
{
	GLfloat light_specular0[] = { 0.8f, 0.8f, 0.8f };
	GLfloat light_position0[] = { 0.0f, 0.0f, 10000.0f, 0.0f };
	GLfloat light_ambient0[] = { 0.5f, 0.5f, 0.5f };
	GLfloat light_diffuse0[] = { 0.5f, 0.5f, 0.5f };
	glLightfv(GL_LIGHT0, GL_POSITION, light_position0);
	glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient0);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse0);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular0);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
}

void Cross(double out[3], double a[3], double b[3])
{
	out[0] = a[1] * b[2] - a[2] * b[1];
	out[1] = a[2] * b[0] - a[0] * b[2];
	out[2] = a[0] * b[1] - a[1] * b[0];
}

void Sub(double out[3], double a[3], double b[3])
{
	out[0] = a[0] - b[0];
	out[1] = a[1] - b[1];
	out[2] = a[2] - b[2];
}

void Add(double out[3], double a[3], double b[3])
{
	out[0] = a[0] + b[0];
	out[1] = a[1] + b[1];
	out[2] = a[2] + b[2];
}

void LoadModel(char *fname)
{
	// Open the user-specified file
	FILE *fp;
	fopen_s(&fp,fname, "r");

	// Read the numbers of vertex and triangle
	int vnum, fnum;
	fscanf_s(fp, "%d", &vnum);
	fscanf_s(fp, "%d", &fnum);

	// Read vertex coordinates
	for (int i = 0; i < vnum; ++i)
	{
		Vertex v;
		fscanf_s(fp, "%lf%lf%lf", &v.P[0], &v.P[1], &v.P[2]);
		v.N[0] = v.N[1] = v.N[2] = 0.0;
		MyModel.VertList.push_back(v);
	}

	// Read triangles
	for (int i = 0; i < fnum; ++i)
	{
		// Read vertex indices from a file.
		int vidx0, vidx1, vidx2;
		fscanf_s(fp, "%d%d%d", &vidx0, &vidx1, &vidx2);

		Face f;
		f.vIdx[0] = vidx0;
		f.vIdx[1] = vidx1;
		f.vIdx[2] = vidx2;
		MyModel.FaceList.push_back(f);
	}

	CalcNormal();
}

void Reshape(int w, int h)
{
	// View-port transformation
	glViewport(0, 0, w, h);
	Width = w;
	Height = h;
}

void SetupViewTransform()
{
	glMatrixMode(GL_MODELVIEW);

	// Load identity matrix
	glLoadIdentity();

	// Translate {WC}
	glTranslatef(0.0, 0.0, Distance);

	// Rotate {WC}
	glRotatef(Angle, Axis[0], Axis[1], Axis[2]);

	// Apply the previous rotation matrix
	glMultMatrixf(RotMat);

	// Save the rotation part of the modelview matrix
	glGetFloatv(GL_MODELVIEW_MATRIX, RotMat);
	RotMat[12] = RotMat[13] = RotMat[14] = 0.0;
}

void SetupViewVolume()
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60.0, (double)Width / (double)Height, 1.0, 10000.0);
}

void Render()
{
	// Clear color buffer with specified clear color (R, G, B, A)
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Set up view transformation
	SetupViewTransform();

	// Set up viewing volume and choose camera lens
	SetupViewVolume();

	glMatrixMode(GL_MODELVIEW);
	RenderFloor();

	if (RenderMode == 1)
		RenderModelAsPoint();

	if (RenderMode == 2)
		RenderModelAsWire();

	if (RenderMode == 3)
		RenderModelAsTri();

	// Swap buffers for double buffering.
	glutSwapBuffers();
}

void RenderFloor()
{
	glDisable(GL_LIGHTING);

	for (int i = 0; i < 20; ++i)
	{
		float x, z;
		x = z = (double)i / 19 * 20.0 - 10.0;


		glColor3f(1.0f, 1.0f, 1.0f);
		glBegin(GL_LINES);
		glVertex3f(x, 0.0, -10.0f);
		glVertex3f(x, 0.0, 10.0f);
		glEnd();

		glColor3f(1.0f, 0.0f, 1.0f);
		glBegin(GL_LINES);
		glVertex3f(-10.0f, 0.0, z);
		glVertex3f(10.0f, 0.0, z);
		glEnd();

	}
	glEnable(GL_LIGHTING);
}

void RenderModelAsPoint()
{
	// Turn off the lighting 
	glDisable(GL_LIGHTING);

	// Specify the color
	glColor3f(1.0, 1.0, 1.0);

	// Draw the model as point clouds
	glBegin(GL_POINTS);
	for (int i = 0; i < MyModel.VertList.size(); ++i)
	{
		glVertex3dv(MyModel.VertList[i].P);
	}
	glEnd();

	// Turn on the lighting
	glEnable(GL_LIGHTING);
}


void RenderModelAsWire()
{
	// Turn off the lighting 
	glDisable(GL_LIGHTING);

	// Specify the color
	glColor3f(1.0, 1.0, 1.0);

	// Draw the model as wireframe
	for (int i = 0; i < MyModel.FaceList.size(); ++i)
	{
		int idx0, idx1, idx2;
		idx0 = MyModel.FaceList[i].vIdx[0];
		idx1 = MyModel.FaceList[i].vIdx[1];
		idx2 = MyModel.FaceList[i].vIdx[2];

		glBegin(GL_LINE_LOOP);
		glVertex3dv(MyModel.VertList[idx0].P);
		glVertex3dv(MyModel.VertList[idx1].P);
		glVertex3dv(MyModel.VertList[idx2].P);
		glEnd();
	}

	// Turn on the lighting
	glEnable(GL_LIGHTING);
}


void RenderModelAsTri()
{
	// Draw the model as triangles
	glBegin(GL_TRIANGLES);
	for (int i = 0; i < MyModel.FaceList.size(); ++i)
	{
		int idx0, idx1, idx2;
		idx0 = MyModel.FaceList[i].vIdx[0];
		idx1 = MyModel.FaceList[i].vIdx[1];
		idx2 = MyModel.FaceList[i].vIdx[2];

		glNormal3dv(MyModel.VertList[idx0].N);
		glVertex3dv(MyModel.VertList[idx0].P);

		glNormal3dv(MyModel.VertList[idx1].N);
		glVertex3dv(MyModel.VertList[idx1].P);

		glNormal3dv(MyModel.VertList[idx2].N);
		glVertex3dv(MyModel.VertList[idx2].P);
	}
	glEnd();
}

void Mouse(int button, int state, int x, int y)
{
	if (state == GLUT_DOWN)
	{
		StartPt[0] = x;
		StartPt[1] = y;

		if (button == GLUT_LEFT_BUTTON)
			ManipulateMode = 1;

		if (button == GLUT_RIGHT_BUTTON)
			ManipulateMode = 2;
	}

	if (state == GLUT_UP)
	{
		ManipulateMode = 0;
		StartPt[0] = StartPt[1] = 0;
		Angle = 0.0;
	}
}

void MouseMove(int x, int y)
{
	if (ManipulateMode == 1)
	{
		double p[3];
		double q[3];

		GetSphereCoord(StartPt[0], StartPt[1], p);
		GetSphereCoord(x, y, q);

		Cross(Axis, p, q);

		float len = Axis[0] * Axis[0] + Axis[1] * Axis[1] + Axis[2] * Axis[2];
		if (len < 0.0001)
		{
			Axis[0] = 1.0;
			Axis[1] = 0.0;
			Axis[2] = 0.0;
			Angle = 0.0;
		}
		else
		{
			Angle = p[0] * q[0] + p[1] * q[1] + p[2] * q[2];
			Angle = acos(Angle) * 180.0f / 3.141592f;
		}
	}
	else
	{
		int dy = StartPt[1] - y;
		Distance += dy;
	}

	StartPt[0] = x;
	StartPt[1] = y;

	glutPostRedisplay();
}


void GetSphereCoord(int x, int y, double *p)
{
	float r;
	p[0] = (2.0 * x - Width) / Width;
	p[1] = (-2.0 * y + Height) / Height;

	r = p[0] * p[0] + p[1] * p[1];
	if (r >= 1.0)
	{
		p[0] = p[0] / sqrt(r);
		p[1] = p[1] / sqrt(r);
		p[2] = 0.0;
	}
	else
	{
		p[2] = sqrt(1.0 - r);
	}
}

void CalcNormal()
{
	int vnum = (int)MyModel.VertList.size();
	int fnum = (int)MyModel.FaceList.size();

	for (int i = 0; i < fnum; ++i)
	{
		int vidx0 = MyModel.FaceList[i].vIdx[0];
		int vidx1 = MyModel.FaceList[i].vIdx[1];
		int vidx2 = MyModel.FaceList[i].vIdx[2];

		// Compute the normal vector of a triangle
		double e1[3], e2[3], n[3];
		double *p0 = MyModel.VertList[vidx0].P;
		double *p1 = MyModel.VertList[vidx1].P;
		double *p2 = MyModel.VertList[vidx2].P;
		Sub(e1, p1, p0);
		Sub(e2, p2, p0);
		Cross(n, e1, e2);

		// Add the normal vector to each vertex
		double *n0 = MyModel.VertList[vidx0].N;
		double *n1 = MyModel.VertList[vidx1].N;
		double *n2 = MyModel.VertList[vidx2].N;
		Add(n0, n0, n);
		Add(n1, n1, n);
		Add(n2, n2, n);
	}

	// Normalize the normal vector assigned to each vertex
	for (int i = 0; i < vnum; ++i)
	{
		double *n = MyModel.VertList[i].N;
		double norm = sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
		n[0] /= norm;
		n[1] /= norm;
		n[2] /= norm;
	}
}

// Adjust the vertex position
void SmoothMesh()
{
	int vnum = (int)MyModel.VertList.size();
	int fnum = (int)MyModel.FaceList.size();

	Vertex tmp;
	tmp.P[0] = tmp.P[1] = tmp.P[2] = 0.0;
	tmp.N[0] = tmp.N[1] = tmp.N[2] = 0.0; //안에 들어가는 것들초기화

	// Consruct a new vertex list
	vector<Vertex> NewVerts;
	NewVerts.assign(vnum, tmp); //뉴 verts 초기화 해주고

	// Construct a degree list
	vector<int> Degree;
	Degree.assign(vnum, 0); //0으로 초기화 해주고

	for (int k = 0; k < fnum; k++)
	{

		int vidx0 = MyModel.FaceList[k].vIdx[0];
		int vidx1 = MyModel.FaceList[k].vIdx[1];
		int vidx2 = MyModel.FaceList[k].vIdx[2];

		Degree[vidx0]++;
		Degree[vidx1]++;
		Degree[vidx2]++;

	}

	for (int w = 0; w < fnum; w++)
	
	{

		int z = MyModel.FaceList[w].vIdx[0];
		int x = MyModel.FaceList[w].vIdx[1];
		int c = MyModel.FaceList[w].vIdx[2];

		NewVerts[z].P[0] += (MyModel.VertList[x].P[0] + MyModel.VertList[c].P[0]);
		NewVerts[z].P[1] += (MyModel.VertList[x].P[1] + MyModel.VertList[c].P[1]);
		NewVerts[z].P[2] += (MyModel.VertList[x].P[2] + MyModel.VertList[c].P[2]);
		
		//z끝

		NewVerts[x].P[0] += (MyModel.VertList[z].P[0] + MyModel.VertList[c].P[0]);
		NewVerts[x].P[1] += (MyModel.VertList[z].P[1] + MyModel.VertList[c].P[1]);
		NewVerts[x].P[2] += (MyModel.VertList[z].P[2] + MyModel.VertList[c].P[2]);
		
		//x끝

		NewVerts[c].P[0] += (MyModel.VertList[x].P[0] + MyModel.VertList[z].P[0]);
		NewVerts[c].P[1] += (MyModel.VertList[x].P[1] + MyModel.VertList[z].P[1]);
		NewVerts[c].P[2] += (MyModel.VertList[x].P[2] + MyModel.VertList[z].P[2]);

		//c끝

	}

	for (int k1 = 0; k1 < vnum; k1++)
	{
		NewVerts[k1].P[0] = NewVerts[k1].P[0] / (2*Degree[k1]);
		NewVerts[k1].P[1] = NewVerts[k1].P[1] / (2*Degree[k1]);
		NewVerts[k1].P[2] = NewVerts[k1].P[2] / (2*Degree[k1]);
	}

	// Implement your mesh smoothing operation here.....




	// Replace old vertex list with new one
	MyModel.VertList = NewVerts;

	// Compute normal of a model

	CalcNormal();

	glutPostRedisplay();

}

void Keyboard(unsigned char key, int x, int y)
{
	switch (key)
	{
	case '1':
		RenderMode = 1;
		break;
	case '2':
		RenderMode = 2;
		break;
	case '3':
		RenderMode = 3;
		break;
	case 's':
		SmoothMesh();
		break;
	case 'S':
		SmoothMesh();
		break;
	}
	glutPostRedisplay();
}
