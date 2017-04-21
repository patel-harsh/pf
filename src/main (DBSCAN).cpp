
#include "r2000_driver.h"
#include "dbscan.h"
#include <iostream>
#include <thread>
#include <iomanip>
#include <chrono>
#include <string>
#include <sstream>
#include <vector>
#include <sys/time.h>
#include <math.h>
#include <fstream>
#include <GL/glut.h>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace chrono;
using namespace cv;

#define FREQUENCY 20
#define SAMPLE_PER_FRAME 12600
#define PI 3.1415927
#define ANGULAR_INCREMENT 360.0/float(SAMPLE_PER_FRAME)
#define INVALID_REFLECTOR_VALUE 0
#define NUM_CIRCLES 75

pepperl_fuchs::R2000Driver driver;

float zoom = 25.0f;
float display_zoom = 0.001;
uint32_t frameCounter = 0;
unsigned char Buttons[3] = {0};
float v = 0;
float h = 0;
float rotx = 0;
float roty = 0.001f;
float tx = 0;
float ty = 0;
int lastx = 0;
int lasty = 0;

bool draw_lines = false;
bool draw_ampl = false;
bool draw_points = false;
bool draw_polygons = false;
bool draw_circle = true;
bool draw_grid = false;
bool draw_center = false;
bool draw_without = false;
bool draw_distance = false;
bool save_data = false;
bool read_file = false;
bool play_live = false;
bool replay = false;
bool start = true;
bool pauseMode = false;
bool nextWhilePauseMode = false;

ofstream pfscan;
ofstream pfsave;
ofstream temp;
ifstream pfread;


GLfloat last_x = 0;
GLfloat last_y = 0;


vector<uint32_t> minReflectorEchoLUT; // Look-Up Table of the minimal echo by reflector material: element value ^= echo value, element index ^= distance in [mm]
vector<uint32_t> maxReflectorEchoLUT; // Look-Up Table of the maximal echo by reflector material: element value ^= echo value, element index ^= distance in [mm]

vector<float> pivotsDistanceValues;
vector<uint32_t> pivotsMinEchoValues;
vector<uint32_t> pivotsMaxEchoValues;

uint32_t minEcho=0;
uint32_t maxEcho=0;
float hi_ref=.6f;
float low_ref=0.05;
uint32_t hi_ref_Thresh=0;
uint32_t low_ref_Thresh=0;

// Initialize pivots according to the echo-distance graph from manual
void initPivots4LUT (void)
{
	pivotsDistanceValues.resize(28, 0.f);
	pivotsMinEchoValues.resize(28, 0);
	pivotsMaxEchoValues.resize(28, 0);

	pivotsDistanceValues.at(0) = 0.1f;
	pivotsDistanceValues.at(1) = 0.2f;
	pivotsDistanceValues.at(2) = 0.3f;
	pivotsDistanceValues.at(3) = 0.4f;
	pivotsDistanceValues.at(4) = 0.5f;
	pivotsDistanceValues.at(5) = 0.6f;
	pivotsDistanceValues.at(6) = 0.7f;
	pivotsDistanceValues.at(7) = 0.8f;
	pivotsDistanceValues.at(8) = 0.9f;
	pivotsDistanceValues.at(9) = 1.f;
	pivotsDistanceValues.at(10) = 2.f;
	pivotsDistanceValues.at(11) = 3.f;
	pivotsDistanceValues.at(12) = 4.f;
	pivotsDistanceValues.at(13) = 5.f;
	pivotsDistanceValues.at(14) = 6.f;
	pivotsDistanceValues.at(15) = 7.f;
	pivotsDistanceValues.at(16) = 8.f;
	pivotsDistanceValues.at(17) = 9.f;
	pivotsDistanceValues.at(18) = 10.f;
	pivotsDistanceValues.at(19) = 20.f;
	pivotsDistanceValues.at(20) = 30.f;
	pivotsDistanceValues.at(21) = 40.f;
	pivotsDistanceValues.at(22) = 50.f;
	pivotsDistanceValues.at(23) = 60.f;
	pivotsDistanceValues.at(24) = 70.f;
	pivotsDistanceValues.at(25) = 80.f;
	pivotsDistanceValues.at(26) = 90.f;
	pivotsDistanceValues.at(27) = 100.f;

	pivotsMinEchoValues.at(0) = 271; // @ 0.1f
	pivotsMinEchoValues.at(1) = 333; // @ 0.2f
	pivotsMinEchoValues.at(2) = 375; // @ 0.3f
	pivotsMinEchoValues.at(3) = 396; // @ 0.4f
	pivotsMinEchoValues.at(4) = 416; // @ 0.5f
	pivotsMinEchoValues.at(5) = 416; // @ 0.6f
	pivotsMinEchoValues.at(6) = 416; // @ 0.7f
	pivotsMinEchoValues.at(7) = 416; // @ 0.8f
	pivotsMinEchoValues.at(8) = 416; // @ 0.9f
	pivotsMinEchoValues.at(9) = 416; // @ 1.f
	pivotsMinEchoValues.at(10) = 396; // @ 2.f
	pivotsMinEchoValues.at(11) = 375; // @ 3.f
	pivotsMinEchoValues.at(12) = 354; // @ 4.f
	pivotsMinEchoValues.at(13) = 333; // @ 5.f
	pivotsMinEchoValues.at(14) = 313; // @ 6.f
	pivotsMinEchoValues.at(15) = 292; // @ 7.f
	pivotsMinEchoValues.at(16) = 292; // @ 8.f
	pivotsMinEchoValues.at(17) = 271; // @ 9.f
	pivotsMinEchoValues.at(18) = 271; // @ 10.f
	pivotsMinEchoValues.at(19) = 167; // @ 20.f
	pivotsMinEchoValues.at(20) = 84; // @ 30.f
	pivotsMinEchoValues.at(21) = 0; // @ 40.f
	pivotsMinEchoValues.at(22) = 0; // @ 50.f
	pivotsMinEchoValues.at(23) = 0; // @ 60.f
	pivotsMinEchoValues.at(24) = 0; // @ 70.f
	pivotsMinEchoValues.at(25) = 0; // @ 80.f
	pivotsMinEchoValues.at(26) = 0; // @ 90.f
	pivotsMinEchoValues.at(27) = 0; // @ 100.f

	pivotsMaxEchoValues.at(0) = 271; // @ 0.1f
	pivotsMaxEchoValues.at(1) = 541; // @ 0.2f
	pivotsMaxEchoValues.at(2) = 958; // @ 0.3f
	pivotsMaxEchoValues.at(3) = 1312; // @ 0.4f
	pivotsMaxEchoValues.at(4) = 1541; // @ 0.5f
	pivotsMaxEchoValues.at(5) = 1666; // @ 0.6f
	pivotsMaxEchoValues.at(6) = 1750; // @ 0.7f
	pivotsMaxEchoValues.at(7) = 1833; // @ 0.8f
	pivotsMaxEchoValues.at(8) = 1895; // @ 0.9f
	pivotsMaxEchoValues.at(9) = 1937; // @ 1.f
	pivotsMaxEchoValues.at(10) = 1895; // @ 2.f
	pivotsMaxEchoValues.at(11) = 1708; // @ 3.f
	pivotsMaxEchoValues.at(12) = 1541; // @ 4.f
	pivotsMaxEchoValues.at(13) = 1417; // @ 5.f
	pivotsMaxEchoValues.at(14) = 1333; // @ 6.f
	pivotsMaxEchoValues.at(15) = 1271; // @ 7.f
	pivotsMaxEchoValues.at(16) = 1208; // @ 8.f
	pivotsMaxEchoValues.at(17) = 1166; // @ 9.f
	pivotsMaxEchoValues.at(18) = 1125; // @ 10.f
	pivotsMaxEchoValues.at(19) = 812; // @ 20.f
	pivotsMaxEchoValues.at(20) = 625; // @ 30.f
	pivotsMaxEchoValues.at(21) = 479; // @ 40.f
	pivotsMaxEchoValues.at(22) = 354; // @ 50.f
	pivotsMaxEchoValues.at(23) = 270; // @ 60.f
	pivotsMaxEchoValues.at(24) = 187; // @ 70.f
	pivotsMaxEchoValues.at(25) = 125; // @ 80.f
	pivotsMaxEchoValues.at(26) = 104; // @ 90.f
	pivotsMaxEchoValues.at(27) = 83; // @ 100.f
}


// Create the Look-Up Table for the minimal and maximal echo values returned by reflector material according to the echo-distance graph from the manual by using a linear interpolation as approximation
void createEcho2DistanceLUTs (void)
{
	minReflectorEchoLUT.resize(110000, INVALID_REFLECTOR_VALUE);
	maxReflectorEchoLUT.resize(110000, INVALID_REFLECTOR_VALUE);

	uint32_t loopStart = pivotsDistanceValues.at(0) * 1000;
	uint32_t loopEnd = pivotsDistanceValues.at(27) * 1000;

	float defaultDistance = 105.f;

	float prevPivotDistance;
	float nextPivotDistance;
	float comparingDistPivot;

	int pivotIndex;

	uint32_t minEcho;
	uint32_t maxEcho;

	float slope;
	float offset;

	float prevPvtEcho;
	float nextPvtEcho;
	float slope_numerator;
	float slope_denominator;
	float log_var;
	int dbg_stopp;

	float distance;

	minReflectorEchoLUT.at(loopStart) = pivotsMinEchoValues.at(0);
	maxReflectorEchoLUT.at(loopStart) = pivotsMaxEchoValues.at(0);
	// itMinRE ^= distance in [mm]
	for (int itMinRE = loopStart+1; itMinRE < loopEnd; ++itMinRE)
	{
		distance = (itMinRE / 1000.);
		prevPivotDistance = nextPivotDistance = defaultDistance;
		pivotIndex = pivotsDistanceValues.size();
		for (int itPivot = 0; itPivot < pivotsDistanceValues.size(); ++itPivot)
		{
			comparingDistPivot = pivotsDistanceValues.at(itPivot);
			if (distance <= comparingDistPivot)
			{
				nextPivotDistance = comparingDistPivot;
				prevPivotDistance = pivotsDistanceValues.at(itPivot - 1);
				pivotIndex = itPivot;
				break;
			}
			//else if (distance == comparingDistPivot)
			//{
			//	minReflectorEchoLUT.at(itMinRE) = pivotsMinEchoValues.at(itPivot);
			//	maxReflectorEchoLUT.at(itMinRE) = pivotsMaxEchoValues.at(itPivot);
			//	break;
			//}
			else
			{
			}
		}

		if ((prevPivotDistance != defaultDistance) && (nextPivotDistance != defaultDistance) && pivotIndex < pivotsDistanceValues.size())
		{
			// Calculation of the interpolated echo value between two pivots according to the mono logarithmical echo-distance graph in the manual:
			// y = a * X + b ==> with X = log(x) ==> y = a * log(x) + b
			// echo_value = slope * log(distance) + offset
			// ---
			// With the values at adjacent pivots p_j and p_k with x_j < x_k:
			// a = {(y_k - y_j) / [log(x_k) - log(x_j)]}
			// b = y_k - {(y_k - y_j) / [log(x_k) - log(x_j)]} * log(x_k)
			// y = {(y_k - y_j) / [log(x_k) - log(x_j)]} * (log(x) - log(x_k)) + y_k
			// ---
			// y = slope' * (log(x) - log(x_k)) + offset'

			minEcho = ceil( ((float(pivotsMinEchoValues.at(pivotIndex)) - float(pivotsMinEchoValues.at(pivotIndex - 1)))/(log10(nextPivotDistance) - log10(prevPivotDistance))) * (log10(distance) - log10(nextPivotDistance)) ) + pivotsMinEchoValues.at(pivotIndex);

			//prevPvtEcho = pivotsMinEchoValues.at(pivotIndex - 1);
			//nextPvtEcho = pivotsMinEchoValues.at(pivotIndex);
			//slope_numerator = nextPvtEcho - prevPvtEcho;
			//slope_denominator = log10(nextPivotDistance) - log10(prevPivotDistance);
			//log_var = log10(distance) - log10(nextPivotDistance);
			//slope = slope_numerator / slope_denominator;
			//offset = pivotsMinEchoValues.at(pivotIndex);
			//minEcho = ceil( slope * log_var ) + offset;

			maxEcho = floor( (((float)pivotsMaxEchoValues.at(pivotIndex) - (float)pivotsMaxEchoValues.at(pivotIndex - 1))/(log10(nextPivotDistance) - log10(prevPivotDistance))) * (log10(distance) - log10(nextPivotDistance)) ) + pivotsMaxEchoValues.at(pivotIndex);

			minReflectorEchoLUT.at(itMinRE) = minEcho;
			maxReflectorEchoLUT.at(itMinRE) = maxEcho;
		}
	}

}


// returns minimal echo value for reflector object at distance in order to distinguish it from regular surfaces
// distance in [mm]
void getMinReflectorEcho2DistanceValue (const uint32_t & distance, uint32_t & minEcho)
{
	if ((100 <= distance) && (distance <= 100000))
		minEcho = minReflectorEchoLUT.at(distance);
	else
		minEcho = 0; // OR minEcho = INVALID_REFLECTOR_VALUE;
}

// returns maximal echo value for reflector object at distance in order to distinguish it from regular surfaces
// distance in [mm]
void getMaxReflectorEcho2DistanceValue (const uint32_t & distance, uint32_t & maxEcho)
{
	if ((100 <= distance) && (distance <= 100000))
		maxEcho = maxReflectorEchoLUT.at(distance);
	else
		maxEcho = 0; // OR maxEcho = INVALID_REFLECTOR_VALUE;
}


void Init()
{
    glEnable(GL_DEPTH_TEST);
}

void reshape(int w, int h)
{
    // prevent divide by 0 error when minimised
    if(w==0)
        h = 1;

    glViewport(0,0,w,h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45,(float)w/h,1,1000);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void Motion(int x,int y)
{
    int diffx=x-lastx;
    int diffy=y-lasty;
    lastx=x;
    lasty=y;

    if( Buttons[0] && Buttons[1] )
    {
        zoom -= (float) 0.05f * diffx;
    }
    else
        if( Buttons[0] )
        {
            rotx += (float) 0.1f * diffy;
            roty += (float) 0.1f * diffx;
        }
        else
            if( Buttons[1] )
            {
                tx += (float) 0.005f * diffx;
                ty -= (float) 0.005f * diffy;
            }

    glutPostRedisplay();
}

void Mouse(int b,int s,int x,int y)
{
    lastx=x;
    lasty=y;
    switch(b)
    {
    case GLUT_LEFT_BUTTON:
        Buttons[0] = ((GLUT_DOWN==s)?1:0);
        break;
    case GLUT_RIGHT_BUTTON:
        Buttons[1] = ((GLUT_DOWN==s)?1:0);
        break;
    case GLUT_MIDDLE_BUTTON:
        Buttons[2] = ((GLUT_DOWN==s)?1:0);
        break;
    case 3:
        zoom -= 0.5;
        break;
    case 4:
        zoom += 0.5;
        break;
    default:
        break;
    }
    glutPostRedisplay();
}

void keyPress(unsigned char key, int x, int y)
{

    if( key == 13 )
        exit(0);

    if( key == '+')
        zoom--;

    if( key == '-')
        zoom++;

    if( key == '2')
        ty --;

    if( key == '4')
        tx --;

    if( key == '6')
        tx ++;

    if( key == '8')
        ty ++;

    if( key=='l')   //turn on/off lines
    {
        draw_lines = !draw_lines;
    }

    if( key=='a')   //turn on/off amplitude
    {
        draw_ampl = !draw_ampl;
    }

    if( key=='p')   //turn on/off points
    {
        draw_points = !draw_points;
    }

    if( key=='d')
    {
        draw_distance =!draw_distance;
    }

    if ( key=='c')  //turn on/off circle
    {
        draw_circle = !draw_circle;
    }

    if ( key=='g')  //turn on/off grid
    {
        draw_grid = !draw_grid;
    }

    if( key=='r')   //reset orintation
    {
        tx=0, ty=0, rotx=0, roty=0;
    }

    if( key=='t')
    {
        draw_center = !draw_center;
    }

    if( key=='w')
    {
        draw_without =!draw_without;
    }

    if( key=='s')
    {
        save_data =! save_data;
    }

    if( key=='0')
    {
        play_live =! play_live;
    }

    if( key=='1')
    {
        replay =! replay;
    }

    if( key==' ')
    {
        pauseMode =! pauseMode;
    }

    if( key=='n')
    {
        nextWhilePauseMode =! nextWhilePauseMode;
    }

    glutPostRedisplay();

}

void rotation(int key, int x, int y)
{
    if (key == GLUT_KEY_UP )
        h += 90;

    if (key == GLUT_KEY_DOWN )
        h -= 90;

    if (key == GLUT_KEY_LEFT )
        v += 90;

    if (key == GLUT_KEY_RIGHT )
        v -= 90;

    glutPostRedisplay();

}

void idle()
{
    glutPostRedisplay();
    frameCounter ++;
}

void grid()
{
    glBegin(GL_LINES);
    glColor3f(0,0.1,0.1);
    glLineWidth(0.50);
    for(int i=-20;i<=20;++i)
    {
        glVertex3f(i,0,-20);
        glVertex3f(i,0,20);

        glVertex3f(20,0,i);
        glVertex3f(-20,0,i);
    }
    glEnd();
}

void circle()
{
    int i;
    int dots = 1000;
    glPointSize(2);
    for(int r=1; r<=NUM_CIRCLES; r++)
    {
//        int m = r%5;
//        cout<<m<<endl;
        if (r%10 == 0)
        {
            glColor3f(0, 0.5, 0.5);
        }
        else if(r%5==0)// || r==10 || r==15 || r==20 || r==25)
        {
            glColor3f(0, 0.35, 0.35);
        }
        else
        {
            glColor3f(0,0.2,0.2);
        }

            glBegin(GL_POINTS);

            for(i=0; i<=dots; i++)
            {
                glVertex3f(r*sin(2*i*PI/dots), 0, r*cos(2*i*PI/dots));
            }
            glEnd();
    }
}



vector<int> regionQuery(vector<Point2f> *points, Point2f *keypoint, float eps)
{
	float dist;
	vector<int> retKeys;

	for(int i = 0; i< points->size(); i++)
	{   //cout << keypoint->x << " " << points->at(i).x << endl;
		dist = sqrt(pow((keypoint->x - points->at(i).x),2) + pow((keypoint->y - points->at(i).y),2));
        //cout << dist << endl;
		if(dist <= eps && dist != 0.0f)
		{
			retKeys.push_back(i);
		}
	}

	return retKeys;
}


vector< vector<Point2f> > DBSCAN_keypoints(vector<Point2f> *points, float eps, int minPts)
{
//    ofstream new1;
//    new1.open("new1.txt");

	vector< vector<Point2f> > clusters;
	vector<bool> clustered;
    vector<int> noise;
	vector<bool> visited;
	vector<int> neighborPts;
	vector<int> neighborPts_;
	int c;

	//init clustered and visited
	for(int k = 0; k < points->size(); k++)
	{
		clustered.push_back(false);
		visited.push_back(false);
	}

	//C =0;
	c = 0;
	clusters.push_back(vector<Point2f>());

	//for each unvisted point P in dataset
	for(int i = 0; i < points->size(); i++)
	{
		if(!visited[i])
        {
			//Mark P as visited
			visited[i] = true;
			neighborPts = regionQuery(points,&points->at(i),eps);

			if(neighborPts.size() < minPts)
			{
				noise.push_back(i);     //Mark P as Noise
			}

			else
			{
				clusters.push_back(vector<Point2f>());
				c++;
				//expand cluster
				// add P to cluster c
				clusters[c].push_back(points->at(i));
				clustered[i] = true;
				//for each point P' in neighborPts
				for(int j = 0; j < neighborPts.size(); j++)
				{
					//if P' is not visited
					if(!visited[neighborPts[j]])
					{
						//Mark P' as visited
						visited[neighborPts[j]] = true;
						neighborPts_ = regionQuery(points,&points->at(neighborPts[j]),eps);
						if(neighborPts_.size() >= minPts)
						{
							neighborPts.insert(neighborPts.end(),neighborPts_.begin(),neighborPts_.end());
						}
					}
					// if P' is not yet a member of any cluster
					// add P' to cluster c
					if(!clustered[neighborPts[j]])
					{
						clusters[c].push_back(points->at(neighborPts[j]));
						clustered[neighborPts[j]] = true;

//						glBegin(GL_POINTS);
//                        glColor3f(1.0, 1.0, 0.0);
//
//                        for ( int r = 0; r < clusters.size(); r++ )
//                        {
//                            for ( int s = 0; s < clusters[r].size(); ++s )
//                            {
//                                glVertex3f(clusters[r][s].x, 0.0, clusters[r][s].y); //.x, 0.0, clusters[i][j].pt.y);
////                                glVertex3f(last_x, 0.0, last_y);
////                                glVertex3f(0.0, 0.0, 0.0);
//
////                                last_x = clusters[r][s].x;
////                                last_y = clusters[r][s].y;
//                            }
//                        }
//
//                        glEnd();
					}
				}
			}
        }
    }

    return clusters;

//    new1.close();
}



vector<uint32_t> amplitudes (SAMPLE_PER_FRAME, 0);
vector<uint32_t> distances (SAMPLE_PER_FRAME, 0);
vector<float> angles (SAMPLE_PER_FRAME, 0.);


void display()
{
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    glTranslatef(0,0,-zoom);
    glTranslatef(tx,ty,0);
    glRotatef(rotx,0.5,0,0);
    glRotatef(roty,0,0.5,0);
    glRotatef(h,1,0,0);
    glRotatef(v,0,1,0);

    pepperl_fuchs::ScanData myFullScan = driver.getFullScan();
    amplitudes = myFullScan.amplitude_data;
    distances  = myFullScan.distance_data;

    vector<uint32_t>::iterator it_dist;
    vector<uint32_t>::iterator it_ampl;
    vector<float>::iterator it_ang;


    uint32_t i = 0;

    if(draw_circle)         //draw circles with 1 meter distance for each circle
    {
        circle();
    }


    if(draw_grid)           //draw squares with 1 meter distance for each square
    {
        grid();
    }


    if(draw_center)         //draw the center of screen with 3 RGB lines
    {
        glBegin(GL_LINES);
        {
            glLineWidth(1);

            glColor3f(1.0, 0.0, 0.0);
            glVertex3f(1.0, 0.0, 0.0);
            glVertex3f(0.0, 0.0, 0.0);

            glColor3f(0.0, 1.0, 0.0);
            glVertex3f(0.0, 1.0, 0.0);
            glVertex3f(0.0, 0.0, 0.0);

            glColor3f(0.0, 0.0, 1.0);
            glVertex3f(0.0, 0.0, 1.0);
            glVertex3f(0.0, 0.0, 0.0);

        }
        glEnd();
    }


    //vector<uint32_t>::iterator last_valid_dist = distances.begin();


    if(draw_without)
    {
        replace(distances.begin(), distances.end(), 0xFFFFF, 10);
    }


    if(save_data)           //to save data in txt file
    {
        time_t tnow = time(0);   // get time now
        struct tm * now = localtime( & tnow );

        char buffer [80];
        strftime (buffer,80,"%H%M%S",now);
//        string filename="Desk_10sec_test-";
//        filename.append(buffer);
//        filename.append(".txt");
//        pfsave.open(filename, ofstream::app);
        pfsave.open("DR1_10.txt", ofstream::app);
        struct timeval tp;
        gettimeofday(&tp, NULL);
        long int ms = tp.tv_sec * 1000 + tp.tv_usec / 1000;

        pfsave<<ms<<"; ";

        int t=0;

        while( it_dist != distances.end() || it_ampl != amplitudes.end() )
        {
            pfsave<< *it_dist <<" " << *it_ampl<< " " << fixed<<setprecision(3)<< ANGULAR_INCREMENT*t - 180.f << "; ";  //<< t <<"\t"<< dis[t] <<"\t" << amp[t]<<"\t"<<fixed<<setprecision(3)<< ANGULAR_INCREMENT*t << "\t" <<endl;

            it_dist++;
            it_ampl++;
            t++;
        }

        pfsave<<"\n";

        pfsave.close();
    }


    if(draw_lines)          //display in line format
    {
        glColor3f(1,1,1);
        glLineWidth(2);

        glBegin(GL_LINES);

        it_dist = distances.begin();
        it_ampl = amplitudes.begin();
        it_ang = angles.begin();

            while( it_dist != distances.end() || it_ampl != amplitudes.end() )
            {
                GLfloat alpha = float(i) / float( SAMPLE_PER_FRAME ) * (2.0*PI);
                GLfloat x = sin( alpha ) * display_zoom * *it_dist;
                GLfloat y = cos( alpha ) * display_zoom * *it_dist;

                glVertex3f(x, 0.05, y);
                glVertex3f(last_x, 0.05, last_y);

                last_x = x;
                last_y = y;

                i++;
                it_dist++;
                it_ampl++;
            }

        glEnd();
    }


    if(draw_ampl)           //display only high reflective points
    {
        glPointSize(1);
        glBegin(GL_POINTS);

        it_dist = distances.begin();
        it_ampl = amplitudes.begin();
        it_ang = angles.begin();

            while( it_dist != distances.end() || it_ampl != amplitudes.end() )
            {
                GLfloat alpha = float(i) / float(SAMPLE_PER_FRAME) * (2.0*PI);
                GLfloat x = sin( alpha ) * display_zoom * *it_dist;
                GLfloat y = cos( alpha ) * display_zoom * *it_dist;

                getMinReflectorEcho2DistanceValue(*it_dist, minEcho);
                getMaxReflectorEcho2DistanceValue(*it_dist, maxEcho);

                hi_ref_Thresh = (maxEcho - minEcho) * hi_ref + minEcho;
                low_ref_Thresh = (maxEcho - minEcho) * low_ref + minEcho;

                    if (*it_ampl != INVALID_REFLECTOR_VALUE)
                    {
                        if (*it_ampl > hi_ref_Thresh)
                        {
                            glColor3f(1, 0, 0);
                        }
                        else if (*it_ampl > low_ref_Thresh)
                        {
                            glColor3f(0, 1, 0);
                        }
                        else
                        {
                            glColor3f(0, 0, 0);
                        }
                    }

                glVertex3f(x, 0, y);

                if (*it_dist < 50000)
                {
                    bool stop=true;
                }

                i++;
                it_dist++;
                it_ampl++;
                //it_ang++;
            }

        glEnd();
    }


    if(draw_points)         //display the whole point cloud
    {
        glPointSize(3);
        glBegin(GL_POINTS);

        it_dist = distances.begin();
        it_ampl = amplitudes.begin();
        it_ang = angles.begin();

            while( it_dist != distances.end() || it_ampl != amplitudes.end() )
            {
                GLfloat alpha = float(i) / float(SAMPLE_PER_FRAME) * (2.0*PI);
                GLfloat x = sin( alpha ) * display_zoom * *it_dist;
                GLfloat y = cos( alpha ) * display_zoom * *it_dist;

                getMinReflectorEcho2DistanceValue(*it_dist, minEcho);
                getMaxReflectorEcho2DistanceValue(*it_dist, maxEcho);

                hi_ref_Thresh = (maxEcho - minEcho) * hi_ref + minEcho;
                low_ref_Thresh = (maxEcho - minEcho) * low_ref + minEcho;

                    if (*it_ampl != INVALID_REFLECTOR_VALUE)
                    {
                        if (*it_ampl > hi_ref_Thresh)
                        {
                            glColor3f(1, 0, 0);
                        }
                        else if (*it_ampl > low_ref_Thresh)
                        {
                            glColor3f(0, 1, 0);
                        }
                        else
                        {
                            glColor3f(1, 1, 1);
                        }
                    }

                glVertex3f(x, 0, y);

                if (*it_dist < 50000)
                {
                    bool stop=true;
                }

                i++;
                it_dist++;
                it_ampl++;
                //it_ang++;
            }

        glEnd();
    }


    if(draw_distance)       //DBSCAN
    {
        ofstream new2;
        new2.open("new2.txt");

        vector< vector<Point2f> > clusters;
        clusters.clear();

        vector<Point2f> temp_points;
        temp_points.clear();

        it_dist = distances.begin();
        it_ampl = amplitudes.begin();
        it_ang = angles.begin();

        while( it_dist != distances.end() || it_ampl != amplitudes.end() )
        {
            float alpha = float(i) / float(SAMPLE_PER_FRAME) * (2.0*PI);
            float x = sin( alpha ) * display_zoom * *it_dist;
            float y = cos( alpha ) * display_zoom * *it_dist;

            getMinReflectorEcho2DistanceValue(*it_dist, minEcho);
            getMaxReflectorEcho2DistanceValue(*it_dist, maxEcho);

            hi_ref_Thresh = (maxEcho - minEcho) * hi_ref + minEcho;
            low_ref_Thresh = (maxEcho - minEcho) * low_ref + minEcho;

                if (*it_ampl != INVALID_REFLECTOR_VALUE)
                {
                    if (*it_ampl > hi_ref_Thresh)
                    {
                        temp_points.push_back(Point2f(x,y));
                    }
                }

            if (*it_dist < 50000)
            {
                bool stop=true;
            }

            i++;
            it_dist++;
            it_ampl++;
            //it_ang++;
        }

        clusters = DBSCAN_keypoints(&temp_points, 0.05, 10);

        glPointSize(2);
        glBegin(GL_POINTS);
        glColor3f(1.0, 1.0, 0.0);

        for ( int r = 0; r < clusters.size(); r++ )
        {
            for ( int s = 0; s < clusters[r].size(); ++s )
            {
                glVertex3f(clusters[r][s].x, 0.0, clusters[r][s].y); //.x, 0.0, clusters[i][j].pt.y);
            }
        }

        glEnd();

        new2.close();
    }

    glutSwapBuffers();
}


    chrono::time_point<chrono::system_clock> tic;// = chrono::system_clock::now();
    chrono::time_point<chrono::system_clock> toc;

    chrono::time_point<chrono::system_clock> ttlTic;// = chrono::system_clock::now();
    chrono::time_point<chrono::system_clock> ttlToc;


    void rePlay (void);

void replayDisplay ()
{
    rePlay();

    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    glTranslatef(0,0,-zoom);
    glTranslatef(tx,ty,0);
    glRotatef(rotx,0.5,0,0);
    glRotatef(roty,0,0.5,0);
    glRotatef(h,1,0,0);
    glRotatef(v,0,1,0);

    GLfloat last_x = 0;
    GLfloat last_y = 0;

    vector<uint32_t>::iterator it_dist = distances.begin();
    vector<uint32_t>::iterator it_ampl = amplitudes.begin();
    vector<float>::iterator it_ang = angles.begin();

    glPointSize(3);
    glColor3f( 1, 1, 1 );

    if(draw_circle)
    {
        circle();
    }

    if(draw_grid)
    {
        grid();
    }

    glBegin(GL_POINTS);

    while( it_dist!=distances.end() || it_ampl!=amplitudes.end() || it_ang!=angles.end())
    {
        GLfloat myampl = float(*it_ampl) / 600;
        GLfloat alpha = *it_ang / 180.f * PI; //float(i) / float( SAMPLE_PER_FRAME ) * 2.0*PI;
        GLfloat x = sin( alpha ) * display_zoom * *it_dist;
        GLfloat y = cos( alpha ) * display_zoom * *it_dist;

        getMinReflectorEcho2DistanceValue(*it_dist, minEcho);
        getMaxReflectorEcho2DistanceValue(*it_dist, maxEcho);

        hi_ref_Thresh = (maxEcho - minEcho) * hi_ref + minEcho;
        low_ref_Thresh = (maxEcho - minEcho) * low_ref + minEcho;

        if (*it_ampl != INVALID_REFLECTOR_VALUE)
        {
            if (*it_ampl > hi_ref_Thresh)
            {
                glColor3f(1,0,0);
            }
            else if (*it_ampl > low_ref_Thresh)
            {
                glColor3f(0,1,0);
            }
            else
            {
                glColor3f(1, 1, 1);
            }
        }

//        if(myampl>0.5)
//        {
//            glColor3f(0,myampl,0);
//
//            if(myampl>0.8)
//            {
//                glColor3f(myampl,0,0);
//            }
//        }
//        else
//        {
//            glColor3f(1, 1, 1);
//        }

        glVertex3f(x, 0.05, y);
        glVertex3f(last_x, 0.05, last_y);
        glVertex3f(0.0, 0.0, 0.0);

        last_x = x;
        last_y = y;

        if (*it_dist < 50000)
        {
            bool stop=true;
        }
        //i++;
        it_dist++;
        it_ampl++;
        it_ang++;

    }

    glEnd();

    glutSwapBuffers();
}

void rePlay()
{
    //bool pauseMode = false;
    string lineScan;
    string timeStamp;
    string lineScanData;
    string scanDataElement;

//        glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGBA|GLUT_DEPTH);
//        glutInitWindowPosition(2000,100);
//        glutCreateWindow("Pepperl Fuchs");
//        glutSpecialFunc(rotation);
//        glutKeyboardFunc(keyPress);
//        glutFullScreen();
//        glutIdleFunc(idle);
//        glutDisplayFunc(replayDisplay);
//        glutReshapeFunc(reshape);
//        glutMouseFunc(Mouse);
//        glutMotionFunc(Motion);
//        Init();
//
    int counter=0;
    static int64_t lastTimeStamp=-1;
    static int64_t currTimeStamp;
    int64_t reqTimeLapse=1/(float)FREQUENCY * 1000;//in [msec]

    //chrono::time_point<chrono::system_clock>
    tic = chrono::system_clock::now();
    //chrono::time_point<chrono::system_clock> toc;
    bool ifsOpenDbg = pfread.is_open();
    bool ifsEofDbg = pfread.eof();
    if(pfread.is_open() && !pfread.eof())
    {
        if (!pauseMode || (pauseMode && nextWhilePauseMode))
        {
            if (pauseMode && nextWhilePauseMode)
            {
                nextWhilePauseMode = false;
            }
            size_t tsPos;
            size_t prevElPos=0;
            size_t nextElPos;
            getline(pfread, lineScan);
            //istringstream ss(line);
            tsPos = lineScan.find(';');
            if (tsPos != string::npos)
            {
                timeStamp = lineScan.substr(0, tsPos);
                //if (lineScan.begin() + tsPos + 2 < lineScan.end())
                    lineScanData = lineScan.substr(tsPos+2);

                istringstream ss_ts(timeStamp);
                int64_t ts64;
                ss_ts >> ts64;
                currTimeStamp = ts64;
                int iter_sample=0;

                while((nextElPos = lineScanData.find(';', prevElPos)) != string::npos)
                {
                    scanDataElement = lineScanData.substr(prevElPos, nextElPos - prevElPos);
                    istringstream ss(scanDataElement);

            //            vector<int> dist;
            //            vector<int> amp;
            //            vector<float> ang;


                    uint32_t dist;
                    uint32_t amp;
                    float ang;


                    ss>>dist>>amp>>ang;

                    if (iter_sample < SAMPLE_PER_FRAME)
                    {
                        distances.at(iter_sample) = dist;
                        amplitudes.at(iter_sample) = amp;
                        angles.at(iter_sample) = ang;
                    }

                    temp<<ang<<" ";

                    prevElPos = nextElPos + 2;
                    iter_sample++;

                }
                temp << "\n";
                reqTimeLapse = (lastTimeStamp == -1) ? reqTimeLapse : (currTimeStamp - lastTimeStamp);
                lastTimeStamp = currTimeStamp;
            }
        }


        auto duration = tic.time_since_epoch();
        auto tic_millisec = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

        toc = chrono::system_clock::now();
        duration = toc.time_since_epoch();
        auto toc_millisec = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

        auto tictoc_millisec = toc_millisec - tic_millisec;

        auto sleep_duration = reqTimeLapse - tictoc_millisec - 3;//offset

        if (sleep_duration > 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_duration));
        }

        //cout << "lastTS (ms) = " << lastTimeStamp << "\ncurrTS (ms) = " << currTimeStamp << "\ntictoc (ms) = " << tictoc_millisec << "\nreqTimeLapse (ms) = " << reqTimeLapse << "\nsleep duration (ms): " << sleep_duration << "\n" << endl;



        // Drawing routine
        //glutDisplayFunc(replayDisplay);
        //replayDisplay();
        tic = toc;
    }
    else if (pfread.eof())
    {
        ttlToc = chrono::system_clock::now();
        auto ttlticms = chrono::duration_cast<chrono::milliseconds>(ttlTic.time_since_epoch()).count();
        auto ttltocms = chrono::duration_cast<chrono::milliseconds>(ttlToc.time_since_epoch()).count();
        auto ttldura = (ttltocms - ttlticms) / 1000.;
        cout << "ttlTic (ms) = " << ttlticms << "\nttlToc (ms) = " << ttltocms << "\nduration replay cycle (s): " << ttldura << "\n" << endl;
        pfread.clear();
        pfread.seekg(0,ios::beg);
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
        ttlTic = chrono::system_clock::now();
//        pfread.close();
//        temp.close();
    }

    //glutMainLoop();
    //Init();

}

int main(int argc, char** argv)
{
    initPivots4LUT();
    createEcho2DistanceLUTs();

    char c;

    cout<<"press 0 for live data \npress 1 for replay"<<endl;
    cin>>c;

    if(c=='0')
    {
        driver.connect("10.0.10.9");
        driver.setScanFrequency( FREQUENCY );
        driver.setSamplesPerScan( SAMPLE_PER_FRAME );
        driver.startCapturingUDP();
        glutInit(&argc,argv);

        glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGBA|GLUT_DEPTH);
        glutInitWindowPosition(2000,100);
        glutCreateWindow("Pepperl Fuchs");
        glutSpecialFunc(rotation);
        glutKeyboardFunc(keyPress);
        //glutInitWindowSize(200,200);
        glutFullScreen();
        glutIdleFunc(idle);
        glutDisplayFunc(display);
        glutReshapeFunc(reshape);
        glutMouseFunc(Mouse);
        glutMotionFunc(Motion);
        glutMainLoop();
        Init();

        driver.stopCapturing();
    }

    if(c=='1')
    {
//         initPivots4LUT();
//         createEcho2DistanceLUTs();
        //string fbasepath="../../Temp/Dynamic_Tests/";
//        string fname;
//        cout << "Enter file name: " << endl;
//        cin >> fname;
        //glutInit(&argc,argv);
        temp.open("temp.txt");
//        if (fname.compare(""))
//        {
            pfread.open("DR1_1050kmh.txt");
//        }
//        else
//        {
//            //string fullfname = fbasepath.append(fname);
//            pfread.open(fname);//fullfname);
//        }

        tic = chrono::system_clock::now();
        ttlTic = chrono::system_clock::now();
        //rePlay();

        glutInit(&argc,argv);
        glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGBA|GLUT_DEPTH);
        glutInitWindowPosition(2000,100);
        glutCreateWindow("Pepperl_Fuchs_Replay");
        glutSpecialFunc(rotation);
        glutKeyboardFunc(keyPress);
        glutFullScreen();
        glutIdleFunc(idle);
        glutDisplayFunc(replayDisplay);
        glutReshapeFunc(reshape);
        glutMouseFunc(Mouse);
        glutMotionFunc(Motion);
        glutMainLoop();
        Init();
        pfread.close();
        temp.close();

    }

    return 0;
}
