
#include "r2000_driver.h"
#include <iostream>
#include <math.h>
#include <numeric>
#include <vector>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <GL/glut.h>
#include <GL/glu.h>
using namespace std;

#define FREQUENCY 20
#define SAMPLE_PER_FRAME 12600
#define PI 3.1415927

pepperl_fuchs::R2000Driver driver;

float zoom = 25.0f;

float display_zoom = 0.001;

float v = 0;
float h = 0;
float rotx = 0;
float roty = 0.001f;
float tx = 0;
float ty = 0;
int lastx = 0;
int lasty = 0;
unsigned char Buttons[3] = {0};

uint32_t frameCounter = 0;

bool draw_lines = false;
bool draw_ampl = false;
bool draw_points = false;
bool draw_polygons = false;
bool draw_circle = true;
bool draw_grid = false;
bool draw_center = false;
bool draw_without = false;
bool draw_distance = false;

ofstream pfscan;

//-------------------------------------------------------------------------------
/// \brief  Initialises the openGL scene
///
void Init()
{
    glEnable(GL_DEPTH_TEST);
}

//-------------------------------------------------------------------------------
/// \brief  Called when the screen gets resized
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

/// \brief Zoom and transfer function
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

    if( key=='f')   //turn on/off polygon
    {
        draw_polygons= !draw_polygons;
    }

    if ( key=='p')  //turn on/off points
    {
        draw_points = !draw_points;
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

    if( key=='d')
    {
        draw_distance =!draw_distance;
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
    glColor3f(0,0.2,0.2);
    glPointSize(2);
    for(int r=1; r<=10; r++)
    {
        glBegin(GL_POINTS);

        for(i=0; i<=dots; i++)
        {
            glVertex3f(r*sin(2*i*PI/dots), 0, r*cos(2*i*PI/dots));
        }
        glEnd();
    }
}

//-------------------------------------------------------------------------------
/// \brief  Draws the scene
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

    GLfloat last_x = 0;
    GLfloat last_y = 0;

    uint32_t i = 0;

    pfscan.open("pfdata_1");

    if(draw_circle)
    {
        circle();
    }

    if(draw_grid)
    {
        grid();
    }

    pepperl_fuchs::ScanData myFullScan = driver.getFullScan();
    vector<uint32_t> amplitudes = myFullScan.amplitude_data;
    vector<uint32_t> distances  = myFullScan.distance_data;

    vector<uint32_t>::iterator dist = distances.begin();
    vector<uint32_t>::iterator ampl = amplitudes.begin();

    vector<uint32_t>::iterator last_valid_dist = distances.begin();


    if(draw_without)
    {
        replace(distances.begin(), distances.end(), 0xFFFFF, 10);
    }

//    replace(distances.begin(), distances.end(), 0xFFFFF, 50);
//    distances.erase(remove(distances.begin(), distances.end(), 0xFFFFF), distances.end());

    if(draw_lines)
    {
        glColor3f(1,1,1);
        glLineWidth(3);

        glBegin(GL_LINES);
        {
            while( dist!=distances.end() || ampl!=amplitudes.end() )
            {
                GLfloat alpha = float(i) / float( SAMPLE_PER_FRAME ) * 2.0*PI;
                GLfloat x = sin( alpha ) * display_zoom * *dist;
                GLfloat y = cos( alpha ) * display_zoom * *dist;

                glVertex3f(x, 0.05, y);
                glVertex3f(last_x, 0.05, last_y);

                last_x = x;
                last_y = y;

                i++;
                dist++;
                ampl++;
            }
        }
        glEnd();
    }


    if(draw_ampl)
    {
        glLineWidth(3);
        dist = distances.begin();
        ampl = amplitudes.begin();

        glBegin(GL_POINTS);
        {
            while( dist!=distances.end() || ampl!=amplitudes.end() )
            {
                GLfloat myampl = float(*ampl) / 600.;
                GLfloat alpha = float(i) / float( SAMPLE_PER_FRAME ) * 2.0*PI;
                GLfloat x = sin( alpha ) * display_zoom * *dist;
                GLfloat y = cos( alpha ) * display_zoom * *dist;

                if(myampl>0.5)
                {
                    glColor3f(0,1,0);

                    if(myampl>0.8)
                    {
                        glColor3f(1,0,0);
                    }
                }
                else
                {
                    //glColor3f(1,1,1);
                    glColor3f(0,0,0);
                    //glColor3f(myampl, myampl, myampl);
                }
                glVertex3f(x, 0, y);
                glVertex3f(last_x, 0, last_y);

                last_x = x;
                last_y = y;

                i++;
                dist++;
                ampl++;
            }
        }
        glEnd();
    }


    if(draw_polygons)
    {
        dist = distances.begin();
        ampl = amplitudes.begin();

        glColor3f( 0.3, 0.3, 0.3 );

        glBegin(GL_TRIANGLES);
        {
            while( dist!=distances.end() || ampl!=amplitudes.end() )
            {
                GLfloat alpha = float(i) / float( SAMPLE_PER_FRAME ) * 2.0*PI;
                GLfloat x = sin( alpha ) * display_zoom * *dist;
                GLfloat y = cos( alpha ) * display_zoom * *dist;

                glVertex3f(x, 0.05, y);
                glVertex3f(last_x, 0.05, last_y);
                glVertex3f(0.0, 0.0, 0.0);

                last_x = x;
                last_y = y;

                i++;
                dist++;
                ampl++;

            }

        }
        glEnd();
    }


    if(draw_distance)
    {
        dist = distances.begin();
        ampl = amplitudes.begin();
        glColor3f(1, 1, 1);

            while( dist!=distances.end() || ampl!=amplitudes.end() )
            {
                float myampl = float(*ampl) / 600;
                float mydis = float(*dist) / 1000;
                GLfloat alpha = float(i) / float( SAMPLE_PER_FRAME ) * 2.0*PI;
//                GLfloat x = sin( alpha ) * display_zoom * *dist;
//                GLfloat y = cos( alpha ) * display_zoom * *dist;

                //pfscan<<setfill('0')<<setw(8)<<mydis<<"\t"<<setfill('0')<<setw(8)<<myampl<<"\t"<<setfill('0')<<setw(8)<<x<<"\t"<<setfill('0')<<setw(8)<<y<<endl;

                char c[3];
                sprintf(c, "%.2f", mydis);

                if(myampl>0.8)
                {
                    if(distances[i]>=distances[i+1])
                    {
                        if((distances[i]-distances[i+1])<500)
                            {
                                vector<uint32_t> dist3 = distances;

                                double sum = accumulate(dist3.begin(), dist3.end(), 0.0);
                                double mean = sum/dist3.size();

                                double sq_sum = std::inner_product(dist3.begin(), dist3.end(), dist3.begin(), 0.0);
                                double stdev = std::sqrt(sq_sum / dist3.size() - mean * mean);

                                cout<<mean<<"\t"<<stdev<<"\t"<<sq_sum<<endl;

                                GLfloat x = sin( alpha ) * display_zoom * dist3[1];
                                GLfloat y = cos( alpha ) * display_zoom * dist3[1];

//                                while( dist3!=distances.end() )
//                                {
                                    float mydis3 = float(mean) / 1000;

                                    char c3[5];
                                    sprintf(c3, "%.2f", mydis3);

                                    glRasterPos3f(x, 0, y);
                                    const char *p = c3;
                                    do glutBitmapCharacter( GLUT_BITMAP_TIMES_ROMAN_24, *p ); while(*(++p));

                                    last_x = x;
                                    last_y = y;

                                //}
                            }
                    }

                    if(distances[i]<distances[i+1])
                    {
                        if((distances[i+1]-distances[i])<500)
                        {
                                vector<uint32_t> dist3 = distances;

                                double mean = (accumulate(dist3.begin(), dist3.end(), 0.0))/dist3.size();

                                cout<<mean<<endl;

                                GLfloat x = sin( alpha ) * display_zoom * mean;
                                GLfloat y = cos( alpha ) * display_zoom * mean;

//                                while( dist3!=distances.end() )
//                                {
                                    float mydis3 = float(mean) / 1000;

                                    char c3[5];
                                    sprintf(c3, "%.2f", mydis3);

                                    glRasterPos3f(x, 0, y);
                                    const char *p = c3;
                                    do glutBitmapCharacter( GLUT_BITMAP_TIMES_ROMAN_24, *p ); while(*(++p));

                                    last_x = x;
                                    last_y = y;

                        }
                    }

                    //distances[i] = distances[i+1];
                }

//                last_x = x;
//                last_y = y;

                i++;
                dist++;
                ampl++;

            }
    }


    if(draw_points)
    {
        dist = distances.begin();
        ampl = amplitudes.begin();

        glPointSize(3);
        glColor3f( 1, 1, 1 );

        glBegin(GL_POINTS);
        {
            while( dist!=distances.end() || ampl!=amplitudes.end())
            {
                GLfloat myampl = float(*ampl) / 600;
                GLfloat alpha = float(i) / float( SAMPLE_PER_FRAME ) * 2.0*PI;
                GLfloat x = sin( alpha ) * display_zoom * *dist;
                GLfloat y = cos( alpha ) * display_zoom * *dist;

                if(myampl>0.5)
                {
                    glColor3f(0,myampl,0);

                    if(myampl>0.8)
                    {
                        glColor3f(myampl,0,0);
                    }
                }
                else
                {
                    glColor3f(1, 1, 1);
                }

                glVertex3f(x, 0.05, y);
                glVertex3f(last_x, 0.05, last_y);
                glVertex3f(0.0, 0.0, 0.0);

                last_x = x;
                last_y = y;

                i++;
                dist++;
                ampl++;

            }

        }
        glEnd();
    }


    /// Display the center of window

    if(draw_center)
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

    glutSwapBuffers();
    //glutPostRedisplay();

    pfscan.close();
}

//-------------------------------------------------------------------------------
///
int main(int argc, char** argv)
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
    //glutFullScreen();
    glutIdleFunc(idle);
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(Mouse);
    glutMotionFunc(Motion);

    Init();

    glutMainLoop();
}
