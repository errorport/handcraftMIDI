#include <opencv/cv.h>
#include <opencv/cxcore.h>
#include <opencv/highgui.h>
#include <opencv/cvwimage.h>
//#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>
#include <queue>
#include <cmath>
#include "rtmidi/RtMidi.h"
#include "signal_producing.cpp"

#define SUBFRAME_BIT_DEPTH   8      //bit depth of the subframe source
#define SUBFRAME_CHANNELS    1      //channels of the subframe source (original:3)

#define GRAY_BIT_DEPTH       8      //bit depth of the gray source
#define GRAY_CHANNELS        1      //channels of the gray source

#define GRAY_FRAME_WIDTH     270    //gray frame size
#define GRAY_FRAME_HEIGHT    270    //gray frame size

#define GRAY_FRAME_SIZE_FROM_ROI  true //in this case the grayFrame's size will be inherited from ROI

#define ROI_COORDINATE_X     240    //Region of Interest coordinate: X
#define ROI_COORDINATE_Y     10    //Region of Interest coordinate: Y

#define ROI_SIZE_X           370    //Region of Interest size: X -- may be eq to GRAY_FRAME_WIDTH
#define ROI_SIZE_Y           300    //Region of Interest size: Y -- may be eq to GRAY_FRAME_HEIGHT

#define BLUR_APERATURE_SIZE   20    //
#define BIN_THRESH            0

#define INVBIN                false // for rebinarizing the binarized image

#define SHOWSRCFIRST          false
#define SHOWMAIN              true
#define SHOWSRC               true
#define SHOWBIN               true  // for showing the binarized image

#define SHOWPOS               false //  write to >>1 the position of fingers
#define SHOWROI               true // to show the ROI on the sourceFrame
#define SHOW_CENTER           true
#define SHOW_THETA            true
#define SHOW_PROXIMITY        true

#define REINIT_DIRECTION      false

#define DEFULT_DIRECTION      0
#define PI                    3.14159265


#define PROXIMITY_CONTROL_SCALE       42000.0 // for normalize the proximity control value - it have to be the maximum
#define PROXIMITY_CONTROL_RANGE       127 // for normalize the proximity control value - it have to be the maximum
#define INVERT_PROXIMITY_CONTROL      false
#define PROXIMITY_STACK_SIZE          10

#define X_CONTROL_OFFSET              -20      //inductive
#define X_CONTROL_RANGE               7
#define INVERT_X_CONTROL              true
#define X_CONTROL_INCREMENT           7
#define X_CONTROL_STACK_SIZE          20

#define Y_CONTROL_OFFSET              -30      //  inductive
#define Y_CONTROL_RANGE               7
#define INVERT_Y_CONTROL              true
#define Y_CONTROL_INCREMENT           14
#define Y_CONTROL_STACK_SIZE          20

#define THETA_CONTROL_OFFSET          35         // inductive
#define THETA_CONTROL_SCALE           60
#define THETA_CONTROL_RANGE           127
#define INVERT_THETA_CONTROL          false
#define THETA_CONTROL_STACK_SIZE      10
#define THETA_CONTROL_CC              5         //it is a MIDI CC address

#define FINGER_CONTROL_CC             6

#define FIRST_NOTE                    21       //
#define MIN_VELO                      70        //minimum velocity to send
#define X_PITCH_OFFSET                24
#define Y_PITCH_OFFSET                12



using namespace     std;
using namespace     cv;

//the capture itself
CvCapture*   capture;

//hand recognition frame size
CvSize       sizeOfCapturing;

//hand recognition frame
IplImage*    sourceFrame;

//binarized subframe
IplImage*           grayFrame;

//first contour for cvFindContours()
CvSeq*              first_contour;

//number for maximum contours for cvFindContours()
CvSeq*              maxitem;

//for counting the contours
int                 contourNumber;

//area of the hand
double              area;
double              max_area;

//threshold for binarization
int                 threshold;

char trackbar_text[50];

CvScalar ROIcolor;

//
CvSeq* ptr;

//
CvPoint pt0;

CvMemStorage* storage1;
CvMemStorage* storage2;

vector<Point> points;

int meanX, meanY;
int nomdef; // number of defections
int delta_x, delta_y, theta;

//controls
int proximity_control, x_control, y_control, theta_control, finger_control;
//it will be useful for calculating moving average

//these vars will contain the actual MA values (after a test it will be dropped)
int proximity_MA, x_control_MA, y_control_MA, theta_control_MA;

//these vars are for moving average FIFO stacks
deque<int> x_control_stack, y_control_stack, proximity_control_stack, theta_control_stack;

//these vars for moving average stack filling untill they got real value
//not yet actual
//have to be initialized!
//int x_control_MA_index, y_control_MA_index, proximity_control_MA_index, theta_control_MA_index;


RtMidiOut *midiout;
//vector<unsigned char> MIDImessage;
unsigned int nPorts;

int actual_note_X, actual_note_Y;

//initializing, running
void run()
{
  //INIT SEQUENCE
            cout << "\n";
            cout << "Initializing..." << endl;

            //init control stacks for moving average
            init_stack(x_control_stack, X_CONTROL_STACK_SIZE);
            init_stack(y_control_stack, Y_CONTROL_STACK_SIZE);
            init_stack(proximity_control_stack, PROXIMITY_STACK_SIZE);
            init_stack(theta_control_stack, THETA_CONTROL_STACK_SIZE);

            actual_note_X = actual_note_Y = 0;


            capture = cvCaptureFromCAM(0);
            //
            /*
            if(!cvQueryFrame(capture))
          	{
          		cout<<"\nCapturing started."<<endl;
          	}
          	else
          	{
          		cout<<"\nVideo capturing failed."<<endl;
          	}
            */
            //setting up the size of the hand recognition box
            sizeOfCapturing = cvGetSize(cvQueryFrame(capture));
            cout  << "Hand recognition subframe size: "
                  << sizeOfCapturing.width
                  <<"x"
                  <<sizeOfCapturing.height
                  <<"px"
                  << endl;
   //eof Initializing

  //define: subframe and grayframe
  sourceFrame   =   cvCreateImage(sizeOfCapturing,
                               SUBFRAME_BIT_DEPTH,
                               SUBFRAME_CHANNELS);

  #if GRAY_FRAME_SIZE_FROM_ROI == true
  grayFrame  =   cvCreateImage(cvSize(ROI_SIZE_X , ROI_SIZE_Y),
                               GRAY_BIT_DEPTH,
                               GRAY_CHANNELS);
  cout << "\ngrayFrame size has been inherited from ROI as: "
          << "width: " << ROI_SIZE_X << "px, "
          << "height: " << ROI_SIZE_Y << "px" << endl;
  #else
  grayFrame  =   cvCreateImage(cvSize(GRAY_FRAME_WIDTH , GRAY_FRAME_HEIGHT),
                               GRAY_BIT_DEPTH,
                               GRAY_CHANNELS);
  cout << "\ngrayFrame size has been given by macro as: "
          << "width: " << GRAY_FRAME_WIDTH << "px, "
          << "height: " << GRAY_FRAME_HEIGHT << "px" << endl;
  #endif

  // Check available ports.
  midiout = new RtMidiOut();
  unsigned int nPorts = midiout->getPortCount();
  if ( nPorts == 0 ) {
    cout << "No ports available! Check the ports' usage!\nHALTED!\n";
    exit( EXIT_FAILURE );
  }

  // Open first available port.
  try {
    midiout->openPort( 0 );
  }
  catch ( RtMidiError &error ) {
    error.printMessage();
    cout << "Failed to open the available MIDI port! Check the ports' usage!\nHALTED!\n";
    exit( EXIT_FAILURE );
  }
  //EOfMIDI section ------------------------------------------------------------

  threshold = BIN_THRESH;

  //main cycle
  while(cvWaitKey(100)!=27)
  {

    //reinit SEQUENCE
    first_contour = NULL;
    maxitem       = NULL;
    contourNumber = 0;
    area=max_area = 0.0;
    ptr           = 0;
    meanX = meanY = 0;
    nomdef        = 0;
    #if REINIT_DIRECTION == true
    delta_x = delta_y = 0;
    theta   = DEFULT_DIRECTION;
    #endif
    //eof reinit


    //capturing a frame from camera
    sourceFrame  =   cvQueryFrame(capture);
    #if SHOWSRCFIRST == true
      //show it
      cvNamedWindow("sourceFrame",1);
      cvShowImage("sourceFrame", sourceFrame);
    #endif


    //setting up the region of interest
    cvSetImageROI(sourceFrame,
                  cvRect(ROI_COORDINATE_X,
                         ROI_COORDINATE_Y,
                         ROI_SIZE_X,
                         ROI_SIZE_Y)
                 );

    //making the actual grayFrame
    cvCvtColor(sourceFrame, grayFrame, CV_BGR2GRAY);

    //smoothing the grayFrame by bluring
    cvSmooth(grayFrame, grayFrame, CV_BLUR, BLUR_APERATURE_SIZE, 0);

    //binarizing
    cvThreshold(grayFrame, grayFrame, threshold, 255, (CV_THRESH_BINARY_INV+CV_THRESH_OTSU));
          cout << "Threshold: " << threshold << endl;
    #if INVBIN == true
      cvThreshold(grayFrame, grayFrame, 250, 255,(CV_THRESH_BINARY_INV));
    #endif

    //show the binarized image
    #if SHOWBIN == true
      cvNamedWindow("Threshold",1);
      cvShowImage( "Threshold",grayFrame);
    #endif

    CvMemStorage* storage = cvCreateMemStorage();

    //setting up the contours
    contourNumber         = cvFindContours(grayFrame,
                                           storage,
                                           &first_contour,
                                           sizeof(CvContour),
                                           CV_RETR_CCOMP,CV_CHAIN_APPROX_SIMPLE,
                                           cvPoint(0,0)
                                          );


    //if any contours has been found
    area = max_area = 0.0;

    if(contourNumber>0)
    {
      for(ptr=first_contour; ptr!=NULL; ptr=ptr->h_next)
			{
				area=fabs(cvContourArea(ptr,CV_WHOLE_SEQ,0));
				if(area>max_area)
				{
					max_area=area;
					maxitem=ptr;
				}
			}

      if(max_area > 1000)
			{

				storage1 = cvCreateMemStorage();
        storage2 = cvCreateMemStorage(0);

				CvSeq* ptseq = cvCreateSeq( CV_SEQ_KIND_GENERIC|CV_32SC2, sizeof(CvContour),sizeof(CvPoint), storage1 );
				CvSeq* hull = NULL;
				CvSeq* defects;
				for(int i = 0; i < maxitem->total; i++ )
				{
					CvPoint* p = CV_GET_SEQ_ELEM( CvPoint, maxitem, i );
					pt0.x = p->x;
					pt0.y = p->y;
					cvSeqPush( ptseq, &pt0 );
				}

				hull = cvConvexHull2( ptseq, 0, CV_CLOCKWISE, 0 );

				int hullcount = hull->total;
				defects= cvConvexityDefects(ptseq,hull,storage2  );
				CvConvexityDefect* defectArray;
				// int j=0;
				for(int i = 1; i <= hullcount; i++ )
				{
					CvPoint pt = **CV_GET_SEQ_ELEM( CvPoint*, hull, i );
					cvLine( sourceFrame, pt0, pt, CV_RGB( 255, 0, 0 ), 1, CV_AA, 0 );
          meanX+=pt0.x;
          meanY+=pt0.y;
					pt0 = pt;
				}

				for( ; defects; defects = defects->h_next)
				{
					nomdef = defects->total; // defect amount
					// outlet_float( m_nomdef, nomdef );
					// printf(" defect no %d \n",nomdef);
					if(nomdef == 0)
					continue;
					// Alloc memory for defect set.
					// fprintf(stderr,"malloc\n");
					defectArray = (CvConvexityDefect*)malloc(sizeof(CvConvexityDefect)*nomdef);
					// Get defect set.
					// fprintf(stderr,"cvCvtSeqToArray\n");
					cvCvtSeqToArray(defects,defectArray, CV_WHOLE_SEQ);
					// Draw marks for all defects.
					int con=0;
					for(int i=0; i<nomdef; i++)
					{
						if(defectArray[i].depth > 40 )
						{
							con=con+1;
							// printf(" defect depth for defect %d %f \n",i,defectArray[i].depth);
							cvLine(sourceFrame, *(defectArray[i].start), *(defectArray[i].depth_point),CV_RGB(255,255,255),1, CV_AA, 0 );
							cvCircle( sourceFrame, *(defectArray[i].depth_point), 5, CV_RGB(0,0,255), 2, 8,0); // finger endpoints
              #if SHOWPOS == true
							       cout << i << ": x: "<<defectArray[i].depth_point->x << "\n";
							       cout << i << ": y: "<<defectArray[i].depth_point->y << "\n";
              #endif

              //calculating the direction
              delta_x = defectArray[i].depth_point->x - defectArray[i].start->x;
              delta_y = defectArray[i].depth_point->x - defectArray[i].start->x;
              //cout << delta_x << endl;
              theta   += (int)(atan2(delta_x, delta_y) * 180 / PI);


							cvCircle( sourceFrame, *(defectArray[i].start), 5, CV_RGB(255,255,0), 2, 8,0); // inbetween finger points
							cvLine( sourceFrame, *(defectArray[i].depth_point), *(defectArray[i].end),CV_RGB(0,255,255),1, CV_AA, 0 );
							//cvDrawContours( sourceFrame ,defects,CV_RGB(0,0,0),CV_RGB(255,0,255),-1,CV_FILLED,8);

						}
					}
          finger_control = con;

					// cout<<con<<"\n";
					char txt[40]="";
					if(con==1)
					{
						char txt1[]="f:2";
						strcat(txt,txt1);
					}
					else if(con==2)
					{
						char txt1[]="f:3";
						strcat(txt,txt1);
					}
					else if(con==3)
					{
						char txt1[]="f:4";
						strcat(txt,txt1);
					}
					else if(con>=4)
					{
						char txt1[]="f:5";
						strcat(txt,txt1);
					}
					else
					{
						char txt1[]="f:0"; // Jarvis can't recognise you
						strcat(txt,txt1);
					}
          #if SHOWSRC == true
					     cvNamedWindow( "contour",1);cvShowImage( "contour", sourceFrame);
          #endif

					cvResetImageROI(sourceFrame);
					/*CvFont font;
					cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 1.5, 1.5, 0, 5, CV_AA);
					cvPutText(sourceFrame, txt, cvPoint(50, 50), &font, cvScalar(0, 0, 255, 0));*/
					//
					//drop event here! ---------------------------------------

          //Free memory

					free(defectArray);
				}
				cvReleaseMemStorage( &storage1 );
				cvReleaseMemStorage( &storage2 );
			}

    }

    // ------------------------------
    cvReleaseMemStorage( &storage );

    //drawing the ROI edges onto the main frame
    #if INVBIN == false
    ROIcolor = CV_RGB(0,0,0);
    #else
    ROIcolor = CV_RGB(255,255,255);
    #endif
    #if SHOWROI == true
      cvLine(sourceFrame,
             cvPoint(ROI_COORDINATE_X, ROI_COORDINATE_Y),
             cvPoint(ROI_COORDINATE_X, ROI_COORDINATE_Y+ROI_SIZE_Y),
             ROIcolor,1 , CV_AA, 0
            );
      cvLine(sourceFrame,
             cvPoint(ROI_COORDINATE_X, ROI_COORDINATE_Y+ROI_SIZE_Y),
             cvPoint(ROI_COORDINATE_X+ROI_SIZE_X, ROI_COORDINATE_Y+ROI_SIZE_Y),
             ROIcolor,1 , CV_AA, 0
            );
      cvLine(sourceFrame,
             cvPoint(ROI_COORDINATE_X+ROI_SIZE_X, ROI_COORDINATE_Y+ROI_SIZE_Y),
             cvPoint(ROI_COORDINATE_X+ROI_SIZE_X, ROI_COORDINATE_Y),
             ROIcolor,1 , CV_AA, 0
            );
      cvLine(sourceFrame,
             cvPoint(ROI_COORDINATE_X+ROI_SIZE_X, ROI_COORDINATE_Y),
             cvPoint(ROI_COORDINATE_X, ROI_COORDINATE_Y),
             ROIcolor,1 , CV_AA, 0
            );
    #endif



    //making the control signals -----------------------------------------------

    // ~ int proximity_control, x_control, y_control, theta_control, finger_control;
    /*
    Control signal normaliing method:
    1., set the interval
    2., set the offset
    3., set the moving average
    4., set the MIDI signal
    5., send the events
    */
    /*

    */

    //get the proximity of the hand --------------------------------------------
    //not strict enough
    //need moving avg too
    /*proximity_control = (int)(max_area/PROXIMITY_CONTROL_SCALE*PROXIMITY_CONTROL_RANGE);
    if(proximity_control > PROXIMITY_CONTROL_RANGE){proximity_control=PROXIMITY_CONTROL_RANGE;}
    #if INVERT_PROXIMITY_CONTROL == true
      proximity_control=PROXIMITY_CONTROL_RANGE - proximity_control;
    #endif*/





    proximity_control = linear_signal_convert((int)max_area,
                                              0,
                                              PROXIMITY_CONTROL_SCALE,
                                              PROXIMITY_CONTROL_RANGE,
                                              INVERT_PROXIMITY_CONTROL
                                            );





    //got the center -----------------------------------------------------------
    //not strict enough
    //moving avg needed for smooth movement of this point
 /*
    vector<Point2f> centerP;
    minEnclosingCircle(*first_contour, &centerP, 10);
*/

    if(nomdef>0){
      meanX/=nomdef; meanX+=ROI_COORDINATE_X;
      meanY/=nomdef; meanY+=ROI_COORDINATE_Y;
    }
    else
    {
      meanX=meanY =0;
    }



    //normalizing by interval
    /*x_control = (meanX - ROI_COORDINATE_X)/ROI_SIZE_X*X_CONTROL_RANGE;
    if(x_control > X_CONTROL_RANGE) {x_control= X_CONTROL_RANGE;}*/



    if(meanX>ROI_COORDINATE_X and meanY>ROI_COORDINATE_Y)
    {
    x_control = linear_signal_convert(meanX,
                                      - ROI_COORDINATE_X,
                                      ROI_SIZE_X + X_CONTROL_OFFSET,
                                      X_CONTROL_RANGE,
                                      INVERT_X_CONTROL,
                                      X_CONTROL_INCREMENT
                                     );
    y_control = linear_signal_convert(meanY,
                                      - ROI_COORDINATE_Y,
                                      ROI_SIZE_Y + Y_CONTROL_OFFSET,
                                      Y_CONTROL_RANGE,
                                      INVERT_Y_CONTROL,
                                      Y_CONTROL_INCREMENT
                                     );
    }





    //calculating the direction ------------------------------------------------
    if(nomdef>0){
    theta = theta/nomdef;

    theta_control = linear_signal_convert(theta,
                                          THETA_CONTROL_OFFSET,
                                          THETA_CONTROL_SCALE,
                                          THETA_CONTROL_RANGE,
                                          INVERT_THETA_CONTROL
                                        );
    }
    else
    {
      theta_control=0;
    }

    //geting moving average for control vars

    x_control_MA      =x_control;  //= moving_average(x_control_stack, x_control);
    y_control_MA      =y_control;  //= moving_average(y_control_stack, y_control);
    proximity_MA        = moving_average(proximity_control_stack, proximity_control);
    theta_control_MA    = moving_average(theta_control_stack, theta_control);

    //showing control and MA data

    #if SHOW_CENTER == true
    cout << "Center: x==" << meanX << " y==" << meanY << endl;
    cout << "x_control: " << x_control << " y_control: " << y_control << endl;
    cout << "x_control_MA, y_control_MA : (" << x_control_MA << ", " << y_control_MA << ")" << endl;
    cvCircle( sourceFrame, cvPoint(x_control_MA, y_control_MA), 10, CV_RGB(255,255,255), 2, 8,0);
    #endif

    #if SHOW_THETA == true
    cout << "Direction angle: " << theta << "°" << endl;
    cout << "Theta control: "   << theta_control << endl;
    cout << "theta_control_MA: (" << theta_control_MA << ")" << endl;
    #endif

    #if SHOW_PROXIMITY == true
      cout << "rel_maxarea: " << max_area << endl;
      cout << "proximity_control: " << proximity_control  << endl;
      cout << "proximity_MA: (" << proximity_MA << ")" << endl;
    #endif

    //number of fingers --------------------------------------------------------
    #if SHOW_FINGERS == true
    cout << "Number of found fingers: " << con << endl;
    #endif

    //making the MIDI signal ---------------------------------------------------

      //velocity <- proximity_MA
      //note1 <- x_control_MA
      //note2 <- y_control_MA
      //sustain


      terminateMidiNote(actual_note_X, proximity_MA, *midiout);
      terminateMidiNote(actual_note_Y, proximity_MA, *midiout);
      actual_note_X = x_control_MA+X_PITCH_OFFSET;
      actual_note_Y = y_control_MA+Y_PITCH_OFFSET;
      if(proximity_MA>=MIN_VELO){
        MidiNote(actual_note_X, proximity_MA, *midiout);
        MidiNote(actual_note_Y, proximity_MA, *midiout);
      }
      MidiCC(THETA_CONTROL_CC, theta_control, *midiout);
      MidiCC(FINGER_CONTROL_CC, linear_signal_convert(
                                  finger_control,
                                  0,
                                  5,
                                  127
                                ),
            *midiout
            );





    //show the main frame
    #if SHOWMAIN == true
      cvNamedWindow( "HandMarker",1);
      //add threshold trackbar
      cvCreateTrackbar( trackbar_text, "HandMarker", &threshold, 255, NULL);
//      cout << "Threshold: " << threshold << endl;
      cvShowImage("HandMarker",sourceFrame);
    #endif


	}//EOSEQ
	cvReleaseCapture( &capture);
	cvDestroyAllWindows();
}

void MIDIinit()
{
  //MIDI init section ----------------------------------------------------------
  // RtMidiOut constructor
  try {
    midiout = new RtMidiOut();
  }
  catch ( RtMidiError &error ) {
    error.printMessage();
    exit( EXIT_FAILURE );
  }
  nPorts = midiout->getPortCount();
  if ( nPorts == 0 ) {
    cout << "No ports available!\n";
    //goto cleanup;
  }
  cout << "There are " << nPorts << " ports are available: " << midiout->getPortName() << endl;
  // Open first available port.
  midiout->openPort( 0 );
}

int main()
{
  MIDIinit();
  run();
  return 0;
}
