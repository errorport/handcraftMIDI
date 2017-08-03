#include <opencv/cv.h>
#include <opencv/cxcore.h>
#include <opencv/highgui.h>
#include <iostream>

#define SUBFRAME_BIT_DEPTH   8      //bit depth of the subframe source
#define SUBFRAME_CHANNELS    1      //channels of the subframe source (original:3)

#define GRAY_BIT_DEPTH       8      //bit depth of the gray source
#define GRAY_CHANNELS        1      //channels of the gray source

#define GRAY_FRAME_WIDTH     270    //gray frame size
#define GRAY_FRAME_HEIGHT    270    //gray frame size

#define GRAY_FRAME_SIZE_FROM_ROI  true //in this case the grayFrame's size will be inherited from ROI

#define ROI_COORDINATE_X     340    //Region of Interest coordinate: X
#define ROI_COORDINATE_Y     10    //Region of Interest coordinate: Y

#define ROI_SIZE_X           270    //Region of Interest size: X -- may be eq to GRAY_FRAME_WIDTH
#define ROI_SIZE_Y           300    //Region of Interest size: Y -- may be eq to GRAY_FRAME_HEIGHT

#define BLUR_APERATURE_SIZE   20    //

#define INVBIN                false  // for rebinarizing the binarized image

#define SHOWSRCFIRST          false
#define SHOWMAIN              true
#define SHOWSRC               true
#define SHOWBIN               true  // for showing the binarized image

#define SHOWPOS               false //  write to >>1 the position of fingers
#define SHOWROI               true // to show the ROI on the sourceFrame

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

//
CvSeq* ptr;

//
CvPoint pt0;

CvMemStorage* storage1;
CvMemStorage* storage2;

//initializing, running
void run()
{
  //INIT SEQUENCE
            cout << "\n";
            cout << "Initializing..." << endl;
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



  //main cycle
  while(cvWaitKey(100)!=27)
  {

    //reinit SEQUENCE
    first_contour = NULL;
    maxitem       = NULL;
    contourNumber = 0;
    area=max_area = 0.0;
    ptr           = 0;
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
    cvThreshold(grayFrame, grayFrame, 0, 255, (CV_THRESH_BINARY_INV+CV_THRESH_OTSU));
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
				CvSeq* hull;
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
					pt0 = pt;
				}
				for( ; defects; defects = defects->h_next)
				{
					int nomdef = defects->total; // defect amount
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
							cvCircle( sourceFrame, *(defectArray[i].start), 5, CV_RGB(255,255,0), 2, 8,0); // inbetween finger points
							cvLine( sourceFrame, *(defectArray[i].depth_point), *(defectArray[i].end),CV_RGB(0,255,255),1, CV_AA, 0 );
							cvDrawContours( sourceFrame ,defects,CV_RGB(0,0,0),CV_RGB(255,0,0),-1,CV_FILLED,8);
						}
					}
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
					else if(con==4)
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
					CvFont font;
					cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 1.5, 1.5, 0, 5, CV_AA);
					cvPutText(sourceFrame, txt, cvPoint(50, 50), &font, cvScalar(0, 0, 255, 0));
					//
					//drop event here! ---------------------------------------

          //Free memory
					free(defectArray);
				}
				cvReleaseMemStorage( &storage1 );
				cvReleaseMemStorage( &storage2 );
			}

    } //EOSEQ

    // ------------------------------
    cvReleaseMemStorage( &storage );

    //drawing the ROI edges onto the main frame
    #if SHOWROI == true
      cvLine(sourceFrame,
             cvPoint(ROI_COORDINATE_X, ROI_COORDINATE_Y),
             cvPoint(ROI_COORDINATE_X, ROI_COORDINATE_Y+ROI_SIZE_Y),
             CV_RGB(255,255,255),1 , CV_AA, 0
            );
      cvLine(sourceFrame,
             cvPoint(ROI_COORDINATE_X, ROI_COORDINATE_Y+ROI_SIZE_Y),
             cvPoint(ROI_COORDINATE_X+ROI_SIZE_X, ROI_COORDINATE_Y+ROI_SIZE_Y),
             CV_RGB(255,255,255),1 , CV_AA, 0
            );
      cvLine(sourceFrame,
             cvPoint(ROI_COORDINATE_X+ROI_SIZE_X, ROI_COORDINATE_Y+ROI_SIZE_Y),
             cvPoint(ROI_COORDINATE_X+ROI_SIZE_X, ROI_COORDINATE_Y),
             CV_RGB(255,255,255),1 , CV_AA, 0
            );
      cvLine(sourceFrame,
             cvPoint(ROI_COORDINATE_X+ROI_SIZE_X, ROI_COORDINATE_Y),
             cvPoint(ROI_COORDINATE_X, ROI_COORDINATE_Y),
             CV_RGB(255,255,255),1 , CV_AA, 0
            );
    #endif

    #if SHOWMAIN == true
  		cvNamedWindow( "HandMarker",1);
      cvShowImage("HandMarker",sourceFrame);
    #endif
	}
	cvReleaseCapture( &capture);
	cvDestroyAllWindows();
}

int main()
{
  //init();
  run();
  return 0;
}
